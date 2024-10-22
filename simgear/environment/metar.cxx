// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2003 Melchior Franz <mfranz@aon.at>

/**
 * @file
 * @brief Interface for encoded Meteorological Aerodrome Reports (METAR).
 *
 * @see WMO-49
 * Technical Regulations, Basic Documents No. 2 (WMO No. 49)
 * Volume II - Meteorological Service for International Air Navigation
 * http://library.wmo.int/pmb_ged/wmo_49-v2_2013_en.pdf
 *
 * Refer to Table A3-2 (Template for METAR and SPECI) following page 78.
 *
 * For general information:
 * World Meteorological Organization http://library.wmo.int
 */

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <iostream>
#include <iomanip>
#include <string>
#include <time.h>
#include <cstring>
#include <ostream>
#include <sstream>

#include <simgear/debug/logstream.hxx>
#include <simgear/math/sg_random.hxx>
#include <simgear/structure/exception.hxx>

#include "metar.hxx"

#define NaN SGMetarNaN
#define TRACE 0

using std::string;
using std::map;
using std::vector;

/**
 * The constructor takes a Metar string
 * The constructor throws sg_io_exceptions on failure. The "METAR"
 * keyword has no effect (apart from incrementing the group counter
 * @a grpcount) and can be left away. A keyword "SPECI" is
 * likewise accepted.
 *
 * @param m     ICAO station id or metar string
 *
 * @par Examples:
 * @code
 * SGMetar *m = new SGMetar("METAR KSFO 061656Z 19004KT 9SM SCT100 OVC200 08/03 A3013");
 * double t = m->getTemperature_F();
 * delete m;

 * @endcode
 */
SGMetar::SGMetar(const string& m) :
	_x_proxy(false),
	_year(-1),
	_month(-1),
	_day(-1),
	_hour(-1),
	_minute(-1),
	_report_type(-1),
	_wind_dir(-1),
	_wind_speed(NaN),
	_gust_speed(NaN),
	_wind_range_from(-1),
	_wind_range_to(-1),
	_temp(NaN),
	_dewp(NaN),
	_pressure(NaN),
	_rain(false),
	_hail(false),
	_snow(false),
	_cavok(false)
{
    _data.resize(m.length() + 2); // make room for " \0"
    strcpy(_data.data(), m.c_str());
    _url = m;

    normalizeData();

    _m = _data.data();
    _icao[0] = '\0';

	// NOAA preample
	if (!scanPreambleDate())
		useCurrentDate();
	scanPreambleTime();

	// METAR header
	scanType();
	if (!scanId() || !scanDate()) {
		throw sg_io_exception("metar data bogus ", sg_location(_url));
	}
	while (scanModifier()) ;

	// base set
	scanWind();
	scanVariability();
	while (scanVisibility()) { /* empty loop body */}
	while (scanRwyVisRange()) { /* empty loop body */}
	while (scanWeather()) { /* empty loop body */}
	while (scanSkyCondition()) { /* empty loop body */}

    if (!scanTemperature()) {
        throw sg_io_exception("metar temperature data malformed or missing ", sg_location(_url));
    }
    if (!scanPressure()) {
        throw sg_io_exception("metar pressure data malformed or missing ", sg_location(_url));
    }

	while (scanSkyCondition()) { /* empty loop body */}
	while (scanRunwayReport()) { /* empty loop body */}
	scanWindShear();

	// appendix
	while (scanColorState()) { /* empty loop body */}
	scanTrendForecast();
	while (scanRunwayReport()) { /* empty loop body */}
	scanRemainder();
	scanRemark();

    _url.clear();
}


/**
  * Clears lists and maps to discourage access after destruction.
  */
SGMetar::~SGMetar()
{
	_clouds.clear();
	_runways.clear();
	_weather.clear();
}


static const char *azimuthName(double d)
{
	const char *dir[] = {
		"N", "NNE", "NE", "ENE",
		"E", "ESE", "SE", "SSE",
		"S", "SSW", "SW", "WSW",
		"W", "WNW", "NW", "NNW"
	};
	d += 11.25;
	while (d < 0)
		d += 360;
	while (d >= 360)
		d -= 360;
	return dir[int(d / 22.5)];
}


// round double to 10^g
static double rnd(double r, int g = 0)
{
	double f = pow(10.0, g);
	return f * floor(r / f + 0.5);
}


/* A manipulator that can use spaces to emulate tab characters. */
struct Tab
{
    /* If <stops> is 0, we simply insert tab characters. Otherwise we insert
    spaces to align with the next column at multiple of <stops>. */
    explicit Tab(int stops)
    :
    _stops(stops)
    {}
    int _stops;
};

std::ostream& operator << (std::ostream& out, const Tab& t)
{
    if (t._stops == 0) {
        return out << '\t';
    }
    
    std::ostringstream& out2 = *(std::ostringstream*) &out;
    std::string s = out2.str();
    
    if (t._stops < 0) {
        if (!s.size() || s[s.size()-1] != ' ') {
            out << ' ';
        }
        return out;
    }
    
    auto nl = s.rfind('\n');
    if (nl == std::string::npos) nl = 0;
    int column = 0;
    for (auto i = nl+1; i != s.size(); ++i) {
        if (s[i] == '\t')
            column = (column + t._stops) / t._stops * t._stops;
        else
            column += 1;
    }
    int column2 = (column + t._stops) / t._stops * t._stops;
    for (int i=column; i<column2; ++i) {
        out << ' ';
    }
    return out;
}

/* Manipulator for SGMetarVisibility using a Tab. */
struct SGMetarVisibilityManip
{
    explicit SGMetarVisibilityManip(const SGMetarVisibility& v, const Tab& tab)
    :
    _v(v),
    _tab(tab)
    {}
    const SGMetarVisibility&    _v;
    const Tab&                  _tab;
};

