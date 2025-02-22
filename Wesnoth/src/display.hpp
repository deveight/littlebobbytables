/* $Id: display.hpp 53971 2012-04-22 23:53:28Z gabba $ */
/*
   Copyright (C) 2003 - 2012 by David White <dave@whitevine.net>
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
 *
 * map_display and display: classes which take care of
 * displaying the map and game-data on the screen.
 *
 * The display is divided into two main sections:
 * - the game area, which displays the tiles of the game board, and units on them,
 * - and the side bar, which appears on the right hand side.
 * The side bar display is divided into three sections:
 * - the minimap, which is displayed at the top right
 * - the game status, which includes the day/night image,
 *   the turn number, information about the current side,
 *   and information about the hex currently moused over (highlighted)
 * - the unit status, which displays an image and stats
 *   for the current unit.
 */

#ifndef DISPLAY_H_INCLUDED
#define DISPLAY_H_INCLUDED

class config;
class terrain_builder;
struct time_of_day;
class map_labels;
class arrow;

#include "font.hpp"
#include "key.hpp"
#include "team.hpp"
#include "time_of_day.hpp"
#include "theme.hpp"
#include "video.hpp"
#include "widgets/button.hpp"

#include <list>

#include <boost/function.hpp>
#include <boost/scoped_ptr.hpp>

class gamemap;

class display
{
public:
	display(CVideo& video, const gamemap* map, const config& theme_cfg,
			const config& level);
	virtual ~display();

	/**
	 * Updates internals that cache map size. This should be called when the map
	 * size has changed.
	 */
	void reload_map();

	void clear_invalidated_hex();

	void change_map(const gamemap* m);

	static Uint32 rgb(Uint8 red, Uint8 green, Uint8 blue)
		{ return 0xFF000000 | (red << 16) | (green << 8) | blue; }
	static Uint8 red(Uint32 color)
		{ return (color & 0x00FF0000) >> 16;}
	static Uint8 green(Uint32 color)
		{ return (color & 0x0000FF00) >> 8;}
	static Uint8 blue(Uint32 color)
		{ return (color & 0x000000FF) ;}
	static Uint32 max_rgb(Uint32 first,Uint32 second)
		{ return rgb(std::max(red(first),red(second)),std::max(green(first),green(second)),std::max(blue(first),blue(second))) ; }

	/**
	 * Add r,g,b from tod_manager to the map
	 *
	 */
	void update_tod();

	/**
	 * Add r,g,b to the colors for all images displayed on the map.
	 *
	 * Used for special effects like flashes.
	 */
	void adjust_color_overlay(int r, int g, int b);


	/** Gets the underlying screen object. */
	CVideo& video() { return screen_; }

	/** return the screen surface or the surface used for map_screenshot. */
	surface get_screen_surface() { return map_screenshot_ ? map_screenshot_surf_ : screen_.getSurface();}

	virtual bool in_game() const { return false; }
	virtual bool in_editor() const { return false; }

	/**
	 * the dimensions of the display. x and y are width/height.
	 * mapx is the width of the portion of the display which shows the game area.
	 * Between mapx and x is the sidebar region.
	 */
	int w() const { return screen_.getx(); }	/**< width */
	int h() const { return screen_.gety(); }	/**< height */
	const SDL_Rect& minimap_area() const
		{ return theme_.mini_map_location(screen_area()); }
	const SDL_Rect& unit_image_area() const
		{ return theme_.unit_image_location(screen_area()); }

	SDL_Rect screen_area() const
		{ return create_rect(0, 0, w(), h()); }

	/**
	 * Returns the maximum area used for the map
	 * regardless to resolution and view size
	 */
	const SDL_Rect& max_map_area() const;

	/**
	 * Returns the area used for the map
	 */
	const SDL_Rect& map_area() const;

	/**
	 * Returns the available area for a map, this may differ
	 * from the above. This area will get the background area
	 * applied to it.
	 */
	const SDL_Rect& map_outside_area() const { return map_screenshot_ ?
		max_map_area() : theme_.main_map_location(screen_area()); }

	/** Check if the bbox of the hex at x,y has pixels outside the area rectangle. */
	bool outside_area(const SDL_Rect& area, const int x,const int y) const;

