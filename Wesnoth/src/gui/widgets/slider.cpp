/* $Id: slider.cpp 54625 2012-07-08 14:26:21Z loonycyborg $ */
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

#include "gui/widgets/slider.hpp"

#include "formatter.hpp"
#include "gui/auxiliary/log.hpp"
#include "gui/auxiliary/widget_definition/slider.hpp"
#include "gui/auxiliary/window_builder/slider.hpp"
#include "gui/widgets/window.hpp"
#include "gui/widgets/settings.hpp"
#include "sound.hpp"
#include "eyetracker/interaction_controller.hpp"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#define LOG_SCOPE_HEADER get_control_type() + " [" + id() + "] " + __func__
#define LOG_HEADER LOG_SCOPE_HEADER + ':'

namespace gui2 {

REGISTER_WIDGET(slider)

static int distance(const int a, const int b)
{
	/**
	 * @todo once this works properly the assert can be removed and the code
	 * inlined.
	 */
	int result =  b - a;
	assert(result >= 0);
	return result;
}

tslider::tslider():
		tscrollbar_(),
		best_slider_length_(0),
		minimum_value_(0),
		minimum_value_label_(),
		maximum_value_label_(),
		xpos_(0),
		mouse_entered_(false),
		steps_(0),
		value_labels_()
{
	connect_signal<event::SDL_KEY_DOWN>(boost::bind(
		&tslider::signal_handler_sdl_key_down, this, _2, _3, _5));
	connect_signal<event::LEFT_BUTTON_UP>(boost::bind(
		&tslider::signal_handler_left_button_up, this, _2, _3));
    	connect_signal<event::MOUSE_MOTION>(boost::bind(
				  &tslider::signal_handler_mouse_motion
				, this
				, _2
				, _3
				, _4
				, _5));
	connect_signal<event::MOUSE_LEAVE>(boost::bind(
				&tslider::signal_handler_mouse_leave, this, _2, _3));
	connect_signal<event::LEFT_BUTTON_DOWN>(boost::bind(
				&tslider::signal_handler_left_button_down, this, _2, _3));

}

tpoint tslider::calculate_best_size() const
{
	log_scope2(log_gui_layout, LOG_SCOPE_HEADER);

	// Inherited.
	tpoint result = tcontrol::calculate_best_size();
	if(best_slider_length_ != 0) {

		// Override length.
		boost::intrusive_ptr<const tslider_definition::tresolution> conf =
			boost::dynamic_pointer_cast<const tslider_definition::tresolution>
			(config());

		assert(conf);

		result.x = conf->left_offset + best_slider_length_ + conf->right_offset;
	}

	DBG_GUI_L << LOG_HEADER
		<< " best_slider_length " << best_slider_length_
		<< " result " << result
		<< ".\n";
	return result;

}

void tslider::set_value(const int value)
{
	if(value == get_value()) {
		return;
	}

	if(value < minimum_value_) {
		set_value(minimum_value_);
	} else if(value > get_maximum_value()) {
		set_value(get_maximum_value());
	} else {
		set_item_position(distance(minimum_value_, value));
	}
}

void tslider::set_minimum_value(const int minimum_value)
{
	if(minimum_value == minimum_value_) {
		return;
	}

	/** @todo maybe make it a VALIDATE. */
	assert(minimum_value <= get_maximum_value());

	const int value = get_value();
	const int maximum_value = get_maximum_value();
	minimum_value_ = minimum_value;

	// The number of items needs to include the begin and end so distance + 1.
	set_item_count(distance(minimum_value_, maximum_value) + 1);

	if(value < minimum_value_) {
		set_item_position(0);
	} else {
		set_item_position(minimum_value_ + value);
	}
}

void tslider::set_maximum_value(const int maximum_value)
{
	if(maximum_value == get_maximum_value()) {
		return;
	}

	/** @todo maybe make it a VALIDATE. */
	assert(minimum_value_ <= maximum_value);

	const int value = get_value();

	// The number of items needs to include the begin and end so distance + 1.
	set_item_count(distance(minimum_value_, maximum_value) + 1);

	if(value > maximum_value) {
		set_item_position(get_maximum_value());
	} else {
		set_item_position(minimum_value_ + value);
	}
}

t_string tslider::get_value_label() const
{
	if(!value_labels_.empty()) {
		assert(value_labels_.size() == get_item_count());
		return value_labels_[get_item_position()];
	} else if(!minimum_value_label_.empty()
			&& get_value() == get_minimum_value()) {
		return minimum_value_label_;
	} else if(!maximum_value_label_.empty()
			&& get_value() == get_maximum_value()) {
		return maximum_value_label_;
	} else {
		return t_string((formatter() << get_value()).str());
	}
}

void tslider::child_callback_positioner_moved()
{
	sound::play_UI_sound(settings::sound_slider_adjust);
}

unsigned tslider::minimum_positioner_length() const
{
	boost::intrusive_ptr<const tslider_definition::tresolution> conf =
		boost::dynamic_pointer_cast<const tslider_definition::tresolution>(config());
	assert(conf);
	return conf->minimum_positioner_length;
}

unsigned tslider::maximum_positioner_length() const
{
	boost::intrusive_ptr<const tslider_definition::tresolution> conf =
		boost::dynamic_pointer_cast<const tslider_definition::tresolution>(config());
	assert(conf);
	return conf->maximum_positioner_length;
}

unsigned tslider::offset_before() const
{
	boost::intrusive_ptr<const tslider_definition::tresolution> conf =
		boost::dynamic_pointer_cast<const tslider_definition::tresolution>(config());
	assert(conf);
	return conf->left_offset;
}

unsigned tslider::offset_after() const
{
	boost::intrusive_ptr<const tslider_definition::tresolution> conf =
		boost::dynamic_pointer_cast<const tslider_definition::tresolution>(config());
	assert(conf);
	return conf->right_offset;
}

bool tslider::on_positioner(const tpoint& coordinate) const
{
	// Note we assume the positioner is over the entire height of the widget.
	return coordinate.x >= static_cast<int>(get_positioner_offset())
		&& coordinate.x < static_cast<int>(get_positioner_offset() + get_positioner_length())
		&& coordinate.y > 0
		&& coordinate.y < static_cast<int>(get_height());
}

int tslider::on_bar(const tpoint& coordinate) const
{
	// Not on the widget, leave.
	if(static_cast<size_t>(coordinate.x) > get_width()
			|| static_cast<size_t>(coordinate.y) > get_height()) {
		return 0;
	}

	// we also assume the bar is over the entire height of the widget.
	if(static_cast<size_t>(coordinate.x) < get_positioner_offset()) {
		return -1;
	} else if(static_cast<size_t>(coordinate.x) >get_positioner_offset() + get_positioner_length()) {
		return 1;
	} else {
		return 0;
	}
}

void tslider::update_canvas()
{
	// Inherited.
	tscrollbar_::update_canvas();

	BOOST_FOREACH(tcanvas& tmp, canvas()) {
		tmp.set_variable("text", variant(get_value_label()));
	}
}

const std::string& tslider::get_control_type() const
{
	static const std::string type = "slider";
	return type;
}

void tslider::handle_key_decrease(bool& handled)
{
	DBG_GUI_E << LOG_HEADER << '\n';

	handled = true;

	scroll(tscrollbar_::ITEM_BACKWARDS);
}

void tslider::handle_key_increase(bool& handled)
{
	DBG_GUI_E << LOG_HEADER << '\n';

	handled = true;

	scroll(tscrollbar_::ITEM_FORWARD);
}

void tslider::signal_handler_sdl_key_down(const event::tevent event
		, bool& handled
		, const SDLKey key)
{

	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	if (key == SDLK_DOWN || key == SDLK_LEFT) {
		handle_key_decrease(handled);
	} else if (key == SDLK_UP || key == SDLK_RIGHT) {
		handle_key_increase(handled);
	} else {
		// Do nothing. Ignore other keys.
	}
}

void tslider::signal_handler_left_button_up(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	get_window()->keyboard_capture(this);

	handled = true;
}

SDL_Rect tslider::indicator_rect(){
    int x = xpos_ - get_height()/4;
    int y = get_y();
    int w = get_height();
    int h = w;
    return {x,y,w,h};
}

//BOBBY - handles mouse motions
void tslider::signal_handler_mouse_motion(const event::tevent event, bool& handled, bool& halt, const tpoint& coordinate){
    	DBG_GUI_E << LOG_HEADER << ' ' << event << " at " << coordinate << ".\n";

    if(abs(step_size_*(coordinate.x - get_x()) + get_minimum_value()) > get_maximum_value()) return;
	tpoint mouse = coordinate;
	mouse.x -= get_x();
	mouse.y -= get_y();

	//Bobby | Christoffer | Added eyetracker support
    if(!mouse_entered_){
        if(get_maximum_value() - get_minimum_value() < 15){
            steps_ = get_maximum_value() - get_minimum_value();
        }
        else{
            steps_ = 10;
        }
        xpos_ = coordinate.x;
        eyetracker::interaction_controller::mouse_enter(this);
        mouse_entered_ = true;
    }
    else{
        //std::cerr<<steps_<<std::endl;//", ";
        int change = (get_width()/steps_)/2;
        if(coordinate.x >= xpos_+ change || coordinate.x <= xpos_- change){
           //std::cerr<<increment_<<std::endl;
            //xpos_ = prevX;
            eyetracker::interaction_controller::mouse_leave(this);
            mouse_entered_ = false;
            eyetracker::interaction_controller::mouse_enter(this);
        }
    }

	switch(state_) {
		case ENABLED :
			if(on_positioner(mouse)) {
				set_state(FOCUSSED);
			}

			break;

		case PRESSED : {
				const int distance = get_length_difference(mouse_, mouse);
				mouse_ = mouse;
				move_positioner(distance);
			}
			break;

		case FOCUSSED :
			if(!on_positioner(mouse)) {
				set_state(ENABLED);
			}
			break;

		case DISABLED :
			// Shouldn't be possible, but seems to happen in the lobby
			// if a resize layout happens during dragging.
			halt = true;
			break;

		default :
			assert(false);
	}
	handled = true;
}


void tslider::signal_handler_mouse_leave(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	if(state_ == FOCUSSED) {
		set_state(ENABLED);
	}
	handled = true;

    //If we were over an item, mark that we have left the slider
    if(mouse_entered_){
        eyetracker::interaction_controller::mouse_leave(this);
        mouse_entered_ = false;
    }
}

void tslider::signal_handler_left_button_down(
		const event::tevent event, bool& handled)
{
	DBG_GUI_E << LOG_HEADER << ' ' << event << ".\n";

	tpoint mouse = get_mouse_position();
	mouse.x -= get_x();
	mouse.y -= get_y();

	if(on_positioner(mouse)) {
		assert(get_window());
		mouse_ = mouse;
		get_window()->mouse_capture();
		set_state(PRESSED);
	}

	const int bar = on_bar(mouse);

	if(bar == -1) {
		scroll(HALF_JUMP_BACKWARDS);
		fire(event::NOTIFY_MODIFIED, *this, NULL);
//		positioner_moved_notifier_.notify();
	} else if(bar == 1) {
		scroll(HALF_JUMP_FORWARD);
		fire(event::NOTIFY_MODIFIED, *this, NULL);
//		positioner_moved_notifier_.notify();
	} else {
		assert(bar == 0);
	}

    //Bobby : Set slider value
    set_value(step_size_*(xpos_ - get_x()) + get_minimum_value());
	handled = true;
}

} // namespace gui2

