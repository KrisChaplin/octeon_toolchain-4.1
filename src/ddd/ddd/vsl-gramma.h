// This file was modified for -*- C++ -*-
// using $RCSfile: vsl-gramma.h,v $ $Revision: 1.1.1.1 $

/* A Bison parser, made by GNU Bison 1.875.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum vsltokentype {
     IDENTIFIER = 258,
     VSL_STRING = 259,
     INTEGER = 260,
     ARROW = 261,
     IF = 262,
     THEN = 263,
     ELSE = 264,
     ELSIF = 265,
     FI = 266,
     OR = 267,
     AND = 268,
     NOT = 269,
     LET = 270,
     IN = 271,
     WHERE = 272,
     OVERRIDE = 273,
     REPLACE = 274,
     EQ = 275,
     NE = 276,
     GT = 277,
     GE = 278,
     LT = 279,
     LE = 280,
     HALIGN = 281,
     VALIGN = 282,
     UALIGN = 283,
     TALIGN = 284,
     APPEND = 285,
     CONS = 286,
     THREEDOTS = 287
   };
#endif
#define IDENTIFIER 258
#define VSL_STRING 259
#define INTEGER 260
#define ARROW 261
#define IF 262
#define THEN 263
#define ELSE 264
#define ELSIF 265
#define FI 266
#define OR 267
#define AND 268
#define NOT 269
#define LET 270
#define IN 271
#define WHERE 272
#define OVERRIDE 273
#define REPLACE 274
#define EQ 275
#define NE 276
#define GT 277
#define GE 278
#define LT 279
#define LE 280
#define HALIGN 281
#define VALIGN 282
#define UALIGN 283
#define TALIGN 284
#define APPEND 285
#define CONS 286
#define THREEDOTS 287




#if ! defined (IGNORED_YYSTYPE) && ! defined (IGNORED_YYSTYPE_IS_DECLARED)
#line 145 "./vsl-gramma.Y"
typedef struct _IGNORED_YYSTYPE  {
    // Our special yacctoC program makes this a struct -- 
    // thus we use an anonymous union (does not harm in other cases)
    union {
	VSLNode *node;
	string *str;
	int num;
	double fnum;
	VSLFunctionHeader function_header;
	VSLVarDefinition  var_definition;
    };
} IGNORED_YYSTYPE;
/* Line 1240 of yacc.c.  */
#line 113 "y.tab.h"
# define IGNORED_vslstype IGNORED_YYSTYPE /* obsolescent; will be withdrawn */
# define IGNORED_YYSTYPE_IS_DECLARED 1
# define IGNORED_YYSTYPE_IS_TRIVIAL 1
#endif

extern IGNORED_YYSTYPE IGNORED_vsllval;



