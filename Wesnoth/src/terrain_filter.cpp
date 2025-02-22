/* $Id: terrain_filter.cpp 54625 2012-07-08 14:26:21Z loonycyborg $ */
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

#define GETTEXT_DOMAIN "wesnoth-lib"

#include "global.hpp"

#include "actions.hpp"
#include "config.hpp"
#include "log.hpp"
#include "map.hpp"
#include "resources.hpp"
#include "terrain_filter.hpp"
#include "tod_manager.hpp"
#include "variable.hpp"

#include <boost/foreach.hpp>

static lg::log_domain log_engine("engine");
#define ERR_NG LOG_STREAM(err, log_engine)

terrain_filter::~terrain_filter()
{
}

#ifdef _MSC_VER
// This is a workaround for a VC bug; this constructor is never called
// and so we don't care about the warnings this quick fix generates
#pragma warning(push)
#pragma warning(disable:4413)
terrain_filter::terrain_filter():
	cfg_(vconfig::unconstructed_vconfig()),
	units_(unit_map()),
	cache_(),
	max_loop_(),
	flat_()
{
	assert(false);
}
#pragma warning(pop)
#endif


terrain_filter::terrain_filter(const vconfig& cfg, const unit_map& units,
		const bool flat_tod, const size_t max_loop) :
	cfg_(cfg),
	units_(units),
	cache_(),
	max_loop_(max_loop),
	flat_(flat_tod)
{
}

terrain_filter::terrain_filter(const vconfig& cfg, const terrain_filter& original) :
	cfg_(cfg),
	units_(original.units_),
	cache_(),
	max_loop_(original.max_loop_),
	flat_(original.flat_)
{
}

terrain_filter::terrain_filter(const terrain_filter& other) :
	xy_pred(), // We should construct this too, since it has no datamembers
	           // use the default constructor.
	cfg_(other.cfg_),
	units_(other.units_),
	cache_(),
	max_loop_(other.max_loop_),
	flat_(other.flat_)
{
}

terrain_filter& terrain_filter::operator=(const terrain_filter& other)
{
	// Use copy constructor to make sure we are coherant
	if (this != &other) {
		this->~terrain_filter();
		new (this) terrain_filter(other) ;
	}
	return *this ;
}

namespace {
	struct cfg_isor {
		bool operator() (std::pair<const std::string,const vconfig> val) {
			return val.first == "or";
		}
	};
} //end anonymous namespace

