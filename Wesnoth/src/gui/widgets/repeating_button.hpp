/* $Id: repeating_button.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#ifndef GUI_WIDGETS_REPEATING_BUTTON_HPP_INCLUDED
#define GUI_WIDGETS_REPEATING_BUTTON_HPP_INCLUDED

#include "gui/widgets/control.hpp"
#include "gui/widgets/clickable.hpp"

namespace gui2 {

class trepeating_button
	: public tcontrol
	, public tclickable_
{
public:
	trepeating_button();
	~trepeating_button();

	/**
	 * Connects a signal handler for a left mouse button down.
	 *
	 * This event is triggered when the button is pressed and, as long as the
	 * button stays down, every x ms afterwards.
	 *
	 * @param signal              The signal to connect.
	 */
	void connect_signal_mouse_left_down(const event::tsignal_function& signal);

	/**
	 * Disconnects a signal handler for a left mouse button down.
	 *
	 * @param signal              The signal to disconnect (should be the same
	 *                            as send to the connect call.
	 */
	void disconnect_signal_mouse_left_down(
			const event::tsignal_function& signal);

	/***** ***** ***** ***** Inherited ***** ***** ***** *****/

	/** Inherited from tcontrol. */
	void set_active(const bool active)
		{ if(get_active() != active) set_state(active ? ENABLED : DISABLED); };

	/** Inherited from tcontrol. */
	bool get_active() const { return state_ != DISABLED; }

	/** Inherited from tcontrol. */
	unsigned get_state() const { return state_; }

	/** Inherited from tclickable. */
	void connect_click_handler(const event::tsignal_function& signal)
	{
		connect_signal_mouse_left_down(signal);
	}

	/** Inherited from tclickable. */
	void disconnect_click_handler(const event::tsignal_function& signal)
	{
		disconnect_signal_mouse_left_down(signal);
	}

	SDL_Rect indicator_rect();

private:
	/**
	 * Possible states of the widget.
	 *
	 * Note the order of the states must be the same as defined in settings.hpp.
	 */
	enum tstate { ENABLED, DISABLED, PRESSED, FOCUSSED, COUNT };

	void set_state(const tstate state);
	/**
	 * Current state of the widget.
	 *
	 * The state of the widget determines what to render and how the widget
	 * reacts to certain 'events'.
	 */
	tstate state_;

	/** The timer for the repeating events. */
	unsigned long repeat_timer_;

	/** Inherited from tcontrol. */
	const std::string& get_control_type() const;

	/***** ***** ***** signal handlers ***** ****** *****/

	void signal_handler_mouse_enter(const event::tevent event, bool& handled);

	void signal_handler_mouse_leave(const event::tevent event, bool& handled);

	void signal_handler_left_button_down(
			const event::tevent event, bool& handled);

	void signal_handler_left_button_up(
			const event::tevent event, bool& handled);
};


} // namespace gui2

#endif

