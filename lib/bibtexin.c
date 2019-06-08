/*
 * bibtexin.c
 *
 * Copyright (c) Chris Putnam 2003-2018
 *
 * Program and source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "is_ws.h"
#include "str.h"
#include "utf8.h"
#include "str_conv.h"
#include "fields.h"
#include "slist.h"
#include "name.h"
#include "title.h"
#include "url.h"
#include "reftypes.h"
#include "bibformats.h"
#include "generic.h"

static slist find    = { 0, 0, 0, NULL };
static slist replace = { 0, 0, 0, NULL };

extern variants bibtex_all[];
extern int bibtex_nall;

/*****************************************************
 PUBLIC: void bibtexin_initparams()
*****************************************************/

static int  bibtexin_convertf( fields *bibin, fields *info, int reftype, param *p );
static int  bibtexin_processf( fields *bibin, char *data, char *filename, long nref, param *p );
static int  bibtexin_cleanf( bibl *bin, param *p );
static int  bibtexin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, str *line, str *reference, int *fcharset );
static int  bibtexin_typef( fields *bibin, char *filename, int nrefs, param *p );

void
bibtexin_initparams( param *p, const char *progname )
{
	p->readformat       = BIBL_BIBTEXIN;
	p->charsetin        = BIBL_CHARSET_DEFAULT;
	p->charsetin_src    = BIBL_SRC_DEFAULT;
	p->latexin          = 1;
	p->xmlin            = 0;
	p->utf8in           = 0;
	p->nosplittitle     = 0;
	p->verbose          = 0;
	p->addcount         = 0;
	p->output_raw       = 0;

	p->readf    = bibtexin_readf;
	p->processf = bibtexin_processf;
	p->cleanf   = bibtexin_cleanf;
	p->typef    = bibtexin_typef;
	p->convertf = bibtexin_convertf;
	p->all      = bibtex_all;
	p->nall     = bibtex_nall;

	slist_init( &(p->asis) );
	slist_init( &(p->corps) );

	if ( !progname ) p->progname = NULL;
	else p->progname = strdup( progname );
}

/*****************************************************
 PUBLIC: int bibtexin_readf()
*****************************************************/

/*
 * readf can "read too far", so we store this information in line, thus
 * the next new text is in line, either from having read too far or
 * from the next chunk obtained via str_fget()
 *
 * return 1 on success, 0 on error/end-of-file
 *
 */
static int
readmore( FILE *fp, char *buf, int bufsize, int *bufpos, str *line )
{
	if ( line->len ) return 1;
	else return str_fget( fp, buf, bufsize, bufpos, line );
}

/*
 * readf()
 *
 * returns zero if cannot get reference and hit end of-file
 * returns 1 if last reference in file, 2 if reference within file
 */
static int
bibtexin_readf( FILE *fp, char *buf, int bufsize, int *bufpos, str *line, str *reference, int *fcharset )
{
	int haveref = 0;
	char *p;
	*fcharset = CHARSET_UNKNOWN;
	while ( haveref!=2 && readmore( fp, buf, bufsize, bufpos, line ) ) {
		if ( line->len == 0 ) continue; /* blank line */
		p = &(line->data[0]);
		/* Recognize UTF8 BOM */
		if ( line->len > 2 && 
				(unsigned char)(p[0])==0xEF &&
				(unsigned char)(p[1])==0xBB &&
				(unsigned char)(p[2])==0xBF ) {
			*fcharset = CHARSET_UNICODE;
			p += 3;
		}
		p = skip_ws( p );
		if ( *p == '%' ) { /* commented out line */
			str_empty( line );
			continue;
		}
		if ( *p == '@' ) haveref++;
		if ( haveref && haveref<2 ) {
			str_strcatc( reference, p );
			str_addchar( reference, '\n' );
			str_empty( line );
		} else if ( !haveref ) str_empty( line );
	
	}
	return haveref;
}

/*****************************************************
 PUBLIC: int bibtexin_processf()
*****************************************************/

static char*
process_bibtextype( char *p, str *type )
{
	str tmp;
	str_init( &tmp );

	if ( *p=='@' ) p++;
	p = str_cpytodelim( &tmp, p, "{( \t\r\n", 0 );
	p = skip_ws( p );
	if ( *p=='{' || *p=='(' ) p++;
	p = skip_ws( p );

	if ( str_has_value( &tmp ) ) str_strcpy( type, &tmp );
	else str_empty( type );

	str_free( &tmp );
	return p;
}

static char*
process_bibtexid( char *p, str *id )
{
	char *start_p = p;
	str tmp;

	str_init( &tmp );
	p = str_cpytodelim( &tmp, p, ",", 1 );

	if ( str_has_value( &tmp ) ) {
		if ( strchr( tmp.data, '=' ) ) {
			/* Endnote writes bibtex files w/o fields, try to
			 * distinguish via presence of an equal sign.... if
			 * it's there, assume that it's a tag/data pair instead
			 * and roll back.
			 */
			p = start_p;
			str_empty( id );
		} else {
			str_strcpy( id, &tmp );
		}
	} else {
		str_empty( id );
	}

	str_free( &tmp );
	return skip_ws( p );
}

static char *
bibtex_tag( char *p, str *tag )
{
	p = str_cpytodelim( tag, skip_ws( p ), "= \t\r\n", 0 );
	if ( str_memerr( tag ) ) return NULL;
	return skip_ws( p );
}

