/* $Id: campaign_selection.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#ifndef GUI_DIALOGS_CAMPAIGN_SELECTION_HPP_INCLUDED
#define GUI_DIALOGS_CAMPAIGN_SELECTION_HPP_INCLUDED

#include "gui/dialogs/dialog.hpp"

#include "config.hpp"

namespace gui2 {

class tcampaign_selection
	: public tdialog
{
public:
	explicit tcampaign_selection(const std::vector<config>& campaigns)
		: campaigns_(campaigns)
		, choice_(-1)

	{
	}

	/***** ***** ***** setters / getters for members ***** ****** *****/

	int get_choice() const { return choice_; }

private:

	/** Called when another campaign is selected. */
	void campaign_selected(twindow& window);

	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	/** Inherited from tdialog. */
	void pre_show(CVideo& video, twindow& window);

	/** Inherited from tdialog. */
	void post_show(twindow& window);

	/** Contains the config objects for all campaigns. */
	const std::vector<config> &campaigns_;

	/** The chosen campaign. */
	int choice_;
};

} // namespace gui2

#endif
