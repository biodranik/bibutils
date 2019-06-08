/*
 * isiin.c
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
#include "name.h"
#include "fields.h"
#include "reftypes.h"
#include "bibformats.h"
#include "generic.h"

extern variants isi_all[];
extern int isi_nall;

static int isiin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, str *line, str *reference, int *fcharset );
static int isiin_typef( fields *isiin, char *filename, int nref, param *p );
static int isiin_convertf( fields *isiin, fields *info, int reftype, param *p );
static int isiin_processf( fields *isiin, char *p, char *filename, long nref, param *pm );


/*****************************************************
 PUBLIC: void isiin_initparams()
*****************************************************/
void
isiin_initparams( param *p, const char *progname )
{
	p->readformat       = BIBL_ISIIN;
	p->charsetin        = BIBL_CHARSET_DEFAULT;
	p->charsetin_src    = BIBL_SRC_DEFAULT;
	p->latexin          = 0;
	p->xmlin            = 0;
	p->utf8in           = 0;
	p->nosplittitle     = 0;
	p->verbose          = 0;
	p->addcount         = 0;
	p->output_raw       = 0;

	p->readf    = isiin_readf;
	p->processf = isiin_processf;
	p->cleanf   = NULL;
	p->typef    = isiin_typef;
	p->convertf = isiin_convertf;
	p->all      = isi_all;
	p->nall     = isi_nall;

	slist_init( &(p->asis) );
	slist_init( &(p->corps) );

	if ( !progname ) p->progname = NULL;
	else p->progname = strdup( progname );
}

/*****************************************************
 PUBLIC: int isiin_readf()
*****************************************************/

/* ISI definition of a tag is strict:
 *   char 1 = uppercase alphabetic character
 *   char 2 = uppercase alphabetic character or digit
 */
static int
is_isi_tag( char *buf )
{
	if ( ! (buf[0]>='A' && buf[0]<='Z') ) return 0;
	if ( ! (((buf[1]>='A' && buf[1]<='Z'))||(buf[1]>='0'&&buf[1]<='9')))
		return 0;
	return 1;
}

static int
readmore( FILE *fp, char *buf, int bufsize, int *bufpos, str *line )
{
	if ( line->len ) return 1;
	else return str_fget( fp, buf, bufsize, bufpos, line );
}

static int
isiin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, str *line, str *reference, int *fcharset )
{
	int haveref = 0, inref = 0;
	char *p;
	*fcharset = CHARSET_UNKNOWN;
	while ( !haveref && readmore( fp, buf, bufsize, bufpos, line ) ) {
		if ( !line->data ) continue;
		p = &(line->data[0]);
		/* Recognize UTF8 BOM */
		if ( line->len > 2 &&
				(unsigned char)(p[0])==0xEF &&
				(unsigned char)(p[1])==0xBB &&
				(unsigned char)(p[2])==0xBF ) {
			*fcharset = CHARSET_UNICODE;
			p += 3;
		}
		/* Each reference ends with 'ER ' */
		if ( is_isi_tag( p ) ) {
			if ( !strncmp( p, "FN ", 3 ) ) {
				if (strncasecmp( p, "FN ISI Export Format",20)){
					fprintf( stderr, ": warning file FN type not '%s' not recognized.\n", /*r->progname,*/ p );
				}
			} else if ( !strncmp( p, "VR ", 3 ) ) {
				if ( strncasecmp( p, "VR 1.0", 6 ) ) {
					fprintf(stderr,": warning file version number '%s' not recognized, expected 'VR 1.0'\n", /*r->progname,*/ p );
				}
			} else if ( !strncmp( p, "ER", 2 ) ) haveref = 1;
			else {
				str_addchar( reference, '\n' );
				str_strcatc( reference, p );
				inref = 1;
			}
			str_empty( line );
		}
		/* not a tag, but we'll append to the last values */
		else if ( inref ) {
			str_addchar( reference, '\n' );
			str_strcatc( reference, p );
			str_empty( line );
		}
		else {
			str_empty( line );
		}
	}
	return haveref;
}

/*****************************************************
 PUBLIC: int isiin_processf()
*****************************************************/

