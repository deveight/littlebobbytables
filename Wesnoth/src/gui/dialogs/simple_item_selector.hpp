/* $Id: simple_item_selector.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2010 - 2012 by Ignacio Riquelme Morelle <shadowm2006@gmail.com>
   Part of the Battle for Wesnoth Project http://www.wesnoth.org/

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_DIALOGS_SIMPLE_ITEM_SELECTOR_HPP_INCLUDED
#define GUI_DIALOGS_SIMPLE_ITEM_SELECTOR_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"

#include <vector>

namespace gui2 {

class tsimple_item_selector : public tdialog
{
public:
	typedef std::vector< std::string > list_type;
	typedef std::string                item_type;

	tsimple_item_selector(
		const std::string& title,
		const std::string& message,
		list_type const& items,
		bool title_uses_markup = false,
		bool message_uses_markup = false);

	/**
	 * Returns the selected item index after displaying.
	 * @return -1 or the originally set index if the dialog was cancelled.
	 */
	int selected_index() const         { return index_;  }

	/** Sets the initially selected item index (-1 by default). */
	void set_selected_index(int index) { index_ = index; }

	/** Sets the caption for the OK button. */
	void set_ok_label(const std::string& s) { ok_label_ = s; }
	const std::string& ok_label() const     { return ok_label_; }

	/** Sets the caption for the Cancel button. */
	void set_cancel_label(const std::string& s) { cancel_label_ = s; }
	const std::string& cancel_label() const     { return cancel_label_; }

	/** Sets whether the Cancel button should be hidden or not. */
	void set_single_button(bool value) { single_button_ = value; }
	bool single_button() const         { return single_button_; }

private:
	int index_;

	bool single_button_;
	list_type items_;

	std::string ok_label_, cancel_label_;

	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	/** Inherited from tdialog. */
	void pre_show(CVideo& video, twindow& window);

	/** Inherited from tdialog. */
	void post_show(twindow& window);
};

}

#endif /* ! GUI_DIALOGS_SIMPLE_ITEM_SELECTOR_HPP_INCLUDED */