	/**
	 * Function which returns the width of a hex in pixels,
	 * up to where the next hex starts.
	 * (i.e. not entirely from tip to tip -- use hex_size()
	 * to get the distance from tip to tip)
	 */
	int hex_width() const { return (zoom_*3)/4; }

	/**
	 * Function which returns the size of a hex in pixels
	 * (from top tip to bottom tip or left edge to right edge).
	 */
	int hex_size() const { return zoom_; }

	/** Returns the current zoom factor. */
	double get_zoom_factor() const { return double(zoom_)/double(image::tile_size); }

	/**
	 * given x,y co-ordinates of an onscreen pixel, will return the
	 * location of the hex that this pixel corresponds to.
	 * Returns an invalid location if the mouse isn't over any valid location.
	 */
	const map_location hex_clicked_on(int x, int y) const;

	/**
	 * given x,y co-ordinates of a pixel on the map, will return the
	 * location of the hex that this pixel corresponds to.
	 * Returns an invalid location if the mouse isn't over any valid location.
	 */
	const map_location pixel_position_to_hex(int x, int y) const;

	/**
	 * given x,y co-ordinates of the mouse, will return the location of the
	 * hex in the minimap that the mouse is currently over, or an invalid
	 * location if the mouse isn't over the minimap.
	 */
	map_location minimap_location_on(int x, int y);

	const map_location& selected_hex() const { return selectedHex_; }
	const map_location& mouseover_hex() const { return mouseoverHex_; }

	virtual void select_hex(map_location hex);
	virtual void highlight_hex(map_location hex);

	/** Function to invalidate the game status displayed on the sidebar. */
	void invalidate_game_status() { invalidateGameStatus_ = true; }

	/** Functions to get the on-screen positions of hexes. */
	int get_location_x(const map_location& loc) const;
	int get_location_y(const map_location& loc) const;

	SDL_Rect indicator_rect();

	/**
	 * Rectangular area of hexes, allowing to decide how the top and bottom
	 * edges handles the vertical shift for each parity of the x coordinate
	 */
	struct rect_of_hexes{
		int left;
		int right;
		int top[2]; // for even and odd values of x, respectively
		int bottom[2];

		/**  very simple iterator to walk into the rect_of_hexes */
		struct iterator {
			iterator(const map_location &loc, const rect_of_hexes &rect)
				: loc_(loc), rect_(rect){};

			/** increment y first, then when reaching bottom, increment x */
			iterator& operator++();
			bool operator==(const iterator &that) const { return that.loc_ == loc_; }
			bool operator!=(const iterator &that) const { return that.loc_ != loc_; }
			const map_location& operator*() const {return loc_;};

			typedef std::forward_iterator_tag iterator_category;
			typedef map_location value_type;
			typedef int difference_type;
			typedef const map_location *pointer;
			typedef const map_location &reference;

			private:
				map_location loc_;
				const rect_of_hexes &rect_;
		};
		typedef iterator const_iterator;

		iterator begin() const;
		iterator end() const;
	};

	/** Return the rectangular area of hexes overlapped by r (r is in screen coordinates) */
	const rect_of_hexes hexes_under_rect(const SDL_Rect& r) const;

	/** Returns the rectangular area of visible hexes */
	const rect_of_hexes get_visible_hexes() const {return hexes_under_rect(map_area());};

	/** Returns true if location (x,y) is covered in shroud. */
	bool shrouded(const map_location& loc) const {
		return viewpoint_ && viewpoint_->shrouded(loc);
	}
	/** Returns true if location (x,y) is covered in fog. */
	bool fogged(const map_location& loc) const {
		return viewpoint_ && viewpoint_->fogged(loc);
	}

	/**
	 * Determines whether a grid should be overlayed on the game board.
	 * (to more clearly show where hexes are)
	 */
	void set_grid(const bool grid) { grid_ = grid; }

	/** Getter for the x,y debug overlay on tiles */
	bool get_draw_coordinates() const { return draw_coordinates_; }
	/** Setter for the x,y debug overlay on tiles */
	void set_draw_coordinates(bool value) { draw_coordinates_ = value; }

	/** Getter for the terrain code debug overlay on tiles */
	bool get_draw_terrain_codes() const { return draw_terrain_codes_; }
	/** Setter for the terrain code debug overlay on tiles */
	void set_draw_terrain_codes(bool value) { draw_terrain_codes_ = value; }

	/** Save a (map-)screenshot and return the estimated file size */
	int screenshot(std::string filename, bool map_screenshot = false);

