// metar interface class
//
// Written by Melchior FRANZ, started December 2003.
//
// Copyright (C) 2003  Melchior FRANZ - mfranz@aon.at
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#ifndef _METAR_HXX
#define _METAR_HXX

#include <vector>
#include <map>
#include <string>

#include <simgear/constants.h>

struct Token {
	const char *id;
	const char *text;
};

const double SGMetarNaN = -1E20;

class SGMetar;

class SGMetarVisibility {
	friend class SGMetar;
public:
	SGMetarVisibility() :
		_distance(SGMetarNaN),
		_direction(-1),
		_modifier(EQUALS),
		_tendency(NONE) {}

	enum Modifier {
		NOGO,
		EQUALS,
		LESS_THAN,
		GREATER_THAN
	};

	enum Tendency {
		NONE,
		STABLE,
		INCREASING,
		DECREASING
	};

	void set(double dist, int dir = -1, int mod = -1, int tend = -1);

	inline double	getVisibility_m()	const { return _distance; }
	inline double	getVisibility_ft()	const { return _distance == SGMetarNaN ? SGMetarNaN : _distance * SG_METER_TO_FEET; }
	inline double	getVisibility_sm()	const { return _distance == SGMetarNaN ? SGMetarNaN : _distance * SG_METER_TO_SM; }
	inline int	getDirection()		const { return _direction; }
	inline int	getModifier()		const { return _modifier; }
	inline int	getTendency()		const { return _tendency; }

protected:
	double	_distance;
	int	_direction;
	int	_modifier;
	int	_tendency;
};


// runway condition (surface and visibility)
class SGMetarRunway {
	friend class SGMetar;
public:
	SGMetarRunway() :
		_deposit(-1),
		_deposit_string(0),
		_extent(-1),
		_extent_string(0),
		_depth(SGMetarNaN),
		_friction(SGMetarNaN),
		_friction_string(0),
		_comment(0),
		_wind_shear(false) {}

	inline int			getDeposit()		const { return _deposit; }
	inline const char		*getDepositString()	const { return _deposit_string; }
	inline double			getExtent()		const { return _extent; }
	inline const char		*getExtentString()	const { return _extent_string; }
	inline double			getDepth()		const { return _depth; }
	inline double			getFriction()		const { return _friction; }
	inline const char		*getFrictionString()	const { return _friction_string; }
	inline const char		*getComment()		const { return _comment; }
	inline       bool		getWindShear()		const { return _wind_shear; }
	inline const SGMetarVisibility&	getMinVisibility()	const { return _min_visibility; }
	inline const SGMetarVisibility&	getMaxVisibility()	const { return _max_visibility; }

protected:
	SGMetarVisibility _min_visibility;
	SGMetarVisibility _max_visibility;
	int		_deposit;
	const char	*_deposit_string;
	int		_extent;
	const char	*_extent_string;
	double		_depth;
	double		_friction;
	const char	*_friction_string;
	const char	*_comment;
	bool		_wind_shear;
};


// cloud layer
class SGMetarCloud {
	friend class SGMetar;
public:
	enum Coverage {
		COVERAGE_NIL = -1,
		COVERAGE_CLEAR = 0,
		COVERAGE_FEW = 1,
		COVERAGE_SCATTERED = 2,
		COVERAGE_BROKEN = 3,
		COVERAGE_OVERCAST = 4
	};

	static const char * COVERAGE_NIL_STRING;
	static const char * COVERAGE_CLEAR_STRING;
	static const char * COVERAGE_FEW_STRING;
	static const char * COVERAGE_SCATTERED_STRING;
	static const char * COVERAGE_BROKEN_STRING;
	static const char * COVERAGE_OVERCAST_STRING;

	SGMetarCloud() : _coverage(COVERAGE_NIL), _altitude(SGMetarNaN), _type(0), _type_long(0) {}

	void set(double alt, Coverage cov = COVERAGE_NIL );

	inline Coverage getCoverage() const { return _coverage; }
	static Coverage getCoverage( const std::string & coverage );
	inline double getAltitude_m() const { return _altitude; }
	inline double getAltitude_ft() const { return _altitude == SGMetarNaN ? SGMetarNaN : _altitude * SG_METER_TO_FEET; }
	inline const char *getTypeString() const { return _type; }
	inline const char *getTypeLongString() const { return _type_long; }

protected:
	Coverage _coverage;     // quarters: 0 -> clear ... 4 -> overcast
	double _altitude;       // 1000 m
	const char *_type;      // CU
	const char *_type_long; // cumulus
};


class SGMetar {
public:
	SGMetar(const std::string& m);
	~SGMetar();

	enum ReportType {
		NONE,
		AUTO,
		COR,
		RTD
	};

	enum Intensity {
		NIL = 0,
		LIGHT = 1,
		MODERATE = 2,
		HEAVY = 3
	};

	struct Weather {
		Weather() { intensity = NIL; vincinity = false; }
		Intensity intensity;
		bool      vincinity;
		std::vector<std::string> descriptions;
		std::vector<std::string> phenomena;
	};