static char *
process_tagged_line( str *tag, str *data, char *p )
{
	int i;

	/* collect tag and skip past it */
	i = 0;
	while ( i<2 && *p && *p!='\r' && *p!='\n') {
		str_addchar( tag, *p++ );
		i++;
	}
	while ( *p==' ' || *p=='\t' ) p++;
	while ( *p && *p!='\r' && *p!='\n' )
		str_addchar( data, *p++ );
	str_trimendingws( data );
	while ( *p=='\r' || *p=='\n' ) p++;
	return p;
}

static char *
process_untagged_line( str *data, char *p )
{
	while ( *p==' ' || *p=='\t' ) p++;
	while ( *p && *p!='\r' && *p!='\n' )
		str_addchar( data, *p++ );
	str_trimendingws( data );
	while ( *p=='\r' || *p=='\n' ) p++;
	return p;
}

static int
add_tag_value( fields *isiin, str *tag, str *value, int *tag_added )
{
	int status;

	if ( str_has_value( tag ) && str_has_value( value ) ) {
		status = fields_add( isiin, str_cstr( tag ), str_cstr( value ), 0 );
		if ( status!=FIELDS_OK ) return BIBL_ERR_MEMERR;
		*tag_added = 1;
	}

	else {
		*tag_added = 0;
	}

	return BIBL_OK;
}

static int
merge_tag_value( fields *isiin, str *tag, str *value, int *tag_added )
{
	int n, status;

	if ( str_has_value( value ) ) {

		if ( *tag_added==1 ) {

			n = fields_num( isiin );
			if ( n==0 ) return BIBL_OK;

			/* only one AU or AF for list of authors */
			if ( !strcmp( str_cstr( tag ), "AU" ) ) {
				status = fields_add( isiin, "AU", str_cstr( value ), 0 );
				if ( status!=FIELDS_OK ) return BIBL_ERR_MEMERR;
			} else if ( !strcmp( str_cstr( tag ), "AF" ) ) {
				status = fields_add( isiin, "AF", str_cstr( value ), 0 );
				if ( status!=FIELDS_OK ) return BIBL_ERR_MEMERR;
			}
			/* otherwise append multiline data */
			else {
				str_addchar( &(isiin->data[n-1]),' ');
				str_strcat( &(isiin->data[n-1]), value );
				if ( str_memerr( &(isiin->data[n-1]) ) ) return BIBL_ERR_MEMERR;
			}
		}

		else {
                        status = fields_add( isiin, str_cstr( tag ), str_cstr( value ), 0 );
                        if ( status!=FIELDS_OK ) return BIBL_ERR_MEMERR;
                        *tag_added = 1;
		}
	}

	return BIBL_OK;
}

static int
isiin_processf( fields *isiin, char *p, char *filename, long nref, param *pm )
{
	int status, tag_added = 0, ret = 1;
	str tag, value;

	strs_init( &tag, &value, NULL );

	while ( *p ) {

		/* ...with tag, add */
		if ( is_isi_tag( p ) ) {
			str_empty( &tag );
			str_empty( &value );
			p = process_tagged_line( &tag, &value, p );
			status = add_tag_value( isiin, &tag, &value, &tag_added );
			if ( status!=BIBL_OK ) {
				ret = 0;
				goto out;
			}
		}

		/* ...untagged, merge -- one AU or AF for list of authors */
		else {
			str_empty( &value );
			p = process_untagged_line( &value, p );
			status = merge_tag_value( isiin, &tag, &value, &tag_added );
			if ( status!=BIBL_OK ) {
				ret = 0;
				goto out;
			}
		}

	}
out:
	strs_free( &value, &tag, NULL );
	return ret;
}

/*****************************************************
 PUBLIC: int isiin_typef()
*****************************************************/
static int
isiin_typef( fields *isiin, char *filename, int nref, param *p )
{
	int ntypename, nrefname, is_default;
	char *refname = "", *typename="";

	ntypename = fields_find( isiin, "PT", LEVEL_MAIN );
	nrefname  = fields_find( isiin, "UT", LEVEL_MAIN );

	if ( nrefname!=FIELDS_NOTFOUND )  refname  = fields_value( isiin, nrefname,  FIELDS_CHRP_NOUSE );
	if ( ntypename!=FIELDS_NOTFOUND ) typename = fields_value( isiin, ntypename, FIELDS_CHRP_NOUSE );

	return get_reftype( typename, nref, p->progname, p->all, p->nall, refname, &is_default, REFTYPE_CHATTY );
}

