/* $Id: info.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2010 - 2012 by Ignacio R. Morelle <shadowm2006@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef ADDON_INFO_HPP_INCLUDED
#define ADDON_INFO_HPP_INCLUDED

#include <string>
#include <vector>

struct addon_info
{
	addon_info()
		: name()
		, description()
		, icon()
		, version()
		, author()
		, sizestr()
		, translations()
	{
	}

	std::string name;
	std::string description;
	std::string icon;
	std::string version;
	std::string author;
	std::string sizestr;
	std::vector<std::string> translations;
};

#endif
