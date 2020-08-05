// String utilities.
//
// Written by Bernie Bright, started 1998
//
// Copyright (C) 1998  Bernie Bright - bbright@bigpond.net.au
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#include <simgear_config.h>

#include <string>
#include <sstream>
#include <algorithm>
#include <type_traits>
#include <cstring>              // strerror_r() and strerror_s()
#include <cctype>
#include <cerrno>
#include <cassert>

#include "strutils.hxx"

#include <simgear/debug/logstream.hxx>
#include <simgear/package/md5.h>
#include <simgear/compiler.h>   // SG_WINDOWS
#include <simgear/structure/exception.hxx>
#include <simgear/math/SGGeod.hxx>

#if defined(SG_WINDOWS)
	#include <windows.h>
    #include <codecvt>
    #include <locale>    
#endif

using std::string;
using std::vector;
using std::stringstream;

namespace simgear {
    namespace strutils {

	/*
	 * utf8ToLatin1() convert utf8 to latin, useful for accent character (i.e éâàîè...)
	 */
	template <typename Iterator> size_t get_length (Iterator p) {
		unsigned char c = static_cast<unsigned char> (*p);
		if (c < 0x80) return 1;
		else if (!(c & 0x20)) return 2;
		else if (!(c & 0x10)) return 3;
		else if (!(c & 0x08)) return 4;
		else if (!(c & 0x04)) return 5;
		else return 6;
	}

	typedef unsigned int value_type;
	template <typename Iterator> value_type get_value (Iterator p) {
		size_t len = get_length (p);
		if (len == 1) return *p;
		value_type res = static_cast<unsigned char> ( *p & (0xff >> (len + 1))) << ((len - 1) * 6 );
		for (--len; len; --len) {
		        value_type next_byte = static_cast<unsigned char> (*(++p)) - 0x80;
		        if (next_byte & 0xC0) return 0x00ffffff; // invalid UTF-8
			res |= next_byte << ((len - 1) * 6);
			}
		return res;
	}

	string utf8ToLatin1( string& s_utf8 ) {
		string s_latin1;
		for (string::iterator p = s_utf8.begin(); p != s_utf8.end(); ++p) {
			value_type value = get_value<string::iterator&>(p);
			if (value > 0x10ffff) return s_utf8; // invalid UTF-8: guess that the input was already Latin-1
			if (value > 0xff) SG_LOG(SG_IO, SG_WARN, "utf8ToLatin1: wrong char value: " << value);
			s_latin1 += static_cast<char>(value);
		}
		return s_latin1;
	}

	/**
	 *
	 */
	static vector<string>
	split_whitespace( const string& str, int maxsplit )
	{
	    vector<string> result;
	    string::size_type len = str.length();
	    string::size_type i = 0;
	    string::size_type j;
	    int countsplit = 0;

	    while (i < len)
	    {
		while (i < len && isspace((unsigned char)str[i]))
		{
		    ++i;
		}

		j = i;

		while (i < len && !isspace((unsigned char)str[i]))
		{
		    ++i;
		}

		if (j < i)
		{
		    result.push_back( str.substr(j, i-j) );
		    ++countsplit;
		    while (i < len && isspace((unsigned char)str[i]))
		    {
			++i;
		    }

		    if (maxsplit && (countsplit >= maxsplit) && i < len)
		    {
			result.push_back( str.substr( i, len-i ) );
			i = len;
		    }
		}
	    }

	    return result;
	}

	/**
	 *
	 */
	vector<string>
	split( const string& str, const char* sep, int maxsplit )
	{
	    if (sep == 0)
		return split_whitespace( str, maxsplit );

	    vector<string> result;
	    int n = std::strlen( sep );
	    if (n == 0)
	    {
		// Error: empty separator string
		return result;
	    }
	    const char* s = str.c_str();
	    string::size_type len = str.length();
	    string::size_type i = 0;
	    string::size_type j = 0;
	    int splitcount = 0;

	    while (i+n <= len)
	    {
		if (s[i] == sep[0] && (n == 1 || std::memcmp(s+i, sep, n) == 0))
		{
		    result.push_back( str.substr(j,i-j) );
		    i = j = i + n;
		    ++splitcount;
		    if (maxsplit && (splitcount >= maxsplit))
			break;
		}
		else
		{
		    ++i;
		}
	    }

	    result.push_back( str.substr(j,len-j) );
	    return result;
	}


        string_list split_on_any_of(const std::string& str, const char* seperators)
        {
            if (seperators == nullptr || (strlen(seperators) == 0)) {
                throw sg_exception("illegal/missing seperator string");
            }

            string_list result;
            size_t pos = 0;
            size_t startPos = str.find_first_not_of(seperators, 0);
            for(;;)
            {
                pos = str.find_first_of(seperators, startPos);
                if (pos == string::npos) {
                    result.push_back(str.substr(startPos));
                    break;
                }
                result.push_back(str.substr(startPos, pos - startPos));
                startPos = str.find_first_not_of(seperators, pos);
                if (startPos == string::npos) {
                    break;
                }
            }
            return result;

        }

	/**
	 * The lstrip(), rstrip() and strip() functions are implemented
	 * in do_strip() which uses an additional parameter to indicate what
	 * type of strip should occur.
	 */
	const int LEFTSTRIP = 0;
	const int RIGHTSTRIP = 1;
	const int BOTHSTRIP = 2;

	static string
	do_strip( const string& s, int striptype )
	{
	    string::size_type len = s.length();
	    if( len == 0 ) // empty string is trivial
		return s;
	    string::size_type i = 0;
	    if (striptype != RIGHTSTRIP)
	    {
		while (i < len && isspace(s[i]))
		{
		    ++i;
		}
	    }

	    string::size_type j = len;
	    if (striptype != LEFTSTRIP)
	    {
		do
		{
		    --j;
		}
		while (j >= 1 && isspace(s[j]));
		++j;
	    }

	    if (i == 0 && j == len)
	    {
		return s;
	    }
	    else
	    {
		return s.substr( i, j - i );
	    }
	}

