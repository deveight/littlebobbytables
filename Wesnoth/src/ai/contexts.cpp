/* $Id: contexts.cpp 54625 2012-07-08 14:26:21Z loonycyborg $ */
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
 * Helper functions for the object which operates in the context of AI for specific side
 * This is part of AI interface
 * @file
 */

#include "actions.hpp"
#include "contexts.hpp"
#include "manager.hpp"

#include "composite/aspect.hpp"
#include "composite/engine.hpp"
#include "composite/goal.hpp"

#include "default/ai.hpp"

#include "../callable_objects.hpp"
#include "../dialogs.hpp"
#include "../formula.hpp"
#include "../formula_callable.hpp"
#include "../formula_function.hpp"
#include "../formula_fwd.hpp"
#include "../game_display.hpp"
#include "../game_end_exceptions.hpp"
#include "../game_events.hpp"
#include "../game_preferences.hpp"
#include "../log.hpp"
#include "../mouse_handler_base.hpp"
#include "../resources.hpp"
#include "../replay.hpp"
#include "../statistics.hpp"
#include "../tod_manager.hpp"
#include "../unit_display.hpp"

#include <boost/foreach.hpp>

static lg::log_domain log_ai("ai/general");
#define DBG_AI LOG_STREAM(debug, log_ai)
#define LOG_AI LOG_STREAM(info, log_ai)
#define WRN_AI LOG_STREAM(warn, log_ai)
#define ERR_AI LOG_STREAM(err, log_ai)

