/* $Id: stage_unit_formulas.cpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2009 - 2012 by Yurii Chernyi <terraninfo@terraninfo.net>
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
 * Defines formula ai unit formulas stage
 * */


#include "stage_unit_formulas.hpp"
#include "ai.hpp"

#include "../../formula.hpp"
#include "../../formula_function.hpp"
#include "../../log.hpp"
#include "../../resources.hpp"
#include <boost/lexical_cast.hpp>

static lg::log_domain log_formula_ai("ai/stage/unit_formulas");
#define LOG_AI LOG_STREAM(info, log_formula_ai)
#define WRN_AI LOG_STREAM(warn, log_formula_ai)
#define ERR_AI LOG_STREAM(err, log_formula_ai)

namespace ai {

stage_unit_formulas::stage_unit_formulas(ai_context &context, const config &cfg, formula_ai &fai)
       	: stage(context,cfg), cfg_(cfg), fai_(fai)
{

}


stage_unit_formulas::~stage_unit_formulas()
{
}

bool stage_unit_formulas::do_play_stage()
{
	//execute units formulas first
	game_logic::unit_formula_set units_with_formulas;

	unit_map &units_ = *resources::units;

	for(unit_map::unit_iterator i = units_.begin() ; i != units_.end() ; ++i)
	{
		if (i->side() == get_side()) {
			if (i->has_formula() || i->has_loop_formula()) {
				int priority = 0;
				if (i->has_priority_formula()) {
					try {
						game_logic::const_formula_ptr priority_formula(fai_.create_optional_formula(i->get_priority_formula()));
						if (priority_formula) {
							game_logic::map_formula_callable callable(&fai_);
							callable.add_ref();
							callable.add("me", variant(new unit_callable(*i)));
							priority = (game_logic::formula::evaluate(priority_formula, callable)).as_int();
						} else {
							WRN_AI << "priority formula skipped, maybe it's empty or incorrect"<< std::endl;
						}
					} catch(game_logic::formula_error& e) {
						if(e.filename == "formula")
							e.line = 0;
						fai_.handle_exception( e, "Unit priority formula error for unit: '" + i->type_id() + "' standing at (" + str_cast(i->get_location().x + 1) + "," + str_cast(i->get_location().y + 1) + ")");

						priority = 0;
					} catch(type_error& e) {
						priority = 0;
						ERR_AI << "formula type error while evaluating unit priority formula  " << e.message << "\n";
					}
				}

				units_with_formulas.insert( game_logic::unit_formula_pair( i, priority ) );
			}
		}
        }

	for(game_logic::unit_formula_set::iterator pair_it = units_with_formulas.begin() ; pair_it != units_with_formulas.end() ; ++pair_it)
	{
		unit_map::iterator i = pair_it->first;

		if( i.valid() ) {

			if (i->has_formula()) {
				try {
					game_logic::const_formula_ptr formula(fai_.create_optional_formula(i->get_formula()));
					if (formula) {
						game_logic::map_formula_callable callable(&fai_);
						callable.add_ref();
						callable.add("me", variant(new unit_callable(*i)));
						fai_.make_action(formula, callable);
					} else {
						WRN_AI << "unit formula skipped, maybe it's empty or incorrect" << std::endl;
					}
				}
				catch(game_logic::formula_error& e) {
					if(e.filename == "formula") {
						e.line = 0;
					}
					fai_.handle_exception( e, "Unit formula error for unit: '" + i->type_id() + "' standing at (" + str_cast(i->get_location().x + 1) + "," + str_cast(i->get_location().y + 1) + ")");
				}
			}
		}

		if( i.valid() ) {
			if (i->has_loop_formula())
			{
				try {
					game_logic::const_formula_ptr loop_formula(fai_.create_optional_formula(i->get_loop_formula()));
					if (loop_formula) {
						game_logic::map_formula_callable callable(&fai_);
						callable.add_ref();
						callable.add("me", variant(new unit_callable(*i)));
						while ( !fai_.make_action(loop_formula, callable).is_empty() && i.valid() )
						{
						}
					} else {
						WRN_AI << "Loop formula skipped, maybe it's empty or incorrect" << std::endl;
					}
				} catch(game_logic::formula_error& e) {
					if (e.filename == "formula") {
						e.line = 0;
					}
					fai_.handle_exception( e, "Unit loop formula error for unit: '" + i->type_id() + "' standing at (" + str_cast(i->get_location().x + 1) + "," + str_cast(i->get_location().y + 1) + ")");
				}
			}
		}
	}
	return false;
}


void stage_unit_formulas::on_create()
{
	//we have no state on our own
}


config stage_unit_formulas::to_config() const
{
	config cfg = stage::to_config();
	//we have no state on our own
	return cfg;
}

} // end of namespace ai
