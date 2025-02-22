/* $Id: mouse_handler_base.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2006 - 2012 by Joerg Hinrichs <joerg.hinrichs@alice-dsl.de>
   wesnoth playturn Copyright (C) 2003 by David White <dave@whitevine.net>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef MOUSE_HANDLER_BASE_H_INCLUDED
#define MOUSE_HANDLER_BASE_H_INCLUDED

#include "map_location.hpp"
#include "SDL.h"
#include "gui/widgets/widget.hpp"
#include "eyetracker/interaction_controller.hpp"

class display;

namespace events {

struct command_disabler
{
	command_disabler();
	~command_disabler();
};

extern int commands_disabled;

class mouse_handler_base {
public:
	mouse_handler_base();
	virtual ~mouse_handler_base() {}

	/**
	 * Reference to the used display objects. Derived classes should ensure
	 * this is always valid. Note the constructor of this class cannot use this.
	 */
	virtual display& gui() = 0;

	/**
	 * Const version.
	 */
	virtual const display& gui() const = 0;

	/**
	 * @return true when the class in the "dragging" state.
	 */
	bool is_dragging() const;

	//minimum dragging distance to fire the drag&drop
	virtual int drag_threshold() const {return 0;};

	void mouse_motion_event(const SDL_MouseMotionEvent& event, const bool browse);

	/** update the mouse with a fake mouse motion */
	void mouse_update(const bool browse, map_location loc);

	bool get_show_menu() const { return show_menu_; }

	/**
	 * This handles minimap scrolling and click-drag.
	 * @returns true when the caller should not process the mouse motion
	 * further (i.e. should return), false otherwise.
	 */
	bool mouse_motion_default(int x, int y, bool update);

	/**
	 * Called when a mouse motion event takes place. Derived classes mustprovide an
	 * implementation, possibly using mouse_motion_default().
	 */
	virtual void mouse_motion(int x, int y, const bool browse, bool update=false, map_location new_loc = map_location::null_location) = 0;

	virtual void mouse_press(const SDL_MouseButtonEvent& event, const bool browse);
	bool is_left_click(const SDL_MouseButtonEvent& event) const;
	bool is_middle_click(const SDL_MouseButtonEvent& event) const;
	bool is_right_click(const SDL_MouseButtonEvent& event) const;

	/**
	 * Derived classes can overrid this to disable mousewheel scrolling under
	 * some circumstances, e.g. when the mouse wheel controls something else,
	 * but the event is also received by this class
	 */
	virtual bool allow_mouse_wheel_scroll(int x, int y);

	/**
	 * Overridden in derived classes, called on a left click (mousedown).
	 * Defaults to process (initiate) minimap scrolling.
	 * @returns true when the click should not process the event further.
	 * This means do not treat the call as a start of drag movement.
	 * FIXME: This return value is currently ignored
	 */
	virtual bool left_click(int x, int y, const bool browse);

	/**
	 * Called whenever the left mouse drag has "ended".
	 */
	virtual void left_drag_end(int x, int y, const bool browse);

	/**
	 * Called when the left mouse button is up
	 */
	virtual void left_mouse_up(int x, int y, const bool browse);

	/**
	 * Overridden in derived classes, called on a right click (mousedown).
	 * Defaults to displaying the menu (by setting the appropriate flag)
	 * if right_click_show_menu returns true.
	 * @returns true when the click should not process the event further.
	 * This means do not treat the call as a start of drag movement.
	 */
	virtual bool right_click(int x, int y, const bool browse);

	/**
	 * Called in the default right_click when the context menu is about to
	 * be shown, can be used for preprocessing and preventing the menu from
	 * being displayed without rewriting the right click function.
	 * @returns true when the menu should be displayed and false otherwise
	 * FIXME: This return value is currently ignored
	 */
	virtual bool right_click_show_menu(int x, int y, const bool browse);

	/**
	 * Called whenever the right mouse drag has "ended".
	 */
	virtual void right_drag_end(int x, int y, const bool browse);

	/**
	 * Called when the right mouse button is up
	 */
	virtual void right_mouse_up(int x, int y, const bool browse);

protected:
	void cancel_dragging();
	void clear_dragging(const SDL_MouseButtonEvent& event, bool browse);
	void init_dragging(bool& dragging_flag);

	/** MMB click (on game map) state flag */
	bool simple_warp_;
	/** minimap scrolling (scroll-drag) state flag */
	bool minimap_scrolling_;
	/** Bobby : used to emulate mouse_enter & mouse_leave*/
	bool inside_minimap_;
	/** LMB drag init flag */
	bool dragging_left_;
	/** Actual drag flag */
	bool dragging_started_;
	/** RMB drag init flag */
	bool dragging_right_;
	/** Drag start position x */
	int drag_from_x_;
	/** Drag start position y */
	int drag_from_y_;
	/** Drag start map location */
	map_location drag_from_hex_;

	/** last highlighted hex */
	map_location last_hex_;

	/** Show context menu flag */
	bool show_menu_;
};

} // end namespace events

#endif
