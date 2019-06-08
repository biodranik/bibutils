/*
 * copacin.c
 *
 * Copyright (c) Chris Putnam 2004-2018
 *
 * Program and source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "is_ws.h"
#include "str.h"
#include "str_conv.h"
#include "slist.h"
#include "name.h"
#include "fields.h"
#include "reftypes.h"
#include "bibformats.h"
#include "generic.h"

extern variants copac_all[];
extern int copac_nall;

/*****************************************************
 PUBLIC: void copacin_initparams()
*****************************************************/

static int copacin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, str *line, str *reference, int *fcharset );
static int copacin_processf( fields *bibin, char *p, char *filename, long nref, param *pm );
static int copacin_convertf( fields *bibin, fields *info, int reftype, param *pm );

void
copacin_initparams( param *p, const char *progname )
{
	p->readformat       = BIBL_COPACIN;
	p->charsetin        = BIBL_CHARSET_DEFAULT;
	p->charsetin_src    = BIBL_SRC_DEFAULT;
	p->latexin          = 0;
	p->xmlin            = 0;
	p->utf8in           = 0;
	p->nosplittitle     = 0;
	p->verbose          = 0;
	p->addcount         = 0;
	p->output_raw       = 0;

	p->readf    = copacin_readf;
	p->processf = copacin_processf;
	p->cleanf   = NULL;
	p->typef    = NULL;
	p->convertf = copacin_convertf;
	p->all      = copac_all;
	p->nall     = copac_nall;

	slist_init( &(p->asis) );
	slist_init( &(p->corps) );

	if ( !progname ) p->progname = NULL;
	else p->progname = strdup( progname );
}

/*****************************************************
 PUBLIC: int copacin_readf()
*****************************************************/

/* Endnote-Refer/Copac tag definition:
    character 1 = alphabetic character
    character 2 = alphabetic character
    character 3 = dash
    character 4 = space
*/
static int
copacin_istag( char *buf )
{
	if (! ((buf[0]>='A' && buf[0]<='Z')) || (buf[0]>='a' && buf[0]<='z') )
		return 0;
	if (! ((buf[1]>='A' && buf[1]<='Z')) || (buf[1]>='a' && buf[1]<='z') )
		return 0;
	if (buf[2]!='-' ) return 0;
	if (buf[3]!=' ' ) return 0;
	return 1; 
}
static int
readmore( FILE *fp, char *buf, int bufsize, int *bufpos, str *line )
{
	if ( line->len ) return 1;
	else return str_fget( fp, buf, bufsize, bufpos, line );
}

static int
copacin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, str *line, str *reference, int *fcharset )
{
	int haveref = 0, inref=0;
	char *p;
	*fcharset = CHARSET_UNKNOWN;
	while ( !haveref && readmore( fp, buf, bufsize, bufpos, line ) ) {
		/* blank line separates */
		if ( line->data==NULL ) continue;
		if ( inref && line->len==0 ) haveref=1; 
		p = &(line->data[0]);
		/* Recognize UTF8 BOM */
		if ( line->len > 2 &&
				(unsigned char)(p[0])==0xEF &&
				(unsigned char)(p[1])==0xBB &&
				(unsigned char)(p[2])==0xBF ) {
			*fcharset = CHARSET_UNICODE;
			p += 3;
		}
		if ( copacin_istag( p ) ) {
			if ( inref ) str_addchar( reference, '\n' );
			str_strcatc( reference, p );
			str_empty( line );
			inref = 1;
		} else if ( inref ) {
			if ( p ) {
				/* copac puts tag only on 1st line */
				str_addchar( reference, ' ' );
				if ( *p ) p++;
				if ( *p ) p++;
				if ( *p ) p++;
				str_strcatc( reference, p );
			}
			str_empty( line );
		} else {
			str_empty( line );
		}
	}
	return haveref;
}

/*****************************************************
 PUBLIC: int copacin_processf()
*****************************************************/

static char*
copacin_addtag2( char *p, str *tag, str *data )
{
	int  i;
	i =0;
	while ( i<3 && *p ) {
		str_addchar( tag, *p++ );
		i++;
	}
	while ( *p==' ' || *p=='\t' ) p++;
	while ( *p && *p!='\r' && *p!='\n' ) {
		str_addchar( data, *p );
		p++;
	}
	str_trimendingws( data );
	while ( *p=='\n' || *p=='\r' ) p++;
	return p;
}

