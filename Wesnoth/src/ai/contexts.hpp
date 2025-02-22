/* $Id: contexts.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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
 * @file
 * Helper functions for the object which operates in the context of AI for specific side
 * this is part of AI interface
 */

#ifndef AI_CONTEXTS_HPP_INCLUDED
#define AI_CONTEXTS_HPP_INCLUDED

#include "game_info.hpp"
#include "../generic_event.hpp"
#include "../config.hpp"


//#include "../unit.hpp"

#ifdef _MSC_VER
#pragma warning(push)
//silence "inherits via dominance" warnings
#pragma warning(disable:4250)
#endif

class battle_context;
struct battle_context_unit_stats;
class game_display;
class gamemap;
class variant;
class terrain_filter;
class terrain_translation;
class unit;
class unit_type;

namespace ai {

class interface;
class ai_context;

typedef ai_context* ai_context_ptr;


// recursion counter
class recursion_counter {
public:
	recursion_counter(int counter)
		: counter_(++counter)
	{
		if (counter > MAX_COUNTER_VALUE ) {
			throw game::game_error("maximum recursion depth reached!");
		}
	}


	/**
	 * Get the current value of the recursion counter
	 */
	int get_count() const
	{
		return counter_;
	}


	//max recursion depth
	static const int MAX_COUNTER_VALUE = 100;


	/**
	 * Check if more recursion is allowed
	 */
	bool is_ok() const
	{
		return counter_ < MAX_COUNTER_VALUE;
	}
private:

	// recursion counter value
	int counter_;
};

//defensive position

struct defensive_position {
	defensive_position() :
		loc(),
		chance_to_hit(0),
		vulnerability(0.0),
		support(0.0)
		{}

	map_location loc;
	int chance_to_hit;
	double vulnerability, support;
};

// keeps cache
class keeps_cache : public events::observer
{
public:
	keeps_cache();
	~keeps_cache();
	void handle_generic_event(const std::string& event_name);
	void clear();
	const std::set<map_location>& get();
	void init(gamemap &map);
private:
	gamemap *map_;
	std::set<map_location> keeps_;
};

// side context

class side_context;

class side_context{
public:

	/**
	 * Get the side number
	 */
	virtual side_number get_side() const  = 0;


	/**
	 * Set the side number
	 */
	virtual void set_side(side_number side) = 0;


	/**
	 * empty destructor
	 */
	virtual ~side_context(){}


	/**
	 * empty constructor
	 */
	side_context() {}


	/**
	 * unwrap
	 */
	virtual side_context& get_side_context() = 0;


	/**
	 * serialize this context to config
	 */
	virtual config to_side_context_config() const = 0;


	/**
	 * Get the value of the recursion counter
	 */
	virtual int get_recursion_count() const = 0;


};

class readonly_context;
class readonly_context : public virtual side_context {
public:
	readonly_context(){}
	virtual ~readonly_context(){}
	virtual readonly_context& get_readonly_context() = 0;
	virtual void on_readonly_context_create() = 0;
	virtual const team& current_team() const = 0;
	virtual void diagnostic(const std::string& msg) = 0;
	virtual void log_message(const std::string& msg) = 0;
	virtual attack_result_ptr check_attack_action(const map_location& attacker_loc, const map_location& defender_loc, int attacker_weapon) = 0;
	virtual move_result_ptr check_move_action(const map_location& from, const map_location& to, bool remove_movement=true) = 0;
	virtual recall_result_ptr check_recall_action(const std::string& id, const map_location &where = map_location::null_location, const map_location &from = map_location::null_location) = 0;
	virtual recruit_result_ptr check_recruit_action(const std::string& unit_name, const map_location &where = map_location::null_location, const map_location &from = map_location::null_location) = 0;
	virtual stopunit_result_ptr check_stopunit_action(const map_location& unit_location, bool remove_movement = true, bool remove_attacks = false) = 0;
	virtual void calculate_possible_moves(std::map<map_location,pathfind::paths>& possible_moves,
		move_map& srcdst, move_map& dstsrc, bool enemy,
		bool assume_full_movement=false,
		const terrain_filter* remove_destinations=NULL) const = 0;
	virtual void calculate_moves(const unit_map& units,
		std::map<map_location,pathfind::paths>& possible_moves, move_map& srcdst,
		move_map& dstsrc, bool enemy, bool assume_full_movement=false,
		const terrain_filter* remove_destinations=NULL,
		bool see_all=false) const = 0;

	virtual const game_info& get_info() const = 0;


	//@note: following part is in alphabetic order
	virtual defensive_position const& best_defensive_position(const map_location& unit,
			const move_map& dstsrc, const move_map& srcdst, const move_map& enemy_dstsrc) const = 0;