// =======================================================================
//
// =======================================================================
namespace ai {

int side_context_impl::get_recursion_count() const
{
	return recursion_counter_.get_count();
}


int readonly_context_impl::get_recursion_count() const
{
	return recursion_counter_.get_count();
}


int readwrite_context_impl::get_recursion_count() const
{
	return recursion_counter_.get_count();
}


void readonly_context_impl::raise_user_interact() const
{
	manager::raise_user_interact();
}


void readwrite_context_impl::raise_gamestate_changed() const
{
	manager::raise_gamestate_changed();
}


team& readwrite_context_impl::current_team_w()
{
	return (*resources::teams)[get_side()-1];
}

attack_result_ptr readwrite_context_impl::execute_attack_action(const map_location& attacker_loc, const map_location& defender_loc, int attacker_weapon){
	unit_map::iterator i = resources::units->find(attacker_loc);
	double m_aggression = i.valid() && i->can_recruit() ? get_leader_aggression() : get_aggression();
	return actions::execute_attack_action(get_side(),true,attacker_loc,defender_loc,attacker_weapon, m_aggression);
}


attack_result_ptr readonly_context_impl::check_attack_action(const map_location& attacker_loc, const map_location& defender_loc, int attacker_weapon){
	unit_map::iterator i = resources::units->find(attacker_loc);
	double m_aggression = i.valid() && i->can_recruit() ? get_leader_aggression() : get_aggression();
	return actions::execute_attack_action(get_side(),false,attacker_loc,defender_loc,attacker_weapon, m_aggression);
}


move_result_ptr readwrite_context_impl::execute_move_action(const map_location& from, const map_location& to, bool remove_movement){
	return actions::execute_move_action(get_side(),true,from,to,remove_movement);
}


move_result_ptr readonly_context_impl::check_move_action(const map_location& from, const map_location& to, bool remove_movement){
	return actions::execute_move_action(get_side(),false,from,to,remove_movement);
}


recall_result_ptr readwrite_context_impl::execute_recall_action(const std::string& id, const map_location &where, const map_location &from){
	return actions::execute_recall_action(get_side(),true,id,where,from);
}


recruit_result_ptr readwrite_context_impl::execute_recruit_action(const std::string& unit_name, const map_location &where, const map_location &from){
	return actions::execute_recruit_action(get_side(),true,unit_name,where,from);
}


recall_result_ptr readonly_context_impl::check_recall_action(const std::string& id, const map_location &where, const map_location &from){
	return actions::execute_recall_action(get_side(),false,id,where,from);
}


recruit_result_ptr readonly_context_impl::check_recruit_action(const std::string& unit_name, const map_location &where, const map_location &from){
	return actions::execute_recruit_action(get_side(),false,unit_name,where,from);
}


stopunit_result_ptr readwrite_context_impl::execute_stopunit_action(const map_location& unit_location, bool remove_movement, bool remove_attacks){
	return actions::execute_stopunit_action(get_side(),true,unit_location,remove_movement,remove_attacks);
}


stopunit_result_ptr readonly_context_impl::check_stopunit_action(const map_location& unit_location, bool remove_movement, bool remove_attacks){
	return actions::execute_stopunit_action(get_side(),false,unit_location,remove_movement,remove_attacks);
}


template<typename T>
void readonly_context_impl::add_known_aspect(const std::string &name, boost::shared_ptr< typesafe_aspect <T> > &where)
{
	boost::shared_ptr< typesafe_known_aspect <T> > ka_ptr(new typesafe_known_aspect<T>(name,where,aspects_));
	known_aspects_.insert(make_pair(name,ka_ptr));
}

readonly_context_impl::readonly_context_impl(side_context &context, const config &cfg)
		: cfg_(cfg),
		engines_(),
		known_aspects_(),
		aggression_(),
		attack_depth_(),
		aspects_(),
		attacks_(),
		avoid_(),
		caution_(),
		defensive_position_cache_(),
		dstsrc_(),enemy_dstsrc_(),
		enemy_possible_moves_(),
		enemy_srcdst_(),
		grouping_(),
		goals_(),
		keeps_(),
		leader_aggression_(),
		leader_goal_(),
		leader_value_(),
		move_maps_enemy_valid_(false),
		move_maps_valid_(false),
		number_of_possible_recruits_to_force_recruit_(),
		passive_leader_(),
		passive_leader_shares_keep_(),
		possible_moves_(),
		recruitment_(),
		recruitment_ignore_bad_combat_(),
		recruitment_ignore_bad_movement_(),
		recruitment_pattern_(),
		recursion_counter_(context.get_recursion_count()),
		scout_village_targeting_(),
		simple_targeting_(),
		srcdst_(),
		support_villages_(),
		unit_stats_cache_(),
		village_value_(),
		villages_per_scout_()
	{
		init_side_context_proxy(context);
		manager::add_gamestate_observer(this);

		add_known_aspect("aggression",aggression_);
		add_known_aspect("attack_depth",attack_depth_);
		add_known_aspect("attacks",attacks_);
		add_known_aspect("avoid",avoid_);
		add_known_aspect("caution",caution_);
		add_known_aspect("grouping",grouping_);
		add_known_aspect("leader_aggression",leader_aggression_);
		add_known_aspect("leader_goal",leader_goal_);
		add_known_aspect("leader_value",leader_value_);
		add_known_aspect("number_of_possible_recruits_to_force_recruit",number_of_possible_recruits_to_force_recruit_);
		add_known_aspect("passive_leader",passive_leader_);
		add_known_aspect("passive_leader_shares_keep",passive_leader_shares_keep_);
		add_known_aspect("recruitment",recruitment_);
		add_known_aspect("recruitment_ignore_bad_combat",recruitment_ignore_bad_combat_);
		add_known_aspect("recruitment_ignore_bad_movement",recruitment_ignore_bad_movement_);
		add_known_aspect("recruitment_pattern",recruitment_pattern_);
		add_known_aspect("scout_village_targeting",scout_village_targeting_);
		add_known_aspect("simple_targeting",simple_targeting_);
		add_known_aspect("support_villages",support_villages_);
		add_known_aspect("village_value",village_value_);
		add_known_aspect("villages_per_scout",villages_per_scout_);
		keeps_.init(*resources::game_map);

	}

void readonly_context_impl::on_readonly_context_create() {
	//init the composite ai engines
	BOOST_FOREACH(const config &cfg_element, cfg_.child_range("engine")){
		engine::parse_engine_from_config(*this,cfg_element,std::back_inserter(engines_));
	}

	// init the composite ai aspects
	BOOST_FOREACH(const config &cfg_element, cfg_.child_range("aspect")){
		std::vector<aspect_ptr> aspects;
		engine::parse_aspect_from_config(*this,cfg_element,cfg_element["id"],std::back_inserter(aspects));
		add_aspects(aspects);
	}

	// init the composite ai goals
	BOOST_FOREACH(const config &cfg_element, cfg_.child_range("goal")){
		engine::parse_goal_from_config(*this,cfg_element,std::back_inserter(get_goals()));
	}
}


config side_context_impl::to_side_context_config() const
{
	return config();
}

config readwrite_context_impl::to_readwrite_context_config() const
{
	return config();
}


config readonly_context_impl::to_readonly_context_config() const
{
	config cfg;
	BOOST_FOREACH(const engine_ptr e, engines_) {
		cfg.add_child("engine",e->to_config());
	}
	BOOST_FOREACH(const aspect_map::value_type a, aspects_) {
		cfg.add_child("aspect",a.second->to_config());
	}
	BOOST_FOREACH(const goal_ptr g, goals_) {
		cfg.add_child("goal",g->to_config());
	}
	return cfg;
}

readonly_context_impl::~readonly_context_impl()
{
	manager::remove_gamestate_observer(this);
}

void readonly_context_impl::handle_generic_event(const std::string& /*event_name*/)
{
	invalidate_move_maps();
}


const game_info& readonly_context_impl::get_info() const{
	return manager::get_active_ai_info_for_side(get_side());
}


game_info& readwrite_context_impl::get_info_w(){
	return manager::get_active_ai_info_for_side(get_side());
}

void readonly_context_impl::diagnostic(const std::string& msg)
{
	if(game_config::debug) {
		resources::screen->set_diagnostic(msg);
	}
}


const team& readonly_context_impl::current_team() const
{
	return (*resources::teams)[get_side()-1];
}


void readonly_context_impl::log_message(const std::string& msg)
{
	if(game_config::debug) {
		resources::screen->add_chat_message(time(NULL), "ai", get_side(), msg,
				events::chat_handler::MESSAGE_PUBLIC, false);
	}
}


void readonly_context_impl::calculate_possible_moves(std::map<map_location,pathfind::paths>& res, move_map& srcdst,
		move_map& dstsrc, bool enemy, bool assume_full_movement,
		const terrain_filter* remove_destinations) const
{
  calculate_moves(*resources::units,res,srcdst,dstsrc,enemy,assume_full_movement,remove_destinations);
}

void readonly_context_impl::calculate_moves(const unit_map& units, std::map<map_location,pathfind::paths>& res, move_map& srcdst,
		move_map& dstsrc, bool enemy, bool assume_full_movement,
		const terrain_filter* remove_destinations,
		bool see_all
          ) const
{

	for(unit_map::const_iterator un_it = units.begin(); un_it != units.end(); ++un_it) {
		// If we are looking for the movement of enemies, then this unit must be an enemy unit.
		// If we are looking for movement of our own units, it must be on our side.
		// If we are assuming full movement, then it may be a unit on our side, or allied.
		if ((enemy && current_team().is_enemy(un_it->side()) == false) ||
		    (!enemy && !assume_full_movement && un_it->side() != get_side()) ||
		    (!enemy && assume_full_movement && current_team().is_enemy(un_it->side()))) {
			continue;
		}
		// Discount incapacitated units
		if (un_it->incapacitated() ||
		    (!assume_full_movement && un_it->movement_left() == 0)) {
			continue;
		}

		// We can't see where invisible enemy units might move.
		if (enemy && un_it->invisible(un_it->get_location()) && !see_all) {
			continue;
		}
		// If it's an enemy unit, reset its moves while we do the calculations.
		unit* held_unit = const_cast<unit *>(&*un_it);
		const unit_movement_resetter move_resetter(*held_unit,enemy || assume_full_movement);

		// Insert the trivial moves of staying on the same map location.
		if (un_it->movement_left() > 0) {
			std::pair<map_location,map_location> trivial_mv(un_it->get_location(), un_it->get_location());
			srcdst.insert(trivial_mv);
			dstsrc.insert(trivial_mv);
		}
		res.insert(std::pair<map_location,pathfind::paths>(
			un_it->get_location(), pathfind::paths(*resources::game_map,
			units, un_it->get_location(), *resources::teams, false,
			true, current_team(), 0, see_all)));
	}

	// deactivate terrain filtering if it's just the dummy 'matches nothing'
	static const config only_not_tag("not");
	if(remove_destinations && remove_destinations->to_config() == only_not_tag) {
		remove_destinations = NULL;
	}

	for(std::map<map_location,pathfind::paths>::iterator m = res.begin(); m != res.end(); ++m) {
		BOOST_FOREACH(const pathfind::paths::step &dest, m->second.destinations)
		{
			const map_location& src = m->first;
			const map_location& dst = dest.curr;

			if(remove_destinations != NULL && remove_destinations->match(dst)) {
				continue;
			}

			bool friend_owns = false;

			// Don't take friendly villages
			if(!enemy && resources::game_map->is_village(dst)) {
				for(size_t n = 0; n != resources::teams->size(); ++n) {
					if((*resources::teams)[n].owns_village(dst)) {
						int side = n + 1;
						if (get_side() != side && !current_team().is_enemy(side)) {
							friend_owns = true;
						}

						break;
					}
				}
			}

			if(friend_owns) {
				continue;
			}

			if(src != dst && (find_visible_unit(dst, current_team()) == resources::units->end()) ) {
				srcdst.insert(std::pair<map_location,map_location>(src,dst));
				dstsrc.insert(std::pair<map_location,map_location>(dst,src));
			}
		}
	}
}


void readonly_context_impl::add_aspects(std::vector< aspect_ptr > &aspects )
{
	BOOST_FOREACH(aspect_ptr a, aspects) {
		const std::string id = a->get_id();
		known_aspect_map::iterator i = known_aspects_.find(id);
		if (i != known_aspects_.end()) {
			i->second->set(a);
		} else {
			ERR_AI << "when adding aspects, unknown aspect id["<<id<<"]"<<std::endl;
		}
	}
}

void readonly_context_impl::add_facet(const std::string &id, const config &cfg) const
{
	known_aspect_map::const_iterator i = known_aspects_.find(id);
	if (i != known_aspects_.end()) {
		i->second->add_facet(cfg);
	} else {
		ERR_AI << "when adding aspects, unknown aspect id["<<id<<"]"<<std::endl;
	}
}

const defensive_position& readonly_context_impl::best_defensive_position(const map_location& loc,
		const move_map& dstsrc, const move_map& srcdst, const move_map& enemy_dstsrc) const
{
	const unit_map::const_iterator itor = resources::units->find(loc);
	if(itor == resources::units->end()) {
		static defensive_position pos;
		pos.chance_to_hit = 0;
		pos.vulnerability = pos.support = 0;
		return pos;
	}

	const std::map<map_location,defensive_position>::const_iterator position =
		defensive_position_cache_.find(loc);

	if(position != defensive_position_cache_.end()) {
		return position->second;
	}

	defensive_position pos;
	pos.chance_to_hit = 100;
	pos.vulnerability = 10000.0;
	pos.support = 0.0;

	typedef move_map::const_iterator Itor;
	const std::pair<Itor,Itor> itors = srcdst.equal_range(loc);
	for(Itor i = itors.first; i != itors.second; ++i) {
		const int defense = itor->defense_modifier(resources::game_map->get_terrain(i->second));
		if(defense > pos.chance_to_hit) {
			continue;
		}

		const double vulnerability = power_projection(i->second,enemy_dstsrc);
		const double support = power_projection(i->second,dstsrc);

		if(defense < pos.chance_to_hit || support - vulnerability > pos.support - pos.vulnerability) {
			pos.loc = i->second;
			pos.chance_to_hit = defense;
			pos.vulnerability = vulnerability;
			pos.support = support;
		}
	}

	defensive_position_cache_.insert(std::pair<map_location,defensive_position>(loc,pos));
	return defensive_position_cache_[loc];
}


std::map<map_location,defensive_position>& readonly_context_impl::defensive_position_cache() const
{
	return defensive_position_cache_;
}


double readonly_context_impl::get_aggression() const
{
	if (aggression_) {
		return aggression_->get();
	}
	return 0;
}


int readonly_context_impl::get_attack_depth() const
{
	if (attack_depth_) {
		return std::max<int>(1,attack_depth_->get()); ///@todo 1.9: add validators, such as minmax filters to aspects
	}
	return 1;
}


const aspect_map& readonly_context_impl::get_aspects() const
{
	return aspects_;
}


aspect_map& readonly_context_impl::get_aspects()
{
	return aspects_;
}


const attacks_vector& readonly_context_impl::get_attacks() const
{
	if (attacks_) {
		return attacks_->get();
	}
	static attacks_vector av;
	return av;
}


const variant& readonly_context_impl::get_attacks_as_variant() const
{
	if (attacks_) {
		return attacks_->get_variant();
	}
	static variant v;///@todo 1.9: replace with variant::null_variant;
	return v;
}

const terrain_filter& readonly_context_impl::get_avoid() const
{
	if (avoid_) {
		return avoid_->get();
	}
	config cfg;
	cfg.add_child("not");
	static terrain_filter tf(vconfig(cfg),*resources::units);
	return tf;
}


double readonly_context_impl::get_caution() const
{
	if (caution_) {
		return caution_->get();
	}
	return 0;
}

const move_map& readonly_context_impl::get_dstsrc() const
{
	if (!move_maps_valid_) {
		recalculate_move_maps();
	}
	return dstsrc_;
}


const move_map& readonly_context_impl::get_enemy_dstsrc() const
{
	if (!move_maps_enemy_valid_) {
		recalculate_move_maps_enemy();
	}
	return enemy_dstsrc_;
}


const moves_map& readonly_context_impl::get_enemy_possible_moves() const
{
	if (!move_maps_enemy_valid_) {
		recalculate_move_maps_enemy();
	}
	return enemy_possible_moves_;
}


const move_map& readonly_context_impl::get_enemy_srcdst() const
{
	if (!move_maps_enemy_valid_) {
		recalculate_move_maps_enemy();
	}
	return enemy_srcdst_;
}


engine_ptr readonly_context_impl::get_engine_by_cfg(const config& cfg)
{
	std::string engine_name = cfg["engine"];
	if (engine_name.empty()) {
		engine_name="cpp";//default engine
	}

	std::vector<engine_ptr>::iterator en = engines_.begin();
	while (en!=engines_.end() && ((*en)->get_name()!=engine_name) && ((*en)->get_id()!=engine_name)) {
		++en;
	}

	if (en != engines_.end()){
		return *en;
	}

	//TODO: fix, removing some code duplication
	engine_factory::factory_map::iterator eng = engine_factory::get_list().find(engine_name);
	if (eng == engine_factory::get_list().end()){
		ERR_AI << "side "<<get_side()<<" : UNABLE TO FIND engine["<<
			engine_name <<"]" << std::endl;
		DBG_AI << "config snippet contains: " << std::endl << cfg << std::endl;
		return engine_ptr();
	}

	engine_ptr new_engine = eng->second->get_new_instance(*this,engine_name);
	if (!new_engine) {
		ERR_AI << "side "<<get_side()<<" : UNABLE TO CREATE engine["<<
			engine_name <<"] " << std::endl;
		DBG_AI << "config snippet contains: " << std::endl << cfg << std::endl;
		return engine_ptr();
	}
	engines_.push_back(new_engine);
	return engines_.back();
}


const std::vector<engine_ptr>& readonly_context_impl::get_engines() const
{
	return engines_;
}


std::vector<engine_ptr>& readonly_context_impl::get_engines()
{
	return engines_;
}


std::string readonly_context_impl::get_grouping() const
{
	if (grouping_) {
		return grouping_->get();
	}
	return std::string();
}


const std::vector<goal_ptr>& readonly_context_impl::get_goals() const
{
	return goals_;
}


std::vector<goal_ptr>& readonly_context_impl::get_goals()
{
	return goals_;
}



double readonly_context_impl::get_leader_aggression() const
{
	if (leader_aggression_) {
		return leader_aggression_->get();
	}
	return 0;
}


config readonly_context_impl::get_leader_goal() const
{
	if (leader_goal_) {
		return leader_goal_->get();
	}
	return config();
}


double readonly_context_impl::get_leader_value() const
{
	if (leader_value_) {
		return leader_value_->get();
	}
	return 0;
}


double readonly_context_impl::get_number_of_possible_recruits_to_force_recruit() const
{
	if (number_of_possible_recruits_to_force_recruit_) {
		return number_of_possible_recruits_to_force_recruit_->get();
	}
	return 0;
}


bool readonly_context_impl::get_passive_leader() const
{
	if (passive_leader_) {
		return passive_leader_->get();
	}
	return false;
}


bool readonly_context_impl::get_passive_leader_shares_keep() const
{
	if (passive_leader_shares_keep_) {
		return passive_leader_shares_keep_->get();
	}
	return false;
}


const moves_map& readonly_context_impl::get_possible_moves() const
{
	if (!move_maps_valid_) {
		recalculate_move_maps();
	}
	return possible_moves_;
}


const std::vector<unit>& readonly_context_impl::get_recall_list() const
{
	static std::vector<unit> dummy_units;
	///@todo 1.9: check for (level_["disallow_recall"]))
	if(!current_team().persistent()) {
		return dummy_units;
	}

	return current_team().recall_list();
}

stage_ptr readonly_context_impl::get_recruitment(ai_context &context) const
{
	if (recruitment_) {
		ministage_ptr m = recruitment_->get_ptr();
		if (m) {
			return m->get_stage_ptr(context);
		}
	}
	return stage_ptr();
}


bool readonly_context_impl::get_recruitment_ignore_bad_combat() const
{
	if (recruitment_ignore_bad_combat_) {
		return recruitment_ignore_bad_combat_->get();
	}
	return false;
}


bool readonly_context_impl::get_recruitment_ignore_bad_movement() const
{
	if (recruitment_ignore_bad_movement_) {
		return recruitment_ignore_bad_movement_->get();
	}
	return false;
}


const std::vector<std::string> readonly_context_impl::get_recruitment_pattern() const
{
	if (recruitment_pattern_) {
		return recruitment_pattern_->get();
	}
	return std::vector<std::string>();
}


double readonly_context_impl::get_scout_village_targeting() const
{
	if (scout_village_targeting_) {
		return scout_village_targeting_->get();
	}
	return 1;
}


bool readonly_context_impl::get_simple_targeting() const
{
	if (simple_targeting_) {
		return simple_targeting_->get();
	}
	return false;
}


const move_map& readonly_context_impl::get_srcdst() const
{
	if (!move_maps_valid_) {
		recalculate_move_maps();
	}
	return srcdst_;
}


bool readonly_context_impl::get_support_villages() const
{
	if (support_villages_) {
		return support_villages_->get();
	}
	return false;
}


double readonly_context_impl::get_village_value() const
{
	if (village_value_) {
		return village_value_->get();
	}
	return 0;
}


int readonly_context_impl::get_villages_per_scout() const
{
	if (villages_per_scout_) {
		return villages_per_scout_->get();
	}
	return 0;
}



void readonly_context_impl::invalidate_defensive_position_cache() const
{
	defensive_position_cache_.clear();
}


void readonly_context_impl::invalidate_keeps_cache() const
{
	keeps_.clear();
}


void keeps_cache::handle_generic_event(const std::string &/*event_name*/)
{
	clear();
}


void readonly_context_impl::invalidate_move_maps() const
{
	move_maps_valid_ = false;
	move_maps_enemy_valid_ = false;
}


const std::set<map_location>& readonly_context_impl::keeps() const
{
	return keeps_.get();
}


keeps_cache::keeps_cache()
	: map_(NULL)
	, keeps_()
{
	ai::manager::add_turn_started_observer(this);
	ai::manager::add_map_changed_observer(this);
}


keeps_cache::~keeps_cache()
{
	ai::manager::remove_turn_started_observer(this);
	ai::manager::remove_map_changed_observer(this);
}

void keeps_cache::clear()
{
	keeps_.clear();
}


void keeps_cache::init(gamemap &map)
{
	map_ = &map;
}

const std::set<map_location>& keeps_cache::get()
{
	if(keeps_.empty()) {
		// Generate the list of keeps:
		// iterate over the entire map and find all keeps.
		for(size_t x = 0; x != size_t(map_->w()); ++x) {
			for(size_t y = 0; y != size_t(map_->h()); ++y) {
				const map_location loc(x,y);
				if(map_->is_keep(loc)) {
					map_location adj[6];
					get_adjacent_tiles(loc,adj);
					for(size_t n = 0; n != 6; ++n) {
						if(map_->is_castle(adj[n])) {
							keeps_.insert(loc);
							break;
						}
					}
				}
			}
		}
	}

	return keeps_;
}


bool readonly_context_impl::leader_can_reach_keep() const
{
	const unit_map::iterator leader = resources::units->find_leader(get_side());
	if(leader == resources::units->end() || leader->incapacitated()) {
		return false;
	}

	const map_location &start_pos = nearest_keep(leader->get_location());
	if(start_pos.valid() == false) {
		return false;
	}

	if (leader->get_location() == start_pos) {
		return true;
	}

	// Find where the leader can move
	const pathfind::paths leader_paths(*resources::game_map, *resources::units,
		leader->get_location(), *resources::teams, false, true, current_team());

	return leader_paths.destinations.contains(start_pos);
}


const map_location& readonly_context_impl::nearest_keep(const map_location& loc) const
{
	std::set<map_location> avoided_locations;
	get_avoid().get_locations(avoided_locations);
	const std::set<map_location>& keeps = this->keeps();
	if(keeps.empty()) {
		static const map_location dummy;
		return dummy;
	}

	const map_location* res = NULL;
	int closest = -1;
	for(std::set<map_location>::const_iterator i = keeps.begin(); i != keeps.end(); ++i) {
		if (avoided_locations.find(*i)!=avoided_locations.end()) {
			continue;
		}
		const int distance = distance_between(*i,loc);
		if(res == NULL || distance < closest) {
			closest = distance;
			res = &*i;
		}
	}
	if (res) {
		return *res;
	} else {
		return map_location::null_location;
	}
}


double readonly_context_impl::power_projection(const map_location& loc, const move_map& dstsrc) const
{
	map_location used_locs[6];
	int ratings[6];
	int num_used_locs = 0;

	map_location locs[6];
	get_adjacent_tiles(loc,locs);

	const int lawful_bonus = resources::tod_manager->get_time_of_day().lawful_bonus;
	gamemap& map_ = *resources::game_map;
	unit_map& units_ = *resources::units;

	int res = 0;

	bool changed = false;
	for (int i = 0;; ++i) {
		if (i == 6) {
			if (!changed) break;
			// Loop once again, in case a unit found a better spot
			// and freed the place for another unit.
			changed = false;
			i = 0;
		}

		if (map_.on_board(locs[i]) == false) {
			continue;
		}

		const t_translation::t_terrain terrain = map_[locs[i]];

		typedef move_map::const_iterator Itor;
		typedef std::pair<Itor,Itor> Range;
		Range its = dstsrc.equal_range(locs[i]);

		map_location* const beg_used = used_locs;
		map_location* end_used = used_locs + num_used_locs;

		int best_rating = 0;
		map_location best_unit;

		for(Itor it = its.first; it != its.second; ++it) {
			const unit_map::const_iterator u = units_.find(it->second);

			// Unit might have been killed, and no longer exist
			if(u == units_.end()) {
				continue;
			}

			const unit& un = *u;

			int tod_modifier = 0;
			if(un.alignment() == unit_type::LAWFUL) {
				tod_modifier = lawful_bonus;
			} else if(un.alignment() == unit_type::CHAOTIC) {
				tod_modifier = -lawful_bonus;
			} else if(un.alignment() == unit_type::LIMINAL) {
				tod_modifier = -(abs(lawful_bonus));
			}

			// The 0.5 power avoids underestimating too much the damage of a wounded unit.
			int hp = int(sqrt(double(un.hitpoints()) / un.max_hitpoints()) * 1000);
			int most_damage = 0;
			BOOST_FOREACH(const attack_type &att, un.attacks())
			{
				int damage = att.damage() * att.num_attacks() * (100 + tod_modifier);
				if (damage > most_damage) {
					most_damage = damage;
				}
			}

			int village_bonus = map_.is_village(terrain) ? 3 : 2;
			int defense = 100 - un.defense_modifier(terrain);
			int rating = hp * defense * most_damage * village_bonus / 200;
			if(rating > best_rating) {
				map_location *pos = std::find(beg_used, end_used, it->second);
				// Check if the spot is the same or better than an older one.
				if (pos == end_used || rating >= ratings[pos - beg_used]) {
					best_rating = rating;
					best_unit = it->second;
				}
			}
		}

		if (!best_unit.valid()) continue;
		map_location *pos = std::find(beg_used, end_used, best_unit);
		int index = pos - beg_used;
		if (index == num_used_locs)
			++num_used_locs;
		else if (best_rating == ratings[index])
			continue;
		else {
			// The unit was in another spot already, so remove its older rating
			// from the final result, and require a new run to fill its old spot.
			res -= ratings[index];
			changed = true;
		}
		used_locs[index] = best_unit;
		ratings[index] = best_rating;
		res += best_rating;
	}

	return res / 100000.;
}

void readonly_context_impl::recalculate_move_maps() const
{
	dstsrc_ = move_map();
	possible_moves_ = moves_map();
	srcdst_ = move_map();
	calculate_possible_moves(possible_moves_,srcdst_,dstsrc_,false,false,&get_avoid());
	if (get_passive_leader()||get_passive_leader_shares_keep()) {
		unit_map::iterator i = resources::units->find_leader(get_side());
		if (i.valid()) {
			map_location loc = i->get_location();
			srcdst_.erase(loc);
			for(move_map::iterator i = dstsrc_.begin(); i != dstsrc_.end(); ) {
				if(i->second == loc) {
					dstsrc_.erase(i++);
				} else {
					++i;
				}
			}
		///@todo 1.9: shall possible moves be modified as well ?
		}
	}
	move_maps_valid_ = true;
}


void readonly_context_impl::recalculate_move_maps_enemy() const
{
	enemy_dstsrc_ = move_map();
	enemy_srcdst_ = move_map();
	enemy_possible_moves_ = moves_map();
	calculate_possible_moves(enemy_possible_moves_,enemy_srcdst_,enemy_dstsrc_,true);
	move_maps_enemy_valid_ = true;
}


const map_location& readonly_context_impl::suitable_keep(const map_location& leader_location, const pathfind::paths& leader_paths){
	if (resources::game_map->is_keep(leader_location)) {
		return leader_location; //if leader already on keep, then return leader_location
	}

	map_location const* best_free_keep = &map_location::null_location;
	double move_left_at_best_free_keep = 0.0;

	map_location const* best_occupied_keep = &map_location::null_location;
	double move_left_at_best_occupied_keep = 0.0;

	BOOST_FOREACH(const pathfind::paths::step &dest, leader_paths.destinations)
	{
		const map_location &loc = dest.curr;
		if (keeps().find(loc)!=keeps().end()){

			const int move_left_at_loc = dest.move_left;
			if (resources::units->count(loc) == 0) {
				if ((*best_free_keep==map_location::null_location)||(move_left_at_loc>move_left_at_best_free_keep)){
					best_free_keep = &loc;
					move_left_at_best_free_keep = move_left_at_loc;
				}
			} else {
				if ((*best_occupied_keep==map_location::null_location)||(move_left_at_loc>move_left_at_best_occupied_keep)){
					best_occupied_keep = &loc;
				        move_left_at_best_occupied_keep = move_left_at_loc;
				}
			}
		}
	}

	if (*best_free_keep != map_location::null_location){
		return *best_free_keep; // if there is a free keep reachable during current turn, return it
	}

	if (*best_occupied_keep != map_location::null_location){
		return *best_occupied_keep; // if there is an occupied keep reachable during current turn, return it
	}

	return nearest_keep(leader_location); // return nearest keep
}


	/** Weapon choice cache, to speed simulations. */
std::map<std::pair<map_location,const unit_type *>,
		std::pair<battle_context_unit_stats,battle_context_unit_stats> >& readonly_context_impl::unit_stats_cache() const
{
	return unit_stats_cache_;
}


bool readonly_context_impl::is_active(const std::string &time_of_day, const std::string &turns) const
{
		if(time_of_day.empty() == false) {
			const std::vector<std::string>& times = utils::split(time_of_day);
			if(std::count(times.begin(),times.end(),resources::tod_manager->get_time_of_day().name) == 0) {
				return false;
			}
		}

		if(turns.empty() == false) {
			int turn = resources::tod_manager->turn();
			const std::vector<std::string>& turns_list = utils::split(turns);
			for(std::vector<std::string>::const_iterator j = turns_list.begin(); j != turns_list.end() ; ++j ) {
				const std::pair<int,int> range = utils::parse_range(*j);
				if(turn >= range.first && turn <= range.second) {
				      return true;
				}
			}
			return false;
		}
		return true;
}

} //of namespace ai