std::ostream& operator << (std::ostream& out, const SGMetarVisibilityManip& v)
{
	int m = v._v.getModifier();
	const char *mod;
	if (m == SGMetarVisibility::GREATER_THAN)
		mod = ">=";
	else if (m == SGMetarVisibility::LESS_THAN)
		mod = "<";
	else
		mod = "";
	out << mod;

	double dist = rnd(v._v.getVisibility_m(), 1);
	if (dist < 1000.0)
		out << rnd(dist, 1) << " m";
	else
		out << rnd(dist / 1000.0, -1) << " km";

	const char *dir = "";
	int i;
	if ((i = v._v.getDirection()) != -1) {
		dir = azimuthName(i);
		out << " " << dir;
	}
        out << v._tab << v._tab << v._tab << v._tab << v._tab;
        out << mod << rnd(v._v.getVisibility_sm(), -1) << " US-miles " << dir;
        return out;
}


std::string SGMetar::getDescription(int tabstops) const
{
    std::ostringstream  out;
	const char *s;
	char buf[256];
	double d;
	int i, lineno;
        Tab tab(tabstops);
        
	if ((i = getReportType()) == SGMetar::AUTO)
		out << "(METAR automatically generated)\n";
	else if (i == SGMetar::COR)
		out << "(METAR manually corrected)\n";
	else if (i == SGMetar::RTD)
		out << "(METAR routine delayed)\n";
        
	out << "Airport-Id:" << tab << tab << getId() << "\n";

	// date/time
	int year = getYear();
	int month = getMonth();
	out << "Report time:" << tab << tab << year << '/' << month << '/' << getDay();
	out << ' ' << getHour() << ':';
	out << std::setw(2) << std::setfill('0') << getMinute() << " UTC\n";


	// visibility
	SGMetarVisibility minvis = getMinVisibility();
	SGMetarVisibility maxvis = getMaxVisibility();
	double min = minvis.getVisibility_m();
	double max = maxvis.getVisibility_m();
	if (min != NaN) {
		if (max != NaN) {
			out << "min. Visibility:" << tab << SGMetarVisibilityManip(minvis, tab) << "\n";
			out << "max. Visibility:" << tab << SGMetarVisibilityManip(maxvis, tab) << "\n";
		} else {
			out << "Visibility:" << tab << tab << SGMetarVisibilityManip(minvis, tab) << "\n";
                }
	}


	// directed visibility
	const SGMetarVisibility *dirvis = getDirVisibility();
	for (i = 0; i < 8; i++, dirvis++)
		if (dirvis->getVisibility_m() != NaN)
			out << tab << tab << tab << SGMetarVisibilityManip(*dirvis, tab) << "\n";


	// vertical visibility
	SGMetarVisibility vertvis = getVertVisibility();
	if ((d = vertvis.getVisibility_ft()) != NaN)
		out << "Vert. visibility:" << tab << SGMetarVisibilityManip(vertvis, tab) << "\n";
	else if (vertvis.getModifier() == SGMetarVisibility::NOGO)
		out << "Vert. visibility:" << tab << "impossible to determine" << "\n";


	// wind
	d = getWindSpeed_kmh();
	out << "Wind:" << tab << tab << tab;
	if (d < .1)
		out << "none" << "\n";
	else {
		if ((i = getWindDir()) == -1)
			out << "from variable directions";
		else
			out << "from the " << azimuthName(i) << " (" << i << " deg)";
		out << " at " << rnd(d, -1) << " km/h";

		out << tab << tab << rnd(getWindSpeed_kt(), -1) << " kt";
		out << " = " << rnd(getWindSpeed_mph(), -1) << " mph";
		out << " = " << rnd(getWindSpeed_mps(), -1) << " m/s";
		out << "\n";

		d = getGustSpeed_kmh();
                if (d != NaN && d != 0) {
			out << tab << tab << tab << "with gusts at " << rnd(d, -1) << " km/h";
			out << tab << tab << tab << rnd(getGustSpeed_kt(), -1) << " kt";
			out << " = " << rnd(getGustSpeed_mph(), -1) << " mph";
			out << " = " << rnd(getGustSpeed_mps(), -1) << " m/s";
			out << "\n";
		}

		int from = getWindRangeFrom();
		int to = getWindRangeTo();
		if (from != to) {
			out << tab << tab << tab << "variable from " << azimuthName(from);
			out << " to " << azimuthName(to);
			out << " (" << from << "deg --" << to << " deg)" << "\n";
		}
	}


	// temperature/humidity/air pressure
	if ((d = getTemperature_C()) != NaN) {
		out << "Temperature:" << tab << tab << d << " C" << tab << tab << tab << tab << tab;
		out << rnd(getTemperature_F(), -1) << " F" << "\n";

		if ((d = getDewpoint_C()) != NaN) {
			out << "Dewpoint:" << tab << tab << d << " C" << tab << tab << tab << tab << tab;
			out << rnd(getDewpoint_F(), -1) << " F"  << "\n";
			out << "Rel. Humidity: " << tab << tab << rnd(getRelHumidity()) << " %" << "\n";
		}
	}
	if ((d = getPressure_hPa()) != NaN) {
		out << "Pressure:" << tab << tab << rnd(d) << " hPa" << tab << tab << tab << tab;
		out << rnd(getPressure_inHg(), -2) << " in. Hg" << "\n";
	}


	// weather phenomena
	vector<string> wv = getWeather();
	vector<string>::iterator weather;
	for (i = 0, weather = wv.begin(); weather != wv.end(); weather++, i++) {
		out << (i ? ", " : "Weather:") << tab << tab << weather->c_str();
	}
	if (i)
		out << "\n";


	// cloud layers
	const char *coverage_string[5] = {
		"clear skies", "few clouds", "scattered clouds", "broken clouds", "sky overcast"
	};
	vector<SGMetarCloud> cv = getClouds();
	vector<SGMetarCloud>::iterator cloud;
	for (lineno = 0, cloud = cv.begin(); cloud != cv.end(); cloud++, lineno++) {
                if (lineno) out << tab << tab << tab;
                else    out << "Sky condition:" << tab << tab;

		if ((i = cloud->getCoverage()) != -1)
			out << coverage_string[i];
		if ((d = cloud->getAltitude_ft()) != NaN)
			out << " at " << rnd(d, 1) << " ft";
		if ((s = cloud->getTypeLongString()))
			out << " (" << s << ')';
		if (d != NaN)
			out << tab << tab << tab << rnd(cloud->getAltitude_m(), 1) << " m";
		out << "\n";
	}


	// runways
	map<string, SGMetarRunway> rm = getRunways();
	map<string, SGMetarRunway>::iterator runway;
	for (runway = rm.begin(); runway != rm.end(); runway++) {
		lineno = 0;
		if (!strcmp(runway->first.c_str(), "ALL"))
			out << "All runways:" << tab << tab;
		else
			out << "Runway " << runway->first << ":" << tab << tab;
		SGMetarRunway rwy = runway->second;

		// assemble surface string
		vector<string> surface;
		if ((s = rwy.getDepositString()) && strlen(s))
			surface.push_back(s);
		if ((s = rwy.getExtentString()) && strlen(s))
			surface.push_back(s);
		if ((d = rwy.getDepth()) != NaN) {
			snprintf(buf, 256, "%.1lf mm", d * 1000.0);
			surface.push_back(buf);
		}
		if ((s = rwy.getFrictionString()) && strlen(s))
			surface.push_back(s);
		if ((d = rwy.getFriction()) != NaN) {
			snprintf(buf, 256, "friction: %.2lf", d);
			surface.push_back(buf);
		}

		if (! surface.empty()) {
			vector<string>::iterator rwysurf = surface.begin();
			for (i = 0; rwysurf != surface.end(); rwysurf++, i++) {
				if (i)
					out << ", ";
				out << *rwysurf;
			}
			lineno++;
		}

		// assemble visibility string
		SGMetarVisibility minvis = rwy.getMinVisibility();
		SGMetarVisibility maxvis = rwy.getMaxVisibility();
		if ((d = minvis.getVisibility_m()) != NaN) {
			if (lineno++)
				out << "\n" << tab << tab << tab;
			out << SGMetarVisibilityManip(minvis, tab);
		}
		if (maxvis.getVisibility_m() != d) {
			out << "\n" << tab << tab << tab << SGMetarVisibilityManip(maxvis, tab) << "\n";
			lineno++;
		}

		if (rwy.getWindShear()) {
			if (lineno++)
				out << "\n" << tab << tab << tab;
			out << "critical wind shear" << "\n";
		}
		out << "\n";
	}
	out << "\n";
        return out.str();
}