	virtual std::map<map_location,defensive_position>& defensive_position_cache() const = 0;


	virtual double get_aggression() const = 0;


	virtual int get_attack_depth() const = 0;


	virtual const aspect_map& get_aspects() const = 0;


	virtual aspect_map& get_aspects() = 0;


	virtual void add_facet(const std::string &id, const config &cfg) const = 0;


	virtual void add_aspects(std::vector< aspect_ptr > &aspects ) = 0;


	virtual const attacks_vector& get_attacks() const = 0;


	virtual const variant& get_attacks_as_variant() const = 0;


	virtual const terrain_filter& get_avoid() const = 0;


	virtual double get_caution() const = 0;


	virtual const move_map& get_dstsrc() const = 0;


	virtual const move_map& get_enemy_dstsrc() const = 0;


	virtual const moves_map& get_enemy_possible_moves() const = 0;


	virtual const move_map& get_enemy_srcdst() const = 0;

	/**
	 * get engine by cfg, creating it if it is not created yet but known
	 */
	virtual engine_ptr get_engine_by_cfg(const config& cfg) = 0;


	virtual const std::vector<engine_ptr>& get_engines() const = 0;


	virtual std::vector<engine_ptr>& get_engines() = 0;


	virtual std::string get_grouping() const = 0;


	virtual const std::vector<goal_ptr>& get_goals() const = 0;


	virtual std::vector<goal_ptr>& get_goals() = 0;


	virtual double get_leader_aggression() const = 0;


	virtual config get_leader_goal() const = 0;


	virtual double get_leader_value() const = 0;


	virtual double get_number_of_possible_recruits_to_force_recruit() const = 0;


	virtual bool get_passive_leader() const = 0;


	virtual bool get_passive_leader_shares_keep() const = 0;


	virtual const moves_map& get_possible_moves() const = 0;


	virtual const std::vector<unit>& get_recall_list() const = 0;


	virtual stage_ptr get_recruitment(ai_context &context) const = 0;


	virtual bool get_recruitment_ignore_bad_combat() const = 0;


	virtual bool get_recruitment_ignore_bad_movement() const = 0;


	virtual const std::vector<std::string> get_recruitment_pattern() const = 0;


	virtual double get_scout_village_targeting() const = 0;


	virtual bool get_simple_targeting() const = 0;


	virtual const move_map& get_srcdst() const = 0;


	virtual bool get_support_villages() const = 0;


	virtual double get_village_value() const = 0;


	virtual int get_villages_per_scout() const = 0;


	virtual bool is_active(const std::string &time_of_day, const std::string &turns) const = 0;


	virtual void invalidate_defensive_position_cache() const = 0;


	virtual void invalidate_move_maps() const = 0;


	virtual void invalidate_keeps_cache() const= 0;


	virtual const std::set<map_location>& keeps() const= 0;


	virtual bool leader_can_reach_keep() const = 0;


	virtual const map_location& nearest_keep(const map_location& loc) const = 0;


	/**
	 * Function which finds how much 'power' a side can attack a certain location with.
	 * This is basically the maximum hp of damage that can be inflicted upon a unit on loc
	 * by full-health units, multiplied by the defense these units will have.
	 * (if 'use_terrain' is false, then it will be multiplied by 0.5)
	 *
	 * Example: 'loc' can be reached by two units, one of whom has a 10-3 attack
	 * and has 48/48 hp, and can defend at 40% on the adjacent grassland.
	 * The other has a 8-2 attack, and has 30/40 hp, and can defend at 60% on the adjacent mountain.
	 * The rating will be 10*3*1.0*0.4 + 8*2*0.75*0.6 = 19.2
	 */
	virtual double power_projection(const map_location& loc, const move_map& dstsrc) const = 0;


	virtual void raise_user_interact() const = 0;


	virtual void recalculate_move_maps() const = 0;


	virtual void recalculate_move_maps_enemy() const = 0;

	/** get most suitable keep for leader - nearest free that can be reached in 1 turn, if none - return nearest occupied that can be reached in 1 turn, if none - return nearest keep, if none - return null_location */
	virtual const map_location& suitable_keep( const map_location& leader_location, const pathfind::paths& leader_paths ) = 0;

	/**
	 * serialize to config
	 */
	virtual config to_readonly_context_config() const = 0;


	virtual std::map<std::pair<map_location,const unit_type *>,
		std::pair<battle_context_unit_stats,battle_context_unit_stats> >& unit_stats_cache() const = 0;

};

class readwrite_context;
class readwrite_context : public virtual readonly_context {
public:
	readwrite_context(){}


	virtual ~readwrite_context(){}


	virtual readwrite_context& get_readwrite_context() = 0;


