/* $Id: text.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
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

#ifndef GUI_WIDGETS_TEXT_HPP_INCLUDED
#define GUI_WIDGETS_TEXT_HPP_INCLUDED

//#include "gui/auxiliary/event/dispatcher.hpp"
#include "gui/widgets/control.hpp"
#include "../../text.hpp"

#include <string>

#include <boost/function.hpp>

namespace gui2 {

/**
 * Abstract base class for text items.
 *
 * All other text classes should inherit from this base class.
 *
 * Common signal handlers:
 * - connect_signal_pre_key_press
 */
class ttext_ : public tcontrol
{

public:
	ttext_();

	/** Inherited from tcontrol. */
	void set_active(const bool active)
		{ if(get_active() != active) set_state(active ? ENABLED : DISABLED); };

	/** Inherited from tcontrol. */
	bool get_active() const { return state_ != DISABLED; }

	/** Inherited from tcontrol. */
	unsigned get_state() const { return state_; }

	/***** ***** ***** ***** expose some functions ***** ***** ***** *****/

	void set_maximum_length(const size_t maximum_length);

	size_t get_length() const { return text_.get_length(); }

	/***** ***** ***** setters / getters for members ***** ****** *****/

	/**
	 * The set_value is virtual for the @ref tpassword_box class.
	 *
	 * That class overrides the set_value function to replace it with asterisk.
	 * There might be more generic way to do it when more classes are needed.
	 */
	virtual void set_value(const std::string& text);
	std::string get_value() const { return text_.text(); }

	const std::string& text() const { return text_.text(); }

	/** Set the text_changed callback. */
	void set_text_changed_callback(
		boost::function< void (ttext_* textbox, const std::string text) > cb)
	{
		text_changed_callback_ = cb;
	}

protected:

	/**
	 * Moves the cursor to the end of the line.
	 *
	 * @param select              Select the text from the original cursor
	 *                            position till the end of the line?
	 */
	virtual void goto_end_of_line(const bool select = false) = 0;

	/**
	 * Moves the cursor to the end of all text.
	 *
	 * For a single line text this is the same as goto_end_of_line().
	 *
	 * @param select              Select the text from the original cursor
	 *                            position till the end of the data?
	 */
	void goto_end_of_data(const bool select = false)
		{ set_cursor(text_.get_length(), select); }

	/**
	 * Moves the cursor to the beginning of the line
	 *
	 * @param select              Select the text from the original cursor
	 *                            position till the beginning of the line?
	 */
	virtual void goto_start_of_line(const bool select = false) = 0;

	/**
	 * Moves the cursor to the beginning of the data.
	 *
	 * @param select              Select the text from the original cursor
	 *                            position till the beginning of the data?
	 */
	void goto_start_of_data(const bool select = false) { set_cursor(0, select); }

	/** Selects all text. */
	void select_all() { selection_start_ = 0; goto_end_of_data(true); }

	/**
	 * Moves the cursor at the wanted position.
	 *
	 * @param offset              The wanted new cursor position.
	 * @param select              Select the text from the original cursor
	 *                            position till the new position?
	 */
	void set_cursor(const size_t offset, const bool select);

	/**
	 * Inserts a character at the cursor.
	 *
	 * This function is preferred over set_text since it's optimized for
	 * updating the internal bookkeeping.
	 *
	 * @param unicode             The unicode value of the character to insert.
	 */
	virtual void insert_char(const Uint16 unicode);

	/**
	 *  Deletes the character.
	 *
	 *  @param before_cursor     If true it deletes the character before the cursor
	 *                           (backspace) else the character after the cursor
	 *                           (delete).
	 */
	virtual void delete_char(const bool before_cursor) = 0;

	/** Deletes the current selection. */
	virtual void delete_selection() = 0;

	/** Copies the current selection. */
	virtual void copy_selection(const bool mouse);

	/** Pastes the current selection. */
	virtual void paste_selection(const bool mouse);

	/***** ***** ***** ***** expose some functions ***** ***** ***** *****/

	gui2::tpoint get_cursor_position(
		const unsigned column, const unsigned line = 0) const
		{ return text_.get_cursor_position(column, line); }

	tpoint get_column_line(const tpoint& position) const
		{ return text_.get_column_line(position); }

	void set_font_size(const unsigned font_size)
		{ text_.set_font_size(font_size); }

	void set_font_style(const unsigned font_style)
		{ text_.set_font_style(font_style); }

	void set_maximum_width(const int width)
		{ text_.set_maximum_width(width); }

	void set_maximum_height(const int height)
		{ text_.set_maximum_height(height); }

	void set_ellipse_mode(const PangoEllipsizeMode ellipse_mode)
		{ text_.set_ellipse_mode(ellipse_mode); }

	/***** ***** ***** setters / getters for members ***** ****** *****/

	size_t get_selection_start() const { return selection_start_; }
	void  set_selection_start(const size_t selection_start);

	size_t get_selection_length() const { return selection_length_; }
	void set_selection_length(const int selection_length);


private:
	/** Note the order of the states must be the same as defined in settings.hpp. */
	enum tstate { ENABLED, DISABLED, FOCUSSED, COUNT };

	void set_state(const tstate state);

	/**
	 * Current state of the widget.
	 *
	 * The state of the widget determines what to render and how the widget
	 * reacts to certain 'events'.
	 */
	tstate state_;

	/** The text entered in the widget. */
	font::ttext text_;

	/** Start of the selected text. */
	size_t selection_start_;

	/**
	 * Length of the selected text.
	 *
	 * * positive selection_len_ means selection to the right.
	 * * negative selection_len_ means selection to the left.
	 * * selection_len_ == 0 means no selection.
	 */
	int selection_length_;

	/****** handling of special keys first the pure virtuals *****/