bool terrain_filter::match_internal(const map_location& loc, const bool ignore_xy) const
{
	if(cfg_.has_attribute("terrain")) {
		if(cache_.parsed_terrain == NULL) {
			cache_.parsed_terrain = new t_translation::t_match(cfg_["terrain"]);
		}
		if(!cache_.parsed_terrain->is_empty) {
			const t_translation::t_terrain letter = resources::game_map->get_terrain_info(loc).number();
			if(!t_translation::terrain_matches(letter, *cache_.parsed_terrain)) {
				return false;
			}
		}
	}

	//Allow filtering on location ranges
	if(!ignore_xy) {
		if(!loc.matches_range(cfg_["x"], cfg_["y"])) {
			return false;
		}
		//allow filtering by searching a stored variable of locations
		if(cfg_.has_attribute("find_in")) {
			variable_info vi(cfg_["find_in"], false, variable_info::TYPE_CONTAINER);
			if(!vi.is_valid) return false;
			if(vi.explicit_index) {
				if(map_location(vi.as_container(),NULL) != loc) {
					return false;
				}
			} else {
				bool found = false;
				BOOST_FOREACH(const config &cfg, vi.as_array()) {
					if (map_location(cfg, NULL) == loc) {
						found = true;
						break;
					}
				}
				if (!found) return false;
			}
		}
	}

	//Allow filtering on unit
	if(cfg_.has_child("filter")) {
		const vconfig& unit_filter = cfg_.child("filter");
		const unit_map::const_iterator u = units_.find(loc);
		if (u == units_.end() || !u->matches_filter(unit_filter, loc, flat_))
			return false;
	}

	//Allow filtering on adjacent locations
	if(cfg_.has_child("filter_adjacent_location")) {
		map_location adjacent[6];
		get_adjacent_tiles(loc, adjacent);
		const vconfig::child_list& adj_cfgs = cfg_.get_children("filter_adjacent_location");
		vconfig::child_list::const_iterator i, i_end, i_begin = adj_cfgs.begin();
		for (i = i_begin, i_end = adj_cfgs.end(); i != i_end; ++i) {
			int match_count = 0;
			vconfig::child_list::difference_type index = i - i_begin;
			static std::vector<map_location::DIRECTION> default_dirs
				= map_location::parse_directions("n,ne,se,s,sw,nw");
			std::vector<map_location::DIRECTION> dirs = (*i).has_attribute("adjacent")
				? map_location::parse_directions((*i)["adjacent"]) : default_dirs;
			std::vector<map_location::DIRECTION>::const_iterator j, j_end = dirs.end();
			for (j = dirs.begin(); j != j_end; ++j) {
				map_location &adj = adjacent[*j];
				if (resources::game_map->on_board(adj)) {
					if(cache_.adjacent_matches == NULL) {
						while(index >= std::distance(cache_.adjacent_match_cache.begin(), cache_.adjacent_match_cache.end())) {
							const vconfig& adj_cfg = adj_cfgs[cache_.adjacent_match_cache.size()];
							std::pair<terrain_filter, std::map<map_location,bool> > amc_pair(
								terrain_filter(adj_cfg, *this),
								std::map<map_location,bool>());
							cache_.adjacent_match_cache.push_back(amc_pair);
						}
						terrain_filter &amc_filter = cache_.adjacent_match_cache[index].first;
						std::map<map_location,bool> &amc = cache_.adjacent_match_cache[index].second;
						std::map<map_location,bool>::iterator lookup = amc.find(adj);
						if(lookup == amc.end()) {
							if(amc_filter(adj)) {
								amc[adj] = true;
								++match_count;
							} else {
								amc[adj] = false;
							}
						} else if(lookup->second) {
							++match_count;
						}
					} else {
						assert(index < std::distance(cache_.adjacent_matches->begin(), cache_.adjacent_matches->end()));
						std::set<map_location> &amc = (*cache_.adjacent_matches)[index];
						if(amc.find(adj) != amc.end()) {
							++match_count;
						}
					}
				}
			}
			static std::vector<std::pair<int,int> > default_counts = utils::parse_ranges("1-6");
			std::vector<std::pair<int,int> > counts = (*i).has_attribute("count")
				? utils::parse_ranges((*i)["count"]) : default_counts;
			if(!in_ranges(match_count, counts)) {
				return false;
			}
		}
	}

	const t_string& t_tod_type = cfg_["time_of_day"];
	const t_string& t_tod_id = cfg_["time_of_day_id"];
	const std::string& tod_type = t_tod_type;
	const std::string& tod_id = t_tod_id;
	static config const dummy_cfg;
	time_of_day tod(dummy_cfg);
	if(!tod_type.empty() || !tod_id.empty()) {
		if(flat_) {
			tod = resources::tod_manager->get_time_of_day(loc);
		} else {
			tod = resources::tod_manager->get_illuminated_time_of_day(loc);
		}
	}
	if(!tod_type.empty()) {
		const std::vector<std::string>& vals = utils::split(tod_type);
		if(tod.lawful_bonus<0) {
			if(std::find(vals.begin(),vals.end(),std::string("chaotic")) == vals.end()) {
				return false;
			}
		} else if(tod.lawful_bonus>0) {
			if(std::find(vals.begin(),vals.end(),std::string("lawful")) == vals.end()) {
				return false;
			}
		} else if(std::find(vals.begin(),vals.end(),std::string("neutral")) == vals.end()) {
			return false;
		}
	}

	if(!tod_id.empty()) {
		if(tod_id != tod.id) {
			if(std::find(tod_id.begin(),tod_id.end(),',') != tod_id.end() &&
				std::search(tod_id.begin(),tod_id.end(),
				tod.id.begin(),tod.id.end()) != tod_id.end()) {
				const std::vector<std::string>& vals = utils::split(tod_id);
				if(std::find(vals.begin(),vals.end(),tod.id) == vals.end()) {
					return false;
				}
			} else {
				return false;
			}
		}
	}

	//allow filtering on owner (for villages)
	const t_string& t_owner_side = cfg_["owner_side"];
	const std::string& owner_side = t_owner_side;
	if(!owner_side.empty()) {
		const int side_index = lexical_cast_default<int>(owner_side,0) - 1;
		if(village_owner(loc, *resources::teams) != side_index) {
			return false;
		}
	}

	return true;
}

