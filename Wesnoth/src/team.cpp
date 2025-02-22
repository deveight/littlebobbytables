/* $Id: team.cpp 55477 2012-10-03 17:03:01Z lipk $ */
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

/**
 *  @file
 *  Team-management, allies, setup at start of scenario.
 */

#include "team.hpp"

#include "ai/manager.hpp"
#include "game_events.hpp"
#include "gamestatus.hpp"
#include "resources.hpp"
#include "game_preferences.hpp"
#include "whiteboard/side_actions.hpp"

#include <boost/foreach.hpp>

static lg::log_domain log_engine("engine");
#define DBG_NG LOG_STREAM(debug, log_engine)
#define LOG_NG LOG_STREAM(info, log_engine)
#define WRN_NG LOG_STREAM(warn, log_engine)

static lg::log_domain log_engine_enemies("engine/enemies");
#define DBG_NGE LOG_STREAM(debug, log_engine_enemies)
#define LOG_NGE LOG_STREAM(info, log_engine_enemies)
#define WRN_NGE LOG_STREAM(warn, log_engine_enemies)


static std::vector<team> *&teams = resources::teams;

//static member initialization
const int team::default_team_gold = 100;

const std::vector<team>& teams_manager::get_teams()
{
	assert(teams);
	return *teams;
}

team::team_info::team_info() :
	name(),
	gold(0),
	start_gold(0),
	gold_add(false),
	income(0),
	income_per_village(0),
	minimum_recruit_price(0),
	recall_cost(0),
	can_recruit(),
	team_name(),
	user_team_name(),
	save_id(),
	current_player(),
	countdown_time(),
	action_bonus_count(0),
	flag(),
	flag_icon(),
	description(),
	scroll_to_leader(true),
	objectives(),
	objectives_changed(false),
	controller(),
	share_maps(false),
	share_view(false),
	disallow_observers(false),
	allow_player(false),
	no_leader(true),
	hidden(true),
	music(),
	color(),
	side(0),
	persistent(false)
{
}