	/** Invalidates entire screen, including all tiles and sidebar. Calls redraw observers. */
	void redraw_everything();

	/** Adds a redraw observer, a function object to be called when redraw_everything is used */
	void add_redraw_observer(boost::function<void(display&)> f);

	/** Clear the redraw observers */
	void clear_redraw_observers();

	theme& get_theme() { return theme_; }

	/**
	 * Retrieves a pointer to a theme UI button.
	 *
	 * @note The returned pointer may either be NULL, meaning the button
	 *       isn't defined by the current theme, or point to a valid
	 *       gui::button object. However, the objects retrieved will be
	 *       destroyed and recreated by draw() method calls. Do *NOT* store
	 *       these pointers for longer than strictly necessary to
	 *       accomplish a specific task before the next screen refresh.
	 */
	gui::button* find_button(const std::string& id);
	gui::button::TYPE string_to_button_type(std::string type);
	void create_buttons();
	void invalidate_theme() { panelsDrawn_ = false; }

	void refresh_report(std::string const &report_name, const config &);

	// Will be overridden in the display subclass
	virtual void draw_minimap_units() {};

	/** Function to invalidate all tiles. */
	void invalidate_all();

	/** Function to invalidate a specific tile for redrawing. */
	bool invalidate(const map_location& loc);

	bool invalidate(const std::set<map_location>& locs);

	/**
	 * If this set is partially invalidated, invalidate all its hexes.
	 * Returns if any new invalidation was needed
	 */
	bool propagate_invalidation(const std::set<map_location>& locs);

	/** invalidate all hexes under the rectangle rect (in screen coordinates) */
	bool invalidate_locations_in_rect(const SDL_Rect& rect);
	bool invalidate_visible_locations_in_rect(const SDL_Rect& rect);

	/**
	 * Function to invalidate animated terrains which may have changed.
	 */
	virtual void invalidate_animations();

	/**
	 * Per-location invalidation called by invalidate_animations()
	 * defaults to no action, overridden by derived classes
	 */
	virtual void invalidate_animations_location(const map_location& /*loc*/) {}

	const gamemap& get_map() const { return *map_; }

	/**
	 * mouseover_hex_overlay_ require a prerendered surface
	 * and is drawn underneath the mouse's location
	 */
	void set_mouseover_hex_overlay(const surface& image)
		{ mouseover_hex_overlay_ = image; }

	void clear_mouseover_hex_overlay()
		{ mouseover_hex_overlay_ = NULL; }

	/**
	 * Debug function to toggle the "sunset" mode.
	 * The map area become progressively darker,
	 * except where hexes are refreshed.
	 * delay is the number of frames between each darkening
	 * (0 to toggle).
	 */
	static void sunset(const size_t delay = 0);

	/** Toggle to continuously redraw the screen. */
	static void toggle_benchmark();

	/**
	 * Toggle to debug foreground terrain.
	 * Separate background and foreground layer
	 * to better spot any error there.
	 */
	static void toggle_debug_foreground();

	terrain_builder& get_builder() {return *builder_;};

	void flip();

	/** Copy the backbuffer to the framebuffer. */
	void update_display();

	/** Rebuild all dynamic terrain. */
	void rebuild_all();

	const theme::menu* menu_pressed();

	/**
	 * Finds the menu which has a given item in it,
	 * and enables or disables it.
	 */
	void enable_menu(const std::string& item, bool enable);

	void set_diagnostic(const std::string& msg);

	/** Delay routines: use these not SDL_Delay (for --nogui). */
	void delay(unsigned int milliseconds) const;

	/**
	 * Set/Get whether 'turbo' mode is on.
	 * When turbo mode is on, everything moves much faster.
	 */
	void set_turbo(const bool turbo) { turbo_ = turbo; }

	double turbo_speed() const;

	void set_turbo_speed(const double speed) { turbo_speed_ = speed; }

	/** control unit idle animations and their frequency */
	void set_idle_anim(bool ison) { idle_anim_ = ison; }
	bool idle_anim() const { return idle_anim_; }
	void set_idle_anim_rate(int rate);
	double idle_anim_rate() const { return idle_anim_rate_; }

	void bounds_check_position();
	void bounds_check_position(int& xpos, int& ypos);

