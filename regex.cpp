/*
  ===========================================================================

  Copyright (C) 2013 Emvivre

  This file is part of REGEXP.

  REGEXP is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  REGEXP is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with REGEXP.  If not, see <http://www.gnu.org/licenses/>.

  ===========================================================================
*/

#include "regex.hh"
#include <regex.h>

#include <iostream>

std::vector<std::string> Regex::search(std::string pattern, std::string buffer) {
	regex_t r;
	int nb_matches = 128;
	regmatch_t matches[nb_matches];		
	if ( regcomp(&r, pattern.c_str(), REG_EXTENDED) != 0 ) {
		throw ExceptionRegexpCompilationError();
	}
	std::vector<std::string> match_str;
	int beg = 0;
	while ( regexec(&r, buffer.c_str() + beg, nb_matches, matches, 0) != REG_NOMATCH ) {
		int i = (matches[1].rm_so == -1) ? 0 : 1;
		for (; i < nb_matches; i++ ) {
			if ( matches[i].rm_so == -1 ) {
				break ;
			}
			int l = matches[i].rm_eo - matches[i].rm_so;
			std::string s(&buffer[beg + matches[i].rm_so], l);
			match_str.push_back(s);
		}
		beg += matches[0].rm_eo;
	}
	regfree(&r);
	return match_str;	       
}
