/* $Id: repeating_button.cpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2009 - 2012 by Mark de Wever <koraq@xs4all.nl>
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

#include "gui/widgets/repeating_button.hpp"

#include "gui/auxiliary/log.hpp"
#include "gui/auxiliary/timer.hpp"
#include "gui/auxiliary/widget_definition/repeating_button.hpp"
#include "gui/auxiliary/window_builder/repeating_button.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "sound.hpp"

#include "eyetracker/interaction_controller.hpp"

#include <boost/bind.hpp>

#define LOG_SCOPE_HEADER get_control_type() + " [" + id() + "] " + __func__
#define LOG_HEADER LOG_SCOPE_HEADER + ':'

namespace gui2 {

REGISTER_WIDGET(repeating_button)

trepeating_button::trepeating_button()
	: tcontrol(COUNT)
	, tclickable_()
	, state_(ENABLED)
	, repeat_timer_(0)
{
	connect_signal<event::MOUSE_ENTER>(boost::bind(
				&trepeating_button::signal_handler_mouse_enter, this, _2, _3));
	connect_signal<event::MOUSE_LEAVE>(boost::bind(
				&trepeating_button::signal_handler_mouse_leave, this, _2, _3));

	connect_signal<event::LEFT_BUTTON_DOWN>(boost::bind(
				  &trepeating_button::signal_handler_left_button_down
				, this
				, _2
				, _3));
	connect_signal<event::LEFT_BUTTON_UP>(boost::bind(
				  &trepeating_button::signal_handler_left_button_up
				, this
				, _2
				, _3));
}

trepeating_button::~trepeating_button()
{
	if(repeat_timer_) {
		remove_timer(repeat_timer_);
	}
}

void trepeating_button::connect_signal_mouse_left_down(
		const event::tsignal_function& signal)
{
	connect_signal<event::LEFT_BUTTON_DOWN>(signal);
}

void trepeating_button::disconnect_signal_mouse_left_down(
		const event::tsignal_function& signal)
{
	disconnect_signal<event::LEFT_BUTTON_DOWN>(signal);
}

void trepeating_button::set_state(const tstate state)
{
	if(state != state_) {
		state_ = state;
		set_dirty(true);

		if(state_ == DISABLED && repeat_timer_) {
			remove_timer(repeat_timer_);
			repeat_timer_ = 0;
		}

		if(state_ == DISABLED) {
            eyetracker::interaction_controller::mouse_leave(this);
		}
	}
}

const std::string& trepeating_button::get_control_type() const
{
	static const std::string type = "repeating_button";
	return type;
}

SDL_Rect trepeating_button::indicator_rect(){
    if(state_ == DISABLED) { // If the button isn't clickable we shouldn't display an indicator
        return {0,0,0,0};
    }
    int h = this->get_height();
    int w = h;
    int x = this->get_x() + this->get_width() / 2 - w / 2;
    int y = this->get_y();
    return {x,y,w,h};
}

void trepeating_button::signal_handler_mouse_enter(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	set_state(FOCUSSED);
	handled = true;

    eyetracker::interaction_controller::mouse_enter(this, eyetracker::interaction_controller::REPEATING_CLICK);
}

void trepeating_button::signal_handler_mouse_leave(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	set_state(ENABLED);
	handled = true;
    eyetracker::interaction_controller::mouse_leave(this);
}

void trepeating_button::signal_handler_left_button_down(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	// If the timer isn't set it's the initial down event.
	if(!repeat_timer_) {

		// mimic the old gui and only play the sound once.
		sound::play_UI_sound(settings::sound_button_click);

		twindow* window = get_window();
		if(window) {
			repeat_timer_ = add_timer(
					  settings::repeat_button_repeat_time
					, boost::bind(
						  &tdispatcher::fire
						, window
						, event::LEFT_BUTTON_DOWN
						, boost::ref(*this))
					, true);

			window->mouse_capture();
		}

		set_state(PRESSED);
	}

	handled = true;
}

void trepeating_button::signal_handler_left_button_up(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	if(repeat_timer_) {
		remove_timer(repeat_timer_);
		repeat_timer_ = 0;
	}

	if(get_active()) {
		set_state(FOCUSSED);
	}
	handled = true;
}

} // namespace gui2