	/**
	 * Scrolls the display by xmov,ymov pixels.
	 * Invalidation and redrawing will be scheduled.
	 * @return true if the map actually moved.
	 */
	bool scroll(int xmov, int ymov);

	/**
	 * Zooms the display by the specified amount.
	 * Negative values zoom out.
	 * Note the amount should be a multiple of four,
	 * otherwise the images might start to look odd
	 * (hex_width() gets rounding errors).
	 */
	void set_zoom(int amount);

	/** Sets the zoom amount to the default. */
	void set_default_zoom();

	enum SCROLL_TYPE { SCROLL, WARP, ONSCREEN, ONSCREEN_WARP };

	/**
	 * Scroll such that location loc is on-screen.
	 * WARP jumps to loc; SCROLL uses scroll speed;
	 * ONSCREEN only scrolls if x,y is offscreen
	 * force : scroll even if preferences tell us not to
	 */
	void scroll_to_tile(const map_location& loc, SCROLL_TYPE scroll_type=ONSCREEN, bool check_fogged=true,bool force = true);

	/**
	 * Scroll such that location loc1 is on-screen.
	 * It will also try to make it such that loc2 is on-screen,
	 * but this is not guaranteed. For ONSCREEN scrolls add_spacing
	 * sets the desired minimum distance from the border in hexes.
	 */
	void scroll_to_tiles(map_location loc1, map_location loc2,
	                     SCROLL_TYPE scroll_type=ONSCREEN, bool check_fogged=true,
			     double add_spacing=0.0, bool force = true);

	/** Scroll to fit as many locations on-screen as possible, starting with the first. */
	void scroll_to_tiles(const std::vector<map_location>& locs,
	                     SCROLL_TYPE scroll_type=ONSCREEN, bool check_fogged=true,
	                     bool only_if_possible=false,
			     double add_spacing=0.0, bool force = true);

	/** Expose the event, so observers can be notified about map scrolling. */
	events::generic_event &scroll_event() const { return scroll_event_; }

	events::generic_event& complete_redraw_event() { return complete_redraw_event_; }

	/** Check if a tile is fully visible on screen. */
	bool tile_fully_on_screen(const map_location& loc);

	/** Checks if location @a loc or one of the adjacent tiles is visible on screen. */
	bool tile_nearly_on_screen(const map_location &loc) const;

	/**
	 * Draws invalidated items.
	 * If update is true, will also copy the display to the frame buffer.
	 * If force is true, will not skip frames, even if running behind.
	 * Not virtual, since it gathers common actions. Calls various protected
	 * virtuals (further below) to allow specialized behaviour in derived classes.
	 */
	void draw(bool update=true, bool force=false);

	map_labels& labels();
	const map_labels& labels() const;

	/** Announce a message prominently. */
	void announce(const std::string& msg,
		       const SDL_Color& color = font::GOOD_COLOR);

	/**
	 * Schedule the minimap for recalculation.
	 * Useful if any terrain in the map has changed.
	 */
	void recalculate_minimap() {minimap_ = NULL; redrawMinimap_ = true; };

	/**
	 * Schedule the minimap to be redrawn.
	 * Useful if units have moved about on the map.
	 */
	void redraw_minimap() { redrawMinimap_ = true; }

	virtual const time_of_day& get_time_of_day(const map_location& loc = map_location::null_location) const;

	virtual bool has_time_area() const {return false;};

	void write(config& cfg) const;
private:
	void read(const config& cfg);

protected:
	/** Clear the screen contents */
	void clear_screen();

	/**
	 * Called near the beginning of each draw() call.
	 * Derived classes can use this to add extra actions before redrawing
	 * invalidated hexes takes place. No action here by default.
	 */
	virtual void pre_draw() {}

	/**
	 * Called at the very end of each draw() call.
	 * Derived classes can use this to add extra actions after redrawing
	 * invalidated hexes takes place. No action here by default.
	 */
	virtual void post_draw() {}

	/**
	 * Get the clipping rectangle for drawing.
	 * Virtual since the editor might use a slightly different approach.
	 */
	virtual const SDL_Rect& get_clip_rect();

	/**
	 * Only called when there's actual redrawing to do. Loops through
	 * invalidated locations and redraws them. Derived classes can override
	 * this, possibly to insert pre- or post-processing around a call to the
	 * base class's function.
	 */
	virtual void draw_invalidated();

