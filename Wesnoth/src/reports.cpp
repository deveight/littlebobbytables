/* $Id: reports.cpp 55068 2012-08-23 20:42:35Z jamit $ */
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

#include "actions.hpp"
#include "attack_prediction.hpp"
#include "editor/editor_controller.hpp"
#include "editor/editor_palettes.hpp"
#include "font.hpp"
#include "game_display.hpp"
#include "game_preferences.hpp"
#include "gettext.hpp"
#include "language.hpp"
#include "map.hpp"
#include "marked-up_text.hpp"
#include "play_controller.hpp"
#include "reports.hpp"
#include "resources.hpp"
#include "team.hpp"
#include "text.hpp"
#include "tod_manager.hpp"
#include "unit.hpp"
#include "whiteboard/manager.hpp"

#include <boost/foreach.hpp>

#include <cassert>
#include <ctime>

static void add_text(config &report, const std::string &text,
	const std::string &tooltip, const std::string &help = "")
{
	config &element = report.add_child("element");
	element["text"] = text;
	if (!tooltip.empty()) element["tooltip"] = tooltip;
	if (!help.empty()) element["help"] = help;
}

static void add_image(config &report, const std::string &image,
	const std::string &tooltip, const std::string &help = "")
{
	config &element = report.add_child("element");
	element["image"] = image;
	if (!tooltip.empty()) element["tooltip"] = tooltip;
	if (!help.empty()) element["help"] = help;
}

static config report()
{
	return config();
}

static config text_report(const std::string &text,
	const std::string &tooltip = "", const std::string &help = "")
{
	config r;
	add_text(r, text, tooltip, help);
	return r;
}

static config image_report(const std::string &image,
	const std::string &tooltip = "", const std::string &help = "")
{
	config r;
	add_image(r, image, tooltip, help);
	return r;
}

using font::span_color;

static void add_status(config &r,
	char const *path, char const *desc1, char const *desc2)
{
	std::ostringstream s;
	s << gettext(desc1) << gettext(desc2);
	add_image(r, path, s.str());
}

static std::string flush(std::ostringstream &s)
{
	std::string r(s.str());
	s.str(std::string());
	return r;
}

typedef config (*generator_function)();

typedef std::map<std::string, generator_function> static_report_generators;
typedef std::map<std::string, reports::generator *> dynamic_report_generators;
static static_report_generators static_generators;
static dynamic_report_generators dynamic_generators;

struct report_generator_helper
{
	report_generator_helper(const char *name, generator_function g)
	{
		static_generators.insert(static_report_generators::value_type(name, g));
	}
};

#define REPORT_GENERATOR(n) \
	static config report_##n(); \
	static report_generator_helper reg_gen_##n(#n, &report_##n); \
	static config report_##n()

static char const *naps = "</span>";

static unit *get_visible_unit()
{
	return get_visible_unit(resources::screen->displayed_unit_hex(),
		(*resources::teams)[resources::screen->viewing_team()],
		resources::screen->show_everything());
}

static unit *get_selected_unit()
{
	return get_visible_unit(resources::screen->selected_hex(),
		(*resources::teams)[resources::screen->viewing_team()],
		resources::screen->show_everything());
}

static config gray_inactive(const std::string &str)
{
	if (resources::screen->viewing_side() == resources::screen->playing_side())
		return text_report(str);
	return text_report(span_color(font::GRAY_COLOR) + str + naps);
}

static config unit_name(unit *u)
{
	if (!u) {
		return report();
	}

	/*
	 * The name needs to be escaped, it might be set by the user and using
	 * markup. Also names often contain a forbidden single quote.
	 */
	const std::string& name = font::escape_text(u->name());
	std::ostringstream str, tooltip;
	str << "<b>" << name << "</b>";
	tooltip << _("Name: ") << "<b>" << name << "</b>";
	return text_report(str.str(), tooltip.str());
}

REPORT_GENERATOR(unit_name)
{
	unit *u = get_visible_unit();
	return unit_name(u);
}
REPORT_GENERATOR(selected_unit_name)
{
	unit *u = get_selected_unit();
	return unit_name(u);
}

static config unit_type(unit* u)
{
	if (!u) return report();
	std::ostringstream str, tooltip;
	str << span_color(font::unit_type_color) << u->type_name() << naps;
	tooltip << _("Type: ") << "<b>" << u->type_name() << "</b>\n"
		<< u->unit_description();
	return text_report(str.str(), tooltip.str(), "unit_" + u->type_id());
}
REPORT_GENERATOR(unit_type)
{
	unit *u = get_visible_unit();
	return unit_type(u);
}
REPORT_GENERATOR(selected_unit_type)
{
	unit *u = get_selected_unit();
	return unit_type(u);
}

static config unit_race(unit* u)
{
	if (!u) return report();
	std::ostringstream str, tooltip;
	str << span_color(font::race_color) << u->race()->name(u->gender()) << naps;
	tooltip << _("Race: ") << "<b>" << u->race()->name(u->gender()) << "</b>";
	return text_report(str.str(), tooltip.str(), "..race_" + u->race()->id());
}
REPORT_GENERATOR(unit_race)
{
	unit *u = get_visible_unit();
	return unit_race(u);
}
REPORT_GENERATOR(selected_unit_race)
{
	unit *u = get_selected_unit();
	return unit_race(u);
}