bool terrain_filter::match(const map_location& loc) const
{
	if(cfg_["x"] == "recall" && cfg_["y"] == "recall") {
		return !resources::game_map->on_board(loc);
	}
	std::set<map_location> hexes;
	std::vector<map_location> loc_vec(1, loc);

	//handle radius
	size_t radius = lexical_cast_default<size_t>(cfg_["radius"], 0);
	if(radius > max_loop_) {
		ERR_NG << "terrain_filter: radius greater than " << max_loop_
		<< ", restricting\n";
		radius = max_loop_;
	}
	if(cfg_.has_child("filter_radius")) {
		terrain_filter r_filter(cfg_.child("filter_radius"), *this);
		get_tiles_radius(*resources::game_map, loc_vec, radius, hexes, &r_filter);
	} else {
		get_tiles_radius(*resources::game_map, loc_vec, radius, hexes);
	}

	size_t loop_count = 0;
	std::set<map_location>::const_iterator i;
	for(i = hexes.begin(); i != hexes.end(); ++i) {
		bool matches = match_internal(*i, false);

		//handle [and], [or], and [not] with in-order precedence
		vconfig::all_children_iterator cond = cfg_.ordered_begin();
		vconfig::all_children_iterator cond_end = cfg_.ordered_end();
		while(cond != cond_end)
		{
			const std::string& cond_name = cond.get_key();
			const vconfig& cond_cfg = cond.get_child();

			//handle [and]
			if(cond_name == "and")
			{
				matches = matches && terrain_filter(cond_cfg, *this)(*i);
			}
			//handle [or]
			else if(cond_name == "or")
			{
				matches = matches || terrain_filter(cond_cfg, *this)(*i);
			}
			//handle [not]
			else if(cond_name == "not")
			{
				matches = matches && !terrain_filter(cond_cfg, *this)(*i);
			}
			++cond;
		}
		if(matches) {
			return true;
		}
		if(++loop_count > max_loop_) {
			std::set<map_location>::const_iterator temp = i;
			if(++temp != hexes.end()) {
				ERR_NG << "terrain_filter: loop count greater than " << max_loop_
				<< ", aborting\n";
				break;
			}
		}
	}
	return false;
}

