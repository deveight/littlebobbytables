/* $Id: text.cpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#include "gui/widgets/text.hpp"

#include "clipboard.hpp"
#include "gui/auxiliary/log.hpp"
#include "serialization/string_utils.hpp"

#include <boost/bind.hpp>

#define LOG_SCOPE_HEADER get_control_type() + " [" + id() + "] " + __func__
#define LOG_HEADER LOG_SCOPE_HEADER + ':'

namespace gui2 {

ttext_::ttext_()
	: tcontrol(COUNT)
	, state_(ENABLED)
	, text_()
	, selection_start_(0)
	, selection_length_(0)
	, text_changed_callback_()
{
#ifdef __unix__
		// pastes on UNIX systems.
	connect_signal<event::MIDDLE_BUTTON_CLICK>(boost::bind(
			&ttext_::signal_handler_middle_button_click, this, _2, _3));

#endif

	connect_signal<event::SDL_KEY_DOWN>(boost::bind(
			&ttext_::signal_handler_sdl_key_down, this, _2, _3, _5, _6, _7));

	connect_signal<event::RECEIVE_KEYBOARD_FOCUS>(boost::bind(
			&ttext_::signal_handler_receive_keyboard_focus, this, _2));
	connect_signal<event::LOSE_KEYBOARD_FOCUS>(boost::bind(
			&ttext_::signal_handler_lose_keyboard_focus, this, _2));
}

void ttext_::set_maximum_length(const size_t maximum_length)
{
	const bool need_update = text_.get_length() > maximum_length;

	text_.set_maximum_length(maximum_length);

	if(need_update) {
		if(selection_start_ > maximum_length) {
			selection_start_ = maximum_length;
			selection_length_ = 0;
		} else if(selection_start_ + selection_length_ > maximum_length) {
			selection_length_ = maximum_length - selection_start_;
		}
		update_canvas();
		set_dirty();
	}
}

void ttext_::set_value(const std::string& text)
{
	if(text != text_.text()) {
		text_.set_text(text, false);

		// default to put the cursor at the end of the buffer.
		selection_start_ = text_.get_length();
		selection_length_ = 0;
		update_canvas();
		set_dirty();
	}
}

void ttext_::set_cursor(const size_t offset, const bool select)
{
	if(select) {

		if(selection_start_ == offset) {
			selection_length_ = 0;
		} else {
			selection_length_ = - static_cast<int>(selection_start_ - offset);
		}

#ifdef __unix__
		// selecting copies on UNIX systems.
		copy_selection(true);
#endif
		update_canvas();
		set_dirty();

	} else {
		assert(offset <= text_.get_length());
		selection_start_ = offset;
		selection_length_ = 0;

		update_canvas();
		set_dirty();
	}
}

void ttext_::insert_char(const Uint16 unicode)
{
	delete_selection();

	if(text_.insert_unicode(selection_start_, unicode)) {

		// Update status
		set_cursor(selection_start_ + 1, false);
		update_canvas();
		set_dirty();
	}
}

void ttext_::copy_selection(const bool mouse)
{
	int length = selection_length_;
	unsigned start = selection_start_;

	if(length == 0) {
		return;
	}

	if(length < 0) {
		length = - length;
		start -= length;
	}

	const wide_string& wtext = utils::string_to_wstring(text_.text());
	const std::string& text = utils::wstring_to_string(
		wide_string(wtext.begin() + start, wtext.begin() + start + length));

	copy_to_clipboard(text, mouse);
}

void ttext_::paste_selection(const bool mouse)
{
	const std::string& text = copy_from_clipboard(mouse);
	if(text.empty()) {
		return;
	}

	delete_selection();

	selection_start_ += text_.insert_text(selection_start_, text);

	update_canvas();
	set_dirty();
}

void  ttext_::set_selection_start(const size_t selection_start)
{
	if(selection_start != selection_start_) {
		selection_start_ = selection_start;
		set_dirty();
	}
}

void ttext_::set_selection_length(const int selection_length)
{
	if(selection_length != selection_length_) {
		selection_length_ = selection_length;
		set_dirty();
	}
}

void ttext_::set_state(const tstate state)
{
	if(state != state_) {
		state_ = state;
		set_dirty(true);
	}
}

void ttext_::handle_key_left_arrow(SDLMod modifier, bool& handled)
{
	/** @todo implement the ctrl key. */
	DBG_GUI_E << LOG_SCOPE_HEADER << '\n';

	handled = true;
	const int offset = selection_start_ - 1 + selection_length_;
	if(offset >= 0) {
		set_cursor(offset, (modifier & KMOD_SHIFT) != 0);
	}
}

void ttext_::handle_key_right_arrow(SDLMod modifier, bool& handled)
{
	/** @todo implement the ctrl key. */
	DBG_GUI_E << LOG_SCOPE_HEADER << '\n';

	handled = true;
	const size_t offset = selection_start_ + 1 + selection_length_;
	if(offset <= text_.get_length()) {
		set_cursor(offset, (modifier & KMOD_SHIFT) != 0);
	}
}

