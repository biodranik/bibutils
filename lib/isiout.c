/*
 * isiout.c
 *
 * Copyright (c) Chris Putnam 2008-2018
 *
 * Source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utf8.h"
#include "str.h"
#include "strsearch.h"
#include "fields.h"
#include "title.h"
#include "bibutils.h"
#include "bibformats.h"

static int  isiout_write( fields *info, FILE *fp, param *p, unsigned long refnum );
static void isiout_writeheader( FILE *outptr, param *p );

void
isiout_initparams( param *p, const char *progname )
{
	p->writeformat      = BIBL_ISIOUT;
	p->format_opts      = 0;
	p->charsetout       = BIBL_CHARSET_DEFAULT;
	p->charsetout_src   = BIBL_SRC_DEFAULT;
	p->latexout         = 0;
	p->utf8out          = BIBL_CHARSET_UTF8_DEFAULT;
	p->utf8bom          = BIBL_CHARSET_BOM_DEFAULT;
	p->xmlout           = BIBL_XMLOUT_FALSE;
	p->nosplittitle     = 0;
	p->verbose          = 0;
	p->addcount         = 0;
	p->singlerefperfile = 0;

	if ( p->charsetout == BIBL_CHARSET_UNICODE ) {
		p->utf8out = p->utf8bom = 1;
	}

	p->headerf = isiout_writeheader;
	p->footerf = NULL;
	p->writef  = isiout_write;
}

enum {
        TYPE_UNKNOWN = 0,
        TYPE_ARTICLE = 1,
        TYPE_INBOOK  = 2,
        TYPE_BOOK    = 3,
};

static int 
get_type( fields *in )
{
        int type = TYPE_UNKNOWN, i, n, level;
	char *tag, *value;
	n = fields_num( in );
        for ( i=0; i<n; ++i ) {
		tag = fields_tag( in, i, FIELDS_CHRP );
                if ( strcasecmp( tag, "GENRE:MARC" ) &&
		     strcasecmp( tag, "GENRE:BIBUTILS" ) &&
		     strcasecmp( tag, "GENRE:UNKNOWN" ) ) continue;
		value = fields_value( in, i, FIELDS_CHRP );
		level = fields_level( in, i );
                if ( !strcasecmp( value, "periodical" ) ||
                     !strcasecmp( value, "academic journal" ) ||
		     !strcasecmp( value, "journal article" ) ) {
                        type = TYPE_ARTICLE;
                } else if ( !strcasecmp( value, "book" ) ) {
                        if ( level==0 ) type=TYPE_BOOK;
                        else type=TYPE_INBOOK;
		} else if ( !strcasecmp( value, "book chapter" ) ) {
			type = TYPE_INBOOK;
                }
        }
        return type;
}

static void
append_type( int type, fields *out, int *status )
{
	int fstatus;
	char *s;

	if ( type==TYPE_ARTICLE ) s = "Journal";
	else if ( type==TYPE_INBOOK ) s = "Chapter";
	else if ( type==TYPE_BOOK ) s = "Book";
	else s = "Unknown";

	fstatus = fields_add( out, "PT", s, LEVEL_MAIN );
	if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
}

static void
append_titlecore( fields *in, char *isitag, int level, char *maintag, char *subtag, fields *out, int *status )
{
	str *mainttl = fields_findv( in, level, FIELDS_STRP, maintag );
	str *subttl  = fields_findv( in, level, FIELDS_STRP, subtag );
	str fullttl;
	int fstatus;

	str_init( &fullttl );
	title_combine( &fullttl, mainttl, subttl );

	if ( str_memerr( &fullttl ) ) {
		*status = BIBL_ERR_MEMERR;
		goto out;
	}

	if ( str_has_value( &fullttl ) ) {
		fstatus = fields_add( out, isitag, str_cstr( &fullttl ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
out:
	str_free( &fullttl );
}

static void
append_title( fields *in, char *isitag, int level, fields *out, int *status )
{
	append_titlecore( in, isitag, level, "TITLE", "SUBTITLE", out, status );
}

static void
append_abbrtitle( fields *in, char *isitag, int level, fields *out, int *status )
{
	append_titlecore( in, isitag, level, "SHORTTITLE", "SHORTSUBTITLE", out, status );
}

static void
append_keywords( fields *in, fields *out, int *status )
{
	vplist_index i;
	str keywords;
	int fstatus;
	vplist kw;

	str_init( &keywords );
	vplist_init( &kw );

	fields_findv_each( in, LEVEL_ANY, FIELDS_STRP, &kw, "KEYWORD" );
	if ( kw.n ) {
		for ( i=0; i<kw.n; ++i ) {
			if ( i>0 ) str_strcatc( &keywords, "; " );
			str_strcat( &keywords, (str *) vplist_get( &kw, i ) );
		}
		if ( str_memerr( &keywords ) ) { *status = BIBL_ERR_MEMERR; goto out; }
		fstatus = fields_add( out, "DE", str_cstr( &keywords ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) { *status = BIBL_ERR_MEMERR; goto out; }
	}
out:
	vplist_free( &kw );
	str_free( &keywords );
}

static void
process_person( str *person, char *name )
{
	str family, given, suffix;
	char *p = name;

	str_empty( person );

	strs_init( &family, &given, &suffix, NULL );

	while ( *p && *p!='|' )
		str_addchar( &family, *p++ );

	while ( *p=='|' && *(p+1)!='|' ) {
		p++;
		if ( *p!='|' ) str_addchar( &given, *p++ );
		while ( *p && *p!='|' ) p++;
	}

	if ( *p=='|' && *(p+1)=='|' ) {
		p += 2;
		while ( *p && *p!='|' ) str_addchar( &suffix, *p++ );
	}

	if ( str_has_value( &family ) ) str_strcat( person, &family );
	if ( str_has_value( &suffix ) ) {
		if ( str_has_value( &family ) ) str_strcatc( person, " " );
		str_strcat( person, &suffix );
	}
	if ( str_has_value( &given ) ) {
		if ( str_has_value( person ) ) str_strcatc( person, ", " );
		str_strcat( person, &given );
	}

	strs_free( &family, &given, &suffix, NULL );
}

static void
append_people( fields *f, char *tag, char *isitag, int level, fields *out, int *status )
{
	vplist_index i;
	vplist people;
	str person;
	int fstatus;

	str_init( &person );
	vplist_init( &people );

	fields_findv_each( f, level, FIELDS_CHRP, &people, tag );
	for ( i=0; i<people.n; ++i ) {
		process_person( &person, (char *)vplist_get( &people, i ) );
		if ( str_memerr( &person ) ) { *status = BIBL_ERR_MEMERR; goto out; }
		if ( i==0 ) fstatus = fields_add_can_dup( out, isitag, str_cstr( &person ), LEVEL_MAIN );
		else        fstatus = fields_add_can_dup( out, "  ",   str_cstr( &person ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) { *status = BIBL_ERR_MEMERR; goto out; }
	}

out:
	vplist_free( &people );
	str_free( &person );
}

static void
append_easy( fields *in, char *tag, char *isitag, int level, fields *out, int *status )
{
	char *value;
	int fstatus;

	value = fields_findv( in, level, FIELDS_CHRP, tag );
	if ( value ) {
		fstatus = fields_add( out, isitag, value, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
}

static void
append_easyall( fields *in, char *tag, char *isitag, int level, fields *out, int *status )
{
	vplist_index i;
	int fstatus;
	vplist a;

	vplist_init( &a );
	fields_findv_each( in, level, FIELDS_CHRP, &a, tag );
	for ( i=0; i<a.n; ++i ) {
		fstatus = fields_add( out, isitag, (char *) vplist_get( &a, i ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	vplist_free( &a );
}

static void
append_date( fields *in, fields *out, int *status )
{
	char *month, *year;
	int fstatus;

	month = fields_findv_firstof( in, LEVEL_ANY, FIELDS_CHRP, "PARTDATE:MONTH", "DATE:MONTH", NULL );
	if ( month ) {
		fstatus = fields_add( out, "PD", month, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	year  = fields_findv_firstof( in, LEVEL_ANY, FIELDS_CHRP, "PARTDATE:YEAR",  "DATE:YEAR",  NULL );
	if ( year ) {
		fstatus = fields_add( out, "PY", year, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
}

static int
append_data( fields *in, fields *out )
{
	int type, status = BIBL_OK;

	type = get_type( in );

	append_type   ( type, out, &status );
	append_people ( in, "AUTHOR",      "AU", LEVEL_MAIN, out, &status );
	append_easyall( in, "AUTHOR:CORP", "AU", LEVEL_MAIN, out, &status );
	append_easyall( in, "AUTHOR:ASIS", "AU", LEVEL_MAIN, out, &status );

	append_title  ( in, "TI", LEVEL_MAIN, out, &status );
	if ( type==TYPE_ARTICLE ) {
		append_title( in, "SO", LEVEL_HOST,   out, &status );
		append_abbrtitle( in, "JI", LEVEL_HOST, out, &status );
		append_title( in, "SE", LEVEL_SERIES, out, &status );
	} else if ( type==TYPE_INBOOK ) {
		append_title( in, "BT", LEVEL_HOST,   out, &status );
		append_title( in, "SE", LEVEL_SERIES, out, &status );
	} else { /* type==BOOK */
		append_title( in, "SE", LEVEL_HOST,   out, &status );
	}

	append_date( in, out, &status );

	append_easy( in, "PAGES:START",    "BP", LEVEL_ANY, out, &status );
	append_easy( in, "PAGES:STOP",     "EP", LEVEL_ANY, out, &status );
	append_easy( in, "ARTICLENUMBER",  "AR", LEVEL_ANY, out, &status );
	append_easy( in, "PAGES:TOTAL",    "PG", LEVEL_ANY, out, &status );

	append_easy( in, "VOLUME",         "VL", LEVEL_ANY, out, &status );
	append_easy( in, "ISSUE",          "IS", LEVEL_ANY, out, &status );
	append_easy( in, "NUMBER",         "IS", LEVEL_ANY, out, &status );
	append_easy( in, "DOI",            "DI", LEVEL_ANY, out, &status );
	append_easy( in, "ISIREFNUM",      "UT", LEVEL_ANY, out, &status );
	append_easy( in, "LANGUAGE",       "LA", LEVEL_ANY, out, &status );
	append_easy( in, "ISIDELIVERNUM",  "GA", LEVEL_ANY, out, &status );
	append_keywords( in, out, &status );
	append_easy( in, "ABSTRACT",       "AB", LEVEL_ANY, out, &status );
	append_easy( in, "TIMESCITED",     "TC", LEVEL_ANY, out, &status );
	append_easy( in, "NUMBERREFS",     "NR", LEVEL_ANY, out, &status );
	append_easy( in, "CITEDREFS",      "CR", LEVEL_ANY, out, &status );
	append_easy( in, "ADDRESS",        "PI", LEVEL_ANY, out, &status );

	return status;
}