static config unit_side(unit* u)
{
	if (!u) return report();
	const team &u_team = (*resources::teams)[u->side() - 1];
	std::string flag_icon = u_team.flag_icon();
	std::string old_rgb = game_config::flag_rgb;
	std::string new_rgb = team::get_side_color_index(u->side());
	std::string mods = "~RC(" + old_rgb + ">" + new_rgb + ")";
	if (flag_icon.empty())
		flag_icon = game_config::images::flag_icon;
	return image_report(flag_icon + mods, u_team.current_player());
}
REPORT_GENERATOR(unit_side)
{
	unit *u = get_visible_unit();
	return unit_side(u);
}
REPORT_GENERATOR(selected_unit_side)
{
	unit *u = get_selected_unit();
	return unit_side(u);
}

static config unit_level(unit* u)
{
	if (!u) return report();
	std::ostringstream str, tooltip;
	str << u->level();
	tooltip << _("Level: ") << "<b>" << u->level() << "</b>\n";
	const std::vector<std::string> &adv_to = u->advances_to_translated();
	if (adv_to.empty())
		tooltip << _("No advancement");
	else
		tooltip << _("Advances to:") << "\n<b>\t"
			<< utils::join(adv_to, "\n\t") << "</b>";
	return text_report(str.str(), tooltip.str());
}
REPORT_GENERATOR(unit_level)
{
	unit *u = get_visible_unit();
	return unit_level(u);
}
REPORT_GENERATOR(selected_unit_level)
{
	unit *u = get_selected_unit();
	return unit_level(u);
}

REPORT_GENERATOR(unit_amla)
{
	unit *u = get_visible_unit();
	if (!u) return report();
	config res;
	typedef std::pair<std::string, std::string> pair_string;
	BOOST_FOREACH(const pair_string &ps, u->amla_icons()) {
		add_image(res, ps.first, ps.second);
	}
	return res;
}

static config unit_traits(unit* u)
{

	if (!u) return report();
	config res;
	const std::vector<t_string> &traits = u->trait_names();
	const std::vector<t_string> &descriptions = u->trait_descriptions();
	unsigned nb = traits.size();
	for (unsigned i = 0; i < nb; ++i)
	{
		std::ostringstream str, tooltip;
		str << traits[i];
		if (i != nb - 1 ) str << ", ";
		tooltip << _("Trait: ") << "<b>" << traits[i] << "</b>\n"
			<< descriptions[i];
		add_text(res, str.str(), tooltip.str());
	}
	return res;
}
REPORT_GENERATOR(unit_traits)
{
	unit *u = get_visible_unit();
	return unit_traits(u);
}
REPORT_GENERATOR(selected_unit_traits)
{
	unit *u = get_selected_unit();
	return unit_traits(u);
}

static config unit_status(unit* u)
{
	if (!u) return report();
	config res;
	map_location displayed_unit_hex = resources::screen->displayed_unit_hex();
	if (resources::game_map->on_board(displayed_unit_hex) && u->invisible(displayed_unit_hex)) {
		add_status(res, "misc/invisible.png", N_("invisible: "),
			N_("This unit is invisible. It cannot be seen or attacked by enemy units."));
	}
	if (u->get_state(unit::STATE_SLOWED)) {
		add_status(res, "misc/slowed.png", N_("slowed: "),
			N_("This unit has been slowed. It will only deal half its normal damage when attacking and its movement cost is doubled."));
	}
	if (u->get_state(unit::STATE_POISONED)) {
		add_status(res, "misc/poisoned.png", N_("poisoned: "),
			N_("This unit is poisoned. It will lose 8 HP every turn until it can seek a cure to the poison in a village or from a friendly unit with the ‘cures’ ability.\n\nUnits cannot be killed by poison alone. The poison will not reduce it below 1 HP."));
	}
	if (u->get_state(unit::STATE_PETRIFIED)) {
		add_status(res, "misc/petrified.png", N_("petrified: "),
			N_("This unit has been petrified. It may not move or attack."));
	}
	return res;
}
REPORT_GENERATOR(unit_status)
{
	unit *u = get_visible_unit();
	return unit_status(u);
}
REPORT_GENERATOR(selected_unit_status)
{
	unit *u = get_selected_unit();
	return unit_status(u);
}

static config unit_alignment(unit* u)
{
	if (!u) return report();
	std::ostringstream str, tooltip;
	char const *align = unit_type::alignment_description(u->alignment(), u->gender());
	std::string align_id = unit_type::alignment_id(u->alignment());
	int cm = combat_modifier(resources::screen->displayed_unit_hex(), u->alignment(), u->is_fearless());
	str << align << " (" << utils::signed_percent(cm) << ")";
	tooltip << _("Alignment: ") << "<b>" << align << "</b>\n"
		<< string_table[align_id + "_description"];
	return text_report(str.str(), tooltip.str(), "time_of_day");
}
REPORT_GENERATOR(unit_alignment)
{
	unit *u = get_visible_unit();
	return unit_alignment(u);
}
REPORT_GENERATOR(selected_unit_alignment)
{
	unit *u = get_selected_unit();
	return unit_alignment(u);
}