static char *
bibtex_data( char *p, fields *bibin, slist *tokens, long nref, param *pm )
{
	unsigned int nbracket = 0, nquotes = 0;
	char *startp = p;
	str tok, *t;

	str_init( &tok );
	while ( p && *p ) {
		if ( !nquotes && !nbracket ) {
			if ( *p==',' || *p=='=' || *p=='}' || *p==')' )
				goto out;
		}
		if ( *p=='\"' && nbracket==0 && ( p==startp || *(p-1)!='\\' ) ) {
			nquotes = !nquotes;
			str_addchar( &tok, *p );
			if ( !nquotes ) {
				if ( str_memerr( &tok ) ) { p=NULL; goto out; }
				t = slist_add( tokens, &tok );
				if ( !t ) { p=NULL; goto out0; }
				str_empty( &tok );
			}
		} else if ( *p=='#' && !nquotes && !nbracket ) {
			if ( str_has_value( &tok ) ) {
				if ( str_memerr( &tok ) ) { p=NULL; goto out; }
				t = slist_add( tokens, &tok );
				if ( !t ) { p=NULL; goto out0; }
			}
			str_strcpyc( &tok, "#" );
			t = slist_add( tokens, &tok );
			if ( !t ) { p=NULL; goto out0; }
			str_empty( &tok );
		} else if ( *p=='{' && !nquotes && ( p==startp || *(p-1)!='\\' ) ) {
			nbracket++;
			str_addchar( &tok, *p );
		} else if ( *p=='}' && !nquotes && ( p==startp || *(p-1)!='\\' ) ) {
			nbracket--;
			str_addchar( &tok, *p );
			if ( nbracket==0 ) {
				if ( str_memerr( &tok ) ) { p=NULL; goto out; }
				t = slist_add( tokens, &tok );
				if ( !t ) { p=NULL; goto out; }
				str_empty( &tok );
			}
		} else if ( !is_ws( *p ) || nquotes || nbracket ) {
			if ( !is_ws( *p ) ) str_addchar( &tok, *p );
			else {
				if ( tok.len!=0 && *p!='\n' && *p!='\r' )
					str_addchar( &tok, *p );
				else if ( tok.len!=0 && (*p=='\n' || *p=='\r')) {
					str_addchar( &tok, ' ' );
					while ( is_ws( *(p+1) ) ) p++;
				}
			}
		} else if ( is_ws( *p ) ) {
			if ( tok.len ) {
				if ( str_memerr( &tok ) ) { p=NULL; goto out; }
				t = slist_add( tokens, &tok );
				if ( !t ) { p=NULL; goto out; }
				str_empty( &tok );
			}
		}
		p++;
	}
out:
	if ( nbracket!=0 ) {
		fprintf( stderr, "%s: Mismatch in number of brackets in reference %ld.\n", pm->progname, nref );
	}
	if ( nquotes!=0 ) {
		fprintf( stderr, "%s: Mismatch in number of quotes in reference %ld.\n", pm->progname, nref );
	}
	if ( str_has_value( &tok ) ) {
		if ( str_memerr( &tok ) ) { p = NULL; goto out; }
		t = slist_add( tokens, &tok );
		if ( !t ) p = NULL;
	}
out0:
	str_free( &tok );
	return p;
}

/* replace_strings()
 *
 * do string replacement -- only if unprotected by quotation marks or curly brackets
 */
static void
replace_strings( slist *tokens, fields *bibin, param *pm )
{
	int i, n, ok;
	char *q;
	str *s;
	i = 0;
	while ( i < tokens->n ) {
		s = slist_str( tokens, i );
		if ( !strcmp( s->data, "#" ) ) {
		} else if ( s->data[0]!='\"' && s->data[0]!='{' ) {
			n = slist_find( &find, s );
			if ( n!=-1 ) {
				str_strcpy( s, slist_str( &replace, n ) );
			} else {
				q = s->data;
				ok = 1;
				while ( *q && ok ) {
					if ( !isdigit( *q ) ) ok = 0;
					q++;
				}
			}
		}
		i++;
	}
}

static int
string_concatenate( slist *tokens, fields *bibin, long nref, param *pm )
{
	int i, status;
	str *s, *t;
	i = 0;
	while ( i < tokens->n ) {
		s = slist_str( tokens, i );
		if ( !strcmp( s->data, "#" ) ) {
			if ( i==0 || i==tokens->n-1 ) {
				fprintf( stderr, "%s: Warning: Stray string concatenation "
					"('#' character) in reference %ld\n", pm->progname, nref );
				status = slist_remove( tokens, i );
				if ( status!=SLIST_OK ) return BIBL_ERR_MEMERR;
				continue;
			}
			s = slist_str( tokens, i-1 );
			if ( s->data[0]!='\"' && s->data[s->len-1]!='\"' )
				fprintf( stderr, "%s: Warning: String concentation should "
					"be used in context of quotations marks in reference %ld\n", pm->progname, nref );
			t = slist_str( tokens, i+1 );
			if ( t->data[0]!='\"' && t->data[s->len-1]!='\"' )
				fprintf( stderr, "%s: Warning: String concentation should "
					"be used in context of quotations marks in reference %ld\n", pm->progname, nref );
			if ( ( s->data[s->len-1]=='\"' && t->data[0]=='\"') || (s->data[s->len-1]=='}' && t->data[0]=='{') ) {
				str_trimend( s, 1 );
				str_trimbegin( t, 1 );
				str_strcat( s, t );
			} else {
				str_strcat( s, t );
			}
			status = slist_remove( tokens, i );
			if ( status!=SLIST_OK ) return BIBL_ERR_MEMERR;
			status = slist_remove( tokens, i );
			if ( status!=SLIST_OK ) return BIBL_ERR_MEMERR;
		} else i++;
	}
	return BIBL_OK;
}