/*****************************************************
 PUBLIC: int isiin_convertf(), returns BIBL_OK or BIBL_ERR_MEMERR
*****************************************************/

/* pull off authors first--use AF before AU */
static int
isiin_addauthors( fields *isiin, fields *info, int reftype, variants *all, int nall, slist *asis, slist *corps )
{
	char *newtag, *authortype, use_af[]="AF", use_au[]="AU";
	int level, i, n, has_af=0, has_au=0, nfields, ok;
	str *t, *d;

	nfields = fields_num( isiin );
	for ( i=0; i<nfields && has_af==0; ++i ) {
		t = fields_tag( isiin, i, FIELDS_STRP );
		if ( !strcasecmp( t->data, "AU" ) ) has_au++;
		if ( !strcasecmp( t->data, "AF" ) ) has_af++;
	}
	if ( has_af ) authortype = use_af;
	else if ( has_au ) authortype = use_au;
	else return BIBL_OK; /* no authors */

	for ( i=0; i<nfields; ++i ) {
		t = fields_tag( isiin, i, FIELDS_STRP );
		if ( strcasecmp( t->data, authortype ) ) continue;
		d = fields_value( isiin, i, FIELDS_STRP );
		n = process_findoldtag( authortype, reftype, all, nall );
		level = ((all[reftype]).tags[n]).level;
		newtag = all[reftype].tags[n].newstr;
		ok = name_add( info, newtag, d->data, level, asis, corps );
		if ( !ok ) return BIBL_ERR_MEMERR;
	}
	return BIBL_OK;
}

static int
isiin_keyword( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	int fstatus, status = BIBL_OK;
	char *p = invalue->data;
	str keyword;

	str_init( &keyword );
	while ( *p ) {
		p = str_cpytodelim( &keyword, skip_ws( p ), ";", 1 );
		if ( str_memerr( &keyword ) ) { status = BIBL_ERR_MEMERR; goto out; }
		if ( str_has_value( &keyword ) ) {
			fstatus = fields_add( bibout, outtag, keyword.data, level );
			if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }
		}
	}
out:
	str_free( &keyword );
	return status;
}

static void
isiin_report_notag( param *p, char *tag )
{
	if ( p->verbose && strcmp( tag, "PT" ) ) {
		if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
		fprintf( stderr, "Did not identify ISI tag '%s'\n", tag );
	}
}

static int
isiin_convertf( fields *bibin, fields *bibout, int reftype, param *p )
{
	static int (*convertfns[NUM_REFTYPES])(fields *, int, str *, str *, int, param *, char *, fields *) = {
		[ 0 ... NUM_REFTYPES-1 ] = generic_null,
		[ SIMPLE       ] = generic_simple,
		[ TITLE        ] = generic_title,
		[ PERSON       ] = generic_person,
		[ SERIALNO     ] = generic_serialno,
		[ DATE         ] = generic_simple,
		[ NOTES        ] = generic_notes,
		[ KEYWORD      ] = isiin_keyword,
	};

	int process, level, i, nfields, status;
	str *intag, *invalue;
	char *outtag;

	status = isiin_addauthors( bibin, bibout, reftype, p->all, p->nall, &(p->asis), &(p->corps) );
	if ( status!=BIBL_OK ) return status;

	nfields = fields_num( bibin );
	for ( i=0; i<nfields; ++i ) {

		intag = fields_tag( bibin, i, FIELDS_STRP );
		if ( !strcasecmp( str_cstr( intag ), "AU" ) || !strcasecmp( str_cstr( intag ), "AF" ) )
			continue;

		if ( !translate_oldtag( str_cstr( intag ), reftype, p->all, p->nall, &process, &level, &outtag ) ) {
			isiin_report_notag( p, str_cstr( intag ) );
			continue;
		}

		invalue = fields_value( bibin, i, FIELDS_STRP );

		status = convertfns[ process ] ( bibin, i, intag, invalue, level, p, outtag, bibout );
		if ( status!=BIBL_OK ) return status;
	}

	if ( status==BIBL_OK && p->verbose ) fields_report( bibout, stderr );

	return status;
}