	/**
	 * Hook for actions to take right after draw() calls drawing_buffer_commit
	 * No action here by default.
	 */
	virtual void post_commit() {}

	/**
	 * Redraws a single gamemap location.
	 */
	virtual void draw_hex(const map_location& loc);

	/**
	 * @returns the image type to be used for the passed hex
	 */
	virtual image::TYPE get_image_type(const map_location& loc);

	/**
	 * Called near the end of a draw operation, derived classes can use this
	 * to render a specific sidebar. Very similar to post_commit.
	 */
	virtual void draw_sidebar();

	/**
	 * Draws the border tile overlay.
	 * The routine determines by itself which border it is on
	 * and draws an overlay accordingly. The definition of the
	 * border is stored in the 'main_map_border' part of the theme.
	 *
	 * @param loc	the map location of the tile
	 * @param xpos	the on-screen pixels x coordinate of the tile
	 * @param ypos	the on-screen pixels y coordinate of the tile
	 */
	virtual void draw_border(const map_location& loc,
		const int xpos, const int ypos);

	void draw_minimap();

	enum TERRAIN_TYPE { BACKGROUND, FOREGROUND};

	std::vector<surface> get_terrain_images(const map_location &loc,
					const std::string& timeid,
					image::TYPE type,
					TERRAIN_TYPE terrain_type);

	std::vector<surface> get_fog_shroud_images(const map_location& loc, image::TYPE image_type);

	void draw_image_for_report(surface& img, SDL_Rect& rect);

	void scroll_to_xy(int screenxpos, int screenypos, SCROLL_TYPE scroll_type,bool force = true);

	void fill_images_list(const std::string& prefix, std::vector<std::string>& images);

	const std::string& get_variant(const std::vector<std::string>& variants, const map_location &loc) const;

	CVideo& screen_;
	const gamemap* map_;
	const team *viewpoint_;
	int xpos_, ypos_;
	theme theme_;
	int zoom_;
	static int last_zoom_;
	boost::scoped_ptr<terrain_builder> builder_;
	surface minimap_;
	SDL_Rect minimap_location_;
	bool redrawMinimap_;
	bool redraw_background_;
	bool invalidateAll_;
	bool grid_;
	int diagnostic_label_;
	bool panelsDrawn_;
	double turbo_speed_;
	bool turbo_;
	bool invalidateGameStatus_;
	boost::scoped_ptr<map_labels> map_labels_;


	/** Event raised when the map is being scrolled */
	mutable events::generic_event scroll_event_;

	/**
	 * notify observers that the screen has been redrawn completely
	 * atm this is used for replay_controller to add replay controls to the standard theme
	 */
	events::generic_event complete_redraw_event_;

	/**
	 * Holds the tick count for when the next drawing event is scheduled.
	 * Drawing shouldn't occur before this time.
	 */
	int nextDraw_;

	// Not set by the initializer:
	std::map<std::string, SDL_Rect> reportRects_;
	std::map<std::string, surface> reportSurfaces_;
	std::map<std::string, config> reports_;
	std::vector<gui::button> buttons_;
	std::set<map_location> invalidated_;
	std::set<map_location> previous_invalidated_;
	surface mouseover_hex_overlay_;
	// If we're transitioning from one time of day to the next,
	// then we will use these two masks on top of all hexes when we blit.
	surface tod_hex_mask1, tod_hex_mask2;
	std::vector<std::string> fog_images_;
	std::vector<std::string> shroud_images_;

	map_location selectedHex_;
	map_location mouseoverHex_;
	CKey keys_;

	/** Local cache for preferences::animate_map, since it is constantly queried. */
	bool animate_map_;