static config unit_abilities(unit* u)
{
	if (!u) return report();
	config res;
	const std::vector<boost::tuple<t_string,t_string,t_string> > &abilities = u->ability_tooltips();
	for (std::vector<boost::tuple<t_string,t_string,t_string> >::const_iterator i = abilities.begin(),
	     i_end = abilities.end(); i != i_end; ++i)
	{
		std::ostringstream str, tooltip;
		const std::string &name = i->get<0>().base_str();
		str << i->get<1>().str();
		if (i + 1 != i_end) str << ", ";
		tooltip << _("Ability: ") << i->get<2>().str();
		add_text(res, str.str(), tooltip.str(), "ability_" + name);
	}
	return res;
}
REPORT_GENERATOR(unit_abilities)
{
	unit *u = get_visible_unit();
	return unit_abilities(u);
}
REPORT_GENERATOR(selected_unit_abilities)
{
	unit *u = get_selected_unit();
	return unit_abilities(u);
}


static config unit_hp(unit* u)
{
	if (!u) return report();
	std::ostringstream str, tooltip;
	str << span_color(u->hp_color()) << u->hitpoints()
		<< '/' << u->max_hitpoints() << naps;

	std::set<std::string> resistances_table;
	utils::string_map resistances = u->get_base_resistances();

	bool att_def_diff = false;
	map_location displayed_unit_hex = resources::screen->displayed_unit_hex();
	BOOST_FOREACH(const utils::string_map::value_type &resist, u->get_base_resistances())
	{
		std::ostringstream line;
		line << gettext(resist.first.c_str()) << ": ";
		// Some units have different resistances when attacking or defending.
		int res_att = 100 - u->resistance_against(resist.first, true, displayed_unit_hex);
		int res_def = 100 - u->resistance_against(resist.first, false, displayed_unit_hex);
		if (res_att == res_def) {
			line << utils::signed_percent(res_def) << "\n";
		} else {
			line << utils::signed_percent(res_att) << " / " << utils::signed_percent(res_def) << '\n';
			att_def_diff = true;
		}
		resistances_table.insert(line.str());
	}

	tooltip << _("Resistances: ");
	if (att_def_diff)
		tooltip << _("(Att / Def)");
	tooltip << '\n';
	BOOST_FOREACH(const std::string &line, resistances_table) {
		tooltip << line;
	}
	return text_report(str.str(), tooltip.str());
}
REPORT_GENERATOR(unit_hp)
{
	unit *u = get_visible_unit();
	return unit_hp(u);
}
REPORT_GENERATOR(selected_unit_hp)
{
	unit *u = get_selected_unit();
	return unit_hp(u);
}

static config unit_xp(unit* u)
{
	if (!u) return report();
	std::ostringstream str, tooltip;
	str << span_color(u->xp_color()) << u->experience()
		<< '/' << u->max_experience() << naps;

	int exp_mod = unit_type::experience_accelerator::get_acceleration();
	tooltip << _("Experience Modifier: ") << exp_mod << '%';
	return text_report(str.str(), tooltip.str());
}
REPORT_GENERATOR(unit_xp)
{
	unit *u = get_visible_unit();
	return unit_xp(u);
}
REPORT_GENERATOR(selected_unit_xp)
{
	unit *u = get_selected_unit();
	return unit_xp(u);
}

static config unit_advancement_options(unit* u)
{
	if (!u) return report();
	config res;
	typedef std::pair<std::string, std::string> pair_string;
	BOOST_FOREACH(const pair_string &ps, u->advancement_icons()) {
		add_image(res, ps.first, ps.second);
	}
	return res;
}
REPORT_GENERATOR(unit_advancement_options)
{
	unit *u = get_visible_unit();
	return unit_advancement_options(u);
}
REPORT_GENERATOR(selected_unit_advancement_options)
{
	unit *u = get_selected_unit();
	return unit_advancement_options(u);
}

static config unit_defense(unit* u, const map_location& displayed_unit_hex)
{
	if(!u) {
		return report();
	}

	std::ostringstream str, tooltip;
	const gamemap &map = *resources::game_map;
	if(!resources::game_map->on_board(displayed_unit_hex)) {
		return report();
	}

	const t_translation::t_terrain &terrain = map[displayed_unit_hex];
	int def = 100 - u->defense_modifier(terrain);
	SDL_Color color = int_to_color(game_config::red_to_green(def));
	str << span_color(color) << def << "%</span>";
	tooltip << _("Terrain: ") << "<b>" << map.get_terrain_info(terrain).description() << "</b>\n";

	const t_translation::t_list &underlyings = map.underlying_def_terrain(terrain);
	std::vector<int> t_defs;
	bool revert = false;
	if (underlyings.size() != 1 || underlyings.front() != terrain)
	{
		BOOST_FOREACH(const t_translation::t_terrain &t, underlyings)
		{
			if (t == t_translation::MINUS) {
				revert = true;
			} else if (t == t_translation::PLUS) {
				revert = false;
			} else {
				int t_def = 100 - u->defense_modifier(t);
				SDL_Color color = int_to_color(game_config::red_to_green(t_def));
				tooltip << '\t' << map.get_terrain_info(t).description() << ": "
					<< span_color(color) << t_def << "%</span> "
					<< (revert ? _("maximum^max.") : _("minimum^min.")) << '\n';
			}
		}
	}

    	tooltip << "<b>" << _("Defense: ") << span_color(color)  << def << "%</span></b>";
	return text_report(str.str(), tooltip.str());
}
REPORT_GENERATOR(unit_defense)
{
	unit *u = get_visible_unit();
	const map_location& displayed_unit_hex = resources::screen->displayed_unit_hex();
	return unit_defense(u, displayed_unit_hex);
}
REPORT_GENERATOR(selected_unit_defense)
{
	unit *u = get_selected_unit();
	const map_location& selected_hex = resources::screen->selected_hex();
	return unit_defense(u, selected_hex);
}

