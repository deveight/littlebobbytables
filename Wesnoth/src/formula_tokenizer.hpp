/* $Id: formula_tokenizer.hpp 52533 2012-01-07 02:35:17Z shadowmaster $ */
/*
   Copyright (C) 2007 - 2012 by David White <dave.net>
   Part of the Silver Tree Project

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by or later.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef FORMULA_TOKENIZER_HPP_INCLUDED
#define FORMULA_TOKENIZER_HPP_INCLUDED

#include <string>

namespace formula_tokenizer
{

typedef std::string::const_iterator iterator;

enum TOKEN_TYPE { TOKEN_OPERATOR, TOKEN_STRING_LITERAL,
			TOKEN_IDENTIFIER, TOKEN_INTEGER, TOKEN_DECIMAL,
			TOKEN_LPARENS, TOKEN_RPARENS,
			TOKEN_LSQUARE, TOKEN_RSQUARE,
			TOKEN_COMMA, TOKEN_SEMICOLON,
			TOKEN_WHITESPACE, TOKEN_EOL, TOKEN_KEYWORD,
			TOKEN_COMMENT, TOKEN_POINTER  };

struct token {

	token() :
		type(TOKEN_COMMENT),
		begin(),
		end(),
		line_number(1),
		filename()
	{
	}

	token(iterator& i1, iterator i2, TOKEN_TYPE type) :
		type(type),
		begin(i1),
		end(i2),
		line_number(1),
		filename()
	{
	}

	TOKEN_TYPE type;
	iterator begin, end;
	int line_number;
	const std::string* filename;
};

token get_token(iterator& i1, iterator i2);

struct token_error
{
	token_error() : description_(), formula_() {}
	token_error(const std::string& dsc, const std::string& formula) : description_(dsc), formula_(formula) {}
	std::string description_;
	std::string formula_;
};

}

#endif