	string
	lstrip( const string& s )
	{
	    return do_strip( s, LEFTSTRIP );
	}

	string
	rstrip( const string& s )
	{
	    return do_strip( s, RIGHTSTRIP );
	}

	string
	strip( const string& s )
	{
	    return do_strip( s, BOTHSTRIP );
	}

    string makeStringSafeForPropertyName(const std::string& str)
    {
        // This function replaces all characters in 'str' that are not
        // alphanumeric or '-'.
        // TODO: make the function multibyte safe.
        string res = str;

        int index = 0;
        for (char& c : res) {
            if (!std::isalpha(c) && !std::isdigit(c) && c != '-') {
                switch (c) {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                case '_':
                case '.':
                case '/':
                case '\\':
                    res[index] = '-';
                    break;
                default:
                    res[index] = '_';
                    SG_LOG(SG_GENERAL, SG_WARN, "makeStringSafeForPropertyName: Modified '" << str << "' to '" << res << "'");
                }
            }
            index++;
        }

        return res;
    }

        void
        stripTrailingNewlines_inplace(string& s)
        {
          // The following (harder to read) implementation is much slower on
          // my system (g++ 6.2.1 on Debian): 11.4 vs. 3.9 seconds on
          // 50,000,000 iterations performed on a short CRLF-terminated
          // string---and it is even a bit slower (12.9 seconds) with
          // std::next(it) instead of (it+1).
          //
          // for (string::reverse_iterator it = s.rbegin();
          //      it != s.rend() && (*it == '\r' || *it == '\n'); /* empty */) {
          //   it = string::reverse_iterator(s.erase( (it+1).base() ));
          // }

          // Simple and fast
          while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) {
            s.pop_back();
          }
        }

        string
        stripTrailingNewlines(const string& s)
        {
          string res = s;
          stripTrailingNewlines_inplace(res);

          return res;
        }

	string
	rpad( const string & s, string::size_type length, char c )
	{
	    string::size_type l = s.length();
	    if( l >= length ) return s;
	    string reply = s;
	    return reply.append( length-l, c );
	}

	string
	lpad( const string & s, size_t length, char c )
	{
	    string::size_type l = s.length();
	    if( l >= length ) return s;
	    string reply = s;
	    return reply.insert( 0, length-l, c );
	}

	bool
	starts_with( const string & s, const string & substr )
	{
	  return s.compare(0, substr.length(), substr) == 0;
	}

	bool
	ends_with( const string & s, const string & substr )
	{
	  if( substr.length() > s.length() )
	    return false;
	  return s.compare( s.length() - substr.length(),
	                    substr.length(),
	                    substr ) == 0;
	}

    string simplify(const string& s)
    {
        string result; // reserve size of 's'?
        string::const_iterator it = s.begin(),
            end = s.end();

    // advance to first non-space char - simplifes logic in main loop,
    // since we can always prepend a single space when we see a
    // space -> non-space transition
        for (; (it != end) && isspace(*it); ++it) { /* nothing */ }

        bool lastWasSpace = false;
        for (; it != end; ++it) {
            char c = *it;
            if (isspace(c)) {
                lastWasSpace = true;
                continue;
            }

            if (lastWasSpace) {
                result.push_back(' ');
            }

            lastWasSpace = false;
            result.push_back(c);
        }

        return result;
    }

    int to_int(const std::string& s, int base)
    {
        stringstream ss(s);
        switch (base) {
        case 8:      ss >> std::oct; break;
        case 16:     ss >> std::hex; break;
        default: break;
        }

        int result;
        ss >> result;
        return result;
    }

    template<>
    int digitValue<10>(char c)
    {
        if ('0' <= c && c <= '9') {
            return static_cast<int>(c - '0');
        } else {
            throw sg_range_exception("invalid as a decimal digit: '" +
                                     std::string(1, c) + "'");
        }
    }

    template<>
    int digitValue<16>(char c)
    {
        if ('0' <= c && c <= '9') {
            return static_cast<int>(c - '0');
        } else if ('a' <= c && c <= 'f') {
            return 10 + static_cast<int>(c - 'a');
        } else if ('A' <= c && c <= 'F') {
            return 10 + static_cast<int>(c - 'A');
        } else {
            throw sg_range_exception("invalid as an hexadecimal digit: '" +
                                     std::string(1, c) + "'");
        }
    }

    template<>
    std::string numerationBaseAdjective<10>()
    { return std::string("decimal"); }

    template<>
    std::string numerationBaseAdjective<16>()
    { return std::string("hexadecimal"); }