void terrain_filter::get_locations(std::set<map_location>& locs, bool with_border) const
{
	std::vector<map_location> xy_vector =
		parse_location_range(cfg_["x"], cfg_["y"], with_border);
	std::set<map_location> xy_set(xy_vector.begin(), xy_vector.end());
	if (!cfg_.has_attribute("x") && !cfg_.has_attribute("y")) {
		//consider all locations on the map
		int bs = resources::game_map->border_size();
		int w = with_border ? resources::game_map->w() + bs : resources::game_map->w();
		int h = with_border ? resources::game_map->h() + bs : resources::game_map->h();
		for (int x = with_border ? 0 - bs : 0; x < w; ++x) {
			for (int y = with_border ? 0 - bs : 0; y < h; ++y) {
				xy_set.insert(map_location(x,y));
			}
		}
	}
	if(cfg_.has_attribute("find_in")) {
		//remove any locations not found in the specified variable
		variable_info vi(cfg_["find_in"], false, variable_info::TYPE_CONTAINER);
		if(!vi.is_valid) {
			xy_set.clear();
		} else if(vi.explicit_index) {
			map_location test_loc(vi.as_container(),NULL);
			if(xy_set.count(test_loc)) {
				xy_set.clear();
				xy_set.insert(test_loc);
			} else {
				xy_set.clear();
			}
		} else {
			std::set<map_location> findin_locs;
			BOOST_FOREACH(const config &cfg, vi.as_array()) {
				map_location test_loc(cfg, NULL);
				if (xy_set.count(test_loc)) {
					findin_locs.insert(test_loc);
				}
			}
			xy_set.swap(findin_locs);
		}
	}

	//handle location filter
	if(cfg_.has_child("filter_adjacent_location")) {
		if(cache_.adjacent_matches == NULL) {
			cache_.adjacent_matches = new std::vector<std::set<map_location> >();
		}
		const vconfig::child_list& adj_cfgs = cfg_.get_children("filter_adjacent_location");
		for (unsigned i = 0; i < adj_cfgs.size(); ++i) {
			std::set<map_location> adj_set;
			/* GCC-3.3 doesn't like operator[] so use at which has the same result */
			terrain_filter(adj_cfgs.at(i), *this).get_locations(adj_set, with_border);
			cache_.adjacent_matches->push_back(adj_set);
			if(i >= max_loop_ && i+1 < adj_cfgs.size()) {
				ERR_NG << "terrain_filter: loop count greater than " << max_loop_
				<< ", aborting\n";
				break;
			}
		}
	}
	std::set<map_location>::iterator loc_itor = xy_set.begin();
	while(loc_itor != xy_set.end()) {
		if(match_internal(*loc_itor, true)) {
			++loc_itor;
		} else {
			xy_set.erase(loc_itor++);
		}
	}

	//handle [and], [or], and [not] with in-order precedence
	vconfig::all_children_iterator cond = cfg_.ordered_begin();
	vconfig::all_children_iterator cond_end = cfg_.ordered_end();
	int ors_left = std::count_if(cond, cond_end, cfg_isor());
	while(cond != cond_end)
	{
		//if there are no locations or [or] conditions left, go ahead and return empty
		if(xy_set.empty() && ors_left <= 0) {
			return;
		}

		const std::string& cond_name = cond.get_key();
		const vconfig& cond_cfg = cond.get_child();

		//handle [and]
		if(cond_name == "and") {
			std::set<map_location> intersect_hexes;
			terrain_filter(cond_cfg, *this).get_locations(intersect_hexes, with_border);
			std::set<map_location>::iterator intersect_itor = xy_set.begin();
			while(intersect_itor != xy_set.end()) {
				if(intersect_hexes.find(*intersect_itor) == intersect_hexes.end()) {
					xy_set.erase(*intersect_itor++);
				} else {
					++intersect_itor;
				}
			}
		}
		//handle [or]
		else if(cond_name == "or") {
			std::set<map_location> union_hexes;
			terrain_filter(cond_cfg, *this).get_locations(union_hexes, with_border);
			//xy_set.insert(union_hexes.begin(), union_hexes.end()); //doesn't compile on MSVC
			std::set<map_location>::iterator insert_itor = union_hexes.begin();
			while(insert_itor != union_hexes.end()) {
				xy_set.insert(*insert_itor++);
			}
			--ors_left;
		}
		//handle [not]
		else if(cond_name == "not") {
			std::set<map_location> removal_hexes;
			terrain_filter(cond_cfg, *this).get_locations(removal_hexes, with_border);
			std::set<map_location>::iterator erase_itor = removal_hexes.begin();
			while(erase_itor != removal_hexes.end()) {
				xy_set.erase(*erase_itor++);
			}
		}
		++cond;
	}
	if(xy_set.empty()) {
		return;
	}

	//handle radius
	size_t radius = lexical_cast_default<size_t>(cfg_["radius"], 0);
	if(radius > max_loop_) {
		ERR_NG << "terrain_filter: radius greater than " << max_loop_
		<< ", restricting\n";
		radius = max_loop_;
	}
	if(radius > 0) {
		xy_vector.clear();
		std::copy(xy_set.begin(),xy_set.end(),std::inserter(xy_vector,xy_vector.end()));
		if(cfg_.has_child("filter_radius")) {
			terrain_filter r_filter(cfg_.child("filter_radius"), *this);
			get_tiles_radius(*resources::game_map, xy_vector, radius, locs, &r_filter);
		} else {
			get_tiles_radius(*resources::game_map, xy_vector, radius, locs);
		}
	} else {
		std::copy(xy_set.begin(),xy_set.end(),std::inserter(locs,locs.end()));
	}
}

config terrain_filter::to_config() const
{
	return cfg_.get_config();
}

terrain_filter::terrain_filter_cache::~terrain_filter_cache() {
	delete parsed_terrain;
	delete adjacent_matches;
}
