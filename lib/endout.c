/*
 * endout.c
 *
 * Copyright (c) Chris Putnam 2004-2018
 *
 * Program and source code released under the GPL version 2
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
#include "name.h"
#include "title.h"
#include "url.h"
#include "bibformats.h"

static int  endout_write( fields *in, FILE *fp, param *p, unsigned long refnum );
static void endout_writeheader( FILE *outptr, param *p );


void
endout_initparams( param *p, const char *progname )
{
	p->writeformat      = BIBL_ENDNOTEOUT;
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

	p->headerf = endout_writeheader;
	p->footerf = NULL;
	p->writef  = endout_write;
}

enum {
	TYPE_UNKNOWN = 0,
	TYPE_GENERIC,                     /* Generic */
	TYPE_ARTWORK,                     /* Artwork */
	TYPE_AUDIOVISUAL,                 /* Audiovisual Material */
	TYPE_BILL,                        /* Bill */
	TYPE_BOOK,                        /* Book */
	TYPE_INBOOK,                      /* Book Section */
	TYPE_CASE,                        /* Case */
	TYPE_CHARTTABLE,                  /* Chart or Table */
	TYPE_CLASSICALWORK,               /* Classical Work */
	TYPE_PROGRAM,                     /* Computer Program */
	TYPE_INPROCEEDINGS,               /* Conference Paper */
	TYPE_PROCEEDINGS,                 /* Conference Proceedings */
	TYPE_EDITEDBOOK,                  /* Edited Book */
	TYPE_EQUATION,                    /* Equation */
	TYPE_ELECTRONICARTICLE,           /* Electronic Article */
	TYPE_ELECTRONICBOOK,              /* Electronic Book */
	TYPE_ELECTRONIC,                  /* Electronic Source */
	TYPE_FIGURE,                      /* Figure */
	TYPE_FILMBROADCAST,               /* Film or Broadcast */
	TYPE_GOVERNMENT,                  /* Government Document */
	TYPE_HEARING,                     /* Hearing */
	TYPE_ARTICLE,                     /* Journal Article */
	TYPE_LEGALRULE,                   /* Legal Rule/Regulation */
	TYPE_MAGARTICLE,                  /* Magazine Article */
	TYPE_MANUSCRIPT,                  /* Manuscript */
	TYPE_MAP,                         /* Map */
	TYPE_NEWSARTICLE,                 /* Newspaper Article */
	TYPE_ONLINEDATABASE,              /* Online Database */
	TYPE_ONLINEMULTIMEDIA,            /* Online Multimedia */
	TYPE_PATENT,                      /* Patent */
	TYPE_COMMUNICATION,               /* Personal Communication */
	TYPE_REPORT,                      /* Report */
	TYPE_STATUTE,                     /* Statute */
	TYPE_THESIS,                      /* Thesis */
	TYPE_MASTERSTHESIS,               /* Thesis */
	TYPE_PHDTHESIS,                   /* Thesis */
	TYPE_DIPLOMATHESIS,               /* Thesis */
	TYPE_DOCTORALTHESIS,              /* Thesis */
	TYPE_HABILITATIONTHESIS,          /* Thesis */
	TYPE_LICENTIATETHESIS,            /* Thesis */
	TYPE_UNPUBLISHED,                 /* Unpublished Work */
};