void SGMetar::useCurrentDate()
{
	struct tm now;
	time_t now_sec = time(0);
#ifdef _WIN32
	now = *gmtime(&now_sec);
#else
	gmtime_r(&now_sec, &now);
#endif
	_year = now.tm_year + 1900;
	_month = now.tm_mon + 1;
}

std::string SGMetar::getDataString() const
{
    return std::string{_data.data()};
}

std::string SGMetar::getUnparsedData() const
{
    if (!_m || (strlen(_m) < 1))
        return {};

    return std::string{_m};
}
/**
  * Replace any number of subsequent spaces by just one space, and add
  * a trailing space. This makes scanning for things like "ALL RWY" easier.
  */
void SGMetar::normalizeData()
{
    // TODO : use strutils::simplify here
    char* src = _data.data();
    char* dest = src;
    for (; (*dest++ = *src++);) {
        while (*src == ' ' && src[1] == ' ')
            src++;
    }

    for (dest--; isspace(*--dest); )
        ;

    *++dest = ' ';
    *++dest = '\0';
}


// \d\d\d\d/\d\d/\d\d
bool SGMetar::scanPreambleDate()
{
	char *m = _m;
	int year, month, day;
	if (!scanNumber(&m, &year, 4))
		return false;
	if (*m++ != '/')
		return false;
	if (!scanNumber(&m, &month, 2))
		return false;
	if (*m++ != '/')
		return false;
	if (!scanNumber(&m, &day, 2))
		return false;
	if (!scanBoundary(&m))
		return false;
	_year = year;
	_month = month;
	_day = day;
	_m = m;
	return true;
}


// \d\d:\d\d
bool SGMetar::scanPreambleTime()
{
	char *m = _m;
	int hour, minute;
	if (!scanNumber(&m, &hour, 2))
		return false;
	if (*m++ != ':')
		return false;
	if (!scanNumber(&m, &minute, 2))
		return false;
	if (!scanBoundary(&m))
		return false;
	_hour = hour;
	_minute = minute;
	_m = m;
	return true;
}


// (METAR|SPECI)
bool SGMetar::scanType()
{
	if (strncmp(_m, "METAR ", 6) && strncmp(_m, "SPECI ", 6))
		return false;
	_m += 6;
	return true;
}


// [A-Z]{4}
bool SGMetar::scanId()
{
	char *m = _m;
	for (int i = 0; i < 4; m++, i++)
		if (!(isalpha(*m) || isdigit(*m)))
			return false;
	if (!scanBoundary(&m))
		return false;
	strncpy(_icao, _m, 4);
	_icao[4] = '\0';
	_m = m;
	return true;
}


// \d{6}Z
bool SGMetar::scanDate()
{
    char* m = _m;
    int day, hour, minute;

    if (!scanNumber(&m, &day, 2))
        return false;
    if (!scanNumber(&m, &hour, 2))
        return false;
    if (!scanNumber(&m, &minute, 2))
        return false;

    if (*m == 'Z')
        m++;
    else if (*m != ' ')
        return false;

    if (!scanBoundary(&m))
        return false;

    _day = day;
    _hour = hour;
    _minute = minute;
    _m = m;
    return true;
}


// (NIL|AUTO|COR|RTD)
bool SGMetar::scanModifier()
{
	char *m = _m;
	int type;
	if (!strncmp(m, "NIL", 3)) {
		_m += strlen(_m);
		return true;
	}
	if (!strncmp(m, "AUTO", 4))			// automatically generated
		m += 4, type = AUTO;
	else if (!strncmp(m, "COR", 3))			// manually corrected
		m += 3, type = COR;
	else if (!strncmp(m, "CC", 2) && *(m+2) >= 'A' && *(m+2) <= 'B')    // correction
		m += 3, type = COR;
	else if (!strncmp(m, "RTD", 3))			// routine delayed
		m += 3, type = RTD;
	else
		return false;
	if (!scanBoundary(&m))
		return false;
	_report_type = type;
	_m = m;
	return true;
}