void ttext_::handle_key_home(SDLMod modifier, bool& handled)
{
	DBG_GUI_E << LOG_SCOPE_HEADER << '\n';

	handled = true;
	if(modifier & KMOD_CTRL) {
		goto_start_of_data((modifier & KMOD_SHIFT) != 0);
	} else {
		goto_start_of_line((modifier & KMOD_SHIFT) != 0);
	}
}

void ttext_::handle_key_end(SDLMod modifier, bool& handled)
{
	DBG_GUI_E << LOG_SCOPE_HEADER << '\n';

	handled = true;
	if(modifier & KMOD_CTRL) {
		goto_end_of_data((modifier & KMOD_SHIFT) != 0);
	} else {
		goto_end_of_line((modifier & KMOD_SHIFT) != 0);
	}
}

void ttext_::handle_key_backspace(SDLMod /*modifier*/, bool& handled)
{
	DBG_GUI_E << LOG_SCOPE_HEADER << '\n';

	handled = true;
	if(selection_length_ != 0) {
		delete_selection();
	} else if(selection_start_){
		delete_char(true);
	}
}

void ttext_::handle_key_delete(SDLMod /*modifier*/, bool& handled)
{
	DBG_GUI_E << LOG_SCOPE_HEADER << '\n';

	handled = true;
	if(selection_length_ != 0) {
		delete_selection();
	} else if (selection_start_ < text_.get_length()) {
		delete_char(false);
	}
}

void ttext_::handle_key_default(
		bool& handled, SDLKey /*key*/, SDLMod /*modifier*/, Uint16 unicode)
{
	DBG_GUI_E << LOG_SCOPE_HEADER << '\n';

	if(unicode >= 32 && unicode != 127) {
		handled = true;
		insert_char(unicode);
	}
}

void ttext_::signal_handler_middle_button_click(
			const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	paste_selection(true);

	handled = true;
}

void ttext_::signal_handler_sdl_key_down(const event::tevent event
		, bool& handled
		, const SDLKey key
		, SDLMod modifier
		, const Uint16 unicode)
{

	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

// For copy/paste we use a different key on the MAC. Other ctrl modifiers won't
// be modifed seems not to be required when I read the comment in
// widgets/textbox.cpp:516. Would be nice if somebody on a MAC would test it.
#ifdef __APPLE__
	const unsigned copypaste_modifier = KMOD_LMETA | KMOD_RMETA;
#else
	const unsigned copypaste_modifier = KMOD_CTRL;
#endif

	switch(key) {

		case SDLK_LEFT :
			handle_key_left_arrow(modifier, handled);
			break;

		case SDLK_RIGHT :
			handle_key_right_arrow(modifier, handled);
			break;

		case SDLK_UP :
			handle_key_up_arrow(modifier, handled);
			break;

		case SDLK_DOWN :
			handle_key_down_arrow(modifier, handled);
			break;

		case SDLK_PAGEUP :
			handle_key_page_up(modifier, handled);
			break;

		case SDLK_PAGEDOWN :
			handle_key_page_down(modifier, handled);
			break;

		case SDLK_a :
			if(!(modifier & KMOD_CTRL)) {
				handle_key_default(handled, key, modifier, unicode);
				break;
			}

			// If ctrl-a is used for home drop the control modifier
			modifier = static_cast<SDLMod>(modifier &~ KMOD_CTRL);
			/* FALL DOWN */

		case SDLK_HOME :
			handle_key_home(modifier, handled);
			break;

		case SDLK_e :
			if(!(modifier & KMOD_CTRL)) {
				handle_key_default(handled, key, modifier, unicode);
				break;
			}

			// If ctrl-e is used for end drop the control modifier
			modifier = static_cast<SDLMod>(modifier &~ KMOD_CTRL);
			/* FALL DOWN */

		case SDLK_END :
			handle_key_end(modifier, handled);
			break;

		case SDLK_BACKSPACE :
			handle_key_backspace(modifier, handled);
			break;

		case SDLK_u :
			if(modifier & KMOD_CTRL) {
				handle_key_clear_line(modifier, handled);
			} else {
				handle_key_default(handled, key, modifier, unicode);
			}
			break;

		case SDLK_DELETE :
			handle_key_delete(modifier, handled);
			break;

		case SDLK_c :
			if(!(modifier & copypaste_modifier)) {
				handle_key_default(handled, key, modifier, unicode);
				break;
			}

			// atm we don't care whether there is something to copy or paste
			// if nothing is there we still don't want to be chained.
			copy_selection(false);
			handled = true;
			break;

		case SDLK_x :
			if(!(modifier & copypaste_modifier)) {
				handle_key_default(handled, key, modifier, unicode);
				break;
			}

			copy_selection(false);
			delete_selection();
			handled = true;
			break;

		case SDLK_v :
			if(!(modifier & copypaste_modifier)) {
				handle_key_default(handled, key, modifier, unicode);
				break;
			}

			paste_selection(false);
			handled = true;
			break;

		default :
			handle_key_default(handled, key, modifier, unicode);

	}

	if(text_changed_callback_) {
		text_changed_callback_(this, this->text());
	}

}

void ttext_::signal_handler_receive_keyboard_focus(const event::tevent event)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	set_state(FOCUSSED);
}

void ttext_::signal_handler_lose_keyboard_focus(const event::tevent event)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	set_state(ENABLED);
}

} // namespace gui2

