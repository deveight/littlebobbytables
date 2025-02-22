/* $Id: map_exception.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2003 - 2012 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/** @file */

#ifndef MAP_EXCEPTION_H_INCLUDED
#define MAP_EXCEPTION_H_INCLUDED

#include "exceptions.hpp"

struct incorrect_map_format_error : game::error {
	incorrect_map_format_error(const std::string& message) : error(message) {}
};

#endif