	virtual attack_result_ptr execute_attack_action(const map_location& attacker_loc, const map_location& defender_loc, int attacker_weapon) = 0;


	virtual move_result_ptr execute_move_action(const map_location& from, const map_location& to, bool remove_movement=true) = 0;


	virtual recall_result_ptr execute_recall_action(const std::string& id, const map_location &where = map_location::null_location, const map_location &from = map_location::null_location) = 0;


	virtual recruit_result_ptr execute_recruit_action(const std::string& unit_name, const map_location &where = map_location::null_location, const map_location &from = map_location::null_location) = 0;


	virtual stopunit_result_ptr execute_stopunit_action(const map_location& unit_location, bool remove_movement = true, bool remove_attacks = false) = 0;


	virtual team& current_team_w() = 0;


	virtual void raise_gamestate_changed() const = 0;


	virtual game_info& get_info_w() = 0;


	/**
	 * serialize this context to config
	 */
	virtual config to_readwrite_context_config() const = 0;

};

//proxies

class side_context_proxy : public virtual side_context {
public:
	side_context_proxy()
		: target_(NULL)
	{
	}

	virtual ~side_context_proxy(){}


	void init_side_context_proxy(side_context &target)
	{
		target_= &target.get_side_context();
	}

	virtual side_number get_side() const
	{
		return target_->get_side();
	}

	virtual void set_side(side_number side)
	{
		return target_->set_side(side);
	}

	virtual side_context& get_side_context()
	{
		return target_->get_side_context();
	}

	virtual int get_recursion_count() const
	{
		return target_->get_recursion_count();
	}


	virtual config to_side_context_config() const
	{
		return target_->to_side_context_config();
	}


private:
	side_context *target_;
};


class readonly_context_proxy : public virtual readonly_context, public virtual side_context_proxy {
public:
	readonly_context_proxy()
		: target_(NULL)
	{
	}

	virtual ~readonly_context_proxy() {}

	void init_readonly_context_proxy(readonly_context &target)
	{
		init_side_context_proxy(target);
		target_ = &target.get_readonly_context();
	}

	virtual readonly_context& get_readonly_context()
	{
		return target_->get_readonly_context();
	}


	virtual void on_readonly_context_create()
	{
		return target_->on_readonly_context_create();
	}


	virtual const team& current_team() const
	{
		return target_->current_team();
	}

	virtual void diagnostic(const std::string& msg)
	{
		target_->diagnostic(msg);
	}

	virtual void log_message(const std::string& msg)
	{
		target_->log_message(msg);
	}

	virtual attack_result_ptr check_attack_action(const map_location &attacker_loc, const map_location &defender_loc, int attacker_weapon)
	{
		return target_->check_attack_action(attacker_loc, defender_loc, attacker_weapon);
	}

	virtual move_result_ptr check_move_action(const map_location &from, const map_location &to, bool remove_movement=true)
	{
		return target_->check_move_action(from, to, remove_movement);
	}


	virtual recall_result_ptr check_recall_action(const std::string &id, const map_location &where = map_location::null_location,
			const map_location &from = map_location::null_location)
	{
		return target_->check_recall_action(id, where, from);
	}


	virtual recruit_result_ptr check_recruit_action(const std::string &unit_name, const map_location &where = map_location::null_location,
			const map_location &from = map_location::null_location)
	{
		return target_->check_recruit_action(unit_name, where, from);
	}

	virtual stopunit_result_ptr check_stopunit_action(const map_location &unit_location, bool remove_movement = true, bool remove_attacks = false)
	{
		return target_->check_stopunit_action(unit_location, remove_movement, remove_attacks);
	}

	virtual void calculate_possible_moves(std::map<map_location,pathfind::paths>& possible_moves,
		move_map& srcdst, move_map& dstsrc, bool enemy,
		bool assume_full_movement=false,
		const terrain_filter* remove_destinations=NULL) const
	{
		target_->calculate_possible_moves(possible_moves, srcdst, dstsrc, enemy, assume_full_movement, remove_destinations);
	}

	virtual void calculate_moves(const unit_map& units,
		std::map<map_location,pathfind::paths>& possible_moves, move_map& srcdst,
		move_map& dstsrc, bool enemy, bool assume_full_movement=false,
		const terrain_filter* remove_destinations=NULL,
		bool see_all=false) const
	{
		target_->calculate_moves(units, possible_moves, srcdst, dstsrc, enemy, assume_full_movement, remove_destinations, see_all);
	}

	virtual const game_info& get_info() const
	{
		return target_->get_info();
	}

	virtual void raise_user_interact() const
	{
		target_->raise_user_interact();
	}


	virtual int get_recursion_count() const
	{
		return target_->get_recursion_count();
	}