	inline const char *getData()		const { return _data; }
	inline const char *getUnusedData()	const { return _m; }
	inline       bool getProxy()		const { return _x_proxy; }
	inline const char *getId()		const { return _icao; }
	inline int	getYear()		const { return _year; }
	inline int	getMonth()		const { return _month; }
	inline int	getDay()		const { return _day; }
	inline int	getHour()		const { return _hour; }
	inline int	getMinute()		const { return _minute; }
	inline int	getReportType()		const { return _report_type; }

	inline int	getWindDir()		const { return _wind_dir; }
	inline double	getWindSpeed_mps()	const { return _wind_speed; }
	inline double	getWindSpeed_kmh()	const { return _wind_speed == SGMetarNaN ? SGMetarNaN : _wind_speed * SG_MPS_TO_KMH; }
	inline double	getWindSpeed_kt()	const { return _wind_speed == SGMetarNaN ? SGMetarNaN : _wind_speed * SG_MPS_TO_KT; }
	inline double	getWindSpeed_mph()	const { return _wind_speed == SGMetarNaN ? SGMetarNaN : _wind_speed * SG_MPS_TO_MPH; }

	inline double	getGustSpeed_mps()	const { return _gust_speed; }
	inline double	getGustSpeed_kmh()	const { return _gust_speed == SGMetarNaN ? SGMetarNaN : _gust_speed * SG_MPS_TO_KMH; }
	inline double	getGustSpeed_kt()	const { return _gust_speed == SGMetarNaN ? SGMetarNaN : _gust_speed * SG_MPS_TO_KT; }
	inline double	getGustSpeed_mph()	const { return _gust_speed == SGMetarNaN ? SGMetarNaN : _gust_speed * SG_MPS_TO_MPH; }

	inline int	getWindRangeFrom()	const { return _wind_range_from; }
	inline int	getWindRangeTo()	const { return _wind_range_to; }

	inline const SGMetarVisibility& getMinVisibility()	const { return _min_visibility; }
	inline const SGMetarVisibility& getMaxVisibility()	const { return _max_visibility; }
	inline const SGMetarVisibility& getVertVisibility()	const { return _vert_visibility; }
	inline const SGMetarVisibility *getDirVisibility()	const { return _dir_visibility; }

	inline double	getTemperature_C()	const { return _temp; }
	inline double	getTemperature_F()	const { return _temp == SGMetarNaN ? SGMetarNaN : 1.8 * _temp + 32; }
	inline double	getDewpoint_C()		const { return _dewp; }
	inline double	getDewpoint_F()		const { return _dewp == SGMetarNaN ? SGMetarNaN : 1.8 * _dewp + 32; }
	inline double	getPressure_hPa()	const { return _pressure == SGMetarNaN ? SGMetarNaN : _pressure / 100; }
	inline double	getPressure_inHg()	const { return _pressure == SGMetarNaN ? SGMetarNaN : _pressure * SG_PA_TO_INHG; }

	inline int	getRain()		const { return _rain; }
	inline int	getHail()		const { return _hail; }
	inline int	getSnow()		const { return _snow; }
	inline bool	getCAVOK()		const { return _cavok; }

	double		getRelHumidity()	const;

	inline const std::vector<SGMetarCloud>& getClouds()	const	{ return _clouds; }
	inline const std::map<std::string, SGMetarRunway>& getRunways()	const	{ return _runways; }
	inline const std::vector<std::string>& getWeather()		const	{ return _weather; }
	inline const std::vector<struct Weather> getWeather2()	const   { return _weather2; }

protected:
	std::string	_url;
	int	_grpcount;
	bool	_x_proxy;
	char	*_data;
	char	*_m;
	char	_icao[5];
	int	_year;
	int	_month;
	int	_day;
	int	_hour;
	int	_minute;
	int	_report_type;
	int	_wind_dir;
	double	_wind_speed;
	double	_gust_speed;
	int	_wind_range_from;
	int	_wind_range_to;
	double	_temp;
	double	_dewp;
	double	_pressure;
	int	_rain;
	int	_hail;
	int	_snow;
	bool	_cavok;
	std::vector<struct Weather> _weather2;

	SGMetarVisibility		_min_visibility;
	SGMetarVisibility		_max_visibility;
	SGMetarVisibility		_vert_visibility;
	SGMetarVisibility		_dir_visibility[8];
	std::vector<SGMetarCloud>		_clouds;
	std::map<std::string, SGMetarRunway>	_runways;
	std::vector<std::string>			_weather;

	bool	scanPreambleDate();
	bool	scanPreambleTime();
	void	useCurrentDate();

	bool	scanType();
	bool	scanId();
	bool	scanDate();
	bool	scanModifier();
	bool	scanWind();
	bool	scanVariability();
	bool	scanVisibility();
	bool	scanRwyVisRange();
	bool	scanSkyCondition();
	bool	scanWeather();
	bool	scanTemperature();
	bool	scanPressure();
	bool	scanRunwayReport();
	bool	scanWindShear();
	bool	scanTrendForecast();
	bool	scanColorState();
	bool	scanRemark();
	bool	scanRemainder();

	int	scanNumber(char **str, int *num, int min, int max = 0);
	bool	scanBoundary(char **str);
	const struct Token *scanToken(char **str, const struct Token *list);
	void	normalizeData();
};

#endif // _METAR_HXX