    template<class T, int BASE, typename>
    T readNonNegativeInt(const std::string& s)
    {
        static_assert(0 < BASE,
                      "template value BASE must be a positive integer");
        static_assert(BASE <= std::numeric_limits<T>::max(),
                      "template type T too small: it cannot represent BASE");
        T res(0);
        T multiplier(1);
        T increment;
        int digit;

        if (s.empty()) {
            throw sg_format_exception("expected a non-empty string", s);
        }

        for (auto it = s.crbegin(); it != s.crend(); it++) {
            if (it != s.crbegin()) {
                // Check if 'multiplier *= BASE' is going to overflow. This is
                // reliable because 'multiplier' and 'BASE' are positive.
                if (multiplier > std::numeric_limits<T>::max() / BASE) {
                    // If all remaining digits are '0', it doesn't matter that
                    // the multiplier overflows.
                    if (std::all_of(it, s.crend(),
                                    [](char c){ return (c == '0'); })) {
                        return res;
                    } else {
                        throw sg_range_exception(
                            "doesn't fit in the specified type: '" + s + "'");
                    }
                }
                multiplier *= BASE;
            }

            try {
                digit = digitValue<BASE>(*it);
            } catch (const sg_range_exception&) {
                throw sg_format_exception(
                    "expected a string containing " +
                    numerationBaseAdjective<BASE>() +
                    " digits only, but got '" + s + "'", s);
            }

            // Reliable because 'multiplier' is positive
            if (digit > 0 &&
                multiplier > std::numeric_limits<T>::max() / digit) {
                throw sg_range_exception(
                    "doesn't fit in the specified type: '" + s + "'");
            }
            increment = multiplier*digit;

            if (res > std::numeric_limits<T>::max() - increment) {
                throw sg_range_exception(
                    "doesn't fit in the specified type: '" + s + "'");
            }
            res += increment;
        }

        return res;
    }

    // Explicit template instantiations.
    //
    // In order to save some bytes for the SimGearCore library[*], we only
    // instantiate a small number of variants of readNonNegativeInt() below.
    // Just enable the ones you need if they are disabled.
    //
    // [*] The exact amount depends a lot on what you measure and in which
    //     circumstances. On Linux amd64 with g++, I measured a cost ranging
    //     from 2 KB per template in a Release build to 19 KB per template in
    //     a RelWithDebInfo build for the in-memory code size of the resulting
    //     fgfs binary (CODE column in 'top', after selecting a suitable
    //     unit). If I look at the fgfs binary size (statically-linked with
    //     SimGear), I measure from 2 KB per template (Release) to 30 KB per
    //     template (RelWithDebInfo). Finally, a Debug build compiled with
    //     '-fno-omit-frame-pointer -O0 -fno-inline' lies between the Release
    //     and the RelWithDebInfo builds.

#if 0
    template
    signed char readNonNegativeInt<signed char, 10>(const std::string& s);
    template
    signed char readNonNegativeInt<signed char, 16>(const std::string& s);
    template
    unsigned char readNonNegativeInt<unsigned char, 10>(const std::string& s);
    template
    unsigned char readNonNegativeInt<unsigned char, 16>(const std::string& s);

    template
    short readNonNegativeInt<short, 10>(const std::string& s);
    template
    short readNonNegativeInt<short, 16>(const std::string& s);
    template
    unsigned short readNonNegativeInt<unsigned short, 10>(const std::string& s);
    template
    unsigned short readNonNegativeInt<unsigned short, 16>(const std::string& s);
#endif

    template
    int readNonNegativeInt<int, 10>(const std::string& s);
    template
    unsigned int readNonNegativeInt<unsigned int, 10>(const std::string& s);

#if 0
    template
    int readNonNegativeInt<int, 16>(const std::string& s);
    template
    unsigned int readNonNegativeInt<unsigned int, 16>(const std::string& s);

    template
    long readNonNegativeInt<long, 10>(const std::string& s);
    template
    long readNonNegativeInt<long, 16>(const std::string& s);
    template
    unsigned long readNonNegativeInt<unsigned long, 10>(const std::string& s);
    template
    unsigned long readNonNegativeInt<unsigned long, 16>(const std::string& s);

    template
    long long readNonNegativeInt<long long, 10>(const std::string& s);
    template
    long long readNonNegativeInt<long long, 16>(const std::string& s);
    template
    unsigned long long readNonNegativeInt<unsigned long long, 10>(
        const std::string& s);
    template
    unsigned long long readNonNegativeInt<unsigned long long, 16>(
        const std::string& s);
#endif
        
        // parse a time string ([+/-]%f[:%f[:%f]]) into hours
        double readTime(const string& time_in)
        {
            if (time_in.empty()) {
                return 0.0;
            }
            
            const bool negativeSign = time_in.front() == '-';
            const string_list pieces = split(time_in, ":");
            if (pieces.size() > 3) {
                throw sg_format_exception("Unable to parse time string, too many pieces", time_in);
            }
            
            const int hours = std::abs(to_int(pieces.front()));
            int minutes = 0, seconds = 0;
            if (pieces.size() > 1) {
                minutes = to_int(pieces.at(1));
                if (pieces.size() > 2) {
                    seconds = to_int(pieces.at(2));
                }
            }
        
            double result = hours + (minutes / 60.0) + (seconds / 3600.0);
            return negativeSign ? -result : result;
        }


    int compare_versions(const string& v1, const string& v2, int maxComponents)
    {
        vector<string> v1parts(split(v1, "."));
        vector<string> v2parts(split(v2, "."));

        int lastPart = std::min(v1parts.size(), v2parts.size());
        if (maxComponents > 0) {
            lastPart = std::min(lastPart, maxComponents);
        }

        for (int part=0; part < lastPart; ++part) {
            int part1 = to_int(v1parts[part]);
            int part2 = to_int(v2parts[part]);

            if (part1 != part2) {
                return part1 - part2;
            }
        } // of parts iteration

        // reached end - longer wins
        return v1parts.size() - v2parts.size();
    }
    
    bool compareVersionToWildcard(const std::string& aVersion, const std::string& aCandidate)
    {
        if (aCandidate == aVersion) {
            return true;
        }

        // examine each dot-seperated component in turn, supporting a wildcard
        // in the versions from the catalog.
        string_list parts = split(aVersion, ".");
        string_list candidateParts = split(aCandidate, ".");

        const size_t partCount = parts.size();
        const size_t candidatePartCount =  candidateParts.size();
        
        bool previousCandidatePartWasWildcard = false;

        for (unsigned int p=0; p < partCount; ++p) {
            // candidate string is too short, can match if it ended with wildcard
            // part. This allows candidate '2016.*' to match '2016.1.2' and so on
            if (candidatePartCount <= p) {
                return previousCandidatePartWasWildcard;
            }

            if (candidateParts.at(p) == "*") {
                // always passes
                previousCandidatePartWasWildcard = true;
            } else if (parts.at(p) != candidateParts.at(p)) {
                return false;
            }
        }

        return true;
    }