// (\d{3}|VRB)\d{1,3}(G\d{2,3})?(KT|KMH|MPS)
bool SGMetar::scanWind()
{
#if TRACE
    std::cout << "metar wind: " << _m << std::endl;
#endif

    char* m = _m;
    int dir;

    if (*m == '?' || *m == 'E') // not spec compliant
        m++;
    if (!strncmp(m, "VRB", 3))
        m += 3, dir = -1;
    else if (!strncmp(m, "///", 3)) // direction not measurable
        m += 3, dir = -1;
    else if (!scanNumber(&m, &dir, 3))
        dir = -1;

    if (*m == '/' && *(m + 1) != '/') // ignore single slash
        m++;

    int i;
    if (!strncmp(m, "//", 2)) // speed not measurable
        m += 2, i = -1;
    else if (!scanNumber(&m, &i, 1, 3))
        i = -1; // not spec compliant
    double speed = i;

    double gust = NaN;
    if (*m == ' ' && *(m + 1) == 'G') // space between direction/velocity and gusts - not spec compliant
        m++;
    if (*m == 'G') {
        m++;
        if (!strncmp(m, "//", 2)) // speed not measurable
            m += 2, i = -1;
        else if (!scanNumber(&m, &i, 2, 3))
            return false;

        if (i != -1)
            gust = i;
    }

    double factor;
    if (!strncmp(m, "KT", 2))
        m += 2, factor = SG_KT_TO_MPS;
    else if (!strncmp(m, "KMH", 3)) // invalid Km/h
        m += 3, factor = SG_KMH_TO_MPS;
    else if (!strncmp(m, "KPH", 3)) // invalid Km/h
        m += 3, factor = SG_KMH_TO_MPS;
    else if (!strncmp(m, "MPS", 3))
        m += 3, factor = 1.0;
    else if (!strncmp(m, " ", 1)) // default to Knots
        factor = SG_KT_TO_MPS;
    else
        return false;

    if (!scanBoundary(&m))
        return false;

    _m = m;
    _wind_dir = dir == -1 ? 0 : dir;
    _wind_speed = speed < 0.0 ? 0.0 : speed * factor;
    if (gust != NaN)
        _gust_speed = gust * factor;
    return true;
}


// \d{3}V\d{3}
bool SGMetar::scanVariability()
{
#if TRACE
    std::cout << "metar variability: " << _m << std::endl;
#endif

    char* m = _m;
    int from, to;

    if (!strncmp(m, "///", 3)) // direction not measurable
        m += 3, from = -1;
    else if (!scanNumber(&m, &from, 1, 3))
        return false;

    if (*m++ != 'V')
        return false;

    if (!strncmp(m, "///", 3)) // direction not measurable
        m += 3, to = -1;
    else if (!scanNumber(&m, &to, 1, 3))
        return false;

    if (!scanBoundary(&m))
        return false;

    _m = m;
    _wind_range_from = from;
    _wind_range_to = to;

    return true;
}


bool SGMetar::scanVisibility()
// TODO: if only directed vis are given, do still set min/max
{
#if TRACE
    std::cout << "metar visibility: " << _m << std::endl;
#endif

    if (!strncmp(_m, "/////", 5))
        return false;       // not a visibility element (e.g. temperature)

    if (!strncmp(_m, "////", 4)) {
        _m += 4;

        if (!strncmp(_m, "SM", 2))
            _m += 2;
        else if (!strncmp(_m, "KM", 2))
            _m += 2;

        return scanBoundary(&_m);
    }

    char *m = _m;
	double distance;
	int i, dir = -1;
	int modifier = SGMetarVisibility::EQUALS;
// \d{4}(N|NE|E|SE|S|SW|W|NW)?
	if (scanNumber(&m, &i, 4)) {
		if( strncmp( m, "NDV",3 ) == 0 ) {
			m+=3; // tolerate NDV (non-directional variation)
		} else if (*m == 'E') {
			m++, dir = 90;
		} else if (*m == 'W') {
			m++, dir = 270;
		} else if (*m == 'N') {
			m++;
			if (*m == 'E')
				m++, dir = 45;
			else if (*m == 'W')
				m++, dir = 315;
			else
				dir = 0;
		} else if (*m == 'S') {
			m++;
			if (*m == 'E')
				m++, dir = 135;
			else if (*m == 'W')
				m++, dir = 225;
			else
				dir = 180;
                }
		if (i == 0)
			i = 50, modifier = SGMetarVisibility::LESS_THAN;
		else if (i == 9999)
			i++, modifier = SGMetarVisibility::GREATER_THAN;
		distance = i;
	} else {
// M?(\d{1,2}|\d{1,2}/\d{1,2}|\d{1,2} \d{1,2}/\d{1,2})(SM|KM)
		if (*m == 'M')
			m++, modifier = SGMetarVisibility::LESS_THAN;

		if (!scanNumber(&m, &i, 1, 3))
			return false;
		distance = i;

		if (*m == '/') {
			m++;
			if (!scanNumber(&m, &i, 1, 2))
				return false;
			distance /= i;
		} else if (*m == ' ') {
			m++;
			int denom;
			if (!scanNumber(&m, &i, 1, 2))
				return false;
			if (*m++ != '/')
				return false;
			if (!scanNumber(&m, &denom, 1, 2))
				return false;
			if (denom != 0)
				distance += (double)i / denom;
		}

		if (!strncmp(m, "SM", 2))
			distance *= SG_SM_TO_METER, m += 2;
		else if (!strncmp(m, "KM", 2))
			distance *= 1000, m += 2;
		else
			return false;
	}
	if (!scanBoundary(&m))
		return false;

	SGMetarVisibility *v;
	if (dir != -1)
		v = &_dir_visibility[dir / 45];
	else if (_min_visibility._distance == NaN)
		v = &_min_visibility;
	else
		v = &_max_visibility;

	v->_distance = distance;
	v->_modifier = modifier;
	v->_direction = dir;
	_m = m;
	return true;
}


