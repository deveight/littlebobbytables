/* $Id: game_preferences.cpp 54625 2012-07-08 14:26:21Z loonycyborg $ */
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

#include "global.hpp"

#define GETTEXT_DOMAIN "wesnoth-lib"

#include "game_display.hpp"
#include "game_preferences.hpp"
#include "gamestatus.hpp"
#include "gettext.hpp"
#include "log.hpp"
#include "map.hpp"
#include "network.hpp" // ping_timeout
#include "serialization/string_utils.hpp"
#include "settings.hpp"
#include "unit.hpp"
#include "unit_map.hpp"
#include "wml_exception.hpp"

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

static lg::log_domain log_config("config");
#define ERR_CFG LOG_STREAM(err , log_config)

namespace {

bool message_private_on = false;

bool haloes = true;

std::set<std::string> encountered_units_set;
std::set<t_translation::t_terrain> encountered_terrains_set;

std::map<std::string, std::vector<std::string> > history_map;
const unsigned max_history_saved = 50;

std::set<std::string> friends;
std::set<std::string> ignores;

bool friends_initialized = false;
bool ignores_initialized = false;

bool authenticated = false;

const char WRAP_CHAR = '@';
const std::string EMPTY_WRAPPED_STRING = "@@";

std::string wrap_credentials_field_value(const std::string& value)
{
	return WRAP_CHAR + value + WRAP_CHAR;
}

std::string parse_wrapped_credentials_field(const std::string& raw)
{
	if(raw.empty() || raw == EMPTY_WRAPPED_STRING) {
		// empty (wrapped or not)
		return raw;
	} else if(raw.length() < 2 || raw[0] != WRAP_CHAR || raw[raw.length() - 1] != WRAP_CHAR ) {
		// malformed/not wrapped (shouldn't happen)
		ERR_CFG << "malformed user credentials (did you manually edit the preferences file?)\n";
		return raw;
	}

	return raw.substr(1, raw.length() - 2);
}

} // anon namespace

