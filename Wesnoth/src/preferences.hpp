/* $Id: preferences.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#ifndef PREFERENCES_HPP_INCLUDED
#define PREFERENCES_HPP_INCLUDED

class config;
class display;

#include "game_config.hpp"
#include "terrain_translation.hpp"

#include <utility>

namespace preferences {

    enum INTERACTION_METHOD { DWELL, BLINK, SWITCH };

	struct base_manager
	{
		base_manager();
		~base_manager();
	};

	void write_preferences();

	void set(const std::string& key, const std::string &value);
	void set(const std::string& key, char const *value);
	void set(const std::string &key, bool value);
	void set(const std::string &key, int value);
	void clear(const std::string& key);
	void set_child(const std::string& key, const config& val);
	const config &get_child(const std::string &key);
	std::string get(const std::string& key);
	bool get(const std::string &key, bool def);
	void erase(const std::string& key);
	bool have_setting(const std::string& key);

	void disable_preferences_save();

	config* get_prefs();

	bool fullscreen();
	void _set_fullscreen(bool ison);

	bool scroll_to_action();
	void _set_scroll_to_action(bool ison);

	int min_allowed_width();
	int min_allowed_height();

	std::pair<int,int> resolution();
	std::pair<int,int>* getResolutionPointer(); //BOBBY | Christoffer | Prototype to new function
	void _set_resolution(const std::pair<int,int>& res);

	bool turbo();
	void _set_turbo(bool ison);

	double turbo_speed();
	void save_turbo_speed(const double speed);

	bool idle_anim();
	void _set_idle_anim(const bool ison);

	int idle_anim_rate();
	void _set_idle_anim_rate(const int rate);

	std::string language();
	void set_language(const std::string& s);

	// Don't rename it to sound() because of a gcc-3.3 branch bug,
	// which will cause it to conflict with the sound namespace.
	bool sound_on();
	bool set_sound(bool ison);

	unsigned int sample_rate();
	void save_sample_rate(const unsigned int rate);

	size_t sound_buffer_size();
	void save_sound_buffer_size(const size_t size);

	int sound_volume();
	void set_sound_volume(int vol);

	int bell_volume();
	void set_bell_volume(int vol);

	int UI_volume();
	void set_UI_volume(int vol);

	bool music_on();
	bool set_music(bool ison);

    enum INTERACTION_METHOD interaction_method();
    void set_interaction_method(enum INTERACTION_METHOD val);

    int blink_length();
    void set_blink_length(int val);

    int gaze_length(); // Bj�rn all up in here
    void set_gaze_length(int length);

	int music_volume();
	void set_music_volume(int vol);

	bool turn_bell();
	bool set_turn_bell(bool ison);

	bool UI_sound_on();
	bool set_UI_sound(bool ison);

	bool message_bell();

	// Proxies for preferences_dialog
	void load_hotkeys();
	void save_hotkeys();

	void add_alias(const std::string& alias, const std::string& command);
	const config &get_alias();

	bool use_color_cursors();
	void _set_color_cursors(bool value);

	bool joystick_support_enabled();
	int joystick_mouse_deadzone();
	int joystick_num_mouse_xaxis();
	int joystick_num_mouse_yaxis();
	int joystick_mouse_xaxis_num();
	int joystick_mouse_yaxis_num();

	int joystick_scroll_deadzone();
	int joystick_num_scroll_xaxis();
	int joystick_num_scroll_yaxis();
	int joystick_scroll_xaxis_num();
	int joystick_scroll_yaxis_num();

	int joystick_cursor_deadzone();
	int joystick_num_cursor_xaxis();
	int joystick_num_cursor_yaxis();
	int joystick_cursor_xaxis_num();
	int joystick_cursor_yaxis_num();
	int joystick_cursor_threshold();

	int joystick_thrusta_deadzone();
	int joystick_num_thrusta_axis();
	int joystick_thrusta_axis_num();

	int joystick_thrustb_deadzone();
	int joystick_num_thrustb_axis();
	int joystick_thrustb_axis_num();

	int scroll_speed();
	void set_scroll_speed(const int scroll);

	bool middle_click_scrolls();
	bool mouse_scroll_enabled();
	void enable_mouse_scroll(bool value);

	/**
	 * Gets the threshold for when to scroll.
	 *
	 * This scrolling happens when the mouse is in the application and near
	 * the border.
	 */
	int mouse_scroll_threshold();

	int draw_delay();
	void set_draw_delay(int value);

	bool animate_map();
	void set_animate_map(bool value);

	bool show_standing_animations();
	void set_show_standing_animations(bool value);

	bool show_fps();
	void set_show_fps(bool value);

	bool ellipses();
	void _set_ellipses(bool ison);

	bool grid();
	void _set_grid(bool ison);

	bool confirm_load_save_from_different_version();

	bool use_twelve_hour_clock_format();
	void set_use_twelve_hour_clock_format(bool value);

} // end namespace preferences

#endif