// R\d\d[LCR]?/([PM]?\d{4}V)?[PM]?\d{4}(FT)?[DNU]?
bool SGMetar::scanRwyVisRange()
{
#if TRACE
    std::cout << "metar runway range: " << _m << std::endl;
#endif

	char *m = _m;
	int i;
	SGMetarRunway r;

	if (*m++ != 'R')
		return false;
	if (!scanNumber(&m, &i, 2))
		return false;
	if (*m == 'L' || *m == 'C' || *m == 'R')
		m++;

	char id[4];
	strncpy(id, _m + 1, i = m - _m - 1);
	id[i] = '\0';

	if (*m++ != '/')
		return false;

	if (!strncmp(m, "////", 4)) {
		// sensor failure... ignore
		_m = m + 4;
		return scanBoundary(&_m);
	}

	int from, to;
	if (*m == 'P')
		m++, r._min_visibility._modifier = SGMetarVisibility::GREATER_THAN;
	else if (*m == 'M')
		m++, r._min_visibility._modifier = SGMetarVisibility::LESS_THAN;
	if (!scanNumber(&m, &from, 4))
		return false;
	if (*m == 'V') {
		m++;
		if (*m == 'P')
			m++, r._max_visibility._modifier = SGMetarVisibility::GREATER_THAN;
		else if (*m == 'M')
			m++, r._max_visibility._modifier = SGMetarVisibility::LESS_THAN;
		if (!scanNumber(&m, &to, 4))
			return false;
	} else
		to = from;

	if (!strncmp(m, "FT", 2)) {
		from = int(from * SG_FEET_TO_METER);
		to = int(to * SG_FEET_TO_METER);
		m += 2;
	}
	r._min_visibility._distance = from;
	r._max_visibility._distance = to;

	if (*m == '/')					// this is not in the spec!
		m++;
	if (*m == 'D')
		m++, r._min_visibility._tendency = SGMetarVisibility::DECREASING;
	else if (*m == 'N')
		m++, r._min_visibility._tendency = SGMetarVisibility::STABLE;
	else if (*m == 'U')
		m++, r._min_visibility._tendency = SGMetarVisibility::INCREASING;

	if (!scanBoundary(&m))
		return false;
	_m = m;

	_runways[id]._min_visibility = r._min_visibility;
	_runways[id]._max_visibility = r._max_visibility;
	return true;
}


static const struct Token special[] = {
	{ "NSW",  "no significant weather" },
/*	{ "VCSH", "showers in the vicinity" },
	{ "VCTS", "thunderstorm in the vicinity" }, */
	{ 0, 0 }
};


static const struct Token description[] = {
	{ "SH", "showers of" },
	{ "TS", "thunderstorm with" },
	{ "BC", "patches of" },
	{ "BL", "blowing" },
	{ "DR", "low drifting" },
	{ "FZ", "freezing" },
	{ "MI", "shallow" },
	{ "PR", "partial" },
	{ "RE", "recent" },
	{ 0, 0 }
};


static const struct Token phenomenon[] = {
	{ "DZ",   "drizzle" },
	{ "GR",   "hail" },
	{ "GS",   "small hail and/or snow pellets" },
	{ "IC",   "ice crystals" },
	{ "PE",   "ice pellets" },
	{ "PL",   "ice pellets" },
	{ "RA",   "rain" },
	{ "SG",   "snow grains" },
	{ "SN",   "snow" },
	{ "UP",   "unknown precipitation" },
	{ "BR",   "mist" },
	{ "DU",   "widespread dust" },
	{ "FG",   "fog" },
	{ "FGBR", "fog bank" },
	{ "FU",   "smoke" },
	{ "HZ",   "haze" },
	{ "PY",   "spray" },
	{ "SA",   "sand" },
	{ "VA",   "volcanic ash" },
	{ "DS",   "dust storm" },
	{ "FC",   "funnel cloud/tornado waterspout" },
	{ "PO",   "well-developed dust/sand whirls" },
	{ "SQ",   "squalls" },
	{ "SS",   "sandstorm" },
	{ 0, 0 }
};


// (+|-|VC)?(NSW|MI|PR|BC|DR|BL|SH|TS|FZ)?((DZ|RA|SN|SG|IC|PE|GR|GS|UP){0,3})(BR|FG|FU|VA|DU|SA|HZ|PY|PO|SQ|FC|SS|DS){0,3}
bool SGMetar::scanWeather()
{
#if TRACE
    std::cout << "metar weather: " << _m << std::endl;
#endif

	char *m = _m;
	string weather;
	const struct Token *a;

	// @see WMO-49 Section 4.4.2.9
	// Denotes a temporary failure of the sensor
	if (!strncmp(m, "// ", 3)) {
		_m += 3;
		return false;
	}

    // maintenance flag
    if (*m == 'M' && *(m+1) == ' ') {
        _m++;
        return scanBoundary(&_m);
    }

	if ((a = scanToken(&m, special))) {
		if (!scanBoundary(&m))
			return false;
		_weather.push_back(a->text);
		_m = m;
		return true;
	}

	string pre, post;
	struct Weather w;
	if (*m == '-')
		m++, pre = "light ", w.intensity = LIGHT;
	else if (*m == '+')
		m++, pre = "heavy ", w.intensity = HEAVY;
	else if (!strncmp(m, "VC", 2))
        m += 2, post = "in the vicinity ", w.vincinity=true;
	else
		pre = "moderate ", w.intensity = MODERATE;

	int i;
	for (i = 0; i < 3; i++) {
		if (!(a = scanToken(&m, description)))
			break;
		w.descriptions.push_back(a->id);
		weather += string(a->text) + " ";
	}

	for (i = 0; i < 3; i++) {
		if (!(a = scanToken(&m, phenomenon)))
			break;
        w.phenomena.push_back(a->id);
		weather += string(a->text) + " ";
		if (!strcmp(a->id, "RA"))
			_rain = w.intensity;
		else if (!strcmp(a->id, "DZ"))
			_rain = LIGHT;
		else if (!strcmp(a->id, "HA"))
			_hail = w.intensity;
		else if (!strcmp(a->id, "SN"))
			_snow = w.intensity;
	}
	if (!weather.length())
		return false;
	if (!scanBoundary(&m))
		return false;
	_m = m;
	weather = pre + weather + post;
	weather.erase(weather.length() - 1);
	_weather.push_back(weather);
    if( ! w.phenomena.empty() ) {
        _weather2.push_back( w );
    }
    return true;
}