static config unit_moves(unit* u)
{
	if (!u) return report();
	std::ostringstream str;
	double movement_frac = 1.0;
	if (u->side() == resources::screen->playing_side()) {
		movement_frac = double(u->movement_left()) / std::max<int>(1, u->total_movement());
		if (movement_frac > 1.0)
			movement_frac = 1.0;
	}

	int grey = 128 + int((255 - 128) * movement_frac);
	SDL_Color c = create_color(grey, grey, grey);
	str << span_color(c) << u->movement_left() << '/' << u->total_movement() << naps;
	return text_report(str.str());
}
REPORT_GENERATOR(unit_moves)
{
	unit *u = get_visible_unit();
	return unit_moves(u);
}
REPORT_GENERATOR(selected_unit_moves)
{
	unit *u = get_selected_unit();
	return unit_moves(u);
}

static int attack_info(const attack_type &at, config &res, unit *u, const map_location &displayed_unit_hex)
{
	std::ostringstream str, tooltip;

	at.set_specials_context(displayed_unit_hex, map_location(), *u);
	int base_damage = at.damage();
	int damage_multiplier = 100;
	int tod_bonus = combat_modifier(displayed_unit_hex, u->alignment(), u->is_fearless());
	damage_multiplier += tod_bonus;
	int leader_bonus = 0;
	if (under_leadership(*resources::units, displayed_unit_hex, &leader_bonus).valid())
		damage_multiplier += leader_bonus;

	bool slowed = u->get_state(unit::STATE_SLOWED);
	int damage_divisor = slowed ? 20000 : 10000;
	// Assume no specific resistance (i.e. multiply by 100).
	int damage = round_damage(base_damage, damage_multiplier * 100, damage_divisor);

	int base_nattacks = at.num_attacks();
	int nattacks = base_nattacks;
	unit_ability_list swarm = at.get_specials("swarm");
	if (!swarm.empty())
	{
		int swarm_max_attacks = swarm.highest("swarm_attacks_max", nattacks).first;
		int swarm_min_attacks = swarm.highest("swarm_attacks_min").first;
		int hitp = u->hitpoints();
		int mhitp = u->max_hitpoints();
		nattacks = swarm_min_attacks + (swarm_max_attacks - swarm_min_attacks) * hitp / mhitp;
	}
	SDL_Color dmg_color = font::weapon_color;
	double dmg_bonus = double(damage) / base_damage;
	if (dmg_bonus > 1.0)
		dmg_color = font::good_dmg_color;
	else if (dmg_bonus < 1.0)
		dmg_color = font::bad_dmg_color;

	str << span_color(dmg_color) << damage << naps << span_color(font::weapon_color)
		<< font::weapon_numbers_sep << nattacks << ' ' << at.name()
		<< "</span>\n";
	tooltip << _("Weapon: ") << "<b>" << at.name() << "</b>\n"
		<< _("Damage: ") << "<b>" << damage << "</b>\n";

	if (tod_bonus || leader_bonus || slowed)
	{
		tooltip << '\t' << _("Base damage: ") << base_damage << '\n';
		if (tod_bonus) {
			tooltip << '\t' << _("Time of day: ")
				<< utils::signed_percent(tod_bonus) << '\n';
		}
		if (leader_bonus) {
			tooltip << '\t' << _("Leadership: ")
				<< utils::signed_percent(leader_bonus) << '\n';
		}
		if (slowed) {
			tooltip << '\t' << _("Slowed: ") << "/ 2" << '\n';
		}
	}

	tooltip << _("Attacks: ") << "<b>" << nattacks << "</b>\n";
	if (nattacks != base_nattacks){
		tooltip << '\t' << _("Base attacks: ") << base_nattacks << '\n';
		int hp_ratio = u->hitpoints() * 100 / u->max_hitpoints();
		tooltip << '\t' << _("Swarm: ") << "* "<< hp_ratio << "%\n";
	}

	add_text(res, flush(str), flush(tooltip));

	std::string range = string_table["range_" + at.range()];
	std::string lang_type = string_table["type_" + at.type()];

	str << span_color(font::weapon_details_color) << "  "
		<< range << font::weapon_details_sep
		<< lang_type << "</span>\n";

	tooltip << _("Weapon range: ") << "<b>" << range << "</b>\n"
		<< _("Damage type: ")  << "<b>" << lang_type << "</b>\n"
		<< _("Damage versus: ") << '\n';

	// Show this weapon damage and resistance against all the different units.
	// We want weak resistances (= good damage) first.
	std::map<int, std::set<std::string>, std::greater<int> > resistances;
	std::set<std::string> seen_types;
	const team &unit_team = (*resources::teams)[u->side() - 1];
	const team &viewing_team = (*resources::teams)[resources::screen->viewing_team()];
	BOOST_FOREACH(const unit &enemy, *resources::units)
	{
		if (!unit_team.is_enemy(enemy.side()))
			continue;
		const map_location &loc = enemy.get_location();
		if (viewing_team.fogged(loc) ||
		    (viewing_team.is_enemy(enemy.side()) && enemy.invisible(loc)))
			continue;
		bool new_type = seen_types.insert(enemy.type_id()).second;
		if (new_type) {
			int resistance = enemy.resistance_against(at, false, loc);
			resistances[resistance].insert(enemy.type_name());
		}
	}

	typedef std::pair<int, std::set<std::string> > resist_units;
	BOOST_FOREACH(const resist_units &resist, resistances) {
		int damage = round_damage(base_damage, damage_multiplier * resist.first, damage_divisor);
		tooltip << "<b>" << damage << "</b>  "
			<< "<i>(" << utils::signed_percent(resist.first-100) << ")</i> : "
			<< utils::join(resist.second, ", ") << '\n';
	}
	add_text(res, flush(str), flush(tooltip));

	const std::string &accuracy_parry = at.accuracy_parry_description();
	if (!accuracy_parry.empty())
	{
		str << span_color(font::weapon_details_color)
			<< "  " << accuracy_parry << "</span>\n";
		int accuracy = at.accuracy();
		if (accuracy) {
			tooltip << _("Accuracy:") << "<b>"
				<< utils::signed_percent(accuracy) << "</b>\n";
		}
		int parry = at.parry();
		if (parry) {
			tooltip << _("Parry:") << "<b>"
				<< utils::signed_percent(parry) << "</b>\n";
			}
		add_text(res, flush(str), flush(tooltip));
	}

	const std::vector<t_string> &specials = at.special_tooltips();
	if (!specials.empty())
	{
		for (std::vector<t_string>::const_iterator sp_it = specials.begin(),
		     sp_end = specials.end(); sp_it != sp_end; ++sp_it)
		{
			str << span_color(font::weapon_details_color)
				<< "  " << *sp_it << "</span>\n";
			std::string help_page = "weaponspecial_" + sp_it->base_str();
			++sp_it;
			//FIXME pull out special's name from description
			tooltip << _("Weapon special: ") << *sp_it << '\n';
			add_text(res, flush(str), flush(tooltip), help_page);
		}
	}
	return damage;
}

