/* $Id: aspect_attacks.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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
 * Aspect: attacks
 * @file
 */

#ifndef AI_TESTING_ASPECT_ATTACKS_HPP_INCLUDED
#define AI_TESTING_ASPECT_ATTACKS_HPP_INCLUDED

#include "../composite/aspect.hpp"
#include "../interface.hpp"

#ifdef _MSC_VER
#pragma warning(push)
//silence "inherits via dominance" warnings
#pragma warning(disable:4250)
#endif

namespace ai {


namespace testing_ai_default {

class aspect_attacks: public typesafe_aspect<attacks_vector> {
public:
	aspect_attacks(readonly_context &context, const config &cfg, const std::string &id);


	virtual ~aspect_attacks();


	virtual void recalculate() const;


	virtual config to_config() const;


protected:
	boost::shared_ptr<attacks_vector> analyze_targets() const;

	static void do_attack_analysis(const map_location& loc,
	                const move_map& srcdst, const move_map& dstsrc,
			const move_map& fullmove_srcdst, const move_map& fullmove_dstsrc,
	                const move_map& enemy_srcdst, const move_map& enemy_dstsrc,
			const map_location* tiles, bool* used_locations,
	                std::vector<map_location>& units,
	                std::vector<attack_analysis>& result,
			attack_analysis& cur_analysis,
			 const team &current_team,
			 const readonly_context *ai_obj);
	static int rate_terrain(const unit& u, const map_location& loc);
	double power_projection(const map_location& loc, const move_map& dstsrc) const;
	static double power_projection2(const map_location& loc, const move_map& dstsrc);
	config filter_own_;
	config filter_enemy_;
};



} // end of namespace testing_ai_default

} // end of namespace ai

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