    string join(const string_list& l, const string& joinWith)
    {
        string result;
        unsigned int count = l.size();
        for (unsigned int i=0; i < count; ++i) {
            result += l[i];
            if (i < (count - 1)) {
                result += joinWith;
            }
        }

        return result;
    }

    string uppercase(const string &s) {
      string rslt(s);
      for(string::iterator p = rslt.begin(); p != rslt.end(); p++){
        *p = toupper(*p);
      }
      return rslt;
    }

    string lowercase(const string &s) {
      string rslt(s);
      for(string::iterator p = rslt.begin(); p != rslt.end(); p++){
        *p = tolower(*p);
      }
      return rslt;
    }

    void lowercase(string &s) {
      for(string::iterator p = s.begin(); p != s.end(); p++){
        *p = tolower(*p);
      }
    }
        
        
bool iequals(const std::string& a, const std::string& b)
{
    const auto lenA = a.length();
    const auto lenB = b.length();
    if (lenA != lenB) return false;
    
    const char* aPtr = a.data();
    const char* bPtr = b.data();
    for (size_t i = 0; i < lenA; ++i) {
        if (tolower(*aPtr++) != tolower(*bPtr++)) return false;
    }
    
    return true;
}

#if defined(SG_WINDOWS)
static std::wstring convertMultiByteToWString(DWORD encoding, const std::string& a)
{
    std::vector<wchar_t> result;
    DWORD flags = 0;
    int requiredWideChars = MultiByteToWideChar(encoding, flags,
                        a.c_str(), a.size(),
                        NULL, 0);
    result.resize(requiredWideChars);
    MultiByteToWideChar(encoding, flags, a.c_str(), a.size(),
                        result.data(), result.size());
	return std::wstring(result.data(), result.size());
}

static std::string convertWStringToMultiByte(DWORD encoding, const std::wstring& w)
{
	std::vector<char> result;
	DWORD flags = 0;
	int requiredMBChars = WideCharToMultiByte(encoding, flags,
		w.data(), w.size(),
		NULL, 0, NULL, NULL);
	result.resize(requiredMBChars);
	WideCharToMultiByte(encoding, flags,
		w.data(), w.size(),
		result.data(), result.size(), NULL, NULL);
	return std::string(result.data(), result.size());
}
#endif

std::wstring convertUtf8ToWString(const std::string& a)
{
#if defined(SG_WINDOWS)
    return convertMultiByteToWString(CP_UTF8, a);
#else
    assert(sizeof(wchar_t) == 4);
    std::wstring result;
    int expectedContinuationCount = 0;
    wchar_t wc = 0;
    
    for (uint8_t utf8CodePoint : a) {
        // ASCII 7-bit range
        if (utf8CodePoint <= 0x7f) {
            if (expectedContinuationCount != 0) {
                throw sg_format_exception();
            }
            
            result.push_back(static_cast<wchar_t>(utf8CodePoint));
        } else if (expectedContinuationCount > 0) {
            if ((utf8CodePoint & 0xC0) != 0x80) {
                throw sg_format_exception();
            }
            
            wc = (wc << 6) | (utf8CodePoint & 0x3F);
            if (--expectedContinuationCount == 0) {
                result.push_back(wc);
            }
        } else {
            if ((utf8CodePoint & 0xE0) == 0xC0) {
                expectedContinuationCount = 1;
                wc = utf8CodePoint & 0x1f;
            } else if ((utf8CodePoint & 0xF0) == 0xE0) {
                expectedContinuationCount = 2;
                wc = utf8CodePoint & 0x0f;
            } else if ((utf8CodePoint & 0xF8) == 0xF0) {
                expectedContinuationCount = 3;
                wc =utf8CodePoint & 0x07;
            } else {
                // illegal UTF-8 encoding
                throw sg_format_exception();
            }
        }
    } // of UTF-8 code point iteration
    
    return result;
    
#endif

}

std::string convertWStringToUtf8(const std::wstring& w)
{
#if defined(SG_WINDOWS)
    return convertWStringToMultiByte(CP_UTF8, w);
#else
    assert(sizeof(wchar_t) == 4);
    std::string result;
    
    for (wchar_t cp : w) {
        if (cp <= 0x7f) {
            result.push_back(static_cast<uint8_t>(cp));
        } else if (cp <= 0x07ff) {
            result.push_back(0xC0 | ((cp >> 6) & 0x1f));
            result.push_back(0x80 | (cp & 0x3f));
        } else if (cp <= 0xffff) {
            result.push_back(0xE0 | ((cp >> 12) & 0x0f));
            result.push_back(0x80 | ((cp >> 6) & 0x3f));
            result.push_back(0x80 | (cp & 0x3f));
        } else if (cp < 0x10ffff) {
            result.push_back(0xF0 | ((cp >> 18) & 0x07));
            result.push_back(0x80 | ((cp >> 12) & 0x3f));
            result.push_back(0x80 | ((cp >> 6) & 0x3f));
            result.push_back(0x80 | (cp & 0x3f));
        } else {
            throw sg_format_exception();
        }
    }
    
    return result;
#endif
}

std::string convertWindowsLocal8BitToUtf8(const std::string& a)
{
#ifdef SG_WINDOWS
	return convertWStringToMultiByte(CP_UTF8, convertMultiByteToWString(CP_ACP, a));
#else
    return a;
#endif
}

std::string convertUtf8ToWindowsLocal8Bit(const std::string& a)
{
#ifdef SG_WINDOWS
	return convertWStringToMultiByte(CP_ACP, convertMultiByteToWString(CP_UTF8, a));
#else
    return a;
#endif
}

//------------------------------------------------------------------------------
std::string md5(const unsigned char* data, size_t num)
{
  SG_MD5_CTX md5_ctx;
  SG_MD5Init(&md5_ctx);
  SG_MD5Update(&md5_ctx, data, num);

  unsigned char digest[MD5_DIGEST_LENGTH];
  SG_MD5Final(digest, &md5_ctx);

  return encodeHex(digest, MD5_DIGEST_LENGTH);
}

//------------------------------------------------------------------------------
std::string md5(const char* data, size_t num)
{
  return md5(reinterpret_cast<const unsigned char*>(data), num);
}

//------------------------------------------------------------------------------
std::string md5(const std::string& str)
{
  return md5(reinterpret_cast<const unsigned char*>(str.c_str()), str.size());
}

//------------------------------------------------------------------------------
static const std::string base64_chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

static const unsigned char base64_decode_map[128] =
{
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127,  62, 127, 127, 127,  63,  52,  53,
    54,  55,  56,  57,  58,  59,  60,  61, 127, 127,
    127,  64, 127, 127, 127,   0,   1,   2,   3,   4,
    5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
    25, 127, 127, 127, 127, 127, 127,  26,  27,  28,
    29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
    39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51, 127, 127, 127, 127, 127
};


static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

static bool is_whitespace(unsigned char c) {
    return ((c == ' ') || (c == '\r') || (c == '\n'));
}

void decodeBase64(const std::string& encoded_string, std::vector<unsigned char>& ret)
{
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];