void team::team_info::read(const config &cfg)
{
	name = cfg["name"].str();
	gold = cfg["gold"];
	income = cfg["income"];
	team_name = cfg["team_name"].str();
	user_team_name = cfg["user_team_name"];
	save_id = cfg["save_id"].str();
	current_player = cfg["current_player"].str();
	countdown_time = cfg["countdown_time"].str();
	action_bonus_count = cfg["action_bonus_count"];
	flag = cfg["flag"].str();
	flag_icon = cfg["flag_icon"].str();
	description = cfg["id"].str();
	scroll_to_leader = cfg["scroll_to_leader"].to_bool(true);
	objectives = cfg["objectives"];
	objectives_changed = cfg["objectives_changed"].to_bool();
	disallow_observers = cfg["disallow_observers"].to_bool();
	allow_player = cfg["allow_player"].to_bool(true);
	no_leader = cfg["no_leader"].to_bool();
	hidden = cfg["hidden"].to_bool();
	music = cfg["music"].str();
	side = cfg["side"].to_int(1);

	if(cfg.has_attribute("color")) {
		color = cfg["color"].str();
	} else {
		color = cfg["side"].str();
	}

	// If arel starting new scenario overide settings from [ai] tags
	if (!user_team_name.translatable())
		user_team_name = t_string::from_serialized(user_team_name);

	if(cfg.has_attribute("ai_config")) {
		ai::manager::add_ai_for_side_from_file(side, cfg["ai_config"], true);
	} else {
		ai::manager::add_ai_for_side_from_config(side, cfg, true);
	}

	std::vector<std::string> recruits = utils::split(cfg["recruit"]);
	for(std::vector<std::string>::const_iterator i = recruits.begin(); i != recruits.end(); ++i) {
		can_recruit.insert(*i);
	}

	// at the start of a scenario "start_gold" is not set, we need to take the
	// value from the gold setting (or fall back to the gold default)
	if (!cfg["start_gold"].empty())
		start_gold = cfg["start_gold"];
	else if (!cfg["gold"].empty())
		start_gold = gold;
	else
		start_gold = default_team_gold;

	if(team_name.empty()) {
		team_name = cfg["side"].str();
	}

	if(save_id.empty()) {
		save_id = description;
	}
	if (current_player.empty()) {
		current_player = save_id;
	}

	const std::string temp_rgb_str = cfg["team_rgb"];
	std::map<std::string, color_range>::iterator global_rgb = game_config::team_rgb_range.find(cfg["side"]);

	if(!temp_rgb_str.empty()){
		std::vector<Uint32> temp_rgb;
		if(string2rgb(temp_rgb_str, temp_rgb))
			team_color_range_[side] = color_range(temp_rgb);
		else
			WRN_NG << "can't parse color string, ignoring: " << temp_rgb_str << "\n";
	}else if(global_rgb != game_config::team_rgb_range.end()){
		team_color_range_[side] = global_rgb->second;
	}

	income_per_village = cfg["village_gold"].to_int(game_config::village_income);
	recall_cost = cfg["recall_cost"].to_int(game_config::recall_cost);

	std::string control = cfg["controller"];
	//by default, persistence of a team is set depending on the controller
	persistent = true;
	if (control == "human")
		controller = HUMAN;
	else if (control == "human_ai")
		controller = HUMAN_AI;
	else if (control == "network")
		controller = NETWORK;
	else if (control == "network_ai")
		controller = NETWORK_AI;
	else if (control == "null")
	{
		disallow_observers = cfg["disallow_observers"].to_bool(true);
		controller = EMPTY;
		persistent = false;
	}
	else
	{
		controller = AI;
		persistent = false;
	}

	//override persistence flag if it is explicitly defined in the config
	persistent = cfg["persistent"].to_bool(persistent);

	//========================================================
	//END OF MESSY CODE

	// Share_view and share_maps can't both be enabled,
	// so share_view overrides share_maps.
	share_view = cfg["share_view"].to_bool();
	share_maps = !share_view && cfg["share_maps"].to_bool(true);

	LOG_NG << "team_info::team_info(...): team_name: " << team_name
	       << ", share_maps: " << share_maps << ", share_view: " << share_view << ".\n";
}

char const *team::team_info::controller_string() const
{
	switch(controller) {
	case AI: return "ai";
	case HUMAN: return "human";
	case HUMAN_AI: return "human_ai";
	case NETWORK: return "network";
	case NETWORK_AI: return "network_ai";
	case EMPTY: return "null";
	default: assert(false); return NULL;
	}
}

void team::team_info::write(config& cfg) const
{
	cfg["gold"] = gold;
	cfg["start_gold"] = start_gold;
	cfg["gold_add"] = gold_add;
	cfg["income"] = income;
	cfg["name"] = name;
	cfg["team_name"] = team_name;
	cfg["user_team_name"] = user_team_name;
	cfg["save_id"] = save_id;
	cfg["current_player"] = current_player;
	cfg["flag"] = flag;
	cfg["flag_icon"] = flag_icon;
	cfg["id"] = description;
	cfg["objectives"] = objectives;
	cfg["objectives_changed"] = objectives_changed;
	cfg["countdown_time"]= countdown_time;
	cfg["action_bonus_count"]= action_bonus_count;
	cfg["village_gold"] = income_per_village;
	cfg["recall_cost"] = recall_cost;
	cfg["disallow_observers"] = disallow_observers;
	cfg["allow_player"] = allow_player;
	cfg["no_leader"] = no_leader;
	cfg["hidden"] = hidden;
	cfg["scroll_to_leader"] = scroll_to_leader;
	cfg["controller"] = controller_string();

	std::stringstream can_recruit_str;
	for(std::set<std::string>::const_iterator cr = can_recruit.begin(); cr != can_recruit.end(); ++cr) {
		if(cr != can_recruit.begin())
			can_recruit_str << ",";

		can_recruit_str << *cr;
	}

	cfg["recruit"] = can_recruit_str.str();

	cfg["share_maps"] = share_maps;
	cfg["share_view"] = share_view;

	if(music.empty() == false)
		cfg["music"] = music;

	cfg["color"] = color;

	cfg["persistent"] = persistent;

	cfg.add_child("ai",ai::manager::to_config(side));
}