	//@note: following part is in alphabetic order
	defensive_position const& best_defensive_position(const map_location& unit,
			const move_map& dstsrc, const move_map& srcdst, const move_map& enemy_dstsrc) const
	{
		return target_->best_defensive_position(unit,dstsrc,srcdst,enemy_dstsrc);
	}


	virtual std::map<map_location,defensive_position>& defensive_position_cache() const
	{
		return target_->defensive_position_cache();
	}


	virtual double get_aggression() const
	{
		return target_->get_aggression();
	}


	virtual int get_attack_depth() const
	{
		return target_->get_attack_depth();
	}


	virtual const aspect_map& get_aspects() const
	{
		return target_->get_aspects();
	}


	virtual aspect_map& get_aspects()
	{
		return target_->get_aspects();
	}


	virtual void add_aspects(std::vector< aspect_ptr > &aspects )
	{
		return target_->add_aspects(aspects);
	}


	virtual void add_facet(const std::string &id, const config &cfg) const
	{
		target_->add_facet(id,cfg);
	}



	virtual const attacks_vector& get_attacks() const
	{
		return target_->get_attacks();
	}



	virtual const variant& get_attacks_as_variant() const
	{
		return target_->get_attacks_as_variant();
	}


	virtual const terrain_filter& get_avoid() const
	{
		return target_->get_avoid();
	}


	virtual double get_caution() const
	{
		return target_->get_caution();
	}


	virtual const move_map& get_dstsrc() const
	{
		return target_->get_dstsrc();
	}


	virtual const move_map& get_enemy_dstsrc() const
	{
		return target_->get_enemy_dstsrc();
	}


	virtual const moves_map& get_enemy_possible_moves() const
	{
		return target_->get_enemy_possible_moves();
	}


	virtual const move_map& get_enemy_srcdst() const
	{
		return target_->get_enemy_srcdst();
	}


	virtual engine_ptr get_engine_by_cfg(const config &cfg)
	{
		return target_->get_engine_by_cfg(cfg);
	}


	virtual const std::vector<engine_ptr>& get_engines() const
	{
		return target_->get_engines();
	}


	virtual std::vector<engine_ptr>& get_engines()
	{
		return target_->get_engines();
	}


	virtual std::string get_grouping() const
	{
		return target_->get_grouping();
	}


	virtual const std::vector<goal_ptr>& get_goals() const
	{
		return target_->get_goals();
	}


	virtual std::vector<goal_ptr>& get_goals()
	{
		return target_->get_goals();
	}


	virtual double get_leader_aggression() const
	{
		return target_->get_leader_aggression();
	}



	virtual config get_leader_goal() const
	{
		return target_->get_leader_goal();
	}


	virtual double get_leader_value() const
	{
		return target_->get_leader_value();
	}


	virtual double get_number_of_possible_recruits_to_force_recruit() const
	{
		return target_->get_number_of_possible_recruits_to_force_recruit();
	}


	virtual bool get_passive_leader() const
	{
		return target_->get_passive_leader();
	}


	virtual bool get_passive_leader_shares_keep() const
	{
		return target_->get_passive_leader_shares_keep();
	}


	virtual const moves_map& get_possible_moves() const
	{
		return target_->get_possible_moves();
	}


	virtual double power_projection(const map_location& loc, const move_map& dstsrc) const
	{
		return target_->power_projection(loc,dstsrc);
	}


	virtual const std::vector<unit>& get_recall_list() const
	{
		return target_->get_recall_list();
	}


	virtual stage_ptr get_recruitment(ai_context &context) const
	{
		return target_->get_recruitment(context);
	}


	virtual bool get_recruitment_ignore_bad_combat() const
	{
		return target_->get_recruitment_ignore_bad_combat();
	}


	virtual bool get_recruitment_ignore_bad_movement() const
	{
		return target_->get_recruitment_ignore_bad_movement();
	}


	virtual const std::vector<std::string> get_recruitment_pattern() const
	{
		return target_->get_recruitment_pattern();
	}


	virtual const move_map& get_srcdst() const
	{
		return target_->get_srcdst();
	}


	virtual double get_scout_village_targeting() const
	{
		return target_->get_scout_village_targeting();
	}


	virtual bool get_simple_targeting() const
	{
		return target_->get_simple_targeting();
	}


	virtual bool get_support_villages() const
	{
		return target_->get_support_villages();
	}


	virtual double get_village_value() const
	{
		return target_->get_village_value();
	}


	virtual int get_villages_per_scout() const
	{
		return target_->get_villages_per_scout();
	}



	virtual bool is_active(const std::string &time_of_day, const std::string &turns) const
	{
		return target_->is_active(time_of_day, turns);
	}


