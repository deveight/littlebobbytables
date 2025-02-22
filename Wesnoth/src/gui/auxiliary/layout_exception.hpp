/* $Id: layout_exception.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

/**
 * @file
 * Defines the exception classes for the layout algorithm.
 */

#ifndef GUI_AUXILIRY_LAYOUT_EXCEPTION_HPP_INCLUDED
#define GUI_AUXILIRY_LAYOUT_EXCEPTION_HPP_INCLUDED

namespace gui2 {

/**
 * Exception thrown when the width has been modified during resizing.
 *
 * @see layout_algorihm for more information.
 */
struct tlayout_exception_width_modified {};

/** Basic exception when the layout doesn't fit. */
struct tlayout_exception_resize_failed {};

/**
 * Exception thrown when the width resizing has failed.
 *
 * @see layout_algorihm for more information.
 */
struct tlayout_exception_width_resize_failed
	: public tlayout_exception_resize_failed
{
};

/**
 * Exception thrown when the height resizing has failed.
 *
 * @see layout_algorihm for more information.
 */
struct tlayout_exception_height_resize_failed
	: public tlayout_exception_resize_failed
{
};

} // namespace gui2

#endif