void team::merge_shroud_map_data(const std::string& shroud_data)
{
	shroud_.merge(shroud_data);
}

team::team() :
	savegame_config(),
	gold_(0),
	villages_(),
	shroud_(),
	fog_(),
	auto_shroud_updates_(true),
	info_(),
	countdown_time_(0),
	action_bonus_count_(0),
	recall_list_(),
	enemies_(),
	seen_(),
	ally_shroud_(),
	ally_fog_(),
	planned_actions_()
{
}

team::~team()
{
}

void team::build(const config &cfg, const gamemap& map, int gold)
{
	gold_ = gold;
	info_.read(cfg);

	fog_.set_enabled(cfg["fog"].to_bool());
	shroud_.set_enabled(cfg["shroud"].to_bool());
	shroud_.read(cfg["shroud_data"]);

	LOG_NG << "team::team(...): team_name: " << info_.team_name
	       << ", shroud: " << uses_shroud() << ", fog: " << uses_fog() << ".\n";

	// To ensure some minimum starting gold,
	// gold is the maximum of 'gold' and what is given in the config file
	gold_ = std::max(gold, info_.gold);
	if (gold_ != info_.gold)
		info_.start_gold = gold;
	// Old code was doing:
	// info_.start_gold = str_cast(gold) + " (" + info_.start_gold + ")";
	// Was it correct?

	// Load in the villages the side controls at the start
	BOOST_FOREACH(const config &v, cfg.child_range("village"))
	{
		map_location loc(v, resources::state_of_game);
		if (map.is_village(loc)) {
			villages_.insert(loc);
		} else {
			WRN_NG << "[side] " << name() << " [village] points to a non-village location " << loc << "\n";
		}
	}

	countdown_time_ = cfg["countdown_time"];
	action_bonus_count_ = cfg["action_bonus_count"];

	planned_actions_.reset(new wb::side_actions());
	planned_actions_->set_team_index(info_.side - 1);
}

void team::write(config& cfg) const
{
	info_.write(cfg);
	cfg["shroud"] = uses_shroud();
	cfg["fog"] = uses_fog();
	cfg["gold"] = gold_;

	// Write village locations
	for(std::set<map_location>::const_iterator t = villages_.begin(); t != villages_.end(); ++t) {
		t->write(cfg.add_child("village"));
	}

	cfg["shroud_data"] = shroud_.write();

	cfg["countdown_time"] = countdown_time_;
	cfg["action_bonus_count"] = action_bonus_count_;
}

bool team::get_village(const map_location& loc, const int owner_side, const bool fire_event)
{
	villages_.insert(loc);
	bool gamestate_changed = false;
	if(fire_event) {
		config::attribute_value& var = resources::state_of_game->get_variable("owner_side");
		const config::attribute_value old_value = var;
		var = owner_side;
		gamestate_changed = game_events::fire("capture",loc);
		if(old_value.blank())
			resources::state_of_game->clear_variable("owner_side");
		else
			var = old_value;
	}
	return gamestate_changed;
}

void team::lose_village(const map_location& loc)
{
	const std::set<map_location>::const_iterator vil = villages_.find(loc);
	assert(vil != villages_.end());
	villages_.erase(vil);
}

void team::set_recruits(const std::set<std::string>& recruits)
{
	info_.can_recruit = recruits;
	info_.minimum_recruit_price = 0;
	ai::manager::raise_recruit_list_changed();
}

void team::add_recruit(const std::string &recruit)
{
	info_.can_recruit.insert(recruit);
	info_.minimum_recruit_price = 0;
	ai::manager::raise_recruit_list_changed();
}

