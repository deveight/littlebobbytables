/* $Id: time_of_day.cpp 54625 2012-07-08 14:26:21Z loonycyborg $ */
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

#include "config.hpp"
#include "time_of_day.hpp"

#include <boost/foreach.hpp>

std::ostream &operator<<(std::ostream &s, const tod_color& c){
	s << c.r << "," << c.g << "," << c.b;
	return s;
}


time_of_day::time_of_day(const config& cfg):
	lawful_bonus(cfg["lawful_bonus"]),
	bonus_modified(0),
	image(cfg["image"]), name(cfg["name"].t_str()), id(cfg["id"]),
	image_mask(cfg["mask"]),
	color(cfg["red"], cfg["green"], cfg["blue"]),
	sounds(cfg["sound"])
{
}

time_of_day::time_of_day()
: lawful_bonus(0)
, bonus_modified(0)
, image()
, name("NULL_TOD")
, id("nulltod")
, image_mask()
, color(0,0,0)
, sounds()
{
}

void time_of_day::write(config& cfg) const
{
	cfg["lawful_bonus"] = lawful_bonus;
	cfg["red"] = color.r;
	cfg["green"] = color.g;
	cfg["blue"] = color.b;
	cfg["image"] = image;
	cfg["name"] = name;
	cfg["id"] = id;
	cfg["mask"] = image_mask;
}

void time_of_day::parse_times(const config& cfg, std::vector<time_of_day>& normal_times)
{
	BOOST_FOREACH(const config &t, cfg.child_range("time")) {
		normal_times.push_back(time_of_day(t));
	}

	if(normal_times.empty())
	{
		// Make sure we have at least default time
		config dummy_cfg;
		normal_times.push_back(time_of_day(dummy_cfg));
	}
}

