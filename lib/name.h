/*
 * name.h
 *
 * mangle names w/ and w/o commas
 *
 * Copyright (c) Chris Putnam 2004-2018
 *
 * Source code released under the GPL version 2
 *
 */
#ifndef NAME_H
#define NAME_H

#include "str.h"
#include "slist.h"
#include "fields.h"

extern int  name_add( fields *info, char *tag, char *q, int level, slist *asis, slist *corps );
extern void name_build_withcomma( str *s, char *p );
extern int  name_parse( str *outname, str *inname, slist *asis, slist *corps );
extern int  name_addsingleelement( fields *info, char *tag, char *name, int level, int corp );
extern int  name_addmultielement( fields *info, char *tag, slist *tokens, int begin, int end, int level );
extern int  name_findetal( slist *tokens );

#endif