int team::minimum_recruit_price() const
{
	if(info_.minimum_recruit_price){
		return info_.minimum_recruit_price;
	}else{
		int min = 20;
		BOOST_FOREACH(std::string recruit, info_.can_recruit){
			const unit_type *ut = unit_types.find(recruit);
			if(!ut)
				continue;
			else{
				if(ut->cost() < min)
					min = ut->cost();
			}

		}
		info_.minimum_recruit_price = min;
	}
	return info_.minimum_recruit_price;
}

bool team::calculate_enemies(size_t index) const
{
	if(teams == NULL || index >= teams->size()) {
		return false;
	}

	while(enemies_.size() <= index) {
		enemies_.push_back(calculate_is_enemy(enemies_.size()));
	}

	return enemies_.back();
}

bool team::calculate_is_enemy(size_t index) const
{
	// We're not enemies of ourselves
	if(&(*teams)[index] == this) {
		return false;
	}

	// We are friends with anyone who we share a teamname with
	std::vector<std::string> our_teams = utils::split(info_.team_name),
						   their_teams = utils::split((*teams)[index].info_.team_name);

	LOG_NGE << "team " << info_.side << " calculates if it has enemy in team "<<index+1 << "; our team_name ["<<info_.team_name<<"], their team_name is ["<<(*teams)[index].info_.team_name<<"]"<< std::endl;
	for(std::vector<std::string>::const_iterator t = our_teams.begin(); t != our_teams.end(); ++t) {
		if(std::find(their_teams.begin(), their_teams.end(), *t) != their_teams.end())
		{
			LOG_NGE << "team " << info_.side << " found same team name [" << *t << "] in team "<< index+1 << std::endl;
			return false;
		} else {
			LOG_NGE << "team " << info_.side << " not found same team name [" << *t << "] in team "<< index+1 << std::endl;
		}
	}
	LOG_NGE << "team " << info_.side << " has enemy in team " << index+1 << std::endl;
	return true;
}

void team::set_share_maps( bool share_maps ){
	// Share_view and share_maps can't both be enabled,
	// so share_view overrides share_maps.
	// If you want to change them, be sure to change share_view FIRST
	info_.share_maps = !info_.share_view && share_maps;
}

void team::set_share_view( bool share_view ){
	info_.share_view = share_view;
}

void team::change_controller(const std::string& controller)
{
	team::team_info::CONTROLLER cid;
	if (controller == "human")
		cid = team::team_info::HUMAN;
	else if (controller == "human_ai")
		cid = team::team_info::HUMAN_AI;
	else if (controller == "network")
		cid = team::team_info::NETWORK;
	else if (controller == "network_ai")
		cid = team::team_info::NETWORK_AI;
	else if (controller == "null")
		cid = team::team_info::EMPTY;
	else
		cid = team::team_info::AI;

	info_.controller = cid;
}

void team::change_team(const std::string &name, const t_string &user_name)
{
	info_.team_name = name;
	if (!user_name.empty())
	{
		info_.user_team_name = user_name;
	}
	else
	{
		info_.user_team_name = name;
	}

	clear_caches();
}

void team::clear_caches(){
	// Reset the cache of allies for all teams
	if(teams != NULL) {
		for(std::vector<team>::const_iterator i = teams->begin(); i != teams->end(); ++i) {
			i->enemies_.clear();
			i->ally_shroud_.clear();
			i->ally_fog_.clear();
		}
	}
}

void team::set_objectives(const t_string& new_objectives, bool silently)
{
	info_.objectives = new_objectives;
	if(!silently)
		info_.objectives_changed = true;
}

bool team::shrouded(const map_location& loc) const
{
	if(!teams)
		return shroud_.value(loc.x+1,loc.y+1);

	return shroud_.shared_value(ally_shroud(*teams),loc.x+1,loc.y+1);
}

bool team::fogged(const map_location& loc) const
{
	if(shrouded(loc)) return true;

	if(!teams)
		return fog_.value(loc.x+1,loc.y+1);

	return fog_.shared_value(ally_fog(*teams),loc.x+1,loc.y+1);
}