	virtual void invalidate_defensive_position_cache() const
	{
		return target_->invalidate_defensive_position_cache();
	}


	virtual void invalidate_move_maps() const
	{
		target_->invalidate_move_maps();
	}


	virtual void invalidate_keeps_cache() const
	{
		return target_->invalidate_keeps_cache();
	}


	virtual const std::set<map_location>& keeps() const
	{
		return target_->keeps();
	}


	virtual bool leader_can_reach_keep() const
	{
		return target_->leader_can_reach_keep();
	}


	virtual const map_location& nearest_keep( const map_location& loc ) const
	{
		return target_->nearest_keep(loc);
	}


	virtual void recalculate_move_maps() const
	{
		target_->recalculate_move_maps();
	}


	virtual void recalculate_move_maps_enemy() const
	{
		target_->recalculate_move_maps_enemy();
	}


	virtual const map_location& suitable_keep( const map_location& leader_location, const pathfind::paths& leader_paths )
	{
		return target_->suitable_keep(leader_location, leader_paths);
	}


	virtual config to_readonly_context_config() const
	{
		return target_->to_readonly_context_config();
	}


	virtual std::map<std::pair<map_location,const unit_type *>,
		std::pair<battle_context_unit_stats,battle_context_unit_stats> >& unit_stats_cache() const
	{
		return target_->unit_stats_cache();
	}


private:
	readonly_context *target_;
};


class readwrite_context_proxy : public virtual readwrite_context, public virtual readonly_context_proxy {
public:
	readwrite_context_proxy()
		: target_(NULL)
	{
	}


	void init_readwrite_context_proxy(readwrite_context &target)
	{
		init_readonly_context_proxy(target);
		target_ = &target.get_readwrite_context();
	}


	virtual readwrite_context& get_readwrite_context()
	{
		return target_->get_readwrite_context();
	}


	virtual attack_result_ptr execute_attack_action(const map_location& attacker_loc, const map_location& defender_loc, int attacker_weapon)
	{
		return target_->execute_attack_action(attacker_loc,defender_loc,attacker_weapon);
	}


	virtual move_result_ptr execute_move_action(const map_location& from, const map_location& to, bool remove_movement=true)
	{
		return target_->execute_move_action(from, to, remove_movement);
	}


	virtual recall_result_ptr execute_recall_action(const std::string& id, const map_location &where = map_location::null_location, const map_location &from = map_location::null_location)
	{
		return target_->execute_recall_action(id,where,from);
	}


	virtual recruit_result_ptr execute_recruit_action(const std::string& unit_name, const map_location &where = map_location::null_location, const map_location &from = map_location::null_location)
	{
		return target_->execute_recruit_action(unit_name,where,from);
	}


	virtual stopunit_result_ptr execute_stopunit_action(const map_location& unit_location, bool remove_movement = true, bool remove_attacks = false)
	{
		return target_->execute_stopunit_action(unit_location,remove_movement,remove_attacks);
	}


	virtual team& current_team_w()
	{
		return target_->current_team_w();
	}


	virtual void raise_gamestate_changed() const
	{
		target_->raise_gamestate_changed();
	}


	virtual game_info& get_info_w()
	{
		return target_->get_info_w();
	}


	virtual int get_recursion_count() const
	{
		return target_->get_recursion_count();
	}


	virtual config to_readwrite_context_config() const
	{
		return target_->to_readwrite_context_config();
	}

private:
	readwrite_context *target_;
};


//implementation
class side_context_impl : public side_context {
public:
	side_context_impl(side_number side, const config  &/*cfg*/)
		: side_(side), recursion_counter_(0)
	{
	}

	virtual ~side_context_impl(){}

	virtual side_number get_side() const
	{
		return side_;
	}

	virtual void set_side(side_number side)
	{
		side_ = side;
	}


	virtual side_context& get_side_context()
	{
		return *this;
	}


	virtual int get_recursion_count() const;


	virtual config to_side_context_config() const;

private:
	side_number side_;
	recursion_counter recursion_counter_;
};


class readonly_context_impl : public virtual side_context_proxy, public readonly_context, public events::observer {
public:

	/**
	 * Constructor
	 */
	readonly_context_impl(side_context &context, const config &cfg);


	/**
	 * Destructor
	 */
	virtual ~readonly_context_impl();


	/**
	 * Unwrap - this class is not a proxy, so return *this
:w
	 */
	virtual readonly_context& get_readonly_context()
	{
		return *this;
	}


	virtual void on_readonly_context_create();


	/** Handle generic event */
	virtual void handle_generic_event(const std::string& event_name);


	/** Return a reference to the 'team' object for the AI. */
	const team& current_team() const;


	/** Show a diagnostic message on the screen. */
	void diagnostic(const std::string& msg);