static void
write_type( FILE *fp, int type )
{
	switch( type ) {
	case TYPE_UNKNOWN:           fprintf( fp, "TYPE_UNKNOWN" );            break;
	case TYPE_GENERIC:           fprintf( fp, "TYPE_GENERIC" );            break;
	case TYPE_ARTWORK:           fprintf( fp, "TYPE_ARTWORK" );            break;
	case TYPE_AUDIOVISUAL:       fprintf( fp, "TYPE_AUDIOVISUAL" );        break;
	case TYPE_BILL:              fprintf( fp, "TYPE_BILL" );               break;
	case TYPE_BOOK:              fprintf( fp, "TYPE_BOOK" );               break;
	case TYPE_INBOOK:            fprintf( fp, "TYPE_INBOOK" );             break;
	case TYPE_CASE:              fprintf( fp, "TYPE_CASE" );               break;
	case TYPE_CHARTTABLE:        fprintf( fp, "TYPE_CHARITABLE" );         break;
	case TYPE_CLASSICALWORK:     fprintf( fp, "TYPE_CLASSICALWORK" );      break;
	case TYPE_PROGRAM:           fprintf( fp, "TYPE_PROGRAM" );            break;
	case TYPE_INPROCEEDINGS:     fprintf( fp, "TYPE_INPROCEEDINGS" );      break;
	case TYPE_PROCEEDINGS:       fprintf( fp, "TYPE_PROCEEDINGS" );        break;
	case TYPE_EDITEDBOOK:        fprintf( fp, "TYPE_EDITEDBOOK" );         break;
	case TYPE_EQUATION:          fprintf( fp, "TYPE_EQUATION" );           break;
	case TYPE_ELECTRONICARTICLE: fprintf( fp, "TYPE_ELECTRONICARTICLE" );  break;
	case TYPE_ELECTRONICBOOK:    fprintf( fp, "TYPE_ELECTRONICBOOK" );     break;
	case TYPE_ELECTRONIC:        fprintf( fp, "TYPE_ELECTRONIC" );         break;
	case TYPE_FIGURE:            fprintf( fp, "TYPE_FIGURE" );             break;
	case TYPE_FILMBROADCAST:     fprintf( fp, "TYPE_FILMBROADCAST" );      break;
	case TYPE_GOVERNMENT:        fprintf( fp, "TYPE_GOVERNMENT" );         break;
	case TYPE_HEARING:           fprintf( fp, "TYPE_HEARING" );            break;
	case TYPE_ARTICLE:           fprintf( fp, "TYPE_ARTICLE" );            break;
	case TYPE_LEGALRULE:         fprintf( fp, "TYPE_LEGALRULE" );          break;
	case TYPE_MAGARTICLE:        fprintf( fp, "TYPE_MAGARTICLE" );         break;
	case TYPE_MANUSCRIPT:        fprintf( fp, "TYPE_MANUSCRIPT" );         break;
	case TYPE_MAP:               fprintf( fp, "TYPE_MAP" );                break;
	case TYPE_NEWSARTICLE:       fprintf( fp, "TYPE_NEWSARTICLE" );        break;
	case TYPE_ONLINEDATABASE:    fprintf( fp, "TYPE_ONLINEDATABASE" );     break;
	case TYPE_ONLINEMULTIMEDIA:  fprintf( fp, "TYPE_ONLINEMULTIMEDIA" );   break;
	case TYPE_PATENT:            fprintf( fp, "TYPE_PATENT" );             break;
	case TYPE_COMMUNICATION:     fprintf( fp, "TYPE_COMMUNICATION" );      break;
	case TYPE_REPORT:            fprintf( fp, "TYPE_REPORT" );             break;
	case TYPE_STATUTE:           fprintf( fp, "TYPE_STATUTE" );            break;
	case TYPE_THESIS:            fprintf( fp, "TYPE_THESIS" );             break;
	case TYPE_MASTERSTHESIS:     fprintf( fp, "TYPE_MASTERSTHESIS" );      break;
	case TYPE_PHDTHESIS:         fprintf( fp, "TYPE_PHDTHESIS" );          break;
	case TYPE_DIPLOMATHESIS:     fprintf( fp, "TYPE_DIPLOMATHESIS" );      break;
	case TYPE_DOCTORALTHESIS:    fprintf( fp, "TYPE_DOCTORALTHESIS" );     break;
	case TYPE_HABILITATIONTHESIS:fprintf( fp, "TYPE_HABILITATIONTHESIS" ); break;
	case TYPE_UNPUBLISHED:       fprintf( fp, "TYPE_UNPUBLISHED" );        break;
	default:                     fprintf( fp, "Error - type not in enum" );break;
	}
}

typedef struct match_type {
	char *name;
	int type;
} match_type;