static void
output_verbose( fields *f, const char *type, unsigned long refnum )
{
	char *tag, *value;
	int i, n, level;

	fprintf( stderr, "REF #%lu %s---\n", refnum+1, type );

	n = fields_num( f );
	for ( i=0; i<n; ++i ) {
		tag   = fields_tag( f, i, FIELDS_CHRP_NOUSE );
		value = fields_value( f, i, FIELDS_CHRP_NOUSE );
		level = fields_level( f, i );
		fprintf( stderr, "\t'%s'\t'%s'\t%d\n", tag, value, level );
	}

	fflush( stderr );
}

static void
output( FILE *fp, fields *out )
{
	int i;

	for ( i=0; i<out->n; ++i ) {
		fprintf( fp, "%s %s\n",
			( char * ) fields_tag  ( out, i, FIELDS_CHRP ),
			( char * ) fields_value( out, i, FIELDS_CHRP )
		);
	}
        fprintf( fp, "ER\n\n" );
        fflush( fp );
}

static int
isiout_write( fields *in, FILE *fp, param *p, unsigned long refnum )
{
	int status;
	fields out;

	fields_init( &out );

	if ( p->format_opts & BIBL_FORMAT_VERBOSE )
		output_verbose( in, "IN", refnum );

	status = append_data( in, &out );

	if ( status==BIBL_OK ) output( fp, &out );

	if ( p->format_opts & BIBL_FORMAT_VERBOSE )
		output_verbose( &out, "OUT", refnum );

	fields_free( &out );

	return status;
}

static void
isiout_writeheader( FILE *outptr, param *p )
{
	if ( p->utf8bom ) utf8_writebom( outptr );
}
