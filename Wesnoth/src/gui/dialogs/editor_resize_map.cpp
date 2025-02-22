/* $Id: editor_resize_map.cpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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
#define GETTEXT_DOMAIN "wesnoth-editor"

#include "gui/dialogs/editor_resize_map.hpp"

#include "gui/dialogs/field.hpp"
#include "gui/dialogs/helper.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/slider.hpp"

#include <boost/bind.hpp>

namespace gui2 {

/*WIKI
 * @page = GUIWindowDefinitionWML
 * @order = 2_editor_resize_map
 *
 * == Editor resize map ==
 *
 * This shows the dialog to resize the current map.
 *
 * @begin{table}{dialog_widgets}
 *
 * old_width & & label & o &
 *         Shows the old width of the map. $
 *
 * old_height & & label & o &
 *         Shows the old height of the map. $
 *
 * width & & slider & m &
 *         Determines the new width of the map. $
 *
 * height & & slider & m &
 *         Determines the new height of the map. $
 *
 * copy_edge_terrain & & boolean_selector & m &
 *         Determines whether the border terrains should be used to expand or
 *         not. $
 *
 * expand0 & & toggle_button & m &
 *         Determines in which direction to expand, shows the north east
 *         marker. $
 *
 * expand1 & & toggle_button & m &
 *         Determines in which direction to expand, shows the north marker. $
 *
 * expand2 & & toggle_button & m &
 *         Determines in which direction to expand, shows the north west
 *         marker. $
 *
 * expand3 & & toggle_button & m &
 *         Determines in which direction to expand, shows the east marker. $
 *
 * expand4 & & toggle_button & m &
 *         Determines in which direction to expand, shows the centre marker. $
 *
 * expand5 & & toggle_button & m &
 *         Determines in which direction to expand, shows the west marker. $
 *
 * expand6 & & toggle_button & m &
 *         Determines in which direction to expand, shows the south east
 *         marker. $
 *
 * expand7 & & toggle_button & m &
 *         Determines in which direction to expand, shows the south marker. $
 *
 * expand8 & & toggle_button & m &
 *         Determines in which direction to expand, shows the south west
 *         marker. $
 *
 * @end{table}
 */

REGISTER_DIALOG(editor_resize_map)

teditor_resize_map::teditor_resize_map(
			  int& width
			, int& height
			, EXPAND_DIRECTION& expand_direction
			, bool& copy_edge_terrain)
	: width_(register_integer("width", true, width))
	, height_(register_integer("height", true, height))
	, old_width_(width)
	, old_height_(height)
	, expand_direction_(expand_direction)
{
	register_bool("copy_edge_terrain", false, copy_edge_terrain);

	register_label("old_width", false, str_cast(width));
	register_label("old_height", false, str_cast(height));
}

void teditor_resize_map::pre_show(CVideo& /*video*/, twindow& window)
{
	tslider& height = find_widget<tslider>(&window, "height", false);
	connect_signal_notify_modified(height
			, boost::bind(
				  &teditor_resize_map::update_expand_direction
				, this
				, boost::ref(window)));

	tslider& width = find_widget<tslider>(&window, "width", false);
	connect_signal_notify_modified(width
			, boost::bind(
				  &teditor_resize_map::update_expand_direction
				, this
				, boost::ref(window)));

	std::string name_prefix = "expand";
	for (int i = 0; i < 9; ++i) {
		std::string name = name_prefix + lexical_cast<std::string>(i);
		direction_buttons_[i] = find_widget<ttoggle_button>(
				&window, name, false, true);

		direction_buttons_[i]->set_callback_state_change(
				dialog_callback<teditor_resize_map
					, &teditor_resize_map::update_expand_direction>);
	}
	direction_buttons_[0]->set_value(true);
	update_expand_direction(window);
}

/**
 * Convert a coordinate on a 3  by 3 grid to an index, return 9 for out of
 * bounds
 */
static int resize_grid_xy_to_idx(const int x, const int y)
{
	if (x < 0 || x > 2 || y < 0 || y > 2) {
		return 9;
	} else {
		return y * 3 + x;
	}
}

void teditor_resize_map::set_direction_icon(int index, std::string icon)
{
	if (index < 9) {
		direction_buttons_[index]->set_icon_name(
				"buttons/resize-direction-" + icon + ".png");
	}
}

void teditor_resize_map::update_expand_direction(twindow& window)
{
	for (int i = 0; i < 9; ++i) {
		if (direction_buttons_[i]->get_value()
					&& static_cast<int>(expand_direction_) != i) {

			expand_direction_ = static_cast<EXPAND_DIRECTION>(i);
			break;
		}
	}
	for (int i = 0; i < static_cast<int>(expand_direction_); ++i) {
		direction_buttons_[i]->set_value(false);
		set_direction_icon(i, "none");
	}
	direction_buttons_[expand_direction_]->set_value(true);
	for (int i = expand_direction_ + 1; i < 9; ++i) {
		direction_buttons_[i]->set_value(false);
		set_direction_icon(i, "none");
	}

	int xdiff = width_->get_widget_value(window) - old_width_ ;
	int ydiff = height_->get_widget_value(window) - old_height_ ;
	int x = static_cast<int>(expand_direction_) % 3;
	int y = static_cast<int>(expand_direction_) / 3;
	set_direction_icon(expand_direction_, "center");
	if (xdiff != 0) {
		int left = resize_grid_xy_to_idx(x - 1, y);
		int right = resize_grid_xy_to_idx(x + 1, y);
		if (xdiff < 0) std::swap(left, right);
		set_direction_icon(left, "left");
		set_direction_icon(right, "right");
	}
	if (ydiff != 0) {
		int top = resize_grid_xy_to_idx(x, y - 1);
		int bottom = resize_grid_xy_to_idx(x, y + 1);
		if (ydiff < 0) std::swap(top, bottom);
		set_direction_icon(top, "top");
		set_direction_icon(bottom, "bottom");
	}
	if (xdiff < 0 || ydiff < 0 || (xdiff > 0 && ydiff > 0)) {
		int nw = resize_grid_xy_to_idx(x - 1, y - 1);
		int ne = resize_grid_xy_to_idx(x + 1, y - 1);
		int sw = resize_grid_xy_to_idx(x - 1, y + 1);
		int se = resize_grid_xy_to_idx(x + 1, y + 1);
		if (xdiff < 0 || ydiff < 0) {
			std::swap(nw, se);
			std::swap(ne, sw);
		}
		set_direction_icon(nw, "top-left");
		set_direction_icon(ne, "top-right");
		set_direction_icon(sw, "bottom-left");
		set_direction_icon(se, "bottom-right");
	}
}

} // namespace gui2