static const struct Token cloud_types[] = {
	{ "AC",    "altocumulus" },
	{ "ACC",   "altocumulus castellanus" },
	{ "ACSL",  "altocumulus standing lenticular" },
	{ "AS",    "altostratus" },
	{ "CB",    "cumulonimbus" },
	{ "CBMAM", "cumulonimbus mammatus" },
	{ "CC",    "cirrocumulus" },
	{ "CCSL",  "cirrocumulus standing lenticular" },
	{ "CI",    "cirrus" },
	{ "CS",    "cirrostratus" },
	{ "CU",    "cumulus" },
	{ "CUFRA", "cumulus fractus" },
	{ "NS",    "nimbostratus" },
	{ "SAC",   "stratoaltocumulus" },		// guessed
	{ "SC",    "stratocumulus" },
	{ "SCSL",  "stratocumulus standing lenticular" },
	{ "ST",    "stratus" },
	{ "STFRA", "stratus fractus" },
	{ "TCU",   "towering cumulus" },
	{ 0, 0 }
};

#include <iostream>
// (FEW|SCT|BKN|OVC|SKC|CLR|CAVOK|VV)([0-9]{3}|///)?[:cloud_type:]?
bool SGMetar::scanSkyCondition()
{
#if TRACE
    std::cout << "metar sky condition: " << _m << std::endl;
#endif

	char *m = _m;
	int i;
	SGMetarCloud cl;
    static SGMetarCloud::Coverage priorCoverage;

	if (!strncmp(m, "//////", 6)) {
		char* m2 = m+6;
		if (scanBoundary(&m2)) {
			_m = m2;
			return true;
		}
	}

	if (!strncmp(m, "CLR", i = 3)				// clear
			|| !strncmp(m, "SKC", i = 3)		// sky clear
			|| !strncmp(m, "NCD", i = 3)		// nil cloud detected
			|| !strncmp(m, "NSC", i = 3)		// no significant clouds
			|| !strncmp(m, "CAVOK", i = 5))	{	// ceiling and visibility OK (implies 9999)
		m += i;
		if (!scanBoundary(&m))
			return false;

		if (i == 3) {
			cl._coverage = SGMetarCloud::COVERAGE_CLEAR;
			_clouds.push_back(cl);
		} else {
			_cavok = true;
		}
		_m = m;
		return true;
	}

    bool verticalVisibility = false;
    if (!strncmp(m, "VV", i = 2)) {             // vertical visibility
        verticalVisibility = true;
    } else if (!strncmp(m, "FEW", i = 3)) {
        cl._coverage = SGMetarCloud::COVERAGE_FEW;
        priorCoverage = cl._coverage;
    } else if (!strncmp(m, "SCT", i = 3)) {
        cl._coverage = SGMetarCloud::COVERAGE_SCATTERED;
        priorCoverage = cl._coverage;
    } else if (!strncmp(m, "BKN", i = 3)) {
        cl._coverage = SGMetarCloud::COVERAGE_BROKEN;
        priorCoverage = cl._coverage;
    } else if (!strncmp(m, "OVC", i = 3)) {
        cl._coverage = SGMetarCloud::COVERAGE_OVERCAST;
        priorCoverage = cl._coverage;
    } else if (!strncmp(m, "///", i = 3))
        cl._coverage = SGMetarCloud::COVERAGE_NIL; // should we add 'unknown'?
    else {
        // check for an implied coverage element
        if (*m >= '0' && *m <= '9' &&
            *(m+1) >= '0' && *(m+1) <= '9' &&
            *(m+2) >= '0' && *(m+2) <= '9' &&
            *(m+3) == ' ') {
            cl._coverage = priorCoverage;
            i = 0;
        } else
            return false;
    }
    m += i;

    if (!strncmp(m, "///", 3)) { // vis not measurable (e.g. because of heavy snowing)
		m += 3, i = -1;
		sg_srandom_time();
		// randomize the base height to avoid the black sky issue
		i = 50 + static_cast<int>(sg_random() * 250.0);		// range [5,000, 30,000]
	} else if (scanBoundary(&m)) {
		_m = m;
		return true;				// ignore single OVC/BKN/...
	} else if (!scanNumber(&m, &i, 3))
		i = -1;

	if (verticalVisibility) {
		 if (!scanBoundary(&m))
		 	return false;
		if (i == -1)			// 'VV///'
			_vert_visibility._modifier = SGMetarVisibility::NOGO;
		else
			_vert_visibility._distance = i * 100 * SG_FEET_TO_METER;
		_m = m;
		return true;
	}

	if (i != -1)
		cl._altitude = i * 100 * SG_FEET_TO_METER;

	const struct Token *a;
	if ((a = scanToken(&m, cloud_types))) {
		cl._type = a->id;
		cl._type_long = a->text;
	}

	// @see WMO-49 Section 4.5.4.5
	// Denotes temporary failure of sensor and covers cases like FEW045///
	if (!strncmp(m, "///", 3))
		m += 3;
	if (!scanBoundary(&m))
		return false;

	// require known coverage and base-height
	if (i != -1 && cl._coverage != SGMetarCloud::COVERAGE_NIL)
		_clouds.push_back(cl);

	_m = m;
	return true;
}