static char *
copacin_nextline( char *p )
{
	while ( *p && *p!='\n' && *p!='\r') p++;
	while ( *p=='\n' || *p=='\r' ) p++;
	return p;
}

static int
copacin_processf( fields *copacin, char *p, char *filename, long nref, param *pm )
{
	str tag, data;
	int status;
	str_init( &tag );
	str_init( &data );
	while ( *p ) {
		p = skip_ws( p );
		if ( copacin_istag( p ) ) {
			p = copacin_addtag2( p, &tag, &data );
			/* don't add empty strings */
			if ( str_has_value( &tag ) && str_has_value( &data ) ) {
				status = fields_add( copacin, tag.data, data.data, 0 );
				if ( status!=FIELDS_OK ) return 0;
			}
			str_empty( &tag );
			str_empty( &data );
		}
		else p = copacin_nextline( p );
	}
	str_free( &tag );
	str_free( &data );
	return 1;
}

/*****************************************************
 PUBLIC: int copacin_convertf(), returns BIBL_OK or BIBL_ERR_MEMERR
*****************************************************/

/* copac names appear to always start with last name first, but don't
 * always seem to have a comma after the name
 *
 * editors seem to be stuck in as authors with the tag "[Editor]" in it
 */
static int
copacin_person( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	char *usetag = outtag, editor[]="EDITOR";
	int comma = 0, i, ok, status;
	str usename, *s;
	slist tokens;

	if ( slist_find( &(pm->asis),  invalue ) !=-1  ||
	     slist_find( &(pm->corps), invalue ) !=-1 ) {
		ok = name_add( bibout, outtag, invalue->data, level, &(pm->asis), &(pm->corps) );
		if ( ok ) return BIBL_OK;
		else return BIBL_ERR_MEMERR;
	}

	slist_init( &tokens );
	str_init( &usename );

	status = slist_tokenize( &tokens, invalue, " ", 1 );
	if ( status!=SLIST_OK ) return BIBL_ERR_MEMERR;

	for ( i=0; i<tokens.n; ++i ) {
		s = slist_str( &tokens, i );
		if ( !strcmp( str_cstr( s ), "[Editor]" ) ) {
			usetag = editor;
			str_empty( s );
		} else if ( s->len && s->data[s->len-1]==',' ) {
			comma++;
		}
	}

	if ( comma==0 && tokens.n ) {
		s = slist_str( &tokens, 0 );
		str_addchar( s, ',' );
	}

	for ( i=0; i<tokens.n; ++i ) {
		s = slist_str( &tokens, i );
		if ( str_is_empty( s ) ) continue;
		if ( i ) str_addchar( &usename, ' ' );
		str_strcat( &usename, s );
	}

	slist_free( &tokens );

	ok = name_add( bibout, usetag, str_cstr( &usename ), level, &(pm->asis), &(pm->corps) );

	str_free( &usename );

	if ( ok ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
}

static void
copacin_report_notag( param *p, char *tag )
{
	if ( p->verbose ) {
		if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
		fprintf( stderr, "Cannot find tag '%s'\n", tag );
	}
}

static int
copacin_convertf( fields *bibin, fields *bibout, int reftype, param *p )
{
	static int (*convertfns[NUM_REFTYPES])(fields *, int, str *, str *, int, param *, char *, fields *) = {
		[ 0 ... NUM_REFTYPES-1 ] = generic_null,
		[ SIMPLE       ] = generic_simple,
		[ TITLE        ] = generic_title,
		[ NOTES        ] = generic_notes,
		[ SERIALNO     ] = generic_serialno,
		[ PERSON       ] = copacin_person
	};

	int  process, level, i, nfields, status = BIBL_OK;
	str *intag, *invalue;
	char *outtag;

	nfields = fields_num( bibin );
	for ( i=0; i<nfields; ++i ) {

		intag = fields_tag( bibin, i, FIELDS_STRP );

		if ( !translate_oldtag( intag->data, reftype, p->all, p->nall, &process, &level, &outtag ) ) {
			copacin_report_notag( p, intag->data );
			continue;
		}

		invalue = fields_value( bibin, i, FIELDS_STRP );

		status = convertfns[ process ] ( bibin, i, intag, invalue, level, p, outtag, bibout );
		if ( status!=BIBL_OK ) return status;

	}

	return status;
}
