/* $Id: interface.cpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2009 - 2012 by Yurii Chernyi <terraninfo@terraninfo.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

/**
 * Base class for the AI and AI-ai_manager contract.
 * @file
 */

#include "interface.hpp"

namespace ai {

// =======================================================================
//
// =======================================================================
std::string interface::describe_self() const
{
	return "? [ai]";
}

} //end of namespace ai