	/** Local cache for preferences "local_tod_lighting" */
	bool local_tod_light_;

public:
	/**
	 * The layers to render something on. This value should never be stored
	 * it's the internal drawing order and adding removing and reordering
	 * the layers should be safe.
	 * If needed in WML use the name and map that to the enum value.
	 */
	enum tdrawing_layer{
		LAYER_TERRAIN_BG,          /**<
		                            * Layer for the terrain drawn behind the
		                            * unit.
		                            */
		LAYER_GRID_TOP,            /**< Top half part of grid image */
		LAYER_MOUSEOVER_OVERLAY,   /**< Mouseover overlay used by editor*/
		LAYER_FOOTSTEPS,           /**< Footsteps showing path from unit to mouse */
		LAYER_MOUSEOVER_TOP,       /**< Top half of image following the mouse */
		LAYER_UNIT_FIRST,          /**< Reserve layers to be selected for WML. */
		LAYER_UNIT_BG = LAYER_UNIT_FIRST+10,             /**< Used for the ellipse behind the unit. */
		LAYER_UNIT_DEFAULT=LAYER_UNIT_FIRST+40,/**<default layer for drawing units */
		LAYER_TERRAIN_FG = LAYER_UNIT_FIRST+50, /**<
		                            * Layer for the terrain drawn in front of
		                            * the unit.
		                            */
		LAYER_GRID_BOTTOM,         /**<
		                            * Used for the bottom half part of grid image.
		                            * Should be under moving units, to avoid masking south move.
		                            */
		LAYER_UNIT_MOVE_DEFAULT=LAYER_UNIT_FIRST+60/**<default layer for drawing moving units */,
		LAYER_UNIT_FG =  LAYER_UNIT_FIRST+80, /**<
		                            * Used for the ellipse in front of the
		                            * unit.
		                            */
		LAYER_UNIT_MISSILE_DEFAULT = LAYER_UNIT_FIRST+90, /**< default layer for missile frames*/
		LAYER_UNIT_LAST=LAYER_UNIT_FIRST+100,
		LAYER_REACHMAP,            /**< "black stripes" on unreachable hexes. */
		LAYER_MOUSEOVER_BOTTOM,    /**< Bottom half of image following the mouse */
		LAYER_FOG_SHROUD,          /**< Fog and shroud. */
		LAYER_ARROWS,              /**< Arrows from the arrows framework. Used for planned moves display. */
		LAYER_ACTIONS_NUMBERING,   /**< Move numbering for the whiteboard. */
		LAYER_SELECTED_HEX,        /**< Image on the selected unit */
		LAYER_ATTACK_INDICATOR,    /**< Layer which holds the attack indicator. */
		LAYER_UNIT_BAR,            /**<
		                            * Unit bars and overlays are drawn on this
		                            * layer (for testing here).
		                            */
		LAYER_MOVE_INFO,           /**< Movement info (defense%, ect...). */
		LAYER_LINGER_OVERLAY,      /**< The overlay used for the linger mode. */
		LAYER_BORDER,              /**< The border of the map. */

		LAYER_LAST_LAYER           /**<
		                            * Don't draw to this layer it's a dummy to
		                            * size the vector.
		                            */
		};

	/**
	 * Draw an image at a certain location.
	 * x,y: pixel location on screen to draw the image
	 * image: the image to draw
	 * reverse: if the image should be flipped across the x axis
	 * greyscale: used for instance to give the petrified appearance to a unit image
	 * alpha: the merging to use with the background
	 * blendto: blend to this color using blend_ratio
	 * submerged: the amount of the unit out of 1.0 that is submerged
	 *            (presumably under water) and thus shouldn't be drawn
	 */
	void render_image(int x, int y, const display::tdrawing_layer drawing_layer,
			const map_location& loc, surface image,
			bool hreverse=false, bool greyscale=false,
			fixed_t alpha=ftofxp(1.0), Uint32 blendto=0,
			double blend_ratio=0, double submerged=0.0,bool vreverse =false);

	/**
	 * Draw text on a hex. (0.5, 0.5) is the center.
	 * The font size is adjusted to the zoom factor
	 * and divided by 2 for tiny-gui.
	 */
	void draw_text_in_hex(const map_location& loc,
		const tdrawing_layer layer, const std::string& text, size_t font_size,
		SDL_Color color, double x_in_hex=0.5, double y_in_hex=0.5);

protected:
	/**
	 * In order to render a hex properly it needs to be rendered per row. On
	 * this row several layers need to be drawn at the same time. Mainly the
	 * unit and the background terrain. This is needed since both can spill
	 * in the next hex. The foreground terrain needs to be drawn before to
	 * avoid decapitation a unit.
	 *
	 * In other words:
	 * for every layer
	 *   for every row (starting from the top)
	 *     for every hex in the row
	 *       ...
	 *
	 * this is modified to:
	 * for every layer group
	 *   for every row (starting from the top)
	 *     for every layer in the group
	 *       for every hex in the row
	 *         ...
	 *
	 * * Surfaces are rendered per level in a map.
	 * * Per level the items are rendered per location these locations are
	 *   stored in the drawing order required for units.
	 * * every location has a vector with surfaces, each with its own screen
	 *   coordinate to render at.
	 * * every vector element has a vector with surfaces to render.
	 */
	class drawing_buffer_key
	{
	private:
		unsigned int key_;