// Conversion routine for both unscathed and damage change percentage.
static void format_prob(char str_buf[10], double prob)
{

	if(prob > 0.9995) {
		snprintf(str_buf, 10, "100 %%");
	} else if(prob >= 0.1) {
		snprintf(str_buf, 10, "%4.1f %%", 100.0 * prob);
	} else {
		snprintf(str_buf, 10, " %3.1f %%", 100.0 * prob);
	}

	str_buf[9] = '\0';  //prevents _snprintf error
}

static void format_hp(char str_buf[10], int hp)
{
	if(hp < 10) {
		snprintf(str_buf, 10, "   %i", hp);
	} else if(hp < 99) {
		snprintf(str_buf, 10, "  %i", hp);
	} else if(hp < 999) {
		snprintf(str_buf, 10, " %i", hp);
	} else {
		snprintf(str_buf, 10, " %i", hp);
	}

	str_buf[9] = '\0';  //prevents _snprintf error
}

static config unit_weapons(unit *attacker, const map_location &attacker_pos, unit *defender, bool show_attacker)
{
	if (!attacker || !defender) return report();

	unit* u = show_attacker ? attacker : defender;
	const map_location unit_loc = show_attacker ? attacker_pos : defender->get_location();

	std::ostringstream str, tooltip;
	config res;

	std::vector<battle_context> weapons;
	for (unsigned int i = 0; i < attacker->attacks().size(); i++) {
		// skip weapons with attack_weight=0
		if (attacker->attacks()[i].attack_weight() > 0) {
			battle_context weapon(*resources::units, attacker_pos, defender->get_location(), i, -1, 0.0, NULL, attacker);
			weapons.push_back(weapon);
		}
	}

	BOOST_FOREACH(const battle_context& weapon, weapons) {

		// Predict the battle outcome.
		combatant attacker_combatant(weapon.get_attacker_stats());
		combatant defender_combatant(weapon.get_defender_stats());
		attacker_combatant.fight(defender_combatant);

		const battle_context_unit_stats& context_unit_stats =
				show_attacker ? weapon.get_attacker_stats() : weapon.get_defender_stats();

		int total_damage = 0;
		int base_damage = 0;
		int num_blows = 0;
		int chance_to_hit = 0;
		t_string weapon_name = _("None");

		SDL_Color dmg_color = font::weapon_color;
		if (context_unit_stats.weapon) {
			base_damage = attack_info(*context_unit_stats.weapon, res, u, unit_loc);
			total_damage = context_unit_stats.damage;
			num_blows = context_unit_stats.num_blows;
			chance_to_hit = context_unit_stats.chance_to_hit;
			weapon_name = context_unit_stats.weapon->name();

			double dmg_bonus = double(total_damage) / base_damage;
			if (dmg_bonus > 1.0)
				dmg_color = font::good_dmg_color;
			else if (dmg_bonus < 1.0)
				dmg_color = font::bad_dmg_color;
		} else {
			str << span_color(font::weapon_color) << weapon_name << naps << "\n";
			tooltip << _("Weapon: ") << "<b>" << weapon_name << "</b>\n"
				<< _("Damage: ") << "<b>" << "0" << "</b>\n";
		}

		SDL_Color chance_color = int_to_color(game_config::red_to_green(chance_to_hit));

		// Total damage.
		str << "  " << span_color(dmg_color) << total_damage << naps << span_color(font::weapon_color)
			<< utils::unicode_en_dash << num_blows
			<< " (" << span_color(chance_color) << chance_to_hit << "%" << naps << ")"
			<< naps << "\n";

		tooltip << _("Weapon: ") << "<b>" << weapon_name << "</b>\n"
				<< _("Total damage") << "<b>" << total_damage << "</b>\n";

		// Create the hitpoints distribution.
		std::vector<std::pair<int, double> > hp_prob_vector;

		// First, we sort the probabilities in ascending order.
		std::vector<std::pair<double, int> > prob_hp_vector;
		int i;

		combatant* c = show_attacker ? &attacker_combatant : &defender_combatant;

		for(i = 0; i < static_cast<int>(c->hp_dist.size()); i++) {
			double prob = c->hp_dist[i];

			// We keep only values above 0.1%.
			if(prob > 0.001)
				prob_hp_vector.push_back(std::pair<double, int>(prob, i));
		}

		std::sort(prob_hp_vector.begin(), prob_hp_vector.end());

		//TODO fendrin -- make that dynamically
		int max_hp_distrib_rows_ = 10;

		// We store a few of the highest probability hitpoint values.
		int nb_elem = std::min<int>(max_hp_distrib_rows_, prob_hp_vector.size());

		for(i = prob_hp_vector.size() - nb_elem;
				i < static_cast<int>(prob_hp_vector.size()); i++) {

			hp_prob_vector.push_back(std::pair<int, double>
			(prob_hp_vector[i].second, prob_hp_vector[i].first));
		}

		// Then, we sort the hitpoint values in ascending order.
		std::sort(hp_prob_vector.begin(), hp_prob_vector.end());
		// And reverse the order. Might be doable in a better manor.
		std::reverse(hp_prob_vector.begin(), hp_prob_vector.end());

		for(i = 0;
				i < static_cast<int>(hp_prob_vector.size()); i++) {

			int hp = hp_prob_vector[i].first;
			double prob = hp_prob_vector[i].second;

			char prob_buf[10];
			format_prob(prob_buf, prob);
			char hp_buf[10];
			format_hp(hp_buf, hp);

			SDL_Color prob_color = int_to_color(game_config::blue_to_white(prob * 100.0, true));

			str		<< span_color(font::weapon_details_color) << "  " << "  "
					<< span_color(u->hp_color(hp)) << hp_buf << naps
					<< " " << font::weapon_numbers_sep << " "
					<< span_color(prob_color) << prob_buf << naps
					<< naps << "\n";
		}

		add_text(res, flush(str), flush(tooltip));
	}
	return res;
}