const std::vector<const team::shroud_map*>& team::ally_shroud(const std::vector<team>& teams) const
{
	if(ally_shroud_.empty()) {
		for(size_t i = 0; i < teams.size(); ++i) {
			if(!is_enemy(i + 1) && (&(teams[i]) == this || teams[i].share_view() || teams[i].share_maps())) {
				ally_shroud_.push_back(&(teams[i].shroud_));
			}
		}
	}

	return ally_shroud_;
}

const std::vector<const team::shroud_map*>& team::ally_fog(const std::vector<team>& teams) const
{
	if(ally_fog_.empty()) {
		for(size_t i = 0; i < teams.size(); ++i) {
			if(!is_enemy(i + 1) && (&(teams[i]) == this || teams[i].share_view())) {
				ally_fog_.push_back(&(teams[i].fog_));
			}
		}
	}

	return ally_fog_;
}

bool team::knows_about_team(size_t index, bool is_multiplayer) const
{
	const team& t = (*teams)[index];

	// We know about our own team
	if(this == &t) return true;

	// If we aren't using shroud or fog, then we know about everyone
	if(!uses_shroud() && !uses_fog()) return true;

	// We don't know about enemies
	if(is_enemy(index+1)) return false;

	// We know our allies in multiplayer
	if (is_multiplayer) return true;

	// We know about allies we're sharing maps with
	if(share_maps() && t.uses_shroud()) return true;

	// We know about allies we're sharing view with
	if(share_view() && (t.uses_fog() || t.uses_shroud())) return true;

	return false;
}

bool team::copy_ally_shroud()
{
	if(!teams || !share_maps())
		return false;

	return shroud_.copy_from(ally_shroud(*teams));
}

int team::nteams()
{
	if(teams == NULL) {
		return 0;
	} else {
		return teams->size();
	}
}

bool is_observer()
{
	if(teams == NULL) {
		return true;
	}

	BOOST_FOREACH(const team &t, *teams) {
		if (t.is_human() || t.is_human_ai())
			return false;
	}

	return true;
}

void validate_side(int side)
{
	if(teams == NULL) {
		return;
	}

	if(side < 1 || side > int(teams->size())) {
		throw game::game_error("invalid side(" + str_cast(side) + ") found in unit definition");
	}
}

bool team::shroud_map::clear(int x, int y)
{
	if(enabled_ == false || x < 0 || y < 0)
		return false;

	if(x >= static_cast<int>(data_.size()))
		data_.resize(x+1);

	if(y >= static_cast<int>(data_[x].size()))
		data_[x].resize(y+1);

	if(data_[x][y] == false) {
		data_[x][y] = true;
		return true;
	} else {
		return false;
	}
}

void team::shroud_map::place(int x, int y)
{
	if(enabled_ == false || x < 0 || y < 0)
		return;

	if (x >= static_cast<int>(data_.size())) {
		DBG_NG << "Couldn't place shroud on invalid x coordinate: ("
			<< x << ", " << y << ") - max x: " << data_.size() - 1 << "\n";
	} else if (y >= static_cast<int>(data_[x].size())) {
		DBG_NG << "Couldn't place shroud on invalid y coordinate: ("
			<< x << ", " << y << ") - max y: " << data_[x].size() - 1 << "\n";
	} else {
		data_[x][y] = false;
	}
}

void team::shroud_map::reset()
{
	if(enabled_ == false)
		return;

	for(std::vector<std::vector<bool> >::iterator i = data_.begin(); i != data_.end(); ++i) {
		std::fill(i->begin(),i->end(),false);
	}
}

bool team::shroud_map::value(int x, int y) const
{
	if(enabled_ == false || x < 0 || y < 0)
		return false;

	if(x >= static_cast<int>(data_.size()))
		return true;

	if(y >= static_cast<int>(data_[x].size()))
		return true;

	if(data_[x][y])
		return false;
	else
		return true;
}

bool team::shroud_map::shared_value(const std::vector<const shroud_map*>& maps, int x, int y) const
{
	if(enabled_ == false || x < 0 || y < 0)
		return false;

	for(std::vector<const shroud_map*>::const_iterator i = maps.begin(); i != maps.end(); ++i) {
		if((*i)->enabled_ == true && (*i)->value(x,y) == false)
			return false;
	}
	return true;
}

