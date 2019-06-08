/*
 * slist.h
 *
 * version: 2017-11-14
 *
 * Copyright (c) Chris Putnam 2004-2018
 *
 * Source code released under the GPL version 2
 *
 */

#ifndef SLIST_H
#define SLIST_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "str.h"

#define SLIST_OK            (0)
#define SLIST_ERR_MEMERR   (-1)
#define SLIST_ERR_CANTOPEN (-2)
#define SLIST_ERR_BADPARAM (-3)

#define SLIST_CHR (0)
#define SLIST_STR (1)

typedef int slist_index;

typedef struct slist {
	slist_index n, max;
	int sorted;
	str *strs;
} slist;


void    slists_init( slist *a, ... );
void    slists_free( slist *a, ... );
void    slists_empty( slist *a, ... );


void    slist_init( slist *a );
int     slist_init_values ( slist *a, ... );
int     slist_init_valuesc( slist *a, ... );
void    slist_free( slist *a );
void    slist_empty( slist *a );

slist * slist_new( void );
void    slist_delete( slist * );
void    slist_deletev( void *v );

slist * slist_dup( slist *a );
int     slist_copy( slist *to, slist *from );
void    slist_swap( slist *a, slist_index n1, slist_index n2 );

str *   slist_addvp( slist *a, int mode, void *vp );
str *   slist_addc( slist *a, const char *value );
str *   slist_add( slist *a, str *value );

int     slist_addvp_all( slist *a, int mode, ... );
int     slist_addc_all( slist *a, ... );
int     slist_add_all( slist *a, ... );

str *   slist_addvp_unique( slist *a, int mode, void *vp );
str *   slist_addc_unique( slist *a, const char *value );
str *   slist_add_unique( slist *a, str *value );

int     slist_append( slist *a, slist *toadd );
int     slist_append_unique( slist *a, slist *toadd );

int     slist_remove( slist *a, slist_index n );

str *   slist_str( slist *a, slist_index n );
char *  slist_cstr( slist *a, slist_index n );

str *   slist_set( slist *a, slist_index n, str *s );
str *   slist_setc( slist *a, slist_index n, const char *s );

void    slist_sort( slist *a );

int     slist_find( slist *a, str *searchstr );
int     slist_findc( slist *a, const char *searchstr );
int     slist_findnocase( slist *a, str *searchstr );
int     slist_findnocasec( slist *a, const char *searchstr );
int     slist_wasfound( slist *a, slist_index n );
int     slist_wasnotfound( slist *a, slist_index n );

int     slist_match_entry( slist *a, slist_index n, const char *s );
void    slist_trimend( slist *a, slist_index n );

unsigned long slist_get_maxlen( slist *a );
void    slist_dump( slist *a, FILE *fp, int newline );

int     slist_fill( slist *a, const char *filename, unsigned char skip_blank_lines );
int     slist_fillfp( slist *a, FILE *fp, unsigned char skip_blank_lines );
int     slist_tokenize( slist *tokens, str *in, const char *delim, int merge_delim );
int     slist_tokenizec( slist *tokens, char *p, const char *delim, int merge_delim );

#endif