	/** Display a debug message as a chat message. */
	void log_message(const std::string& msg);


	/**
	 * Check if it is possible to attack enemy defender using our unit attacker from attackers current location,
	 * @param attacker_loc location of attacker
	 * @param defender_loc location of defender
	 * @param attacker_weapon weapon of attacker
	 * @retval possible result: ok
	 * @retval possible result: something wrong
	 * @retval possible result: attacker and/or defender are invalid
	 * @retval possible result: attacker and/or defender are invalid
	 * @retval possible result: attacker doesn't have the specified weapon
	 */
	attack_result_ptr check_attack_action(const map_location& attacker_loc, const map_location& defender_loc, int attacker_weapon);


	/**
	 * Check if it is possible to move our unit from location 'from' to location 'to'
	 * @param from location of our unit
	 * @param to where to move
	 * @param remove_movement set unit movement to 0 in case of successful move
	 * @retval possible result: ok
	 * @retval possible result: something wrong
	 * @retval possible result: move is interrupted
	 * @retval possible result: move is impossible
	 */
	move_result_ptr check_move_action(const map_location& from, const map_location& to, bool remove_movement=true);



	/**
	 * Check if it is possible to recall a unit for us on specified location
	 * @param id the id of the unit to be recruited.
	 * @param where location where the unit is to be recruited.
	 * @retval possible result: ok
	 * @retval possible_result: something wrong
	 * @retval possible_result: leader not on keep
	 * @retval possible_result: no free space on keep
	 * @retval possible_result: not enough gold
	 */
	recall_result_ptr check_recall_action(const std::string& id, const map_location &where = map_location::null_location, const map_location &from = map_location::null_location);


	/**
	 * Check if it is possible to recruit a unit for us on specified location
	 * @param unit_name the name of the unit to be recruited.
	 * @param where location where the unit is to be recruited.
	 * @retval possible result: ok
	 * @retval possible_result: something wrong
	 * @retval possible_result: leader not on keep
	 * @retval possible_result: no free space on keep
	 * @retval possible_result: not enough gold
	 */
	recruit_result_ptr check_recruit_action(const std::string& unit_name, const map_location &where = map_location::null_location, const map_location &from = map_location::null_location);


	/**
	 * Check if it is possible to remove unit movements and/or attack
	 * @param unit_location the location of our unit
	 * @param remove_movement set remaining movements to 0
	 * @param remove_attacks set remaining attacks to 0
	 * @retval possible result: ok
	 * @retval possible_result: something wrong
	 * @retval possible_result: nothing to do
	 */
	stopunit_result_ptr check_stopunit_action(const map_location& unit_location, bool remove_movement = true, bool remove_attacks = false);


	/**
	 * Calculate the moves units may possibly make.
	 *
	 * @param possible_moves      A map which will be filled with the paths
	 *                            each unit can take to get to every possible
	 *                            destination. You probably don't want to use
	 *                            this object at all, except to pass to
	 *                            'move_unit'.
	 * @param srcdst              A map of units to all their possible
	 *                            destinations.
	 * @param dstsrc              A map of destinations to all the units that
	 *                            can move to that destination.
	 * @param enemy               if true, a map of possible moves for enemies
	 *                            will be calculated. If false, a map of
	 *                            possible moves for units on the AI's side
	 *                            will be calculated.  The AI's own leader will
	 *                            not be included in this map.
	 * @param assume_full_movement
	 *                            If true, the function will operate on the
	 *                            assumption that all units can move their full
	 *                            movement allotment.
	 * @param remove_destinations a pointer to a terrain filter for possible destinations
	 *                            to omit.
	 */
	void calculate_possible_moves(std::map<map_location,pathfind::paths>& possible_moves,
		move_map& srcdst, move_map& dstsrc, bool enemy,
		bool assume_full_movement=false,
		const terrain_filter* remove_destinations=NULL) const;

 	/**
	 * A more fundamental version of calculate_possible_moves which allows the
	 * use of a speculative unit map.
	 */
	void calculate_moves(const unit_map& units,
		std::map<map_location,pathfind::paths>& possible_moves, move_map& srcdst,
		move_map& dstsrc, bool enemy, bool assume_full_movement=false,
		const terrain_filter* remove_destinations=NULL,
		bool see_all=false) const;


	virtual const game_info& get_info() const;

	/**
	 * Function which should be called frequently to allow the user to interact
	 * with the interface. This function will make sure that interaction
	 * doesn't occur too often, so there is no problem with calling it very
	 * regularly.
	 */
	void raise_user_interact() const;


	virtual int get_recursion_count() const;


