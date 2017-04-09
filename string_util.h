/*
  ===========================================================================

  Copyright (C) 2014 Emvivre

  This file is part of STRING_UTIL.

  STRING_UTIL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  STRING_UTIL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with STRING_UTIL.  If not, see <http://www.gnu.org/licenses/>.

  ===========================================================================
*/

#ifndef _STRING_UTIL_H_
#define _STRING_UTIL_H_

#include <algorithm>
#include <vector>

namespace StringUtil
{
	inline std::string upper(const std::string& s_orig)
	{
		std::string s = s_orig;
		std::transform(s.begin(), s.end(), s.begin(), ::toupper);
		return s;
	}
	inline std::string lower(const std::string& s_orig) 
	{
		std::string s = s_orig;
		std::transform(s.begin(), s.end(), s.begin(), ::tolower);
		return s;
	}
	inline bool contains(const std::string& s, const std::string& pattern)
	{
		return s.find(pattern) != std::string::npos;
	}
	inline bool startswith(const std::string& s, const std::string& pattern) 
	{
		return std::equal(pattern.begin(), pattern.end(), s.begin());
	}
	inline bool endswith(const std::string& s, const std::string& pattern) 
	{
		return std::equal(pattern.begin(), pattern.end(), s.end()-pattern.size());
	}
	inline std::vector<std::string> split(const std::string& s, char c)
        {
		std::vector<std::string> v;
                unsigned int beg = 0;
                for ( unsigned int i = 0; i < s.size(); i++ ) {
                        if ( s[i] == c ) {
                                if ( beg == i ) {
                                        beg++;
                                        continue;
                                }
				std::string e = s.substr(beg, i - beg);
                                v.push_back(e);
                                beg = i+1;
                        }
                }
                if ( beg < s.size() ) {
			std::string e = s.substr(beg, s.size() - beg);
                        v.push_back(e);
                }
                return v;
        }
	inline std::string extension(const std::string& file)
	{
		int i = file.size();
		while (i-- && file[i] != '.');
		i++;
		return file.substr(i, file.size() - i);
	}	
}

#endif /* !_STRING_UTIL_H_ */