	/**
	 * Every key can have several behaviours.
	 *
	 * Unmodified                 No modifier is pressed.
	 * Control                    The control key is pressed.
	 * Shift                      The shift key is pressed.
	 * Alt                        The alt key is pressed.
	 *
	 * If modifiers together do something else as the sum of the modifiers
	 * it's listed separately eg.
	 *
	 * Control                    Moves 10 steps at the time.
	 * Shift                      Selects the text.
	 * Control + Shift            Inserts 42 in the text.
	 *
	 * There are some predefined actions for results.
	 * Unhandled                  The key/modifier is ignored and also reported
	 *                            unhandled.
	 * Ignored                    The key/modifier is ignored and it's _expected_
	 *                            the inherited classes do the same.
	 * Implementation defined     The key/modifier is ignored and it's expected
	 *                            the inherited classes will define some meaning
	 *                            to it.
	 */

	/**
	 * Up arrow key pressed.
	 *
	 * The behaviour is implementation defined.
	 */
	virtual void handle_key_up_arrow(SDLMod modifier, bool& handled) = 0;

	/**
	 * Down arrow key pressed.
	 *
	 * The behaviour is implementation defined.
	 */
	virtual void handle_key_down_arrow(SDLMod modifier, bool& handled) = 0;

	/**
	 * Clears the current line.
	 *
	 * Unmodified                 Clears the current line.
	 * Control                    Ignored.
	 * Shift                      Ignored.
	 * Alt                        Ignored.
	 */
	virtual void handle_key_clear_line(SDLMod modifier, bool& handled) = 0;

	/**
	 * Left arrow key pressed.
	 *
	 * Unmodified                 Moves the cursor a character to the left.
	 * Control                    Like unmodified but a word instead of a letter
	 *                            at the time.
	 * Shift                      Selects the text while moving.
	 * Alt                        Ignored.
	 */
	virtual void handle_key_left_arrow(SDLMod modifier, bool& handled);

	/**
	 * Right arrow key pressed.
	 *
	 * Unmodified                 Moves the cursor a character to the right.
	 * Control                    Like unmodified but a word instead of a letter
	 *                            at the time.
	 * Shift                      Selects the text while moving.
	 * Alt                        Ignored.
	 */
	virtual void handle_key_right_arrow(SDLMod modifier, bool& handled);

	/**
	 * Home key pressed.
	 *
	 * Unmodified                 Moves the cursor a to the beginning of the
	 *                            line.
	 * Control                    Like unmodified but to the beginning of the
	 *                            data.
	 * Shift                      Selects the text while moving.
	 * Alt                        Ignored.
	 */
	virtual void handle_key_home(SDLMod modifier, bool& handled);

	/**
	 * End key pressed.
	 *
	 * Unmodified                 Moves the cursor a to the end of the line.
	 * Control                    Like unmodified but to the end of the data.
	 * Shift                      Selects the text while moving.
	 * Alt                        Ignored.
	 */
	virtual void handle_key_end(SDLMod modifier, bool& handled);

	/**
	 * Backspace key pressed.
	 *
	 * Unmodified                 Deletes the character before the cursor,
	 *                            ignored if at the beginning of the data.
	 * Control                    Ignored.
	 * Shift                      Ignored.
	 * Alt                        Ignored.
	 */
	virtual void handle_key_backspace(SDLMod modifier, bool& handled);

	/**
	 * Delete key pressed.
	 *
	 * Unmodified                 If there is a selection that's deleted.
	 *                            Else if not at the end of the data the
	 *                            character after the cursor is deleted.
	 *                            Else the key is ignored.
	 *                            ignored if at the beginning of the data.
	 * Control                    Ignored.
	 * Shift                      Ignored.
	 * Alt                        Ignored.
	 */
	virtual void handle_key_delete(SDLMod modifier, bool& handled);

	/**
	 * Page up key.
	 *
	 * Unmodified                 Unhandled.
	 * Control                    Ignored.
	 * Shift                      Ignored.
	 * Alt                        Ignored.
	 */
	virtual void handle_key_page_up(SDLMod /*modifier*/, bool& /*handled*/) {}

	/**
	 * Page down key.
	 *
	 * Unmodified                 Unhandled.
	 * Control                    Ignored.
	 * Shift                      Ignored.
	 * Alt                        Ignored.
	 */
	virtual void handle_key_page_down(SDLMod /*modifier*/, bool& /*handled*/) {}

protected:
	/**
	 * Default key handler if none of the above functions is called.
	 *
	 * Unmodified                 If invalid unicode it's ignored.
	 *                            Else if text selected the selected text is
	 *                            replaced with the unicode character send.
	 *                            Else the unicode character is inserted after
	 *                            the cursor.
	 * Control                    Ignored.
	 * Shift                      Ignored (already in the unicode value).
	 * Alt                        Ignored.
	 */
	virtual void handle_key_default(
		bool& handled, SDLKey key, SDLMod modifier, Uint16 unicode);

private:

	/**
	 * Text changed callback.
	 *
	 * This callback is called in key_press after the key_press event has been
	 * handled by the control. The parameters to the function are:
	 * - The widget invoking the callback
	 * - The new text of the textbox.
	 */
	boost::function< void (ttext_* textbox, const std::string text) > text_changed_callback_;

	/***** ***** ***** signal handlers ***** ****** *****/

	void signal_handler_middle_button_click(
			const event::tevent event, bool& handled);

	void signal_handler_sdl_key_down(const event::tevent event, bool& handled
			, const SDLKey key, SDLMod modifier, const Uint16 unicode);

	void signal_handler_receive_keyboard_focus(const event::tevent event);
	void signal_handler_lose_keyboard_focus(const event::tevent event);
};

} // namespace gui2

#endif