	//@note: following functions are in alphabetic order
	defensive_position const& best_defensive_position(const map_location& unit,
			const move_map& dstsrc, const move_map& srcdst, const move_map& enemy_dstsrc) const;


	virtual std::map<map_location,defensive_position>& defensive_position_cache() const;


	virtual double get_aggression() const;


	virtual int get_attack_depth() const;


	virtual const aspect_map& get_aspects() const;


	virtual aspect_map& get_aspects();


	virtual const attacks_vector& get_attacks() const;


	virtual const variant& get_attacks_as_variant() const;


	virtual const terrain_filter& get_avoid() const;


	virtual double get_caution() const;


	virtual const move_map& get_dstsrc() const;


	virtual const move_map& get_enemy_dstsrc() const;


	virtual const moves_map& get_enemy_possible_moves() const;


	virtual const move_map& get_enemy_srcdst() const;


	virtual engine_ptr get_engine_by_cfg(const config& cfg);


	virtual const std::vector<engine_ptr>& get_engines() const;


	virtual std::vector<engine_ptr>& get_engines();


	virtual std::string get_grouping() const;


	virtual const std::vector<goal_ptr>& get_goals() const;


	virtual std::vector<goal_ptr>& get_goals();


	virtual double get_number_of_possible_recruits_to_force_recruit() const;


	virtual double get_leader_aggression() const;


	virtual config get_leader_goal() const;


	virtual double get_leader_value() const;


	virtual bool get_passive_leader() const;


	virtual bool get_passive_leader_shares_keep() const;


	virtual const moves_map& get_possible_moves() const;


	virtual const std::vector<unit>& get_recall_list() const;


	virtual stage_ptr get_recruitment(ai_context &context) const;


	virtual bool get_recruitment_ignore_bad_combat() const;


	virtual bool get_recruitment_ignore_bad_movement() const;


	virtual const std::vector<std::string> get_recruitment_pattern() const;


	virtual double get_scout_village_targeting() const;


	virtual bool get_simple_targeting() const;


	virtual const move_map& get_srcdst() const;


	virtual bool get_support_villages() const;


	virtual double get_village_value() const;


	virtual int get_villages_per_scout() const;


	virtual bool is_active(const std::string &time_of_day, const std::string &turns) const;


	virtual void invalidate_defensive_position_cache() const;


	virtual void invalidate_move_maps() const;


	virtual void invalidate_keeps_cache() const;


	virtual const std::set<map_location>& keeps() const;


	virtual bool leader_can_reach_keep() const;


	virtual const map_location& nearest_keep(const map_location& loc) const;


	virtual double power_projection(const map_location& loc, const move_map& dstsrc) const;


	virtual void recalculate_move_maps() const;


	virtual void recalculate_move_maps_enemy() const;


	virtual void add_aspects(std::vector< aspect_ptr > &aspects);


	virtual void add_facet(const std::string &id, const config &cfg) const;


	void on_create();


	virtual const map_location& suitable_keep( const map_location& leader_location, const pathfind::paths& leader_paths );


	virtual config to_readonly_context_config() const;


	virtual std::map<std::pair<map_location,const unit_type *>,
		std::pair<battle_context_unit_stats,battle_context_unit_stats> >& unit_stats_cache() const;

private:
	template<typename T>
	void add_known_aspect(const std::string &name, boost::shared_ptr< typesafe_aspect <T> >& where);

	const config cfg_;

	/**
	 * AI Support Engines
	 */
	std::vector< engine_ptr > engines_;

	known_aspect_map known_aspects_;

	aspect_type<double>::typesafe_ptr aggression_;
	aspect_type<int>::typesafe_ptr attack_depth_;
	aspect_map aspects_;
	aspect_type< attacks_vector >::typesafe_ptr attacks_;
	mutable aspect_type<terrain_filter>::typesafe_ptr avoid_;
	aspect_type<double>::typesafe_ptr caution_;
	mutable std::map<map_location,defensive_position> defensive_position_cache_;
	mutable move_map dstsrc_;
	mutable move_map enemy_dstsrc_;
	mutable moves_map enemy_possible_moves_;
	mutable move_map enemy_srcdst_;
	aspect_type< std::string >::typesafe_ptr grouping_;
	std::vector< goal_ptr > goals_;
	mutable keeps_cache keeps_;
	aspect_type<double>::typesafe_ptr leader_aggression_;
	aspect_type< config >::typesafe_ptr leader_goal_;
	aspect_type< double >::typesafe_ptr leader_value_;
	mutable bool move_maps_enemy_valid_;
	mutable bool move_maps_valid_;
	aspect_type<double>::typesafe_ptr number_of_possible_recruits_to_force_recruit_;
	aspect_type<bool>::typesafe_ptr passive_leader_;
	aspect_type<bool>::typesafe_ptr passive_leader_shares_keep_;
	mutable moves_map possible_moves_;
	aspect_type< ministage >::typesafe_ptr recruitment_;
	aspect_type< bool >::typesafe_ptr recruitment_ignore_bad_combat_;
	aspect_type< bool >::typesafe_ptr recruitment_ignore_bad_movement_;
	aspect_type< std::vector<std::string> >::typesafe_ptr recruitment_pattern_;
	recursion_counter recursion_counter_;
	aspect_type< double >::typesafe_ptr scout_village_targeting_;
	aspect_type< bool >::typesafe_ptr simple_targeting_;
	mutable move_map srcdst_;
	aspect_type< bool >::typesafe_ptr support_villages_;
	mutable std::map<std::pair<map_location,const unit_type *>,
			 std::pair<battle_context_unit_stats,battle_context_unit_stats> > unit_stats_cache_;
	aspect_type< double >::typesafe_ptr village_value_;
	aspect_type< int >::typesafe_ptr villages_per_scout_;


};

class readwrite_context_impl : public virtual readonly_context_proxy, public readwrite_context {
public:
	/**
	 * Unwrap - this class is not a proxy, so return *this
	 */
	virtual readwrite_context& get_readwrite_context()
	{
		return *this;
	}