namespace preferences {

manager::manager() :
	base()
{
	set_music_volume(music_volume());
	set_sound_volume(sound_volume());

	set_show_haloes(preferences::get("show_haloes", true));
	if (!preferences::get("remember_timer_settings", false)) {
		preferences::erase("mp_countdown_init_time");
		preferences::erase("mp_countdown_reservoir_time");
		preferences::erase("mp_countdown_turn_bonus");
		preferences::erase("mp_countdown_action_bonus");
	}

	const std::vector<std::string> v = utils::split(preferences::get("encountered_units"));
	std::copy(v.begin(), v.end(),
			std::inserter(encountered_units_set, encountered_units_set.begin()));

	const t_translation::t_list terrain =
			t_translation::read_list(preferences::get("encountered_terrain_list"));
	std::copy(terrain.begin(), terrain.end(),
			std::inserter(encountered_terrains_set, encountered_terrains_set.begin()));

	if (const config &history = preferences::get_child("history"))
	{
/* Structure of the history
	[history]
		[history_id]
			[line]
				message = foobar
			[/line]
*/
		BOOST_FOREACH(const config::any_child &h, history.all_children_range())
		{
			BOOST_FOREACH(const config &l, h.cfg.child_range("line")) {
				history_map[h.key].push_back(l["message"]);
			}
		}
	}

	network::ping_timeout = get_ping_timeout();
}

manager::~manager()
{
	std::vector<std::string> v;
	std::copy(encountered_units_set.begin(), encountered_units_set.end(), std::back_inserter(v));
	preferences::set("encountered_units", utils::join(v));
	t_translation::t_list terrain;
	std::copy(encountered_terrains_set.begin(), encountered_terrains_set.end(),
			  std::back_inserter(terrain));
	preferences::set("encountered_terrain_list", t_translation::write_list(terrain));

/* Structure of the history
	[history]
		[history_id]
			[line]
				message = foobar
			[/line]
*/
	config history;
	typedef std::pair<std::string, std::vector<std::string> > hack;
	BOOST_FOREACH(const hack& history_id, history_map) {

		config history_id_cfg; // [history_id]
		BOOST_FOREACH(const std::string& line, history_id.second) {
			config cfg; // [line]

			cfg["message"] = line;
			history_id_cfg.add_child("line", cfg);
		}

		history.add_child(history_id.first, history_id_cfg);
	}
	preferences::set_child("history", history);

	history_map.clear();
	encountered_units_set.clear();
	encountered_terrains_set.clear();
}

bool is_authenticated() {
	return authenticated;
}

void parse_admin_authentication(const std::string& sender, const std::string& message) {
	if(sender != "server") return;
	if(message.find("You are now recognized as an administrator.") == 0) {
		authenticated = true;
	} else if(message.find("You are no longer recognized as an administrator.") == 0) {
		authenticated = false;
	}
}

static void initialize_friends() {
	if(!friends_initialized) {
		std::vector<std::string> names = utils::split(preferences::get("friends"));
		std::set<std::string> tmp(names.begin(), names.end());
		friends.swap(tmp);

		friends_initialized = true;
	}
}

const std::set<std::string> & get_friends() {
	initialize_friends();
	return friends;
}

static void initialize_ignores() {
	if(!ignores_initialized) {
		std::vector<std::string> names = utils::split(preferences::get("ignores"));
		std::set<std::string> tmp(names.begin(), names.end());
		ignores.swap(tmp);

		ignores_initialized = true;
	}
}

const std::set<std::string> & get_ignores() {
	return ignores;
}

bool add_friend(const std::string& nick) {
	if (!utils::isvalid_wildcard(nick)) return false;
	friends.insert(nick);
	preferences::set("friends", utils::join(friends));
	return true;
}

bool add_ignore(const std::string& nick) {
	if (!utils::isvalid_wildcard(nick)) return false;
	ignores.insert(nick);
	preferences::set("ignores", utils::join(ignores));
	return true;
}

void remove_friend(const std::string& nick) {
	std::set<std::string>::iterator i = friends.find(nick);
	if(i != friends.end()) {
		friends.erase(i);
		preferences::set("friends", utils::join(friends));
	}
}

void remove_ignore(const std::string& nick) {
	std::set<std::string>::iterator i = ignores.find(nick);
	if(i != ignores.end()) {
		ignores.erase(i);
		preferences::set("ignores", utils::join(ignores));
	}
}

bool is_friend(const std::string& nick) {
	initialize_friends();
	return friends.find(nick) != friends.end();
}

bool is_ignored(const std::string& nick) {
	initialize_ignores();
	return ignores.find(nick) != ignores.end();
}

void add_completed_campaign(const std::string& campaign_id) {
	std::vector<std::string> completed = utils::split(preferences::get("completed_campaigns"));

	if(std::find(completed.begin(), completed.end(), campaign_id) != completed.end())
		return;

	completed.push_back(campaign_id);
	preferences::set("completed_campaigns", utils::join(completed));
}

bool is_campaign_completed(const std::string& campaign_id) {
	std::vector<std::string> completed = utils::split(preferences::get("completed_campaigns"));

	return std::find(completed.begin(), completed.end(), campaign_id) != completed.end();
}

bool parse_should_show_lobby_join(const std::string &sender, const std::string &message)
{
	// If it's actually not a lobby join message return true (show it).
	if (sender != "server") return true;
	std::string::size_type pos = message.find(" has logged into the lobby");
	if (pos == std::string::npos) return true;
	int lj = lobby_joins();
	if (lj == SHOW_NONE) return false;
	if (lj == SHOW_ALL) return true;
	return is_friend(message.substr(0, pos));
}

int lobby_joins()
{
	std::string pref = preferences::get("lobby_joins");
	if (pref == "friends") {
		return SHOW_FRIENDS;
	} else if (pref == "all") {
		return SHOW_ALL;
	} else if (pref == "none") {
		return SHOW_NONE;
	} else {
		return SHOW_FRIENDS;
	}
}


void _set_lobby_joins(int show)
{
	if (show == SHOW_FRIENDS) {
		preferences::set("lobby_joins", "friends");
	} else if (show == SHOW_ALL) {
		preferences::set("lobby_joins", "all");
	} else if (show == SHOW_NONE) {
		preferences::set("lobby_joins", "none");
	}
}

bool new_lobby()
{
	return get("new_lobby", false);
}

void set_new_lobby(bool value)
{
	preferences::set("new_lobby", value);
}

const std::vector<game_config::server_info>& server_list()
{
	static std::vector<game_config::server_info> pref_servers;
	if(pref_servers.empty()) {
		std::vector<game_config::server_info> &game_servers = game_config::server_list;
		VALIDATE(!game_servers.empty(), _("No server has been defined."));
		pref_servers.insert(pref_servers.begin(), game_servers.begin(), game_servers.end());
		BOOST_FOREACH(const config &server, get_prefs()->child_range("server")) {
			game_config::server_info sinf;
			sinf.name = server["name"].str();
			sinf.address = server["address"].str();
			pref_servers.push_back(sinf);
		}
	}
	return pref_servers;
}

std::string network_host()
{
	const std::string res = preferences::get("host");
	if(res.empty()) {
		return server_list().front().address;
	} else {
		return res;
	}
}

void set_network_host(const std::string& host)
{
	preferences::set("host", host);
}

unsigned int get_ping_timeout()
{
	return lexical_cast_default<unsigned>(preferences::get("ping_timeout"), 0);
}

std::string campaign_server()
{
	if(!preferences::get("campaign_server").empty()) {
		return preferences::get("campaign_server");
	} else {
		return "add-ons.wesnoth.org";
	}
}

void set_campaign_server(const std::string& host)
{
	preferences::set("campaign_server", host);
}

bool wrap_password()
{
	const bool have_old_password_format =
		(!preferences::have_setting("password_is_wrapped")) && preferences::have_setting("password");
	return have_old_password_format ? false : preferences::get("password_is_wrapped", true);
}

void set_wrap_password(bool wrap)
{
	preferences::set("password_is_wrapped", wrap);
}

bool wrap_login()
{
	const bool have_old_login_format =
		(!preferences::have_setting("login_is_wrapped")) && preferences::have_setting("login");
	return have_old_login_format ? false : preferences::get("login_is_wrapped", true);
}

void set_wrap_login(bool wrap)
{
	preferences::set("login_is_wrapped", wrap);
}

std::string login()
{
	const std::string res = preferences::get("login");
	if(res.empty()) {
		char* const login = getenv("USER");
		if(login != NULL) {
			return login;
		}

		if(res.empty()) {
			return "eyetracker" + boost::lexical_cast<std::string>(rand());
		}
	}

	if(!wrap_login()) {
		return res;
	} else {
		return parse_wrapped_credentials_field(res);
	}
}

void set_login(const std::string& username)
{
	set_wrap_login(true);
	preferences::set("login", wrap_credentials_field_value(username));
}

namespace prv {
	std::string password;
}

std::string password()
{
	if(remember_password()) {
		const std::string saved_pass = preferences::get("password");
		if(!wrap_password()) {
			return saved_pass;
		} else {
			return parse_wrapped_credentials_field(saved_pass);
		}
	} else {
		return prv::password;
	}
}

void set_password(const std::string& password)
{
	prv::password = password;
	if(remember_password()) {
		set_wrap_password(true);
		preferences::set("password", wrap_credentials_field_value(password));
	}
}

bool remember_password()
{
	return preferences::get("remember_password", false);
}

void set_remember_password(bool remember)
{
	preferences::set("remember_password", remember);
	preferences::set("password", remember ? prv::password : "");
}

bool turn_dialog()
{
	return preferences::get("turn_dialog", false);
}

void set_turn_dialog(bool ison)
{
	preferences::set("turn_dialog", ison);
}

bool enable_whiteboard_mode_on_start()
{
	return preferences::get("enable_planning_mode_on_start", false);
}

void set_enable_whiteboard_mode_on_start(bool value)
{
	preferences::set("enable_planning_mode_on_start", value);
}

bool hide_whiteboard()
{
	return preferences::get("hide_whiteboard", false);
}

void set_hide_whiteboard(bool value)
{
	preferences::set("hide_whiteboard", value);
}

bool show_combat()
{
	return preferences::get("show_combat", true);
}

bool allow_observers()
{
	return preferences::get("allow_observers", true);
}

void set_allow_observers(bool value)
{
	preferences::set("allow_observers", value);
}

bool shuffle_sides()
{
	return preferences::get("shuffle_sides", false);
}

void set_shuffle_sides(bool value)
{
	preferences::set("shuffle_sides", value);
}

bool use_map_settings()
{
	return preferences::get("mp_use_map_settings", true);
}

void set_use_map_settings(bool value)
{
	preferences::set("mp_use_map_settings", value);
}

int mp_server_warning_disabled()
{
	return lexical_cast_default<int>(preferences::get("mp_server_warning_disabled"), 0);
}

void set_mp_server_warning_disabled(int value)
{
	preferences::set("mp_server_warning_disabled", value);
}

void set_mp_server_program_name(const std::string& path)
{
	if (path.empty())
	{
		preferences::clear("mp_server_program_name");
	}
	else
	{
		preferences::set("mp_server_program_name", path);
	}
}

std::string get_mp_server_program_name()
{
	return preferences::get("mp_server_program_name");
}

bool random_start_time()
{
	return preferences::get("mp_random_start_time", true);
}

void set_random_start_time(bool value)
{
	preferences::set("mp_random_start_time", value);
}

bool fog()
{
	return preferences::get("mp_fog", true);
}

void set_fog(bool value)
{
	preferences::set("mp_fog", value);
}

bool shroud()
{
	return preferences::get("mp_shroud", false);
}

void set_shroud(bool value)
{
	preferences::set("mp_shroud", value);
}

int turns()
{
	return settings::get_turns(preferences::get("mp_turns"));
}

void set_turns(int value)
{
	preferences::set("mp_turns", value);
}

bool skip_mp_replay()
{
	return preferences::get("skip_mp_replay", false);
}

void set_skip_mp_replay(bool value)
{
	preferences::set("skip_mp_replay", value);
}

bool countdown()
{
	return preferences::get("mp_countdown", false);
}

void set_countdown(bool value)
{
	preferences::set("mp_countdown", value);
}

int countdown_init_time()
{
	return lexical_cast_in_range<int>
		(preferences::get("mp_countdown_init_time"), 270, 0, 1500);
}

void set_countdown_init_time(int value)
{
	preferences::set("mp_countdown_init_time", value);
}

int countdown_reservoir_time()
{
	return lexical_cast_in_range<int>(
		preferences::get("mp_countdown_reservoir_time"), 330, 30, 1500);
}

void set_countdown_reservoir_time(int value)
{
	preferences::set("mp_countdown_reservoir_time", value);
}

int countdown_turn_bonus()
{
	return lexical_cast_in_range<int>(
		preferences::get("mp_countdown_turn_bonus"), 60, 0, 300);
}

void set_countdown_turn_bonus(int value)
{
	preferences::set("mp_countdown_turn_bonus", value);
}

int countdown_action_bonus()
{
	return lexical_cast_in_range<int>(
		preferences::get("mp_countdown_action_bonus"), 13, 0, 30);
}

void set_countdown_action_bonus(int value)
{
	preferences::set("mp_countdown_action_bonus", value);
}

int village_gold()
{
	return settings::get_village_gold(preferences::get("mp_village_gold"));
}

void set_village_gold(int value)
{
	preferences::set("mp_village_gold", value);
}

int xp_modifier()
{
	return settings::get_xp_modifier(preferences::get("mp_xp_modifier"));
}

void set_xp_modifier(int value)
{
	preferences::set("mp_xp_modifier", value);
}

int era()
{
	return lexical_cast_default<int>(preferences::get("mp_era"), 0);
}

void set_era(int value)
{
	preferences::set("mp_era", value);
}

int map()
{
	return lexical_cast_default<int>(preferences::get("mp_map"), 0);
}

void set_map(int value)
{
	preferences::set("mp_map", value);
}

bool show_ai_moves()
{
	return preferences::get("show_ai_moves", true);
}

void set_show_ai_moves(bool value)
{
	preferences::set("show_ai_moves", value);
}

void set_show_side_colors(bool value)
{
	preferences::set("show_side_colors", value);
}

bool show_side_colors()
{
	return preferences::get("show_side_colors", true);
}

void set_save_replays(bool value)
{
	preferences::set("save_replays", value);
}

bool save_replays()
{
	return preferences::get("save_replays", false);
}

void set_delete_saves(bool value)
{
	preferences::set("delete_saves", value);
}

bool delete_saves()
{
	return preferences::get("delete_saves", false);
}

void set_ask_delete_saves(bool value)
{
	preferences::set("ask_delete", value);
}

bool ask_delete_saves()
{
	return preferences::get("ask_delete", true);
}

void set_interrupt_when_ally_sighted(bool value)
{
	preferences::set("ally_sighted_interrupts", value);
}

bool interrupt_when_ally_sighted()
{
	return preferences::get("ally_sighted_interrupts", true);
}

int autosavemax()
{
	return lexical_cast_default<int>(preferences::get("auto_save_max"), 10);
}

void set_autosavemax(int value)
{
	preferences::set("auto_save_max", value);
}

std::string client_type()
{
	return preferences::get("client_type") == "ai" ? "ai" : "human";
}

std::string theme()
{
	if(non_interactive()) {
		static const std::string null_theme = "null";
		return null_theme;
	}

	std::string res = preferences::get("theme");
	if(res.empty()) {
		return "Default";
	}

	return res;
}

void set_theme(const std::string& theme)
{
	if(theme != "null") {
		preferences::set("theme", theme);
	}
}

bool show_floating_labels()
{
	return preferences::get("floating_labels", true);
}

void set_show_floating_labels(bool value)
{
	preferences::set("floating_labels", value);
}

bool message_private()
{
	return message_private_on;
}

void set_message_private(bool value)
{
	message_private_on = value;
}

bool show_haloes()
{
	return haloes;
}

void set_show_haloes(bool value)
{
	haloes = value;
	preferences::set("show_haloes", value);
}

bool flip_time()
{
	return preferences::get("flip_time", false);
}

void set_flip_time(bool value)
{
	preferences::set("flip_time", value);
}

bool compress_saves()
{
	return preferences::get("compress_saves", true);
}

bool startup_effect()
{
	return preferences::get("startup_effect", false);
}

std::string get_chat_timestamp(const time_t& t) {
	if (chat_timestamping()) {
		if(preferences::use_twelve_hour_clock_format() == false) {
			return lg::get_timestamp(t, _("%H:%M")) + " ";
		}
		else {
			return lg::get_timestamp(t, _("%I:%M %p")) + " ";
		}
	}
	return "";
}

bool chat_timestamping() {
	return preferences::get("chat_timestamp", false);
}

void set_chat_timestamping(bool value) {
	preferences::set("chat_timestamp", value);
}

int chat_lines()
{
	return lexical_cast_default<int>(preferences::get("chat_lines"), 6);
}

void set_chat_lines(int lines)
{
	preferences::set("chat_lines", lines);
}

void set_chat_message_aging(const int aging)
{
	preferences::set("chat_message_aging", aging);
}

int chat_message_aging()
{
	return lexical_cast_default<int>(preferences::get("chat_message_aging"), 20);
}

bool show_all_units_in_help() {
	return preferences::get("show_all_units_in_help", false);
}

void set_show_all_units_in_help(bool value) {
	preferences::set("show_all_units_in_help", value);
}

std::set<std::string> &encountered_units() {
	return encountered_units_set;
}

std::set<t_translation::t_terrain> &encountered_terrains() {
	return encountered_terrains_set;
}

std::string custom_command() {
	return preferences::get("custom_command");
}

void set_custom_command(const std::string& command) {
	preferences::set("custom_command", command);
}

/**
 * Returns a pointer to the history vector associated with given id
 * making a new one if it doesn't exist.
 *
 * @todo FIXME only used for gui2. Could be used for the above histories.
 */
std::vector<std::string>* get_history(const std::string& id) {
	return &history_map[id];
}

bool green_confirm()
{
	std::string confirmation = preferences::get("confirm_end_turn");

	if (confirmation == "green" || confirmation == "yes")
		return true;
	return false;
}

bool yellow_confirm()
{
	return preferences::get("confirm_end_turn") == "yellow";
}

bool confirm_no_moves()
{
	//This is very non-intrusive so it is on by default
	const std::string confirmation = preferences::get("confirm_end_turn");
	return confirmation == "no_moves" || confirmation.empty();
}


void encounter_recruitable_units(std::vector<team>& teams){
	for (std::vector<team>::iterator help_team_it = teams.begin();
		help_team_it != teams.end(); ++help_team_it) {
		help_team_it->log_recruitable();
		std::copy(help_team_it->recruits().begin(), help_team_it->recruits().end(),
				  std::inserter(encountered_units_set, encountered_units_set.begin()));
	}
}

void encounter_start_units(unit_map& units){
	for (unit_map::const_iterator help_unit_it = units.begin();
		 help_unit_it != units.end(); ++help_unit_it) {
		const std::string name = help_unit_it->type_id();
		encountered_units_set.insert(name);
	}
}

void encounter_recallable_units(std::vector<team>& teams){
	BOOST_FOREACH(const team& t, teams) {
		BOOST_FOREACH(const unit& u, t.recall_list()) {
			encountered_units_set.insert(u.type_id());
		}
	}
}

void encounter_map_terrain(gamemap& map){
	for (int map_x = 0; map_x < map.w(); ++map_x) {
		for (int map_y = 0; map_y < map.h(); ++map_y) {
			const t_translation::t_terrain t = map.get_terrain(map_location(map_x, map_y));
			preferences::encountered_terrains().insert(t);
			const t_translation::t_list& underlaying_list = map.underlying_union_terrain(map_location(map_x, map_y));
			for (std::vector<t_translation::t_terrain>::const_iterator ut = underlaying_list.begin(); ut != underlaying_list.end(); ++ut) {
				preferences::encountered_terrains().insert(*ut);
			};
		}
	}
}

} // preferences namespace