static config unit_weapons(unit *u)
{
	if (!u) return report();
	map_location displayed_unit_hex = resources::screen->displayed_unit_hex();
	config res;

	BOOST_FOREACH(const attack_type &at, u->attacks())
	{
		attack_info(at, res, u, displayed_unit_hex);
	}
	return res;
}
REPORT_GENERATOR(unit_weapons)
{
	unit *u = get_visible_unit();
	if (!u) return config();

	return unit_weapons(u);
}
REPORT_GENERATOR(highlighted_unit_weapons)
{
	unit *u = get_selected_unit();
	unit *sec_u = get_visible_unit();

	if (!u) return config();
	if (!sec_u || u == sec_u) return unit_weapons(sec_u);

	map_location highlighted_hex = resources::screen->displayed_unit_hex();
	map_location attack_loc =
			resources::controller->get_mouse_handler_base().current_unit_attacks_from(highlighted_hex);

	if (!attack_loc.valid())
		return unit_weapons(sec_u);

	return unit_weapons(u, attack_loc, sec_u, false);
}
REPORT_GENERATOR(selected_unit_weapons)
{
	unit *u = get_selected_unit();
	unit *sec_u = get_visible_unit();

	if (!u) return config();
	if (!sec_u || u == sec_u) return unit_weapons(u);

	map_location highlighted_hex = resources::screen->displayed_unit_hex();
	map_location attack_loc =
			resources::controller->get_mouse_handler_base().current_unit_attacks_from(highlighted_hex);

	if (!attack_loc.valid())
		return unit_weapons(u);

	return unit_weapons(u, attack_loc, sec_u, true);
}

REPORT_GENERATOR(unit_image)
{
	unit *u = get_visible_unit();
	if (!u) return report();
	return image_report(u->absolute_image() + u->image_mods());
}
REPORT_GENERATOR(selected_unit_image)
{
	unit *u = get_selected_unit();
	if (!u) return report();
	return image_report(u->absolute_image() + u->image_mods());
}

REPORT_GENERATOR(selected_unit_profile)
{
	unit *u = get_selected_unit();
	if (!u) return report();
	return image_report(u->small_profile());
}
REPORT_GENERATOR(unit_profile)
{
	unit *u = get_visible_unit();
	if (!u) return report();
	return image_report(u->small_profile());
}