	/**
	 * Ask the game to attack an enemy defender using our unit attacker from attackers current location,
	 * @param attacker_loc location of attacker
	 * @param defender_loc location of defender
	 * @param attacker_weapon weapon of attacker
	 * @retval possible result: ok
	 * @retval possible result: something wrong
	 * @retval possible result: attacker and/or defender are invalid
	 * @retval possible result: attacker and/or defender are invalid
	 * @retval possible result: attacker doesn't have the specified weapon
	 */
	virtual attack_result_ptr execute_attack_action(const map_location& attacker_loc, const map_location& defender_loc, int attacker_weapon);


	/**
	 * Ask the game to move our unit from location 'from' to location 'to', optionally - doing a partial move
	 * @param from location of our unit
	 * @param to where to move
	 * @param remove_movement set unit movement to 0 in case of successful move
	 * @retval possible result: ok
	 * @retval possible result: something wrong
	 * @retval possible result: move is interrupted
	 * @retval possible result: move is impossible
	 */
	virtual move_result_ptr execute_move_action(const map_location& from, const map_location& to, bool remove_movement=true);


	/**
	 * Ask the game to recall a unit for us on specified location
	 * @param id the id of the unit to be recalled.
	 * @param where location where the unit is to be recalled.
	 * @retval possible result: ok
	 * @retval possible_result: something wrong
	 * @retval possible_result: leader not on keep
	 * @retval possible_result: no free space on keep
	 * @retval possible_result: not enough gold
	 */
	virtual recall_result_ptr execute_recall_action(const std::string& id, const map_location &where = map_location::null_location, const map_location &from = map_location::null_location);


	/**
	 * Ask the game to recruit a unit for us on specified location
	 * @param unit_name the name of the unit to be recruited.
	 * @param where location where the unit is to be recruited.
	 * @retval possible result: ok
	 * @retval possible_result: something wrong
	 * @retval possible_result: leader not on keep
	 * @retval possible_result: no free space on keep
	 * @retval possible_result: not enough gold
	 */
	virtual recruit_result_ptr execute_recruit_action(const std::string& unit_name, const map_location &where = map_location::null_location, const map_location &from = map_location::null_location);


	/**
	 * Ask the game to remove unit movements and/or attack
	 * @param unit_location the location of our unit
	 * @param remove_movement set remaining movements to 0
	 * @param remove_attacks set remaining attacks to 0
	 * @retval possible result: ok
	 * @retval possible_result: something wrong
	 * @retval possible_result: nothing to do
	 */
	virtual stopunit_result_ptr execute_stopunit_action(const map_location& unit_location, bool remove_movement = true, bool remove_attacks = false);


	/** Return a reference to the 'team' object for the AI. */
	virtual team& current_team_w();


	/** Notifies all interested observers of the event respectively. */
	void raise_gamestate_changed() const;


	/**
	 * Constructor.
	 */
	readwrite_context_impl(readonly_context &context, const config &/*cfg*/)
		: recursion_counter_(context.get_recursion_count())
	{
		init_readonly_context_proxy(context);
	}


	virtual ~readwrite_context_impl()
	{
	}

	/**
	 * Functions to retrieve the 'info' object.
	 * Used by derived classes to discover all necessary game information.
	 */
	virtual game_info& get_info_w();


	virtual int get_recursion_count() const;


	virtual config to_readwrite_context_config() const;

private:
	recursion_counter recursion_counter_;
};


} //end of namespace ai

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
