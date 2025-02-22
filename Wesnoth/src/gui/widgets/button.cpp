/* $Id: button.cpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2008 - 2012 by Mark de Wever <koraq@xs4all.nl>
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

#include "game_preferences.hpp"

#include "gui/widgets/button.hpp"

#include "gui/auxiliary/log.hpp"
#include "gui/auxiliary/widget_definition/button.hpp"
#include "gui/auxiliary/window_builder/button.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "sound.hpp"
#include "eyetracker/interaction_controller.hpp"

#include <boost/bind.hpp>

#define LOG_SCOPE_HEADER get_control_type() + " [" + id() + "] " + __func__
#define LOG_HEADER LOG_SCOPE_HEADER + ':'

namespace gui2 {

REGISTER_WIDGET(button)

tbutton::tbutton()
	: tcontrol(COUNT)
	, tclickable_()
	, state_(ENABLED)
	, retval_(0)
{
	connect_signal<event::MOUSE_ENTER>(boost::bind(
				&tbutton::signal_handler_mouse_enter, this, _2, _3));
	connect_signal<event::MOUSE_LEAVE>(boost::bind(
				&tbutton::signal_handler_mouse_leave, this, _2, _3));

	connect_signal<event::LEFT_BUTTON_DOWN>(boost::bind(
				&tbutton::signal_handler_left_button_down, this, _2, _3));
	connect_signal<event::LEFT_BUTTON_UP>(boost::bind(
				&tbutton::signal_handler_left_button_up, this, _2, _3));
	connect_signal<event::LEFT_BUTTON_CLICK>(boost::bind(
				&tbutton::signal_handler_left_button_click, this, _2, _3));
}

void tbutton::set_state(const tstate state)
{
	if(state != state_) {
		state_ = state;
		set_dirty(true);
	}
}

SDL_Rect tbutton::indicator_rect() {
    int h = this->get_height();
    int w = h;
    int x = this->get_x() + this->get_width() / 2 - w / 2;
    int y = this->get_y();
    return {x,y,w,h};
}

const std::string& tbutton::get_control_type() const
{
	static const std::string type = "button";
	return type;
}

void tbutton::signal_handler_mouse_enter(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	set_state(FOCUSSED);
	handled = true;
    eyetracker::interaction_controller::mouse_enter(this);
}

void tbutton::signal_handler_mouse_leave(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	set_state(ENABLED);
	handled = true;
    eyetracker::interaction_controller::mouse_leave(this);
}

void tbutton::signal_handler_left_button_down(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	twindow* window = get_window();
	if(window) {
		window->mouse_capture();
	}

	set_state(PRESSED);
	handled = true;
}

void tbutton::signal_handler_left_button_up(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	set_state(FOCUSSED);
	handled = true;
}

void tbutton::signal_handler_left_button_click(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	sound::play_UI_sound(settings::sound_button_click);

	// If a button has a retval do the default handling.
	if(retval_ != 0) {
		twindow* window = get_window();
		if(window) {
			window->set_retval(retval_);
			return;
		}
	}

	handled = true;
}

} // namespace gui2