static int
get_type( fields *in, param *p, unsigned long refnum )
{
	/* Comment out TYPE_GENERIC entries as that is default, but
         * keep in source as record of mapping decision. */
	match_type match_genres[] = {
		/* MARC Authority elements */
		{ "art original",              TYPE_ARTWORK },
		{ "art reproduction",          TYPE_ARTWORK },
		{ "article",                   TYPE_ARTICLE },
		{ "atlas",                     TYPE_MAP },
		{ "autobiography",             TYPE_BOOK },
/*		{ "bibliography",              TYPE_GENERIC },*/
		{ "biography",                 TYPE_BOOK },
		{ "book",                      TYPE_BOOK },
/*		{ "calendar",                  TYPE_GENERIC },*/
/*		{ "catalog",                   TYPE_GENERIC },*/
		{ "chart",                     TYPE_CHARTTABLE },
/*		{ "comic or graphic novel",    TYPE_GENERIC },*/
/*		{ "comic strip",               TYPE_GENERIC },*/
		{ "conference publication",    TYPE_PROCEEDINGS },
		{ "database",                  TYPE_ONLINEDATABASE },
/*		{ "dictionary",                TYPE_GENERIC },*/
		{ "diorama",                   TYPE_ARTWORK },
/*		{ "directory",                 TYPE_GENERIC },*/
		{ "discography",               TYPE_AUDIOVISUAL },
/*		{ "drama",                     TYPE_GENERIC },*/
		{ "encyclopedia",              TYPE_BOOK },
/*		{ "essay",                     TYPE_GENERIC }, */
/*		{ "festschrift",               TYPE_GENERIC },*/
		{ "fiction",                   TYPE_BOOK },
		{ "filmography",               TYPE_FILMBROADCAST },
		{ "filmstrip",                 TYPE_FILMBROADCAST },
/*		{ "finding aid",               TYPE_GENERIC },*/
/*		{ "flash card",                TYPE_GENERIC },*/
		{ "folktale",                  TYPE_CLASSICALWORK },
		{ "font",                      TYPE_ELECTRONIC },
/*		{ "game",                      TYPE_GENERIC },*/
		{ "government publication",    TYPE_GOVERNMENT },
		{ "graphic",                   TYPE_FIGURE },
		{ "globe",                     TYPE_MAP },
/*		{ "handbook",                  TYPE_GENERIC },*/
		{ "history",                   TYPE_BOOK },
		{ "hymnal",                    TYPE_BOOK },
/*		{ "humor, satire",             TYPE_GENERIC },*/
/*		{ "index",                     TYPE_GENERIC },*/
/*		{ "instruction",               TYPE_GENERIC },*/
/*		{ "interview",                 TYPE_GENERIC },*/
		{ "issue",                     TYPE_ARTICLE },
		{ "journal",                   TYPE_ARTICLE },
/*		{ "kit",                       TYPE_GENERIC },*/
/*		{ "language instruction",      TYPE_GENERIC },*/
/*		{ "law report or digest",      TYPE_GENERIC },*/
/*		{ "legal article",             TYPE_GENERIC },*/
		{ "legal case and case notes", TYPE_CASE },
		{ "legislation",               TYPE_BILL },
		{ "letter",                    TYPE_COMMUNICATION },
		{ "loose-leaf",                TYPE_GENERIC },
		{ "map",                       TYPE_MAP },
/*		{ "memoir",                    TYPE_GENERIC },*/
/*		{ "microscope slide",          TYPE_GENERIC },*/
/*		{ "model",                     TYPE_GENERIC },*/
		{ "motion picture",            TYPE_AUDIOVISUAL },
		{ "multivolume monograph",     TYPE_BOOK },
		{ "newspaper",                 TYPE_NEWSARTICLE },
		{ "novel",                     TYPE_BOOK },
/*		{ "numeric data",              TYPE_GENERIC },*/
/*		{ "offprint",                  TYPE_GENERIC },*/
		{ "online system or service",  TYPE_ELECTRONIC },
		{ "patent",                    TYPE_PATENT },
		{ "periodical",                TYPE_MAGARTICLE },
		{ "picture",                   TYPE_ARTWORK },
/*		{ "poetry",                    TYPE_GENERIC },*/
		{ "programmed text",           TYPE_PROGRAM },
/*		{ "realia",                    TYPE_GENERIC },*/
		{ "rehearsal",                 TYPE_AUDIOVISUAL },
/*		{ "remote sensing image",      TYPE_GENERIC },*/
/*		{ "reporting",                 TYPE_GENERIC },*/
		{ "report",                    TYPE_REPORT },
/*		{ "review",                    TYPE_GENERIC },*/
/*		{ "script",                    TYPE_GENERIC },*/
/*		{ "series",                    TYPE_GENERIC },*/
/*		{ "short story",               TYPE_GENERIC },*/
/*		{ "slide",                     TYPE_GENERIC },*/
		{ "sound",                     TYPE_AUDIOVISUAL },
/*		{ "speech",                    TYPE_GENERIC },*/
/*		{ "standard or specification", TYPE_GENERIC },*/
/*		{ "statistics",                TYPE_GENERIC },*/
/*		{ "survey of literature",      TYPE_GENERIC },*/
		{ "technical drawing",         TYPE_ARTWORK },
		{ "technical report",          TYPE_REPORT },
		{ "thesis",                    TYPE_THESIS },
/*		{ "toy",                       TYPE_GENERIC },*/
/*		{ "transparency",              TYPE_GENERIC },*/
/*		{ "treaty",                    TYPE_GENERIC },*/
		{ "videorecording",            TYPE_AUDIOVISUAL },
		{ "web site",                  TYPE_ELECTRONIC },
		/* Non-MARC Authority elements */
		{ "academic journal",          TYPE_ARTICLE },
		{ "magazine",                  TYPE_MAGARTICLE },
		{ "hearing",                   TYPE_HEARING },
		{ "Ph.D. thesis",              TYPE_PHDTHESIS },
		{ "Masters thesis",            TYPE_MASTERSTHESIS },
		{ "Diploma thesis",            TYPE_DIPLOMATHESIS },
		{ "Doctoral thesis",           TYPE_DOCTORALTHESIS },
		{ "Habilitation thesis",       TYPE_HABILITATIONTHESIS },
		{ "Licentiate thesis",         TYPE_LICENTIATETHESIS },
		{ "communication",             TYPE_COMMUNICATION },
		{ "manuscript",                TYPE_MANUSCRIPT },
		{ "unpublished",               TYPE_UNPUBLISHED },
	};
	int nmatch_genres = sizeof( match_genres ) / sizeof( match_genres[0] );

	int i, j, n, maxlevel, type = TYPE_UNKNOWN;
	char *tag, *data;

	/* Determine type from genre information */
	for ( i=0; i<in->n; ++i ) {
		tag = fields_tag( in, i, FIELDS_CHRP );
		if ( strcasecmp( tag, "GENRE:MARC" )!=0 &&
		     strcasecmp( tag, "GENRE:BIBUTILS" )!=0 &&
		     strcasecmp( tag, "GENRE:UNKNOWN" )!=0 ) continue;
		data = fields_value( in, i, FIELDS_CHRP );
		for ( j=0; j<nmatch_genres; ++j ) {
			if ( !strcasecmp( data, match_genres[j].name ) ) {
				type = match_genres[j].type;
				fields_setused( in, i );
			}
		}
		if ( p->verbose ) {
			if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
			fprintf( stderr, "Type from tag '%s' data '%s': ", tag, data );
			write_type( stderr, type );
			fprintf( stderr, "\n" );
		}
		if ( type==TYPE_UNKNOWN ) {
			if ( !strcasecmp( data, "periodical" ) )
				type = TYPE_ARTICLE;
			else if ( !strcasecmp( data, "thesis" ) )
				type = TYPE_THESIS;
			else if ( !strcasecmp( data, "book" ) ) {
				if ( in->level[i]==0 ) type = TYPE_BOOK;
				else type = TYPE_INBOOK;
			}
			else if ( !strcasecmp( data, "collection" ) ) {
				if ( in->level[i]==0 ) type = TYPE_BOOK;
				else type = TYPE_INBOOK;
			}
			if ( type!=TYPE_UNKNOWN ) fields_setused( in, i );
		}
		/* the inbook type should be defined if 'book' in host */
		if ( type==TYPE_BOOK && in->level[i]>0 ) type = TYPE_INBOOK;
	}
	if ( p->verbose ) {
		if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
		fprintf( stderr, "Type from genre element: " );
		write_type( stderr, type );
		fprintf( stderr, "\n" );
	}

	/* Determine from resource information */
	if ( type==TYPE_UNKNOWN ) {
		for ( i=0; i<in->n; ++i ) {
			if ( strcasecmp( fields_tag( in, i, FIELDS_CHRP ), "RESOURCE" ) )
				continue;
			data = fields_value( in, i, FIELDS_CHRP );
			if ( !strcasecmp( data, "moving image" ) )
				type = TYPE_FILMBROADCAST;
			else if ( !strcasecmp( data, "software, multimedia" ) )
				type = TYPE_PROGRAM;
			if ( type!=TYPE_UNKNOWN ) fields_setused( in, i );
		}
		if ( p->verbose ) {
			if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
			fprintf( stderr, "Type from resource element: " );
			write_type( stderr, type );
			fprintf( stderr, "\n" );
		}
	}

	/* Determine from issuance information */
	if ( type==TYPE_UNKNOWN ) {
		for ( i=0; i<in->n; ++i ) {
			if ( strcasecmp( fields_tag( in, i, FIELDS_CHRP ), "ISSUANCE" ) )
				continue;
			data = fields_value( in, i, FIELDS_CHRP );
			if ( !strcasecmp( data, "monographic" ) ) {
				if ( in->level[i]==0 ) type = TYPE_BOOK;
				else type = TYPE_INBOOK;
			}
		}
		if ( p->verbose ) {
			if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
			fprintf( stderr, "Type from issuance element: " );
			write_type( stderr, type );
			fprintf( stderr, "\n" );
		}
	}

	/* default to generic or book chapter, depending on maxlevel */
	if ( type==TYPE_UNKNOWN ) {
		maxlevel = fields_maxlevel( in );
		if ( maxlevel > 0 ) type = TYPE_INBOOK;
		else {
			if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
			fprintf( stderr, "Cannot identify TYPE in reference %lu ", refnum+1 );
			n = fields_find( in, "REFNUM", LEVEL_ANY );
			if ( n!=FIELDS_NOTFOUND )
				fprintf( stderr, " %s", (char *) fields_value( in, n, FIELDS_CHRP ) );
			fprintf( stderr, " (defaulting to generic)\n" );
			type = TYPE_GENERIC;
		}
	}

	if ( p->verbose ) {
		if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
		fprintf( stderr, "Final type: " );
		write_type( stderr, type );
		fprintf( stderr, "\n" );
	}
	
	return type;
}