// M?[0-9]{2}/(M?[0-9]{2})?            (spec)
// (M?[0-9]{2}|XX)/(M?[0-9]{2}|XX)?    (Namibia)
bool SGMetar::scanTemperature()
{
#if TRACE
    std::cout << "metar temp: " << _m << std::endl;
#endif

	char *m = _m;
	int sign = 1, temp, dew;

    // sniff test to confirm that this is a temperature element
    std::string s = "M/0123456789";
    for (int i=0; i<7; ++i) {
        if (*(m+i) == ' ')
            break;
        if (s.find(*(m+i)) == std::string::npos)
            return true;        // nope, bail
    }

	if (!strncmp(m, "XX/XX", 5)) {		            // not spec compliant!
		_m += 5;
		return scanBoundary(&_m);
	}
	if (!strncmp(m, "/////", 5)) {
		// sensor failure... assume standard temperature
		_temp = 15.0;
		_dewp = 3.0;
		_m += 5;
		return scanBoundary(&_m);
	}

    // maintenance flag
    if (*m == 'M' && *(m+1) == ' ') {
        _m++;
        return scanBoundary(&_m);
    }

	if (*m == 'M')
		m++, sign = -1;
	if (!scanNumber(&m, &temp, 1, 2))
		return false;
	temp *= sign;

	if (*m++ != '/')
		return false;
	if (!scanBoundary(&m)) {
		if (!strncmp(m, "XX", 2))	                // not spec compliant!
			m += 2, sign = 0, dew = temp - 10;
		else if (!strncmp(m, "//", 2))	                // sensor failure... assume relative dew point
			m += 2, sign = 0, dew = temp - 10;
		else {
			sign = 1;
			if (*m == 'M')
				m++, sign = -1;
			if (!scanNumber(&m, &dew, 1, 2))
				return false;
		}
		if (!scanBoundary(&m))
			return false;
		if (sign)
			_dewp = sign * dew;
	}
	_temp = temp;
	_m = m;
	return true;
}


double SGMetar::getRelHumidity() const
{
	if (_temp == NaN || _dewp == NaN)
		return NaN;
	double dewp = pow(10.0, 7.5 * _dewp / (237.7 + _dewp));
	double temp = pow(10.0, 7.5 * _temp / (237.7 + _temp));
	return dewp * 100 / temp;
}


// [AQ]\d{4}             (spec)
// [AQ]\d{2}(\d{2}|//)   (Namibia)
bool SGMetar::scanPressure()
{
#if TRACE
    std::cout << "metar pressure: " << _m << std::endl;
#endif

	char *m = _m;
	double factor;
    bool unitProvided = true;
    bool valueProvided = true;
	int press, i;

	if (*m == '\0') {
		_pressure = 101300.0;	// pressure not provided... assume standard pressure
		return true;
	}

	if (*m == 'A') {
		factor = SG_INHG_TO_PA / 100.0;
        m++;
    }
	else if (*m == 'Q') {
		factor = 100.0;
        m++;
    }
	else
        unitProvided = false;

	if (*m == ' ')	// ignore space
		m++;
    if (!strncmp(m, "////", 4)) {
        // sensor failure... assume standard pressure
        if (*(m - 1) == 'A')
            press = 2992;
        else
            press = 1013;
        m += 4;
    } else {
        if (!scanNumber(&m, &press, 2, 4)) {
			valueProvided = false;

			// sensor failure... assume standard pressure
			if (*(m - 1) == 'A')
				press = 2992;
			else
				press = 1013;
		}

		if (press < 100) {				// 2-digit pressure may have further data following
        	press *= 100;
        	if (!strncmp(m, "//", 2)) 	// not spec compliant!
        	    m += 2;
        	else if (scanNumber(&m, &i, 2))
        	    press += i;
		}
    }

	if (*m == ',' || *m == '=')	// ignore trailing comma, equals
		m++;

    if ((unitProvided || valueProvided) && !scanBoundary(&m))
        return false;

    // derive unit when not explicitly provided
    if (!unitProvided)
        factor = (press > 2000 ? SG_INHG_TO_PA / 100.0 : 100.0);

    _pressure = press * factor;
	_m = m;
	return true;
}


static const char *runway_deposit[] = {
	"clear and dry",
	"damp",
	"wet or puddles",
	"frost",
	"dry snow",
	"wet snow",
	"slush",
	"ice",
	"compacted snow",
	"frozen ridges"
};


static const char *runway_deposit_extent[] = {
	0, "1-10%", "11-25%", 0, 0, "26-50%", 0, 0, 0, "51-100%"
};


static const char *runway_friction[] = {
	0,
	"poor braking action",
	"poor/medium braking action",
	"medium braking action",
	"medium/good braking action",
	"good braking action",
	0, 0, 0,
	"friction: unreliable measurement"
};


// \d\d(CLRD|[\d/]{4})(\d\d|//)
bool SGMetar::scanRunwayReport()
{
	char *m = _m;
	int i;
	char id[4];
	SGMetarRunway r;

	if (!scanNumber(&m, &i, 2))
		return false;
	if (i == 88)
		strcpy(id, "ALL");
	else if (i == 99)
		strcpy(id, "REP");		// repetition of previous report
	else if (i >= 50) {
		i -= 50;
		id[0] = i / 10 + '0', id[1] = i % 10 + '0', id[2] = 'R', id[3] = '\0';
	} else
		id[0] = i / 10 + '0', id[1] = i % 10 + '0', id[2] = '\0';

	if (!strncmp(m, "CLRD", 4)) {
		m += 4;							// runway cleared
		r._deposit_string = "cleared";
	} else {
		if (scanNumber(&m, &i, 1)) {
			r._deposit = i;
			r._deposit_string = runway_deposit[i];
		} else if (*m == '/')
			m++;
		else
			return false;

		if (*m == '1' || *m == '2' || *m == '5' || *m == '9') {	// extent of deposit
			r._extent = *m - '0';
			r._extent_string = runway_deposit_extent[*m - '0'];
		} else if (*m != '/')
			return false;

		m++;
		i = -1;
		if (!strncmp(m, "//", 2))
			m += 2;
		else if (!scanNumber(&m, &i, 2))
			return false;

		if (i == 0)
			r._depth = 0.0005;				// < 1 mm deep (let's say 0.5 :-)
		else if (i > 0 && i <= 90)
			r._depth = i / 1000.0;				// i mm deep
		else if (i >= 92 && i <= 98)
			r._depth = (i - 90) / 20.0;
		else if (i == 99)
			r._comment = "runway not in use";
		else if (i == -1)					// no depth given ("//")
			;
		else
			return false;
	}
	i = -1;
	if (m[0] == '/' && m[1] == '/')
		m += 2;
	else if (!scanNumber(&m, &i, 2))
		return false;
	if (i >= 1 && i < 90) {
		r._friction = i / 100.0;
	} else if ((i >= 91 && i <= 95) || i == 99) {
		r._friction_string = runway_friction[i - 90];
	}
	if (!scanBoundary(&m))
		return false;

	_runways[id]._deposit = r._deposit;
	_runways[id]._deposit_string = r._deposit_string;
	_runways[id]._extent = r._extent;
	_runways[id]._extent_string = r._extent_string;
	_runways[id]._depth = r._depth;
	_runways[id]._friction = r._friction;
	_runways[id]._friction_string = r._friction_string;
	_runways[id]._comment = r._comment;
	_m = m;
	return true;
}