  while (in_len-- && ( encoded_string[in_] != '=')) {
    if (is_whitespace( encoded_string[in_])) {
        in_++;
        continue;
    }

    if (!is_base64(encoded_string[in_])) {
        break;
    }

    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_decode_map[char_array_4[i]];

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret.push_back(char_array_3[i]);
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_decode_map[char_array_4[j]];

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
  }
}

//------------------------------------------------------------------------------
const char hexChar[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

std::string encodeHex(const std::string& bytes)
{
  return encodeHex(
    reinterpret_cast<const unsigned char*>(bytes.c_str()),
    bytes.size()
  );
}

std::string encodeHex(const unsigned char* rawBytes, unsigned int length)
{
  std::string hex(length * 2, '\0');
  for (unsigned int i=0; i<length;++i) {
      unsigned char c = *rawBytes++;
      hex[i * 2] = hexChar[c >> 4];
      hex[i * 2 + 1] = hexChar[c & 0x0f];
  }

  return hex;
}

std::vector<uint8_t> decodeHex(const std::string& input)
{
    std::vector<uint8_t> result;
    char* ptr = const_cast<char*>(input.data());
    const char* end = ptr + input.length();
    
    bool highNibble = true;
    uint8_t b = 0;
    
    while (ptr != end) {
        const char c = *ptr;
        char val = 0;
        
        if (c == '0') {
            val = 0;
            if ((ptr + 1) < end) {
                const auto peek = *(ptr + 1);
                if (peek == 'x') {
                    // tolerate 0x prefixing
                    highNibble = true;
                    ptr += 2; // skip both bytes
                    continue;
                }
            }
        } else if (isdigit(c)) {
            val =  c - '0';
        } else if ((c >= 'A') && (c <= 'F')) {
            val = c - 'A' + 10;
        } else if ((c >= 'a') && (c <= 'f')) {
            val = c - 'a' + 10;
        } else {
            // any other input: newline, space, tab, comma...
            if (!highNibble) {
                // allow a single digit to work, if we have spacing
                highNibble = true;
                result.push_back(b >> 4);
            }
            
            ++ptr;
            continue;
        }
        
        if (highNibble) {
            highNibble = false;
            b = val << 4;
        } else {
            highNibble = true;
            b |= val;
            result.push_back(b);
        }
        
        ++ptr;
    }
    
    // watch for trailing single digit
    // this is reqquired so a stirng ending in 0x3 is decoded.
    if (!highNibble) {
        result.push_back(b >> 4);
    }
    
    return result;
}
    
// Write an octal backslash-escaped respresentation of 'val' to 'buf'.
//
// At least 4 write positions must be available at 'buf'. The result is *not*
// null-terminated. Only the 8 least significant bits of 'val' are used;
// higher-order bits have no influence on the chars written to 'buf'.
static void writeOctalBackslashEscapedRepr(char *buf, unsigned char val)
{
  buf[0] = '\\';
  buf[1] = '0' + ((val >> 6) & 3); // 2 bits
  buf[2] = '0' + ((val >> 3) & 7); // 3 bits
  buf[3] = '0' + (val & 7);        // 3 bits
}

// Backslash-escape a string for C/C++ string literal syntax.
std::string escape(const std::string& s) {
  string res;
  char buf[4];

  for (const char c: s) {
    // We don't really *need* to special-case \a, \b, \f, \n, \r, \t and \v,
    // because they could be handled like the other non-ASCII or non-printable
    // characters. However, doing so will make the output string both shorter
    // and more readable.
    if (c == '\a') {
      res += "\\a";
    } else if (c == '\b') {
      res += "\\b";
    } else if (c == '\f') {
      res += "\\f";
    } else if (c == '\n') {
      res += "\\n";
    } else if (c == '\r') {
      res += "\\r";
    } else if (c == '\t') {
      res += "\\t";
    } else if (c == '\v') {
      res += "\\v";
    } else if (c < 0x20 || c > 0x7e) { // non-ASCII or non-printable character
      // This is fast (no memory allocation nor IOStreams needed)
      writeOctalBackslashEscapedRepr(buf, static_cast<unsigned char>(c));
      res.append(buf, 4);
    } else if (c == '\\') {
      res += "\\\\";
    } else if (c == '"') {
      res += "\\\"";
    } else {
      res += c;
    }
  }

  return res;
}

//------------------------------------------------------------------------------
std::string unescape(const char* s)
{
  std::string r;
  while( *s )
  {
    if( *s != '\\' )
    {
      r += *s++;
      continue;
    }

    if( !*++s )
      break;

    if (*s == '\\') {
        r += '\\';
    } else if (*s == 'n') {
        r += '\n';
    } else if (*s == 'r') {
        r += '\r';
    } else if (*s == 't') {
        r += '\t';
    } else if (*s == 'v') {
        r += '\v';
    } else if (*s == 'f') {
        r += '\f';
    } else if (*s == 'a') {
        r += '\a';
    } else if (*s == 'b') {
        r += '\b';
    } else if (*s == 'x') {
        if (!*++s)
            break;
        int v = 0;
        for (/* empty */; isxdigit(*s); s++) {
            v = v * 16 + (isdigit(*s) ? *s - '0' : 10 + tolower(*s) - 'a');
        }
        r += static_cast<char>(v);
        continue;

    } else if (*s >= '0' && *s <= '7') {
        int v = *s++ - '0';
        for (int i = 0; i < 2 && *s >= '0' && *s <= '7'; i++, s++)
            v = v * 8 + *s - '0';
        r += static_cast<char>(v);
        continue;

    } else {
        r += *s;
    }
    s++;
  }
  return r;
}
std::string replace(std::string source, const std::string search, const std::string replacement, std::size_t start_pos)
{
    if (start_pos < source.length()) {
        while ((start_pos = source.find(search, start_pos)) != std::string::npos) {
            source.replace(start_pos, search.length(), replacement);
            start_pos += replacement.length();
        }
    }
    return source;
}

string sanitizePrintfFormat(const string& input)
{
    string::size_type i = input.find("%n");
    if (i != string::npos) {
        SG_LOG(SG_IO, SG_WARN, "sanitizePrintfFormat: bad format string:" << input);
        return string();
    }

    return input;
}

std::string error_string(int errnum)
{
  char buf[512];                // somewhat arbitrary...
  // This could be simplified with C11 (annex K, optional...), which offers:
  //
  //   errno_t strerror_s( char *buf, rsize_t bufsz, errno_t errnum );
  //   size_t strerrorlen_s( errno_t errnum );

#if defined(_WIN32)
  errno_t retcode;
  // Always makes the string in 'buf' null-terminated
  retcode = strerror_s(buf, sizeof(buf), errnum);
#elif defined(_GNU_SOURCE)
  return std::string(strerror_r(errnum, buf, sizeof(buf)));
#elif (_POSIX_C_SOURCE >= 200112L) || defined(SG_MAC) || defined(__FreeBSD__) || defined(__OpenBSD__)
  int retcode;
  // POSIX.1-2001 and POSIX.1-2008
  retcode = strerror_r(errnum, buf, sizeof(buf));
#else
#error "Could not find a thread-safe alternative to strerror()."
#endif

#if !defined(_GNU_SOURCE)
  if (retcode) {
    std::string msg = "unable to get error message for a given error number";
    // C++11 would make this shorter with std::to_string()
    std::ostringstream ostr;
    ostr << errnum;

#if !defined(_WIN32)
    if (retcode == ERANGE) {    // more specific error message in this case
      msg = std::string("buffer too small to hold the error message for "
                        "the specified error number");
    }
#endif

    throw sg_error(msg, ostr.str());
  }

  return std::string(buf);
#endif  // !defined(_GNU_SOURCE)
}

bool to_bool(const std::string& s)
{
    if (!strcasecmp(s.c_str(), "yes")) return true;
    if (!strcasecmp(s.c_str(), "no")) return false;
    if (!strcasecmp(s.c_str(), "true")) return true;
    if (!strcasecmp(s.c_str(), "false")) return false;
    if (s == "1") return true;
    if (s == "0") return false;

    SG_LOG(SG_GENERAL, SG_WARN, "Unable to parse string as boolean:" << s);
    return false;
}

enum PropMatchState
{
    MATCH_LITERAL = 0,
    MATCH_WILD_INDEX,
    MATCH_WILD_NAME
};

bool matchPropPathToTemplate(const std::string& path, const std::string& templatePath)
{
    if (path.empty()) {
        return false;
    }

    const char* pathPtr = path.c_str();
    const char* tPtr = templatePath.c_str();
    PropMatchState state = MATCH_LITERAL;

    while (true) {
        bool advanceInTemplate = true;
        const char p = *pathPtr;
        if (p == 0) {
            // ran out of chars in the path. If we are matching a trailing
            // wildcard, this is a match, otherwise it's a fail
            if (state == MATCH_WILD_NAME) {
                // check this is the last * in the template string
                if (*(tPtr + 1) == 0) {
                    return true;
                }
            }

            return false;
        }

        switch (state) {
        case MATCH_LITERAL:
            if (*tPtr != p) {
                // literal mismatch
                return false;
            }
            ++pathPtr;
            break;
        case MATCH_WILD_NAME:
            if ((p == '-') || isalpha(p)) {
                advanceInTemplate = false;
                ++pathPtr;
            } else {
                // something else, we will advance in the template
            }
            break;
        case MATCH_WILD_INDEX:
            if (isdigit(p)) {
                advanceInTemplate = false;
                ++pathPtr;
            } else {
                // something else, we will advance in the template
            }
            break;
        } // of state switch

        if (advanceInTemplate) {
            const char nextTemplate = *(++tPtr);
            if (nextTemplate == 0) {
                // end of template, successful match
                return true;
            } else if (nextTemplate == '*') {
                state = (*(tPtr - 1) == '[') ? MATCH_WILD_INDEX : MATCH_WILD_NAME;
            } else {
                state = MATCH_LITERAL;
            }
        }
    }

    // unreachable
}
 
bool parseStringAsLatLonValue(const std::string& s, double& degrees)
{
    try {
            string ss = simplify(s);
            auto spacePos = ss.find_first_of(" *");
        
            if (spacePos == std::string::npos) {
                degrees = std::stod(ss);
            } else {
                degrees = std::stod(ss.substr(0, spacePos));
                
                double minutes = 0.0, seconds = 0.0;
                
                // check for minutes marker
                auto quotePos = ss.find('\'');
                if (quotePos == std::string::npos) {
                    const auto minutesStr = ss.substr(spacePos+1);
                    if (!minutesStr.empty()) {
                        minutes = std::stod(minutesStr);
                    }
                } else {
                    minutes = std::stod(ss.substr(spacePos+1, quotePos - spacePos));
                    const auto secondsStr = ss.substr(quotePos+1);
                    if (!secondsStr.empty()) {
                        seconds = std::stod(secondsStr);
                    }
                }
                
                if ((seconds < 0.0) || (minutes < 0.0)) {
                    // don't allow sign information in minutes or seconds
                    return false;
                }
                
                double offset = (minutes / 60.0) + (seconds / 3600.0);
                degrees += (degrees >= 0.0) ? offset : -offset;
            }
        
            // since we simplified, any trailing N/S/E/W must be the last char
            const char lastChar = ::toupper(ss.back());
            if ((lastChar == 'W') || (lastChar == 'S')) {
                degrees = -degrees;
            }
    } catch (std::exception&) {
        // std::stdo can throw
        return false;
    }
    
    return true;
}
        
namespace {
    bool isLatString(const std::string &s)
    {
        const char lastChar = ::toupper(s.back());
        return (lastChar == 'N') || (lastChar == 'S');
    }
    
    bool isLonString(const std::string &s)
    {
        const char lastChar = ::toupper(s.back());
        return (lastChar == 'E') || (lastChar == 'W');
    }
} // of anonymous namespace
        
bool parseStringAsGeod(const std::string& s, SGGeod* result, bool assumeLonLatOrder)
{
    if (s.empty())
        return false;
    
    const auto commaPos = s.find(',');
    if (commaPos == string::npos) {
        return false;
    }
    
    auto termA = simplify(s.substr(0, commaPos)),
        termB = simplify(s.substr(commaPos+1));
    double valueA, valueB;
    if (!parseStringAsLatLonValue(termA, valueA) || !parseStringAsLatLonValue(termB, valueB)) {
        return false;
    }
    
    if (result) {
        // explicit ordering
        if (isLatString(termA) && isLonString(termB)) {
            *result = SGGeod::fromDeg(valueB, valueA);
        } else if (isLonString(termA) && isLatString(termB)) {
            *result = SGGeod::fromDeg(valueA, valueB);
        } else {
            // implicit ordering
            // SGGeod wants longitude, latitude
            *result = assumeLonLatOrder ? SGGeod::fromDeg(valueA, valueB)
                                        : SGGeod::fromDeg(valueB, valueA);
        }
    }
    
    return true;
}
        
namespace {
    const char* static_degreeSymbols[] = {
        "*",
        " ",
        "\xB0",     // Latin-1 B0 codepoint
        "\xC2\xB0"  // UTF-8 equivalent
    };
} // of anonymous namespace
        
std::string formatLatLonValueAsString(double deg, LatLonFormat format,
                                      char c,
                                      DegreeSymbol degreeSymbol)
{
    double min, sec;
    const int sign = deg < 0.0 ? -1 : 1;
    deg = fabs(deg);
    char buf[128];
    const char* degSym = static_degreeSymbols[static_cast<int>(degreeSymbol)];
    
    switch (format) {
    case LatLonFormat::DECIMAL_DEGREES:
        ::snprintf(buf, sizeof(buf), "%3.6f%c", deg, c);
        break;
        
    case LatLonFormat::DEGREES_MINUTES:
        // d mm.mmm' (DMM format) -- uses a round-off factor tailored to the
        // required precision of the minutes field (three decimal places),
        // preventing minute values of 60.
        min = (deg - int(deg)) * 60.0;
        if (min >= 59.9995) {
            min -= 60.0;
            deg += 1.0;
        }
        snprintf(buf, sizeof(buf), "%d%s%06.3f'%c", int(deg), degSym, fabs(min), c);
        break;
        
    case LatLonFormat::DEGREES_MINUTES_SECONDS:
        // d mm'ss.s" (DMS format) -- uses a round-off factor tailored to the
        // required precision of the seconds field (one decimal place),
        // preventing second values of 60.
        min = (deg - int(deg)) * 60.0;
        sec = (min - int(min)) * 60.0;
        if (sec >= 59.95) {
            sec -= 60.0;
            min += 1.0;
            if (min >= 60.0) {
                min -= 60.0;
                deg += 1.0;
            }
        }
        ::snprintf(buf, sizeof(buf), "%d%s%02d'%04.1f\"%c", int(deg), degSym,
                   int(min), fabs(sec), c);
        break;
            
    case LatLonFormat::SIGNED_DECIMAL_DEGREES:
        // d.dddddd' (signed DDD format).
        ::snprintf(buf, sizeof(buf), "%3.6f", sign*deg);
        break;
    
    case LatLonFormat::SIGNED_DEGREES_MINUTES:
        // d mm.mmm' (signed DMM format).
        min = (deg - int(deg)) * 60.0;
        if (min >= 59.9995) {
            min -= 60.0;
            deg += 1.0;
        }
        if (sign == 1) {
            snprintf(buf, sizeof(buf), "%d%s%06.3f'", int(deg), degSym, fabs(min));
        } else {
            snprintf(buf, sizeof(buf), "-%d%s%06.3f'", int(deg), degSym, fabs(min));
        }
        break;
        
            
    case LatLonFormat::SIGNED_DEGREES_MINUTES_SECONDS:
        // d mm'ss.s" (signed DMS format).
        min = (deg - int(deg)) * 60.0;
        sec = (min - int(min)) * 60.0;
        if (sec >= 59.95) {
            sec -= 60.0;
            min += 1.0;
            if (min >= 60.0) {
                min -= 60.0;
                deg += 1.0;
            }
        }
        if (sign == 1) {
            snprintf(buf, sizeof(buf), "%d%s%02d'%04.1f\"", int(deg), degSym, int(min), fabs(sec));
        } else {
            snprintf(buf, sizeof(buf), "-%d%s%02d'%04.1f\"", int(deg), degSym, int(min), fabs(sec));
        }
        break;
            
    case LatLonFormat::ZERO_PAD_DECIMAL_DEGRESS:
        // dd.dddddd X, ddd.dddddd X (zero padded DDD format).
        if (c == 'N' || c == 'S') {
            snprintf(buf, sizeof(buf), "%09.6f%c", deg, c);
        } else {
            snprintf(buf, sizeof(buf), "%010.6f%c", deg, c);
        }
        break;
            
    case LatLonFormat::ZERO_PAD_DEGREES_MINUTES:
        // dd mm.mmm' X, ddd mm.mmm' X (zero padded DMM format).
        min = (deg - int(deg)) * 60.0;
        if (min >= 59.9995) {
            min -= 60.0;
            deg += 1.0;
        }
        if (c == 'N' || c == 'S') {
            snprintf(buf, sizeof(buf), "%02d%s%06.3f'%c", int(deg), degSym, fabs(min), c);
        } else {
            snprintf(buf, sizeof(buf), "%03d%s%06.3f'%c", int(deg), degSym, fabs(min), c);
        }
        break;
            
    case LatLonFormat::ZERO_PAD_DEGREES_MINUTES_SECONDS:
        // dd mm'ss.s" X, dd mm'ss.s" X (zero padded DMS format).
        min = (deg - int(deg)) * 60.0;
        sec = (min - int(min)) * 60.0;
        if (sec >= 59.95) {
            sec -= 60.0;
            min += 1.0;
            if (min >= 60.0) {
                min -= 60.0;
                deg += 1.0;
            }
        }
        if (c == 'N' || c == 'S') {
            snprintf(buf, sizeof(buf), "%02d%s%02d'%04.1f\"%c", int(deg), degSym, int(min), fabs(sec), c);
        } else {
            snprintf(buf, sizeof(buf), "%03d%s%02d'%04.1f\"%c", int(deg), degSym, int(min), fabs(sec), c);
        }
        break;
            
    case LatLonFormat::TRINITY_HOUSE:
        // dd* mm'.mmm X, ddd* mm'.mmm X (Trinity House Navigation standard).
        min = (deg - int(deg)) * 60.0;
        if (min >= 59.9995) {
            min -= 60.0;
            deg += 1.0;
        }
        if (c == 'N' || c == 'S') {
            snprintf(buf, sizeof(buf), "%02d* %02d'.%03d%c", int(deg), int(min), int(SGMisc<double>::round((min-int(min))*1000)), c);
        } else {
            snprintf(buf, sizeof(buf), "%03d* %02d'.%03d%c", int(deg), int(min), int(SGMisc<double>::round((min-int(min))*1000)), c);
        }
        break;
            
    case LatLonFormat::DECIMAL_DEGREES_SYMBOL:
        ::snprintf(buf, sizeof(buf), "%3.6f%s%c", deg, degSym, c);
        break;
            
    case LatLonFormat::ICAO_ROUTE_DEGREES:
    {
        min = (deg - int(deg)) * 60.0;
        if (min >= 59.9995) {
            min -= 60.0;
            deg += 1.0;
        }
        
        if (static_cast<int>(min) == 0) {
            // 7-digit mode
            if (c == 'N' || c == 'S') {
                snprintf(buf, sizeof(buf), "%02d%c", int(deg), c);
            } else {
                snprintf(buf, sizeof(buf), "%03d%c", int(deg), c);
            }
        } else {
            // 11-digit mode
            if (c == 'N' || c == 'S') {
                snprintf(buf, sizeof(buf), "%02d%02d%c", int(deg), int(min), c);
            } else {
                snprintf(buf, sizeof(buf), "%03d%02d%c", int(deg), int(min), c);
            }
        }
        break;
    }
            
    default:
        break;
    }
 
    return std::string(buf);
}

std::string formatGeodAsString(const SGGeod& geod, LatLonFormat format,
                               DegreeSymbol degreeSymbol)
{
    const char ns = (geod.getLatitudeDeg() > 0.0) ? 'N' : 'S';
    const char ew = (geod.getLongitudeDeg() > 0.0) ? 'E' : 'W';
    
    // no comma seperator
    if (format == LatLonFormat::ICAO_ROUTE_DEGREES) {
        return formatLatLonValueAsString(geod.getLatitudeDeg(), format, ns, degreeSymbol) +
            formatLatLonValueAsString(geod.getLongitudeDeg(), format, ew, degreeSymbol);
    }
    
    return formatLatLonValueAsString(geod.getLatitudeDeg(), format, ns, degreeSymbol) + ","
        + formatLatLonValueAsString(geod.getLongitudeDeg(), format, ew, degreeSymbol);
}

} // end namespace strutils

} // end namespace simgear