static void
append_type( int type, fields *out, param *p, int *status )
{
	/* These are restricted to Endnote-defined types */
	match_type genrenames[] = {
		{ "Generic",                TYPE_GENERIC },
		{ "Artwork",                TYPE_ARTWORK },
		{ "Audiovisual Material",   TYPE_AUDIOVISUAL },
		{ "Bill",                   TYPE_BILL },
		{ "Book",                   TYPE_BOOK },
		{ "Book Section",           TYPE_INBOOK },
		{ "Case",                   TYPE_CASE },
		{ "Chart or Table",         TYPE_CHARTTABLE },
		{ "Classical Work",         TYPE_CLASSICALWORK },
		{ "Computer Program",       TYPE_PROGRAM },
		{ "Conference Paper",       TYPE_INPROCEEDINGS },
		{ "Conference Proceedings", TYPE_PROCEEDINGS },
		{ "Edited Book",            TYPE_EDITEDBOOK },
		{ "Equation",               TYPE_EQUATION },
		{ "Electronic Article",     TYPE_ELECTRONICARTICLE },
		{ "Electronic Book",        TYPE_ELECTRONICBOOK },
		{ "Electronic Source",      TYPE_ELECTRONIC },
		{ "Figure",                 TYPE_FIGURE },
		{ "Film or Broadcast",      TYPE_FILMBROADCAST },
		{ "Government Document",    TYPE_GOVERNMENT },
		{ "Hearing",                TYPE_HEARING },
		{ "Journal Article",        TYPE_ARTICLE },
		{ "Legal Rule/Regulation",  TYPE_LEGALRULE },
		{ "Magazine Article",       TYPE_MAGARTICLE },
		{ "Manuscript",             TYPE_MANUSCRIPT },
		{ "Map",                    TYPE_MAP },
		{ "Newspaper Article",      TYPE_NEWSARTICLE },
		{ "Online Database",        TYPE_ONLINEDATABASE },
		{ "Online Multimedia",      TYPE_ONLINEMULTIMEDIA },
		{ "Patent",                 TYPE_PATENT },
		{ "Personal Communication", TYPE_COMMUNICATION },
		{ "Report",                 TYPE_REPORT },
		{ "Statute",                TYPE_STATUTE },
		{ "Thesis",                 TYPE_THESIS }, 
		{ "Thesis",                 TYPE_PHDTHESIS },
		{ "Thesis",                 TYPE_MASTERSTHESIS },
		{ "Thesis",                 TYPE_DIPLOMATHESIS },
		{ "Thesis",                 TYPE_DOCTORALTHESIS },
		{ "Thesis",                 TYPE_HABILITATIONTHESIS },
		{ "Unpublished Work",       TYPE_UNPUBLISHED },
	};
	int ngenrenames = sizeof( genrenames ) / sizeof( genrenames[0] );
	int i, fstatus, found = 0;
	for ( i=0; i<ngenrenames && !found; ++i ) {
		if ( genrenames[i].type == type ) {
			fstatus = fields_add( out, "%0", genrenames[i].name, LEVEL_MAIN );
			if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
			found = 1;
		}
	}
	if ( !found ) {
		fstatus = fields_add( out, "%0", "Generic", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
		if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
		fprintf( stderr, "Cannot identify type %d\n", type );
	}
}

static int
append_title( fields *in, char *full, char *sub, char *endtag,
		int level, fields *out, int *status )
{
	str *mainttl = fields_findv( in, level, FIELDS_STRP, full );
	str *subttl  = fields_findv( in, level, FIELDS_STRP, sub );
	str fullttl;
	int fstatus;

	str_init( &fullttl );
	title_combine( &fullttl, mainttl, subttl );

	if ( str_memerr( &fullttl ) ) {
		*status = BIBL_ERR_MEMERR;
		goto out;
	}

	if ( str_has_value( &fullttl ) ) {
		fstatus = fields_add( out, endtag, str_cstr( &fullttl ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
out:
	str_free( &fullttl );
	return 1;
}

static void
append_people( fields *in, char *tag, char *entag, int level, fields *out, int *status )
{
	int i, n, flvl, fstatus;
	str oneperson;
	char *ftag;

	str_init( &oneperson );
	n = fields_num( in );
	for ( i=0; i<n; ++i ) {
		flvl = fields_level( in, i );
		if ( level!=LEVEL_ANY && flvl!=level ) continue;
		ftag = fields_tag( in, i, FIELDS_CHRP );
		if ( !strcasecmp( ftag, tag ) ) {
			name_build_withcomma( &oneperson, fields_value( in, i, FIELDS_CHRP ) );
			fstatus = fields_add_can_dup( out, entag, str_cstr( &oneperson ), LEVEL_MAIN );
			if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
		}
	}
	str_free( &oneperson );
}

static void
append_pages( fields *in, fields *out, int *status )
{
	str *sn, *en;
	int fstatus;
	str pages;
	char *ar;

	sn = fields_findv( in, LEVEL_ANY, FIELDS_STRP, "PAGES:START" );
	en = fields_findv( in, LEVEL_ANY, FIELDS_STRP, "PAGES:STOP" );
	if ( sn || en ) {
		str_init( &pages );
		if ( sn ) str_strcpy( &pages, sn );
		if ( sn && en ) str_strcatc( &pages, "-" );
		if ( en ) str_strcat( &pages, en );
		if ( str_memerr( &pages ) ) { *status = BIBL_ERR_MEMERR; str_free( &pages ); return; }
		fstatus = fields_add( out, "%P", str_cstr( &pages ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
		str_free( &pages );
	} else {
		ar = fields_findv( in, LEVEL_ANY, FIELDS_CHRP, "ARTICLENUMBER" );
		if ( ar ) {
			fstatus = fields_add( out, "%P", ar, LEVEL_MAIN );
			if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
		}
	}
}

static void
append_urls( fields *in, fields *out, int *status )
{
	int lstatus;
	slist types;

	lstatus = slist_init_valuesc( &types, "URL", "DOI", "PMID", "PMC", "ARXIV", "JSTOR", "MRNUMBER", NULL );
	if ( lstatus!=SLIST_OK ) {
		*status = BIBL_ERR_MEMERR;
		return;
	}

	*status = urls_merge_and_add( in, LEVEL_ANY, out, "%U", LEVEL_MAIN, &types );

	slist_free( &types );
}

static void
append_year( fields *in, fields *out, int *status )
{
	int fstatus;
	char *year;

	year = fields_findv_firstof( in, LEVEL_ANY, FIELDS_CHRP, "DATE:YEAR", "PARTDATE:YEAR", NULL );
	if ( year ) {
		fstatus = fields_add( out, "%D", year, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
}

static void
append_monthday( fields *in, fields *out, int *status )
{
	char *months[12] = { "January", "February", "March", "April",
		"May", "June", "July", "August", "September", "October",
		"November", "December" };
	char *month, *day;
	int m, fstatus;
	str monday;

	str_init( &monday );
	month = fields_findv_firstof( in, LEVEL_ANY, FIELDS_CHRP, "DATE:MONTH", "PARTDATE:MONTH", NULL );
	day   = fields_findv_firstof( in, LEVEL_ANY, FIELDS_CHRP, "DATE:DAY",   "PARTDATE:DAY",   NULL );
	if ( month || day ) {
		if ( month ) {
			m = atoi( month );
			if ( m>0 && m<13 ) str_strcpyc( &monday, months[m-1] );
			else str_strcpyc( &monday, month );
		}
		if ( month && day ) str_strcatc( &monday, " " );
		if ( day ) str_strcatc( &monday, day );
		fstatus = fields_add( out, "%8", str_cstr( &monday ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	str_free( &monday );
}

static void
append_genrehint( int type, fields *out, vplist *a, int *status )
{
	vplist_index i;
	int fstatus;
	char *g;

	for ( i=0; i<a->n; ++i ) {
		g = ( char * ) vplist_get( a, i );
		if ( !strcmp( g, "journal article" ) && type==TYPE_ARTICLE ) continue;
		if ( !strcmp( g, "academic journal" ) && type==TYPE_ARTICLE ) continue;
		if ( !strcmp( g, "collection" ) && type==TYPE_INBOOK ) continue;
		if ( !strcmp( g, "television broadcast" ) && type==TYPE_FILMBROADCAST ) continue;
		if ( !strcmp( g, "electronic" ) && type==TYPE_PROGRAM ) continue;
		if ( !strcmp( g, "magazine" ) && type==TYPE_MAGARTICLE ) continue;
		if ( !strcmp( g, "miscellaneous" ) && type==TYPE_GENERIC ) continue;
		if ( !strcmp( g, "hearing" ) && type==TYPE_HEARING ) continue;
		if ( !strcmp( g, "communication" ) && type==TYPE_COMMUNICATION ) continue;
		if ( !strcmp( g, "report" ) && type==TYPE_REPORT ) continue;
		if ( !strcmp( g, "book chapter" ) && type==TYPE_INBOOK ) continue;
		fstatus = fields_add( out, "%9", g, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) {
			*status = BIBL_ERR_MEMERR;
			return;
		}
	}
}

static void
append_all_genrehint( int type, fields *in, fields *out, int *status )
{
	vplist a;

	vplist_init( &a );

	fields_findv_each( in, LEVEL_ANY, FIELDS_CHRP, &a, "GENRE:BIBUTILS" );
	append_genrehint( type, out, &a, status );

	vplist_empty( &a );

	fields_findv_each( in, LEVEL_ANY, FIELDS_CHRP, &a, "GENRE:UNKNOWN" );
	append_genrehint( type, out, &a, status );

	vplist_free( &a );
}

static void
append_thesishint( int type, fields *out, int *status )
{
	int fstatus;

	if ( type==TYPE_MASTERSTHESIS ) {
		fstatus = fields_add( out, "%9", "Masters thesis", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	else if ( type==TYPE_PHDTHESIS ) {
		fstatus = fields_add( out, "%9", "Ph.D. thesis", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	else if ( type==TYPE_DIPLOMATHESIS ) {
		fstatus = fields_add( out, "%9", "Diploma thesis", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	else if ( type==TYPE_DOCTORALTHESIS ) {
		fstatus = fields_add( out, "%9", "Doctoral thesis", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	else if ( type==TYPE_HABILITATIONTHESIS ) {
		fstatus = fields_add( out, "%9", "Habilitation thesis", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	else if ( type==TYPE_LICENTIATETHESIS ) {
		fstatus = fields_add( out, "%9", "Licentiate thesis", LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
}

static void
append_easyall( fields *in, char *tag, char *entag, int level, fields *out, int *status )
{
	vplist_index i;
	int fstatus;
	vplist a;
	vplist_init( &a );
	fields_findv_each( in, level, FIELDS_CHRP, &a, tag );
	for ( i=0; i<a.n; ++i ) {
		fstatus = fields_add( out, entag, (char *) vplist_get( &a, i ), LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
	vplist_free( &a );
}

static void
append_easy( fields *in, char *tag, char *entag, int level, fields *out, int *status )
{
	char *value;
	int fstatus;

	value = fields_findv( in, level, FIELDS_CHRP, tag );
	if ( value ) {
		fstatus = fields_add( out, entag, value, LEVEL_MAIN );
		if ( fstatus!=FIELDS_OK ) *status = BIBL_ERR_MEMERR;
	}
}

static int
append_data( fields *in, fields *out, param *p, unsigned long refnum )
{
	int added, type, status = BIBL_OK;

	fields_clearused( in );

	type = get_type( in, p, refnum );

	append_type( type, out, p, &status );

	added = append_title( in, "TITLE",      "SUBTITLE",      "%T", LEVEL_MAIN, out, &status );
	if ( added==0 ) append_title( in, "SHORTTITLE", "SHORTSUBTITLE", "%T", LEVEL_MAIN, out, &status );
	else            append_title( in, "SHORTTITLE", "SHORTSUBTITLE", "%!", LEVEL_MAIN, out, &status );

	append_people( in, "AUTHOR",     "%A", LEVEL_MAIN, out, &status );
	append_people( in, "EDITOR",     "%E", LEVEL_MAIN, out, &status );
	if ( type==TYPE_ARTICLE || type==TYPE_MAGARTICLE || type==TYPE_ELECTRONICARTICLE || type==TYPE_NEWSARTICLE )
		append_people( in, "EDITOR", "%E", LEVEL_HOST, out, &status );
	else if ( type==TYPE_INBOOK || type==TYPE_INPROCEEDINGS ) {
		append_people( in, "EDITOR", "%E", LEVEL_HOST, out, &status );
	} else {
		append_people( in, "EDITOR", "%Y", LEVEL_HOST, out, &status );
	}
	append_people( in, "TRANSLATOR", "%H", LEVEL_ANY,    out, &status  );

	append_people( in, "AUTHOR",     "%Y", LEVEL_SERIES, out, &status );
	append_people( in, "EDITOR",     "%Y", LEVEL_SERIES, out, &status );

	if ( type==TYPE_CASE ) {
		append_easy(    in, "AUTHOR:CORP", "%I", LEVEL_MAIN, out, &status );
		append_easy(    in, "AUTHOR:ASIS", "%I", LEVEL_MAIN, out, &status );
	}
	else if ( type==TYPE_HEARING ) {
		append_easyall( in, "AUTHOR:CORP", "%S", LEVEL_MAIN, out, &status );
		append_easyall( in, "AUTHOR:ASIS", "%S", LEVEL_MAIN, out, &status );
	}
	else if ( type==TYPE_NEWSARTICLE ) {
		append_people(  in, "REPORTER",        "%A", LEVEL_MAIN, out, &status );
		append_people(  in, "REPORTER:CORP",   "%A", LEVEL_MAIN, out, &status );
		append_people(  in, "REPORTER:ASIS",   "%A", LEVEL_MAIN, out, &status );
	}
	else if ( type==TYPE_COMMUNICATION ) {
		append_people(  in, "ADDRESSEE",       "%E", LEVEL_ANY,  out, &status  );
		append_people(  in, "ADDRESSEE:CORP",  "%E", LEVEL_ANY,  out, &status  );
		append_people(  in, "ADDRESSEE:ASIS",  "%E", LEVEL_ANY,  out, &status  );
	}
	else {
		append_easyall( in, "AUTHOR:CORP",     "%A", LEVEL_MAIN, out, &status );
		append_easyall( in, "AUTHOR:ASIS",     "%A", LEVEL_MAIN, out, &status );
		append_easyall( in, "EDITOR:CORP",     "%E", LEVEL_ANY,  out, &status  );
		append_easyall( in, "EDITOR:ASIS",     "%E", LEVEL_ANY,  out, &status  );
		append_easyall( in, "TRANSLATOR:CORP", "%H", LEVEL_ANY,  out, &status  );
		append_easyall( in, "TRANSLATOR:ASIS", "%H", LEVEL_ANY,  out, &status  );
	}

	if ( type==TYPE_ARTICLE || type==TYPE_MAGARTICLE || type==TYPE_ELECTRONICARTICLE || type==TYPE_NEWSARTICLE ) {
		added = append_title( in, "TITLE", "SUBTITLE", "%J", LEVEL_HOST, out, &status );
		if ( added==0 ) append_title( in, "SHORTTITLE", "SHORTSUBTITLE", "%J", LEVEL_HOST, out, &status );
	}

	else if ( type==TYPE_INBOOK || type==TYPE_INPROCEEDINGS ) {
		added = append_title( in, "TITLE", "SUBTITLE", "%B", LEVEL_HOST, out, &status );
		if ( added==0 ) append_title( in, "SHORTTITLE", "SHORTSUBTITLE", "%B", LEVEL_HOST, out, &status );
	}

	else {
		added = append_title( in, "TITLE", "SUBTITLE", "%S", LEVEL_HOST, out, &status );
		if ( added==0 ) append_title( in, "SHORTTITLE", "SHORTSUBTITLE", "%S", LEVEL_HOST, out, &status );
	}

	if ( type!=TYPE_CASE && type!=TYPE_HEARING ) {
		append_title( in, "TITLE", "SUBTITLE", "%S", LEVEL_SERIES, out, &status );
	}

	append_year    ( in, out, &status );
	append_monthday( in, out, &status );

	append_easy    ( in, "VOLUME",             "%V", LEVEL_ANY, out, &status );
	append_easy    ( in, "ISSUE",              "%N", LEVEL_ANY, out, &status );
	append_easy    ( in, "NUMBER",             "%N", LEVEL_ANY, out, &status );
	append_easy    ( in, "EDITION",            "%7", LEVEL_ANY, out, &status );
	append_easy    ( in, "PUBLISHER",          "%I", LEVEL_ANY, out, &status );
	append_easy    ( in, "ADDRESS",            "%C", LEVEL_ANY, out, &status );
	append_easy    ( in, "DEGREEGRANTOR",      "%C", LEVEL_ANY, out, &status );
	append_easy    ( in, "DEGREEGRANTOR:CORP", "%C", LEVEL_ANY, out, &status );
	append_easy    ( in, "DEGREEGRANTOR:ASIS", "%C", LEVEL_ANY, out, &status );
	append_easy    ( in, "SERIALNUMBER",       "%@", LEVEL_ANY, out, &status );
	append_easy    ( in, "ISSN",               "%@", LEVEL_ANY, out, &status );
	append_easy    ( in, "ISBN",               "%@", LEVEL_ANY, out, &status );
	append_easy    ( in, "LANGUAGE",           "%G", LEVEL_ANY, out, &status );
	append_easy    ( in, "REFNUM",             "%F", LEVEL_ANY, out, &status );
	append_easyall ( in, "NOTES",              "%O", LEVEL_ANY, out, &status );
	append_easy    ( in, "ABSTRACT",           "%X", LEVEL_ANY, out, &status );
	append_easy    ( in, "CLASSIFICATION"   ,  "%L", LEVEL_ANY, out, &status );
	append_easyall ( in, "KEYWORD",            "%K", LEVEL_ANY, out, &status );
	append_all_genrehint(  type, in, out, &status );
	append_thesishint( type, out, &status );
	append_easyall ( in, "DOI",                "%R", LEVEL_ANY, out, &status );
	append_easyall ( in, "URL",                "%U", LEVEL_ANY, out, &status );
	append_easyall ( in, "FILEATTACH",         "%U", LEVEL_ANY, out, &status );
	append_urls    ( in, out, &status );
	append_pages   ( in, out, &status );

	return status;
}

static void
output( FILE *fp, fields *out )
{
	int i;

	for ( i=0; i<out->n; ++i ) {
		fprintf( fp, "%s %s\n",
			(char*) fields_tag( out, i, FIELDS_CHRP ),
			(char*) fields_value( out, i, FIELDS_CHRP )
		);
	}

	fprintf( fp, "\n" );
	fflush( fp );
}

static int
endout_write( fields *in, FILE *fp, param *p, unsigned long refnum )
{
	int status;
	fields out;

	fields_init( &out );
	status = append_data( in, &out, p, refnum );
	if ( status==BIBL_OK ) output( fp, &out );
	fields_free( &out );

	return status;
}

static void
endout_writeheader( FILE *outptr, param *p )
{
	if ( p->utf8bom ) utf8_writebom( outptr );
}