static config time_of_day_at(map_location& mouseover_hex)
{
	std::ostringstream tooltip;
	time_of_day tod;
	const team &viewing_team = (*resources::teams)[resources::screen->viewing_team()];
	if (viewing_team.shrouded(mouseover_hex)) {
		// Don't show time on shrouded tiles.
		tod = resources::tod_manager->get_time_of_day();
	} else if (viewing_team.fogged(mouseover_hex)) {
		// Don't show illuminated time on fogged tiles.
		tod = resources::tod_manager->get_time_of_day(mouseover_hex);
	} else {
		tod = resources::tod_manager->get_illuminated_time_of_day(mouseover_hex);
	}

	int b = tod.lawful_bonus;

	tooltip << tod.name << '\n'
		<< _("Lawful units: ") << utils::signed_percent(b) << '\n'
		<< _("Neutral units: ") << utils::signed_percent(0) << '\n'
		<< _("Chaotic units: ") << utils::signed_percent(-b) << '\n'
		<< _("Liminal units: ") << utils::signed_percent(-(abs(b)));

	std::string tod_image = tod.image;
	if (tod.bonus_modified > 0) tod_image += "~BRIGHTEN()";
	else if (tod.bonus_modified < 0) tod_image += "~DARKEN()";
	if (preferences::flip_time()) tod_image += "~FL(horiz)";

	return image_report(tod_image, tooltip.str(), "time_of_day");
}
REPORT_GENERATOR(time_of_day)
{
	map_location mouseover_hex = resources::screen->mouseover_hex();
	return time_of_day_at(mouseover_hex);
}
REPORT_GENERATOR(selected_time_of_day)
{
	map_location selected_hex = resources::screen->selected_hex();
	return time_of_day_at(selected_hex);
}

REPORT_GENERATOR(turn)
{
	std::ostringstream str;
	str << resources::tod_manager->turn();
	int nb = resources::tod_manager->number_of_turns();
	if (nb != -1) str << '/' << nb;
	return text_report(str.str());
}

REPORT_GENERATOR(gold)
{
	std::ostringstream str;
	int viewing_side = resources::screen->viewing_side();
	// Suppose the full unit map is applied.
	int fake_gold = (*resources::teams)[viewing_side - 1].gold() -
		resources::whiteboard->get_spent_gold_for(viewing_side);
	char const *end = naps;
	if (viewing_side != resources::screen->playing_side()) {
		str << span_color(font::GRAY_COLOR);
	}
	else if (fake_gold < 0) {
		str << span_color(font::BAD_COLOR);
	}
	else {
		end = "";
	}
	str << utils::half_signed_value(fake_gold) << end;
	return text_report(str.str());
}

REPORT_GENERATOR(villages)
{
	std::ostringstream str;
	int viewing_side = resources::screen->viewing_side();
	const team &viewing_team = (*resources::teams)[viewing_side - 1];
	team_data td = calculate_team_data(viewing_team, viewing_side);
	str << td.villages << '/';
	if (viewing_team.uses_shroud()) {
		int unshrouded_villages = 0;
		BOOST_FOREACH(const map_location &loc, resources::game_map->villages()) {
			if (!viewing_team.shrouded(loc))
				++unshrouded_villages;
		}
		str << unshrouded_villages;
	} else {
		str << resources::game_map->villages().size();
	}
	return gray_inactive(str.str());
}

REPORT_GENERATOR(num_units)
{
	return gray_inactive(str_cast(side_units(resources::screen->viewing_side())));
}

REPORT_GENERATOR(upkeep)
{
	std::ostringstream str;
	int viewing_side = resources::screen->viewing_side();
	const team &viewing_team = (*resources::teams)[viewing_side - 1];
	team_data td = calculate_team_data(viewing_team, viewing_side);
	str << td.expenses << " (" << td.upkeep << ")";
	return gray_inactive(str.str());
}

REPORT_GENERATOR(expenses)
{
	int viewing_side = resources::screen->viewing_side();
	const team &viewing_team = (*resources::teams)[viewing_side - 1];
	team_data td = calculate_team_data(viewing_team, resources::screen->viewing_side());
	return gray_inactive(str_cast(td.expenses));
}

REPORT_GENERATOR(income)
{
	std::ostringstream str;
	int viewing_side = resources::screen->viewing_side();
	const team &viewing_team = (*resources::teams)[viewing_side - 1];
	team_data td = calculate_team_data(viewing_team, viewing_side);
	char const *end = naps;
	if (viewing_side != resources::screen->playing_side()) {
		if (td.net_income < 0) {
			td.net_income = - td.net_income;
			str << span_color(font::GRAY_COLOR);
			str << utils::unicode_minus;
		}
		else {
			str << span_color(font::GRAY_COLOR);
		}
	}
	else if (td.net_income < 0) {
		td.net_income = - td.net_income;
		str << span_color(font::BAD_COLOR);
		str << utils::unicode_minus;
	}
	else {
		end = "";
	}
	str << td.net_income << end;
	return text_report(str.str());
}

REPORT_GENERATOR(terrain)
{
	gamemap &map = *resources::game_map;
	int viewing_side = resources::screen->viewing_side();
	const team &viewing_team = (*resources::teams)[viewing_side - 1];
	map_location mouseover_hex = resources::screen->mouseover_hex();
	if (!map.on_board(mouseover_hex) || viewing_team.shrouded(mouseover_hex))
		return report();

	t_translation::t_terrain terrain = map.get_terrain(mouseover_hex);
	if (terrain == t_translation::OFF_MAP_USER)
		return report();

	std::ostringstream str;
	if (map.is_village(mouseover_hex))
	{
		int owner = village_owner(mouseover_hex, *resources::teams) + 1;
		if (owner == 0 || viewing_team.fogged(mouseover_hex)) {
			str << map.get_terrain_info(terrain).income_description();
		} else if (owner == viewing_side) {
			str << map.get_terrain_info(terrain).income_description_own();
		} else if (viewing_team.is_enemy(owner)) {
			str << map.get_terrain_info(terrain).income_description_enemy();
		} else {
			str << map.get_terrain_info(terrain).income_description_ally();
		}

		const std::string& underlying_desc = map.get_underlying_terrain_string(terrain);
		if(!underlying_desc.empty()) {
			str << underlying_desc;
		}
	} else {
		str << map.get_terrain_string(terrain);
	}

	return text_report(str.str());
}