/* return NULL on memory error */
static char *
process_bibtexline( char *p, str *tag, str *data, uchar stripquotes, fields *bibin, long nref, param *pm )
{
	int i, status;
	slist tokens;
	str *s;

	str_empty( data );

	p = bibtex_tag( p, tag );

	if ( str_is_empty( tag ) ) {
		/* ...skip this line */
		while ( *p && *p!='\n' && *p!='\r' ) p++;
		while ( *p=='\n' || *p=='\r' ) p++;
		return p;
	}

	slist_init( &tokens );

	if ( *p=='=' ) {
		p = bibtex_data( p+1, bibin, &tokens, nref, pm );
		if ( p==NULL ) goto out;
	}

	replace_strings( &tokens, bibin, pm );

	status = string_concatenate( &tokens, bibin, nref, pm );
	if ( status!=BIBL_OK ) {
		p = NULL;
		goto out;
	}

	for ( i=0; i<tokens.n; i++ ) {
		s = slist_str( &tokens, i );
		if ( ( stripquotes && s->data[0]=='\"' && s->data[s->len-1]=='\"' ) ||
		     ( s->data[0]=='{' && s->data[s->len-1]=='}' ) ) {
			str_trimbegin( s, 1 );
			str_trimend( s, 1 );
		}
		str_strcat( data, slist_str( &tokens, i ) );
	}
out:
	slist_free( &tokens );
	return p;
}

/* process_cite()
 *
 */
static int
process_cite( fields *bibin, char *p, char *filename, long nref, param *pm )
{
	int fstatus, status = BIBL_OK;
	str type, id, tag, data;

	strs_init( &type, &id, &tag, &data, NULL );

	p = process_bibtextype( p, &type );
	p = process_bibtexid( p, &id );

	if ( str_is_empty( &type ) || str_is_empty( &id ) ) goto out;

	fstatus = fields_add( bibin, "INTERNAL_TYPE", str_cstr( &type ), 0 );
	if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }

	fstatus = fields_add( bibin, "REFNUM", str_cstr( &id), 0 );
	if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }

	while ( *p ) {
		p = process_bibtexline( p, &tag, &data, 1, bibin, nref, pm );
		if ( p==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
		/* no anonymous or empty fields allowed */
		if ( str_has_value( &tag ) && str_has_value( &data ) ) {
			fstatus = fields_add( bibin, str_cstr( &tag ), str_cstr( &data ), 0 );
			if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }
		}
		strs_empty( &tag, &data, NULL );
	}
out:
	strs_free( &type, &id, &tag, &data, NULL );
	return status;
}

/* process_string()
 *
 * Handle lines like:
 *
 * '@STRING{TL = {Tetrahedron Lett.}}'
 *
 * p should point to just after '@STRING'
 *
 * In BibTeX, if a string is defined several times, the last one is kept.
 *
 */