// WS (ALL RWYS?|RWY ?\d\d[LCR]?)?
bool SGMetar::scanWindShear()
{
	char *m = _m;
	if (strncmp(m, "WS", 2))
		return false;
	m += 2;
	if (!scanBoundary(&m))
		return false;

	if (!strncmp(m, "ALL", 3)) {
		m += 3;
		if (!scanBoundary(&m))
			return false;
		if (strncmp(m, "RWY", 3))
			return false;
		m += 3;
		if (*m == 'S')
			m++;
		if (!scanBoundary(&m))
			return false;
		_runways["ALL"]._wind_shear = true;
		_m = m;
		return true;
	}

	char id[4], *mm;
	int i, cnt;
	for (cnt = 0;; cnt++) {			// ??
		if (strncmp(m, "RWY", 3))
			break;
		m += 3;
		scanBoundary(&m);
		mm = m;
		if (!scanNumber(&m, &i, 2))
			return false;
		if (*m == 'L' || *m == 'C' || *m == 'R')
			m++;
		strncpy(id, mm, i = m - mm);
		id[i] = '\0';
		if (!scanBoundary(&m))
			return false;
		_runways[id]._wind_shear = true;
	}
	if (!cnt)
		_runways["ALL"]._wind_shear = true;
	_m = m;
	return true;
}


bool SGMetar::scanTrendForecast()
{
	char *m = _m;
	if (strncmp(m, "NOSIG", 5))
		return false;

	m += 5;
	if (!scanBoundary(&m))
		return false;
	_m = m;
	return true;
}


// (BLU|WHT|GRN|YLO|AMB|RED)
static const struct Token colors[] = {
	{ "BLU", "Blue" },      // 2500 ft,  8.0 km
	{ "WHT", "White" },     // 1500 ft,  5.0 km
	{ "GRN", "Green" },     //  700 ft,  3.7 km
	{ "YLO", "Yellow" },	//  300 ft,  1.6 km
	{ "AMB", "Amber" },     //  200 ft,  0.8 km
	{ "RED", "Red" },       // <200 ft, <0.8 km
	{ 0, 0 }
};


bool SGMetar::scanColorState()
{
	char *m = _m;
	const struct Token *a;
	if (!(a = scanToken(&m, colors)))
		return false;
	if (!scanBoundary(&m))
		return false;
	//printf(Y"Code %s\n"N, a->text);
	_m = m;
	return true;
}


bool SGMetar::scanRemark()
{
	if (strncmp(_m, "RMK", 3))
		return false;
	_m += 3;
	if (!scanBoundary(&_m))
		return false;

	while (*_m) {
		if (!scanRunwayReport()) {
			while (*_m && !isspace(*_m))
				_m++;
			scanBoundary(&_m);
		}
	}
	return true;
}


bool SGMetar::scanRemainder()
{
	char *m = _m;
	if (!(strncmp(m, "NOSIG", 5))) {
		m += 5;
		if (scanBoundary(&m))
			_m = m; //_comment.push_back("No significant tendency");
	}

	if (!scanBoundary(&m))
		return false;
	_m = m;
	return true;
}


bool SGMetar::scanBoundary(char **s)
{
	if (**s && !isspace(**s))
		return false;
	while (isspace(**s))
		(*s)++;
	return true;
}


int SGMetar::scanNumber(char **src, int *num, int min, int max)
{
	int i;
	char *s = *src;
	*num = 0;
	for (i = 0; i < min; i++) {
		if (!isdigit(*s))
			return 0;
		else
			*num = *num * 10 + *s++ - '0';
	}
	for (; i < max && isdigit(*s); i++)
		*num = *num * 10 + *s++ - '0';
	*src = s;
	return i;
}


// find longest match of str in list
const struct Token *SGMetar::scanToken(char **str, const struct Token *list)
{
	const struct Token *longest = 0;
	int maxlen = 0, len;
	const char *s;
	for (int i = 0; (s = list[i].id); i++) {
		len = strlen(s);
		if (!strncmp(s, *str, len) && len > maxlen) {
			maxlen = len;
			longest = &list[i];
		}
	}
	*str += maxlen;
	return longest;
}


void SGMetarCloud::set(double alt, Coverage cov)
{
	_altitude = alt;
	if (cov != -1)
		_coverage = cov;
}

SGMetarCloud::Coverage SGMetarCloud::getCoverage( const std::string & coverage ) 
{
	if( coverage == "clear" ) return COVERAGE_CLEAR;
	if( coverage == "few" ) return COVERAGE_FEW;
	if( coverage == "scattered" ) return COVERAGE_SCATTERED;
	if( coverage == "broken" ) return COVERAGE_BROKEN;
	if( coverage == "overcast" ) return COVERAGE_OVERCAST;
	return COVERAGE_NIL;
}

const char * SGMetarCloud::COVERAGE_NIL_STRING = "nil";
const char * SGMetarCloud::COVERAGE_CLEAR_STRING = "clear";
const char * SGMetarCloud::COVERAGE_FEW_STRING = "few";
const char * SGMetarCloud::COVERAGE_SCATTERED_STRING = "scattered";
const char * SGMetarCloud::COVERAGE_BROKEN_STRING = "broken";
const char * SGMetarCloud::COVERAGE_OVERCAST_STRING = "overcast";

void SGMetarVisibility::set(double dist, int dir, int mod, int tend)
{
	_distance = dist;
	if (dir != -1)
		_direction = dir;
	if (mod != -1)
		_modifier = mod;
	if (tend != 1)
		_tendency = tend;
}

#undef NaN