REPORT_GENERATOR(position)
{
	gamemap &map = *resources::game_map;
	map_location mouseover_hex = resources::screen->mouseover_hex(),
		displayed_unit_hex = resources::screen->displayed_unit_hex();
	if (!map.on_board(mouseover_hex))
		return report();
	t_translation::t_terrain terrain = map[mouseover_hex];
	if (terrain == t_translation::OFF_MAP_USER)
		return report();

	std::ostringstream str;
	str << mouseover_hex;

	const unit *u = get_visible_unit();
	const team &viewing_team = (*resources::teams)[resources::screen->viewing_team()];
	if (!u ||
	    (displayed_unit_hex != mouseover_hex &&
	     displayed_unit_hex != resources::screen->selected_hex()) ||
	    viewing_team.shrouded(mouseover_hex))
		return text_report(str.str());

	int move_cost = u->movement_cost(terrain);
	int defense = 100 - u->defense_modifier(terrain);
	if (move_cost < unit_movement_type::UNREACHABLE) {
		str << " (" << defense << "%," << move_cost << ')';
	} else if (mouseover_hex == displayed_unit_hex) {
		str << " (" << defense << "%,‒)";
	} else {
		str << " (‒)";
	}
	return text_report(str.str());
}

REPORT_GENERATOR(side_playing)
{
	const team &active_team = (*resources::teams)[resources::screen->playing_team()];
	std::string flag_icon = active_team.flag_icon();
	std::string old_rgb = game_config::flag_rgb;
	std::string new_rgb = team::get_side_color_index(resources::screen->playing_side());
	std::string mods = "~RC(" + old_rgb + ">" + new_rgb + ")";
	if (flag_icon.empty())
		flag_icon = game_config::images::flag_icon;
	return image_report(flag_icon + mods, active_team.current_player());
}

REPORT_GENERATOR(observers)
{
	const std::set<std::string> &observers = resources::screen->observers();
	if (observers.empty())
		return report();

	std::ostringstream str;
	str << _("Observers:") << '\n';
	BOOST_FOREACH(const std::string &obs, observers) {
		str << obs << '\n';
	}
	return image_report(game_config::images::observer, str.str());
}

REPORT_GENERATOR(selected_terrain)
{
	const std::string selected_terrain = editor::get_selected_terrain();
	if (selected_terrain.empty())
		return report();
	else
		return text_report(selected_terrain);
}

REPORT_GENERATOR(edit_left_button_function)
{
	const std::string left_button_function = editor::get_left_button_function();
	if (left_button_function.empty())
		return report();
	else
		return text_report(left_button_function);
}

REPORT_GENERATOR(editor_tool_hint)
{
	return report();
}

REPORT_GENERATOR(report_clock)
{
	time_t t = std::time(NULL);
	struct tm *lt = std::localtime(&t);
	if (!lt) return report();
	char temp[15];
	size_t s = std::strftime(temp, 15,
		(preferences::use_twelve_hour_clock_format() ? _("%I:%M %p") : _("%H:%M")),
		lt);
	return s ? text_report(temp) : report();

}

REPORT_GENERATOR(report_countdown)
{
	int viewing_side = resources::screen->viewing_side();
	const team &viewing_team = (*resources::teams)[viewing_side - 1];
	int min, sec;
	if (viewing_team.countdown_time() == 0)
		return report_report_clock();
	std::ostringstream str;
	sec = viewing_team.countdown_time() / 1000;
	char const *end = naps;
	if (viewing_side != resources::screen->playing_side())
		str << span_color(font::GRAY_COLOR);
	else if (sec < 60)
		str << "<span foreground=\"#c80000\">";
	else if (sec < 120)
		str << "<span foreground=\"#c8c800\">";
	else
		end = "";
	min = sec / 60;
	str << min << ':';
	sec = sec % 60;
	if (sec < 10) str << '0';
	str << sec << end;
	return text_report(str.str());
}

static std::set<std::string> all_reports;

void reports::reset_generators()
{
	BOOST_FOREACH(dynamic_report_generators::value_type &rg, dynamic_generators) {
		delete rg.second;
	}
	dynamic_generators.clear();
	all_reports.clear();
}

void reports::register_generator(const std::string &name, reports::generator *g)
{
	std::pair<dynamic_report_generators::iterator, bool> ib =
		dynamic_generators.insert(std::make_pair(name, g));
	if (!ib.second) {
		delete ib.first->second;
		ib.first->second = g;
	}
}

config reports::generate_report(const std::string &name, bool only_static)
{
	if (!only_static) {
		dynamic_report_generators::const_iterator i = dynamic_generators.find(name);
		if (i != dynamic_generators.end())
			return i->second->generate();
	}
	static_report_generators::const_iterator j = static_generators.find(name);
	if (j != static_generators.end())
		return j->second();
	return report();
}

const std::set<std::string> &reports::report_list()
{
	if (!all_reports.empty()) return all_reports;
	BOOST_FOREACH(const static_report_generators::value_type &v, static_generators) {
		all_reports.insert(v.first);
	}
	BOOST_FOREACH(const dynamic_report_generators::value_type &v, dynamic_generators) {
		all_reports.insert(v.first);
	}
	return all_reports;
}