static int
process_string( char *p, long nref, param *pm )
{
	int n, status = BIBL_OK;
	str s1, s2, *t;
	strs_init( &s1, &s2, NULL );
	while ( *p && *p!='{' && *p!='(' ) p++;
	if ( *p=='{' || *p=='(' ) p++;
	p = process_bibtexline( skip_ws( p ), &s1, &s2, 0, NULL, nref, pm );
	if ( p==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
	if ( str_has_value( &s2 ) ) {
		str_findreplace( &s2, "\\ ", " " );
	}
	if ( str_has_value( &s1 ) ) {
		n = slist_find( &find, &s1 );
		if ( n==-1 ) {
			t = slist_add( &find, &s1 );
			if ( t==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
			if ( str_has_value( &s2 ) ) t = slist_add( &replace, &s2 );
			else t = slist_addc( &replace, "" );
			if ( t==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
		} else {
			if ( str_has_value( &s2 ) ) t = slist_set( &replace, n, &s2 );
			else t = slist_setc( &replace, n, "" );
			if ( t==NULL ) { status = BIBL_ERR_MEMERR; goto out; }
		}
	}
out:
	strs_free( &s1, &s2, NULL );
	return status;
}

/* bibtexin_processf()
 *
 * Handle '@STRING', '@reftype', and ignore '@COMMENT'
 */
static int
bibtexin_processf( fields *bibin, char *data, char *filename, long nref, param *p )
{
	if ( !strncasecmp( data, "@STRING", 7 ) ) {
		process_string( data+7, nref, p );
		return 0;
	} else if ( !strncasecmp( data, "@COMMENT", 8 ) ) {
		/* Not sure if these are real Bibtex, but not references */
		return 0;
	} else {
		process_cite( bibin, data, filename, nref, p );
		return 1;
	}
}

/*****************************************************
 PUBLIC: void bibtexin_cleanf()
*****************************************************/

static int
bibtex_protected( str *data )
{
	if ( data->data[0]=='{' && data->data[data->len-1]=='}' ) return 1;
	if ( data->data[0]=='\"' && data->data[data->len-1]=='\"' ) return 1;
	return 0;
}

static int
bibtex_split( slist *tokens, str *s )
{
	int i, n = s->len, nbrackets = 0, status = BIBL_OK;
	str tok, *t;

	str_init( &tok );

	for ( i=0; i<n; ++i ) {
		if ( s->data[i]=='{' && ( i==0 || s->data[i-1]!='\\' ) ) {
			nbrackets++;
			str_addchar( &tok, '{' );
		} else if ( s->data[i]=='}' && ( i==0 || s->data[i-1]!='\\' ) ) {
			nbrackets--;
			str_addchar( &tok, '}' );
		} else if ( !is_ws( s->data[i] ) || nbrackets ) {
			str_addchar( &tok, s->data[i] );
		} else if ( is_ws( s->data[i] ) ) {
			if ( str_has_value( &tok ) ) {
				t = slist_add( tokens, &tok );
				if ( !t ) {
					status = BIBL_ERR_MEMERR;
					goto out;
				}
			}
			str_empty( &tok );
		}
	}
	if ( str_has_value( &tok ) ) {
		t = slist_add( tokens, &tok );
		if ( !t ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
	}

	for ( i=0; i<tokens->n; ++i ) {
		str_trimstartingws( slist_str( tokens, i ) );
		str_trimendingws( slist_str( tokens, i ) );
	}
out:
	str_free( &tok );
	return status;
}

static int
bibtex_addtitleurl( fields *info, str *in )
{
	int fstatus, status = BIBL_OK;
	str s;
	char *p;

	str_init( &s );

	/* ...skip past "\href{" and copy to "}" */
	p = str_cpytodelim( &s, in->data + 6, "}", 1 );
	if ( str_memerr( &s ) ) { status = BIBL_ERR_MEMERR; goto out; }

	/* ...add to URL */
	fstatus = fields_add( info, "URL", s.data, 0 );
	if ( fstatus!=FIELDS_OK ) { status = BIBL_ERR_MEMERR; goto out; }

	/* ...return deleted fragment to str in */
	(void) str_cpytodelim( &s, p, "", 0 );
	if ( str_memerr( &s ) ) { status = BIBL_ERR_MEMERR; goto out; }
	str_swapstrings( &s, in );

out:
	str_free( &s );
	return status;
}

static int
is_url_tag( str *tag )
{
	if ( str_has_value( tag ) ) {
		if ( !strcasecmp( str_cstr( tag ), "url" ) ) return 1;
	}
	return 0;
}

static int
is_name_tag( str *tag )
{
	if ( str_has_value( tag ) ) {
		if ( !strcasecmp( str_cstr( tag ), "author" ) ) return 1;
		if ( !strcasecmp( str_cstr( tag ), "editor" ) ) return 1;
	}
	return 0;
}

static void
bibtex_process_tilde( str *s )
{
	char *p, *q;
	int n = 0;

	p = q = s->data;
	if ( !p ) return;
	while ( *p ) {
		if ( *p=='~' ) {
			*q = ' ';
		} else if ( *p=='\\' && *(p+1)=='~' ) {
			n++;
			p++;
			*q = '~';
		} else {
			*q = *p;
		}
		p++;
		q++;
	}
	*q = '\0';
	s->len -= n;
}

static void
bibtex_process_bracket( str *s )
{
	char *p, *q;
	int n = 0;

	p = q = s->data;
	if ( !p ) return;
	while ( *p ) {
		if ( *p=='\\' && ( *(p+1)=='{' || *(p+1)=='}' ) ) {
			n++;
			p++;
			*q = *p;
			q++;
		} else if ( *p=='{' || *p=='}' ) {
			n++;
		} else {
			*q = *p;
			q++;
		}
		p++;
	}
	*q = '\0';
	s->len -= n;
}

static void
bibtex_cleantoken( str *s )
{
	/* 'textcomp' annotations */
	str_findreplace( s, "\\textit", "" );
	str_findreplace( s, "\\textbf", "" );
	str_findreplace( s, "\\textsl", "" );
	str_findreplace( s, "\\textsc", "" );
	str_findreplace( s, "\\textsf", "" );
	str_findreplace( s, "\\texttt", "" );
	str_findreplace( s, "\\textsubscript", "" );
	str_findreplace( s, "\\textsuperscript", "" );
	str_findreplace( s, "\\emph", "" );
	str_findreplace( s, "\\url", "" );
	str_findreplace( s, "\\mbox", "" );

	/* Other text annotations */
	str_findreplace( s, "\\it ", "" );
	str_findreplace( s, "\\em ", "" );

	str_findreplace( s, "\\%", "%" );
	str_findreplace( s, "\\$", "$" );
	while ( str_findreplace( s, "  ", " " ) ) {}

	/* 'textcomp' annotations that we don't want to substitute on output*/
	str_findreplace( s, "\\textdollar", "$" );
	str_findreplace( s, "\\textunderscore", "_" );

	bibtex_process_bracket( s );
	bibtex_process_tilde( s );

}

static int
bibtex_cleandata( str *tag, str *s, fields *info, param *p )
{
	int i, status;
	slist tokens;
	str *tok;
	if ( str_is_empty( s ) ) return BIBL_OK;
	/* protect url from undergoing any parsing */
	if ( is_url_tag( tag ) ) return BIBL_OK;
	slist_init( &tokens );
	status = bibtex_split( &tokens, s );
	if ( status!=BIBL_OK ) goto out;
	for ( i=0; i<tokens.n; ++i ) {
		tok = slist_str( &tokens, i );
		if ( bibtex_protected( tok ) ) {
			if (!strncasecmp(tok->data,"\\href{", 6)) {
				bibtex_addtitleurl( info, tok );
			}
		}
		if ( p->latexin && !is_name_tag( tag ) && !is_url_tag( tag ) )
			bibtex_cleantoken( tok );
	}
	str_empty( s );
	for ( i=0; i<tokens.n; ++i ) {
		tok = slist_str( &tokens, i );
		if ( i>0 ) str_addchar( s, ' ' );
		str_strcat( s, tok );
	}
out:
	slist_free( &tokens );
	return status;
}

static int
bibtexin_cleanref( fields *bibin, param *p )
{
	int i, n, status;
	str *t, *d;
	n = fields_num( bibin );
	for ( i=0; i<n; ++i ) {
		t = fields_tag( bibin, i, FIELDS_STRP_NOUSE );
		d = fields_value( bibin, i, FIELDS_STRP_NOUSE );
		status = bibtex_cleandata( t, d, bibin, p );
		if ( status!=BIBL_OK ) return status;
	}
	return BIBL_OK;
}

static long
bibtexin_findref( bibl *bin, char *citekey )
{
	int n;
	long i;
	for ( i=0; i<bin->nrefs; ++i ) {
		n = fields_find( bin->ref[i], "refnum", LEVEL_ANY );
		if ( n==FIELDS_NOTFOUND ) continue;
		if ( !strcmp( bin->ref[i]->data[n].data, citekey ) ) return i;
	}
	return -1;
}

static void
bibtexin_nocrossref( bibl *bin, long i, int n, param *p )
{
	int n1 = fields_find( bin->ref[i], "REFNUM", LEVEL_ANY );
	if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
	fprintf( stderr, "Cannot find cross-reference '%s'",
			bin->ref[i]->data[n].data );
	if ( n1!=FIELDS_NOTFOUND ) fprintf( stderr, " for reference '%s'\n",
			bin->ref[i]->data[n1].data );
	fprintf( stderr, "\n" );
}

static int
bibtexin_crossref_oneref( fields *bibref, fields *bibcross )
{
	int j, n, nl, ntype, fstatus, status = BIBL_OK;
	char *type, *nt, *nv;

	ntype = fields_find( bibref, "INTERNAL_TYPE", LEVEL_ANY );
	type = ( char * ) fields_value( bibref, ntype, FIELDS_CHRP_NOUSE );

	n = fields_num( bibcross );
	for ( j=0; j<n; ++j ) {
		nt = ( char * ) fields_tag( bibcross, j, FIELDS_CHRP_NOUSE );
		if ( !strcasecmp( nt, "INTERNAL_TYPE" ) ) continue;
		if ( !strcasecmp( nt, "REFNUM" ) ) continue;
		if ( !strcasecmp( nt, "TITLE" ) ) {
			if ( !strcasecmp( type, "Inproceedings" ) ||
			     !strcasecmp( type, "Incollection" ) )
				nt = "booktitle";
		}
		nv = ( char * ) fields_value( bibcross, j, FIELDS_CHRP_NOUSE );
		nl = fields_level( bibcross, j ) + 1;
		fstatus = fields_add( bibref, nt, nv, nl );
		if ( fstatus!=FIELDS_OK ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
	}
out:
	return status;
}

static int
bibtexin_crossref( bibl *bin, param *p )
{
	int i, n, ncross, status = BIBL_OK;
	fields *bibref, *bibcross;

	for ( i=0; i<bin->nrefs; ++i ) {
		bibref = bin->ref[i];
		n = fields_find( bibref, "CROSSREF", LEVEL_ANY );
		if ( n==FIELDS_NOTFOUND ) continue;
		fields_setused( bibref, n );
		ncross = bibtexin_findref( bin, (char*) fields_value( bibref, n, FIELDS_CHRP ) );
		if ( ncross==-1 ) {
			bibtexin_nocrossref( bin, i, n, p );
			continue;
		}
		bibcross = bin->ref[ncross];
		status = bibtexin_crossref_oneref( bibref, bibcross );
		if ( status!=BIBL_OK ) goto out;
	}
out:
	return status;
}

static int
bibtexin_cleanf( bibl *bin, param *p )
{
	int status = BIBL_OK;
	long i;

        for ( i=0; i<bin->nrefs; ++i )
		status = bibtexin_cleanref( bin->ref[i], p );
	bibtexin_crossref( bin, p );
	return status;
}

/*****************************************************
 PUBLIC: int bibtexin_typef()
*****************************************************/

static int
bibtexin_typef( fields *bibin, char *filename, int nrefs, param *p )
{
	int ntypename, nrefname, is_default;
	char *refname = "", *typename = "";

	ntypename = fields_find( bibin, "INTERNAL_TYPE", LEVEL_MAIN );
	nrefname  = fields_find( bibin, "REFNUM",        LEVEL_MAIN );
	if ( nrefname!=FIELDS_NOTFOUND )  refname  = fields_value( bibin, nrefname,  FIELDS_CHRP_NOUSE );
	if ( ntypename!=FIELDS_NOTFOUND ) typename = fields_value( bibin, ntypename, FIELDS_CHRP_NOUSE );

	return get_reftype( typename, nrefs, p->progname, p->all, p->nall, refname, &is_default, REFTYPE_CHATTY );
}

/*****************************************************
 PUBLIC: int bibtexin_convertf(), returns BIBL_OK or BIBL_ERR_MEMERR
*****************************************************/

static int
bibtex_matches_list( fields *bibout, char *tag, char *suffix, str *data, int level,
		slist *names, int *match )
{
	int i, fstatus, status = BIBL_OK;
	str newtag;

	*match = 0;
	if ( names->n==0 ) return status;

	str_init( &newtag );

	for ( i=0; i<names->n; ++i ) {
		if ( strcmp( str_cstr( data ), slist_cstr( names, i ) ) ) continue;
		str_initstrc( &newtag, tag );
		str_strcatc( &newtag, suffix );
		fstatus = fields_add( bibout, str_cstr( &newtag ), str_cstr( data ), level );
		if ( fstatus!=FIELDS_OK ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
		*match = 1;
		goto out;
	}

out:
	str_free( &newtag );
	return status;
}

/**** bibtexin_btorg ****/

/*
 * BibTeX uses 'organization' in lieu of publisher if that field is missing.
 * Otherwise output as
 * <name type="corporate">
 *    <namePart>The organization</namePart>
 *    <role>
 *       <roleTerm authority="marcrelator" type="text">organizer of meeting</roleTerm>
 *    </role>
 * </name>
 */

static int
bibtexin_btorg( fields *bibin, int m, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	int n, fstatus;
	n = fields_find( bibin, "publisher", LEVEL_ANY );
	if ( n==FIELDS_NOTFOUND )
		fstatus = fields_add( bibout, "PUBLISHER", str_cstr( invalue ), level );
	else
		fstatus = fields_add( bibout, "ORGANIZER:CORP", str_cstr( invalue ), level );
	if ( fstatus==FIELDS_OK ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
}

/**** bibtexin_btsente() ****/

/*
 * sentelink = {file://localhost/full/path/to/file.pdf,Sente,PDF}
 *
 * Sente is an academic reference manager for MacOSX and Apple iPad.
 */

static int
bibtexin_btsente( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	int fstatus, status = BIBL_OK;
	str link;

	str_init( &link );
	str_cpytodelim( &link, skip_ws( invalue->data ), ",", 0 );
	str_trimendingws( &link );
	if ( str_memerr( &link ) ) status = BIBL_ERR_MEMERR;

	if ( status==BIBL_OK && link.len ) {
		fstatus = fields_add( bibout, "FILEATTACH", str_cstr( &link ), level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}

	str_free( &link );
	return status;
}

/**** bibtexin_linkedfile() ****/

static int
count_colons( char *p )
{
	int n = 0;
	while ( *p ) {
		if ( *p==':' ) n++;
		p++;
	}
	return n;
}

static int
first_colon( char *p )
{
	int n = 0;
	while ( p[n] && p[n]!=':' ) n++;
	return n;
}

static int
last_colon( char *p )
{
	int n = strlen( p ) - 1;
	while ( n>0 && p[n]!=':' ) n--;
	return n;
}

/*
 * file={Description:/full/path/to/file.pdf:PDF}
 */
static int
bibtexin_linkedfile( fields *bibin, int m, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	int fstatus, status = BIBL_OK;
	char *p = invalue->data;
	int i, n, n1, n2;
	str link;

	n = count_colons( p );
	if ( n > 1 ) {
		/* A DOS file can contain a colon ":C:/....pdf:PDF" */
		/* Extract after 1st and up to last colons */
		n1 = first_colon( p ) + 1;
		n2 = last_colon( p );
		str_init( &link );
		for ( i=n1; i<n2; ++i ) {
			str_addchar( &link, p[i] );
		}
		str_trimstartingws( &link );
		str_trimendingws( &link );
		if ( str_memerr( &link ) ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
		if ( link.len ) {
			fstatus = fields_add( bibout, "FILEATTACH", link.data, level );
			if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
		}
out:
		str_free( &link );
	} else {
		/* This field isn't formatted properly, so just copy directly */
		fstatus = fields_add( bibout, "FILEATTACH", p, level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}
	return status;

}

/**** bibtexin_howpublished() ****/

/*    howpublished={},
 *
 * Normally indicates the manner in which something was
 * published in lieu of a formal publisher, so typically
 * 'howpublished' and 'publisher' will never be in the
 * same reference.
 *
 * Occassionally, people put Diploma thesis information
 * into the field, so check that first.
 *
 * Returns BIBL_OK or BIBL_ERR_MEMERR
 */

static int
bibtexin_howpublished( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	int fstatus, status = BIBL_OK;
	if ( !strncasecmp( str_cstr( invalue ), "Diplom", 6 ) ) {
		fstatus = fields_replace_or_add( bibout, "GENRE:BIBUTILS", "Diploma thesis", level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}
	else if ( !strncasecmp( str_cstr( invalue ), "HSabilitation", 13 ) ) {
		fstatus = fields_replace_or_add( bibout, "GENRE:BIBUTILS", "Habilitation thesis", level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}
	else if ( !strncasecmp( str_cstr( invalue ), "Licentiate", 10 ) ) {
		fstatus = fields_replace_or_add( bibout, "GENRE:BIBUTILS", "Licentiate thesis", level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}
	else if ( is_embedded_link( str_cstr( invalue ) ) ) {
		status =  urls_split_and_add( str_cstr( invalue ), bibout, level );
	}
	else {
		fstatus = fields_add( bibout, "PUBLISHER", str_cstr( invalue ), level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}
	return status;
}

/**** bibtexin_eprint() ****/

/* Try to capture situations like
 *
 * eprint="1605.02026",
 * archivePrefix="arXiv",
 *
 * or
 *
 * eprint="13211131",
 * eprinttype="medline",
 *
 * If we don't know anything, concatenate archivePrefix:eprint
 * and push into URL. (Could be wrong)
 *
 * If no info, just push eprint into URL. (Could be wrong)
 */
static int
process_eprint_with_prefix( fields *bibout, char *prefix, str *value, int level )
{
	int fstatus, status = BIBL_OK;
	str merge;

	if ( !strcmp( prefix, "arXiv" ) ) {
		fstatus = fields_add( bibout, "ARXIV", value->data, level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}

	else if ( !strcmp( prefix, "jstor" ) ) {
		fstatus = fields_add( bibout, "JSTOR", value->data, level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}

	else if ( !strcmp( prefix, "medline" ) ) {
		fstatus = fields_add( bibout, "MEDLINE", value->data, level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}

	else if ( !strcmp( prefix, "pubmed" ) ) {
		fstatus = fields_add( bibout, "PMID", value->data, level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
	}

	/* ...if this is unknown prefix, merge prefix & eprint */
	else {
		str_init( &merge );
		str_mergestrs( &merge, prefix, ":", value->data, NULL );
		fstatus = fields_add( bibout, "URL", merge.data, level );
		if ( fstatus!=FIELDS_OK ) status = BIBL_ERR_MEMERR;
		str_free( &merge );
	}

	return status;
}
static int
process_eprint_without_prefix( fields *bibout, str *value, int level )
{
	int fstatus;

	/* ...no archivePrefix, need to handle just 'eprint' tag */
	fstatus = fields_add( bibout, "URL", value->data, level );

	if ( fstatus!=FIELDS_OK ) return BIBL_ERR_MEMERR;
	else return BIBL_OK;
}

static int
bibtexin_eprint( fields *bibin, int m, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	char *prefix;
	int n;

	/* ...do we have an archivePrefix too? */
	n = fields_find( bibin, "ARCHIVEPREFIX", level );
	if ( n==FIELDS_NOTFOUND ) n = fields_find( bibin, "EPRINTTYPE", level );
	if ( n!=FIELDS_NOTFOUND ) {
		prefix = fields_value( bibin, n, FIELDS_CHRP );
		return process_eprint_with_prefix( bibout, prefix, invalue, level );
	}

	/* ...no we don't */
	return process_eprint_without_prefix( bibout, invalue, level );
}

/**** bibtexin_keyword() ****/

/* Split keywords="" with semicolons.
 * Commas are also frequently used, but will break
 * entries like:
 *       keywords="Microscopy, Confocal"
 * Returns BIBL_OK or BIBL_ERR_MEMERR
 */

static int
bibtexin_keyword( fields *bibin, int m, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	int fstatus, status = BIBL_OK;
	str keyword;
	char *p;

	p = invalue->data;
	str_init( &keyword );

	while ( *p ) {
		p = str_cpytodelim( &keyword, skip_ws( p ), ";", 1 );
		str_trimendingws( &keyword );
		if ( str_memerr( &keyword ) ) {
			status = BIBL_ERR_MEMERR;
			goto out;
		}
		if ( keyword.len ) {
			fstatus = fields_add( bibout, "KEYWORD", keyword.data, level );
			if ( fstatus!=FIELDS_OK ) {
				status = BIBL_ERR_MEMERR;
				goto out;
			}
		}
	}
out:
	str_free( &keyword );
	return status;
}

/*
 * bibtex_names( bibout, newtag, field, level);
 *
 * split names in author list separated by and's (use '|' character)
 * and add names
 *
 * returns BIBL_OK on success, BIBL_ERR_MEMERR on memory error
 */

static int
bibtexin_person( fields *bibin, int m, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	int begin, end, ok, n, etal, i, status, match;
	slist tokens;

	/* If we match the asis or corps list add and bail. */
	status = bibtex_matches_list( bibout, outtag, ":ASIS", invalue, level, &(pm->asis), &match );
	if ( match==1 || status!=BIBL_OK ) return status;
	status = bibtex_matches_list( bibout, outtag, ":CORP", invalue, level, &(pm->corps), &match );
	if ( match==1 || status!=BIBL_OK ) return status;

	slist_init( &tokens );

	bibtex_split( &tokens, invalue );
	for ( i=0; i<tokens.n; ++i )
		bibtex_cleantoken( slist_str( &tokens, i ) );

	etal = name_findetal( &tokens );

	begin = 0;
	n = tokens.n - etal;
	while ( begin < n ) {

		end = begin + 1;

		while ( end < n && strcasecmp( slist_cstr( &tokens, end ), "and" ) )
			end++;

		if ( end - begin == 1 ) {
			ok = name_addsingleelement( bibout, outtag, slist_cstr( &tokens, begin ), level, 0 );
			if ( !ok ) { status = BIBL_ERR_MEMERR; goto out; }
		} else {
			ok = name_addmultielement( bibout, outtag, &tokens, begin, end, level );
			if ( !ok ) { status = BIBL_ERR_MEMERR; goto out; }
		}

		begin = end + 1;

		/* Handle repeated 'and' errors: authors="G. F. Author and and B. K. Author" */
		while ( begin < n && !strcasecmp( slist_cstr( &tokens, begin ), "and" ) )
			begin++;
	}

	if ( etal ) {
		ok = name_addsingleelement( bibout, outtag, "et al.", level, 0 );
		if ( !ok ) status = BIBL_ERR_MEMERR;
	}

out:
	slist_free( &tokens );
	return status;

}

/**** bibtexin_title() ****/

/* bibtexin_titleinbook_isbooktitle()
 *
 * Normally, the title field of inbook refers to the book.  The
 * section in a @inbook reference is untitled.  If it's titled,
 * the @incollection should be used.  For example, in:
 *
 * @inbook{
 *    title="xxx"
 * }
 *
 * the booktitle is "xxx".
 *
 * However, @inbook is frequently abused (and treated like
 * @incollection) so that title and booktitle are present
 * and title is now 'supposed' to refer to the section.  For example:
 *
 * @inbook{
 *     title="yyy",
 *     booktitle="xxx"
 * }
 *
 * Therefore report whether or not booktitle is present as well
 * as title in @inbook references.  If not, then make 'title'
 * correspond to the title of the book, not the section.
 *
 */
static int
bibtexin_titleinbook_isbooktitle( fields *bibin, char *intag )
{
	int n;

	/* ...look only at 'title="xxx"' elements */
	if ( strcasecmp( intag, "TITLE" ) ) return 0;

	/* ...look only at '@inbook' references */
	n = fields_find( bibin, "INTERNAL_TYPE", LEVEL_ANY );
	if ( n==FIELDS_NOTFOUND ) return 0;
	if ( strcasecmp( fields_value( bibin, n, FIELDS_CHRP ), "INBOOK" ) ) return 0;

	/* ...look to see if 'booktitle="yyy"' exists */
	n = fields_find( bibin, "BOOKTITLE", LEVEL_ANY );
	if ( n==FIELDS_NOTFOUND ) return 0;
	else return 1;
}

static int
bibtexin_title( fields *bibin, int n, str *intag, str *invalue, int level, param *pm, char *outtag, fields *bibout )
{
	int ok;

	if ( bibtexin_titleinbook_isbooktitle( bibin, intag->data ) ) level=LEVEL_MAIN;
	ok = title_process( bibout, "TITLE", invalue->data, level, pm->nosplittitle );
	if ( ok ) return BIBL_OK;
	else return BIBL_ERR_MEMERR;
}

static void
bibtexin_notag( param *p, char *tag )
{
	if ( p->verbose && strcmp( tag, "INTERNAL_TYPE" ) ) {
		if ( p->progname ) fprintf( stderr, "%s: ", p->progname );
		fprintf( stderr, "Cannot find tag '%s'\n", tag );
	}
}

static int
bibtexin_convertf( fields *bibin, fields *bibout, int reftype, param *p )
{
	static int (*convertfns[NUM_REFTYPES])(fields *, int, str *, str *, int, param *, char *, fields *) = {
		[ 0 ... NUM_REFTYPES-1 ] = generic_null,
		[ SIMPLE       ] = generic_simple,
		[ TITLE        ] = bibtexin_title,
		[ PERSON       ] = bibtexin_person,
		[ PAGES        ] = generic_pages,
		[ KEYWORD      ] = bibtexin_keyword,
		[ EPRINT       ] = bibtexin_eprint,
		[ HOWPUBLISHED ] = bibtexin_howpublished,
		[ LINKEDFILE   ] = bibtexin_linkedfile,
		[ NOTES        ] = generic_notes,
		[ GENRE        ] = generic_genre,
		[ BT_SENTE     ] = bibtexin_btsente,
		[ BT_ORG       ] = bibtexin_btorg,
		[ URL          ] = generic_url
	};

	int process, level, i, nfields, status = BIBL_OK;
	str *intag, *invalue;
	char *outtag;

	nfields = fields_num( bibin );
	for ( i=0; i<nfields; ++i ) {

		if ( fields_used( bibin, i ) )   continue; /* e.g. successful crossref */
		if ( fields_notag( bibin, i ) )  continue;
		if ( fields_nodata( bibin, i ) ) continue;

		intag   = fields_tag( bibin, i, FIELDS_STRP );
		invalue = fields_value( bibin, i, FIELDS_STRP );

		if ( !translate_oldtag( str_cstr( intag ), reftype, p->all, p->nall, &process, &level, &outtag ) ) {
			bibtexin_notag( p, str_cstr( intag ) );
			continue;
		}

		status = convertfns[ process ] ( bibin, i, intag, invalue, level, p, outtag, bibout );
		if ( status!=BIBL_OK ) return status;
	}

	if ( status==BIBL_OK && p->verbose ) fields_report( bibout, stderr );

	return status;
}
