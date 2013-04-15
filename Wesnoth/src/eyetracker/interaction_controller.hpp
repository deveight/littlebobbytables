/*
* Author: Bj�rn
*/
#include "../widgets/widget.hpp"
#include "../gui/widgets/widget.hpp"
#include "map_location.hpp"
#include "display.hpp"

namespace eyetracker
{
class interaction_controller
{

public:
    enum EVENT_TO_SEND {CLICK, DOUBLE_CLICK, REPEATING_CLICK, RIGHT_CLICK};

private:
    static SDL_TimerID timer_id_;
    static gui::widget* selected_widget_g1_;
    static gui2::twidget* selected_widget_g2_;
    static gui2::twindow* selected_window_;
    static map_location* map_loc_;
    static display* disp;

    static void click(int mousex, int mousey, Uint8 mousebutton = SDL_BUTTON_LEFT);
    static void double_click(int mousex, int mousey);

    static Uint32 callback(Uint32 interval, void* widget); // Called after a timer has expired
    static void stop_timer();
    static void reset(); // resets timer and internal state
    static void start_timer(interaction_controller::EVENT_TO_SEND event); // Sets the selected widget and timer id, starts a timer
    static void mouse_leave_base();

public:
    static void checkStillDwelling();
    static void blink(int x, int y);

    static void toggle_right_click(bool value);
    static bool get_right_click();

    static void mouse_enter(gui::widget* widget, interaction_controller::EVENT_TO_SEND event = CLICK); // Should be called by GUI1 widgets when the mouse enters over it
    static void mouse_enter(gui2::twidget* widget, interaction_controller::EVENT_TO_SEND event = CLICK); // Should be called by GUI2 widgets when the mouse enters over it
    static void mouse_enter(map_location* loc, display* d, interaction_controller::EVENT_TO_SEND event = CLICK); //Should be called by display when hex is selected
    static void init_window(gui2::twindow* window, interaction_controller::EVENT_TO_SEND event = CLICK);
    static void mouse_leave(gui::widget* widget);
    static void mouse_leave(gui2::twidget* widget);
};
}