		static const tdrawing_layer layer_groups[];
		static const unsigned int max_layer_group;

	public:
		drawing_buffer_key(const map_location &loc, tdrawing_layer layer);

		bool operator<(const drawing_buffer_key &rhs) const { return key_ < rhs.key_; }
	};

	/** Helper structure for rendering the terrains. */
	class tblit
	{
	public:
		tblit(const tdrawing_layer layer, const map_location& loc,
				const int x, const int y, const surface& surf,
				const SDL_Rect& clip)
			: x_(x), y_(y), surf_(1, surf), clip_(clip),
			key_(loc, layer)
		{}

		tblit(const tdrawing_layer layer, const map_location& loc,
				const int x, const int y, const std::vector<surface>& surf,
				const SDL_Rect& clip)
			: x_(x), y_(y), surf_(surf), clip_(clip),
			key_(loc, layer)
		{}

		int x() const { return x_; }
		int y() const { return y_; }
		const std::vector<surface> &surf() const { return surf_; }
		const SDL_Rect &clip() const { return clip_; }

		bool operator<(const tblit &rhs) const { return key_ < rhs.key_; }

	private:
		int x_;                      /**< x screen coordinate to render at. */
		int y_;                      /**< y screen coordinate to render at. */
		std::vector<surface> surf_;  /**< surface(s) to render. */
		SDL_Rect clip_;              /**<
		                              * The clipping area of the source if
		                              * ommitted the entire source is used.
		                              */
		drawing_buffer_key key_;
	};

	typedef std::list<tblit> tdrawing_buffer;
	tdrawing_buffer drawing_buffer_;

public:

	/**
	 * Add an item to the drawing buffer. You need to update screen on affected area
	 *
	 * @param layer              The layer to draw on.
	 * @param loc                The hex the image belongs to, needed for the
	 *                           drawing order.
	 * @param blit               The structure to blit.
	 */
	void drawing_buffer_add(const tdrawing_layer layer,
			const map_location& loc, int x, int y, const surface& surf,
			const SDL_Rect &clip = SDL_Rect());

	void drawing_buffer_add(const tdrawing_layer layer,
			const map_location& loc, int x, int y,
			const std::vector<surface> &surf,
			const SDL_Rect &clip = SDL_Rect());

protected:

	/** Draws the drawing_buffer_ and clears it. */
	void drawing_buffer_commit();

	/** Clears the drawing buffer. */
	void drawing_buffer_clear();

	/** redraw all panels associated with the map display */
	void draw_all_panels();

	/**
	 * Initiate a redraw.
	 *
	 * Invalidate controls and panels when changed after they have been drawn
	 * initially. Useful for dynamic theme modification.
	 */
	void draw_init();
	void draw_wrap(bool update,bool force);

	/** Used to indicate to drawing functions that we are doing a map screenshot */
	bool map_screenshot_;

public: //operations for the arrow framework

	void add_arrow(arrow&);

	void remove_arrow(arrow&);

	/** Called by arrow objects when they change. You should not need to call this directly. */
	void update_arrow(arrow & a);

private:
	/** Handle for the label which displays frames per second. */
	int fps_handle_;
	/** Count work done for the debug info displayed under fps */
	int invalidated_hexes_;
	int drawn_hexes_;

	bool idle_anim_;
	double idle_anim_rate_;

	surface map_screenshot_surf_;

	std::vector<boost::function<void(display&)> > redraw_observers_;

	/** Debug flag - overlay x,y coords on tiles */
	bool draw_coordinates_;
	/** Debug flag - overlay terrain codes on tiles */
	bool draw_terrain_codes_;

	typedef std::list<arrow*> arrows_list_t;
	typedef std::map<map_location, arrows_list_t > arrows_map_t;
	/** Maps the list of arrows for each location */
	arrows_map_t arrows_map_;

	tod_color color_adjust_;

#if defined(__GLIBC__)
	/** Flag for bug #17573 - this is set in the constructor **/
	bool do_reverse_memcpy_workaround_;
#endif
};

#endif

