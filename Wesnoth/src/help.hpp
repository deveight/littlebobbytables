/* $Id: help.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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
#ifndef HELP_HPP_INCLUDED
#define HELP_HPP_INCLUDED

class config;
class display;
class gamemap;

#include "hotkeys.hpp"
#include "construct_dialog.hpp"

namespace help {

struct help_manager {
	help_manager(const config *game_config, gamemap *map);
	~help_manager();
};

struct section;
/// Open a help dialog using a toplevel other than the default. This
/// allows for complete customization of the contents, although not in a
/// very easy way.
void show_help(display &disp, const section &toplevel, const std::string& show_topic="",
			   int xloc=-1, int yloc=-1);

/// Open the help browser. The help browser will have the topic with id
/// show_topic open if it is not the empty string. The default topic
/// will be shown if show_topic is the empty string.
void show_help(display &disp, const std::string& show_topic="", int xloc=-1, int yloc=-1);

/// wrapper to add unit prefix and hidding symbol
void show_unit_help(display &disp, const std::string& unit_id, bool hidden = false,
				int xloc=-1, int yloc=-1);

class help_button : public gui::dialog_button, public hotkey::command_executor {
public:
	help_button(display& disp, const std::string &help_topic);
	~help_button();
	int action(gui::dialog_process_info &info);
	std::string topic() const { return topic_; }
	void join();
	void leave();
private:
	void show_help();
	bool can_execute_command(hotkey::HOTKEY_COMMAND cmd, int/*index*/ =-1) const;

	display &disp_;
	const std::string topic_;
	hotkey::basic_handler *help_hand_;
};


} // End namespace help.

#endif