std::string team::shroud_map::write() const
{
	std::stringstream shroud_str;
	for(std::vector<std::vector<bool> >::const_iterator sh = data_.begin(); sh != data_.end(); ++sh) {
		shroud_str << '|';

		for(std::vector<bool>::const_iterator i = sh->begin(); i != sh->end(); ++i) {
			shroud_str << (*i ? '1' : '0');
		}

		shroud_str << '\n';
	}

	return shroud_str.str();
}

void team::shroud_map::read(const std::string& str)
{
	data_.clear();
	for(std::string::const_iterator sh = str.begin(); sh != str.end(); ++sh) {
		if(*sh == '|')
			data_.resize(data_.size()+1);

		if(data_.empty() == false) {
			if(*sh == '1')
				data_.back().push_back(true);
			else if(*sh == '0')
				data_.back().push_back(false);
		}
	}
}

void team::shroud_map::merge(const std::string& str)
{
	int x=0, y=0;
	for(std::string::const_iterator sh = str.begin(); sh != str.end(); ++sh) {
		if(*sh == '|' && sh != str.begin()) {
			y=0;
			x++;
		} else if(*sh == '1') {
			clear(x,y);
			y++;
		} else if(*sh == '0') {
			y++;
		}
	}
}

bool team::shroud_map::copy_from(const std::vector<const shroud_map*>& maps)
{
	if(enabled_ == false)
		return false;

	bool cleared = false;
	for(std::vector<const shroud_map*>::const_iterator i = maps.begin(); i != maps.end(); ++i) {
		if((*i)->enabled_ == false)
			continue;

		const std::vector<std::vector<bool> >& v = (*i)->data_;
		for(size_t x = 0; x != v.size(); ++x) {
			for(size_t y = 0; y != v[x].size(); ++y) {
				if(v[x][y]) {
					cleared |= clear(x,y);
				}
			}
		}
	}
	return cleared;
}

std::map<int, color_range> team::team_color_range_;

const color_range team::get_side_color_range(int side){
  std::string index = get_side_color_index(side);
  std::map<std::string, color_range>::iterator gp=game_config::team_rgb_range.find(index);

  if(gp != game_config::team_rgb_range.end()){
    return(gp->second);
  }

  return(color_range(0x00FF0000,0x00FFFFFF,0x00000000,0x00FF0000));
}

SDL_Color team::get_side_color(int side)
{
	return int_to_color(get_side_color_range(side).mid());
}

SDL_Color team::get_minimap_color(int side)
{
	// Note: use mid() instead of rep() unless
	// high contrast is needed over a map or minimap!
	return int_to_color(get_side_color_range(side).rep());
}

std::string team::get_side_color_index(int side)
{
	size_t index = size_t(side-1);

	if(teams != NULL && index < teams->size()) {
		const std::string side_map = (*teams)[index].map_color_to();
		if(!side_map.empty()) {
			return side_map;
		}
	}
	return str_cast(side);
}

std::string team::get_side_highlight(int side)
{
	return rgb2highlight(get_side_color_range(side+1).mid());
}

std::string team::get_side_highlight_pango(int side)
{
	return rgb2highlight_pango(get_side_color_range(side+1).mid());
}

void team::log_recruitable(){
	LOG_NG << "Adding recruitable units: \n";
	for (std::set<std::string>::const_iterator it = info_.can_recruit.begin();
		 it != info_.can_recruit.end(); ++it) {
		LOG_NG << *it << std::endl;
	}
	LOG_NG << "Added all recruitable units\n";
}

config team::to_config() const
{
	config cfg;
	config& result = cfg.add_child("side");
	write(result);
	return result;
}

namespace player_teams {
int village_owner(const map_location& loc)
{
	if(! teams) {
		return -1;
	}
	for(size_t i = 0; i != teams->size(); ++i) {
		if((*teams)[i].owns_village(loc))
			return i;
	}
	return -1;
}
}
