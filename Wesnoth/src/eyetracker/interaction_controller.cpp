/*
* Author: Bj�rn
*/
#include "interaction_controller.hpp"
#include "../game_preferences.hpp"
#include "map_location.hpp"
#include "display.hpp"

namespace eyetracker
{
interaction_controller::INTERACTION_METHOD interaction_controller::interaction_method_;
SDL_TimerID interaction_controller::timer_id_ = NULL;
gui::widget* interaction_controller::selected_widget_g1_ = NULL;
gui2::twidget* interaction_controller::selected_widget_g2_ = NULL;
map_location* interaction_controller::map_loc_ = NULL;
display* interaction_controller::disp = NULL;
bool right_click_ = false;

void interaction_controller::init(){
    if(preferences::interaction_blink()){
        interaction_method_ = BLINK;
    }
    else if(preferences::interaction_switch()){
        interaction_method_ = SWITCH;
    }
    else{
        interaction_method_ = DWELL;
    }
}

void interaction_controller::set_interaction_method(interaction_controller::INTERACTION_METHOD interaction_method)
{
    interaction_method_ = interaction_method;
}
// REMEMBER: mouse_leave and mouse_enter may not be called one at a time.
// Sometimes there are several calls to mouse_enter in a row or vice versa.
void interaction_controller::mouse_enter(gui::widget* widget, interaction_controller::EVENT_TO_SEND event)
{
//    std::cerr << "Entered GUI1\n";
    if(timer_id_ != NULL)
        stop_timer();
    if(selected_widget_g1_ != NULL || selected_widget_g2_ != NULL)
    {
        reset();
    }
    /*
    When the mouse enters over a widget a timer (or something else, depending on interaction method) will be started.
    When the timer runs out widget.click() will be called, so the widget must implement this method.
    Also, widget.indicate() should produce a visible indicator over it, so that we can see how long time is left.
    */
    selected_widget_g1_ = widget;

    switch (interaction_method_)
    {
    case interaction_controller::DWELL:
        start_timer(event);
        break;
    case interaction_controller::BLINK:
        // Blink
        break;
    case interaction_controller::SWITCH:
        // Switch
        break;
    }
}


// REMEMBER: mouse_leave and mouse_enter may not be called one at a time.
// Sometimes there are several calls to mouse_enter in a row or vice versa.
void interaction_controller::mouse_enter(gui2::twidget* widget,interaction_controller::EVENT_TO_SEND event)
{
//    std::cerr << "Entered GUI2\n";
    if(timer_id_ != NULL)
        stop_timer();
    if(selected_widget_g1_ != NULL || selected_widget_g2_ != NULL)
    {
        reset();
    }

    selected_widget_g2_ = widget;

    switch (interaction_method_)
    {
    case interaction_controller::DWELL:
        start_timer(event);
        break;
    case interaction_controller::BLINK:
        // Blink
        break;
    case interaction_controller::SWITCH:
        // Switch
        break;
    }
}
void interaction_controller::mouse_enter(map_location* loc, display* d, interaction_controller::EVENT_TO_SEND event)
{
    if(timer_id_ != NULL)
        stop_timer();

    map_loc_ = loc;
    disp = d;

    switch (interaction_method_)
    {
    case interaction_controller::DWELL:
        start_timer(event);
        break;
    case interaction_controller::BLINK:
        // Blink
        break;
    case interaction_controller::SWITCH:
        // Switch
        break;
    }
}

// REMEMBER: mouse_leave and mouse_enter may not be called one at a time.
// Sometimes there are several calls to mouse_enter in a row or vice versa.
void interaction_controller::mouse_leave()
{
/*    if(selected_widget_g1_ != NULL)
        std::cerr << "Left GUI1\n";
    else if(selected_widget_g2_ != NULL)
        std::cerr << "Left GUI2\n";
*/
    switch (interaction_method_)
    {
    case interaction_controller::DWELL:
        if(timer_id_ != NULL)
            stop_timer();
        break;
    case interaction_controller::BLINK:
        // Blink is selected
        break;
    case interaction_controller::SWITCH:
        // Switch
        break;
    }

    reset();
}
void interaction_controller::click(int mousex, int mousey, Uint8 mousebutton)
{
    std::cerr << "MOUSEBUTTON: " << mousebutton << std::endl;
    SDL_Event fake_event;
    fake_event.type = SDL_MOUSEBUTTONDOWN;
    fake_event.button.button = mousebutton;
    fake_event.button.type = SDL_MOUSEBUTTONDOWN;
    fake_event.button.state = SDL_PRESSED;
    fake_event.button.which = 0;
    fake_event.button.x = mousex;
    fake_event.button.y = mousey;
    SDL_PushEvent(&fake_event);
    fake_event.type=SDL_MOUSEBUTTONUP;
    fake_event.button.type=SDL_MOUSEBUTTONUP;
    SDL_PushEvent(&fake_event);
}
void interaction_controller::double_click(int mousex, int mousey)
{
    click(mousex, mousey);
    click(mousex, mousey);
}

void interaction_controller::reset()
{
    selected_widget_g1_ = NULL;
    selected_widget_g2_ = NULL;
    map_loc_ = NULL;
}

Uint32 interaction_controller::callback(Uint32 interval, void* param)
{
    (void)interval;
    int tmp = (int) param;
    int x,y;
    interaction_controller::EVENT_TO_SEND event = (interaction_controller::EVENT_TO_SEND) tmp;
    if(selected_widget_g1_ != NULL)
    {
        SDL_Rect rect = selected_widget_g1_->location();
        x = rect.x + rect.w/2;
        y = rect.y + rect.h/2;
    }
    else if(selected_widget_g2_ != NULL)
    {
        x = selected_widget_g2_->get_x() + selected_widget_g2_->get_width()/2;
        y = selected_widget_g2_->get_y() + selected_widget_g2_->get_height()/2;
    }
    else if(map_loc_ != NULL){
        SDL_GetMouseState(&x,&y);
    }
    else
    {
        throw "InteractionController: Trying to click a widget but no widget has been selected.";
    }

    if(event == CLICK)
    {
        if(right_click_){
            click(x,y,SDL_BUTTON_RIGHT);
            right_click_ = false;
        }
        else{
            click(x,y);
        }
    }
    else if(event == DOUBLE_CLICK)
    {
        double_click(x,y);
    }
    else if(event == REPEATING_CLICK)
    {
        click(x,y);
        return interval; // returning interval to next click
    }

    mouse_leave();
    return 0;
}
// BOBBY TEMP
void interaction_controller::blink(){
    if(interaction_method_ == BLINK){
        int x,y;

        if(selected_widget_g1_ != NULL)
        {
            SDL_Rect rect = selected_widget_g1_->location();
            x = rect.x + rect.w/2;
            y = rect.y + rect.h/2;
        }
        else if(selected_widget_g2_ != NULL)
        {
            x = selected_widget_g2_->get_x() + selected_widget_g2_->get_width()/2;
            y = selected_widget_g2_->get_y() + selected_widget_g2_->get_height()/2;
        }
        else if(map_loc_ != NULL){
            x = disp->get_location_x(*map_loc_) + disp->hex_width() / 2;
            y = disp->get_location_y(*map_loc_) + disp->hex_size() / 2;
        }
        else
        {
            return;
        }

        if(right_click_){
            click(x,y,SDL_BUTTON_RIGHT);
            right_click_ = false;
        }
        else{
            click(x,y);
        }

        mouse_leave();
    }
    return;
}

void interaction_controller::set_right_click(){
    right_click_ = true;
}

void interaction_controller::start_timer(interaction_controller::EVENT_TO_SEND event)
{
    if(timer_id_ == NULL && (selected_widget_g1_ != NULL || selected_widget_g2_ != NULL || map_loc_ != NULL))
    {
        timer_id_ = SDL_AddTimer(preferences::gaze_length(), callback, (void*) event);
    }
    else
    {
        throw "Trying to start timer without stopping last timer or without selected widget";
    }
}
void interaction_controller::stop_timer()
{
    SDL_RemoveTimer(timer_id_);
    timer_id_ = NULL;
}
}
