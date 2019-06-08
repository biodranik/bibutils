/*
 * bibdiff.c
 * 
 * Copyright (c) Chris Putnam 2017-2018
 *
 * Program and source code released under the GPL version 2
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include "bibutils.h"
#include "intlist.h"
#include "args.h"

char progname[] = "bibdiff";

/* find_tag_level_matches()
 *
 * Find all possible matches for tag/level combinations
 */

int
find_tag_level_matches( fields *f, char *tag, int level, intlist *matchs )
{
	int i, n = 0;
	char *ftag;

	for ( i=0; i<f->n; ++i )
		intlist_set( matchs, i, 0 );

	for ( i=0; i<f->n; ++i ) {
		if ( fields_level( f, i ) != level ) continue;
		ftag = fields_tag( f, i, FIELDS_CHRP_NOLEN );
		if ( !strcmp( tag, ftag ) ) {
			intlist_set( matchs, i, 1 );
			n++;
		}
	}

	return n;
}

int
compare_references( fields *f1, const char *fname1, fields *f2, const char *fname2, long n )
{
	int i, j, cnt, cnt1, cnt2, level, diff = 0;
	intlist found1, found2, matches;
	char *tag, *data;

	intlist_init_fill( &found1, f1->n, -1 );
	intlist_init_fill( &found2, f2->n, -1 );
	intlist_init_fill( &matches, f2->n, 0 );

	for ( i=0; i<f1->n; ++i ) {

		tag   = fields_tag(   f1, i, FIELDS_CHRP_NOLEN );
		data  = fields_value( f1, i, FIELDS_CHRP_NOLEN );
		level = fields_level( f1, i );

		cnt = find_tag_level_matches( f2, tag, level, &matches );

		/* ...no matches */
		if ( cnt==0 ) continue;

		for ( j=0; j<f2->n; ++j ) {
			if ( intlist_get( &matches, j ) == 0 ) continue; /* not a potential match */
			if ( intlist_get( &found2, j ) != -1 ) continue; /* already claimed */
			if ( strcmp( data, fields_value( f2, j, FIELDS_CHRP_NOLEN ) ) ) continue; /* values don't match */
			intlist_set( &found1, i, j );
			intlist_set( &found2, j, i );
			break;
		}
	}

	cnt1 = 0;
	for ( i=0; i<f1->n; ++i )
		if ( intlist_get( &found1, i ) != -1 ) cnt1++;

	cnt2 = 0;
	for ( i=0; i<f2->n; ++i )
		if ( intlist_get( &found2, i ) != -1 ) cnt2++;

	if ( cnt1!=f1->n || cnt2!=f2->n ) {

		printf( "reference %ld < %s > %s\n", n, fname1, fname2 );

		for ( i=0; i<f1->n; ++i ) {
			if ( intlist_get( &found1, i ) != -1 ) continue;
			tag = fields_tag( f1, i, FIELDS_CHRP_NOLEN );
			data = fields_value( f1, i, FIELDS_CHRP_NOLEN );
			level = fields_level( f1, i );
			printf( "< '%s' '%s' %d\n", tag, data, level );
		}

		for ( i=0; i<f2->n; ++i ) {
			if ( intlist_get( &found2, i ) != -1 ) continue;
			tag = fields_tag( f2, i, FIELDS_CHRP_NOLEN );
			data = fields_value( f2, i, FIELDS_CHRP_NOLEN );
			level = fields_level( f2, i );
			printf( "> '%s' '%s' %d\n", tag, data, level );
		}

	}

	intlist_free( &found1 );
	intlist_free( &found2 );
	intlist_free( &matches );

	return diff;
}

int
compare_bibliographies( bibl *b1, const char *fname1, bibl *b2, const char *fname2 )
{
	fields *f1, *f2;
	long i;

	if ( b1->nrefs != b2->nrefs ) {
		printf( "%s: %s has %ld references and %s has %ld references\n", progname,
			fname1, b1->nrefs, fname2, b2->nrefs );
		return 1;
	}

	for ( i=0; i<b1->nrefs; ++i ) {
		f1 = b1->ref[i];
		f2 = b2->ref[i];
		if ( compare_references( f1, fname1, f2, fname2, i+1 ) ) return 1;
	}

	return 0;
}

void
version( void )
{
	args_tellversion( progname );
	exit( EXIT_FAILURE );
}

void
help( void )
{
	args_tellversion( progname );
	fprintf( stderr, "Compares references after reading them from their native formats\n\n" );

	fprintf( stderr, "usage: %s ref1_file ref2_file\n\n", progname );

	fprintf( stderr, "-h,  --help               display this help\n" );
	fprintf( stderr, "-v,  --version            display version\n" );
	fprintf( stderr, "-f1, --format1 FORMAT     specify input format for ref1_file\n" );
	fprintf( stderr, "-f2, --format2 FORMAT     specify input format for ref1_file\n" );
	fprintf( stderr, "\n" );

	fprintf( stderr, "Valid format specifiers are 'bibtex', 'biblatex', 'copac', 'ebi', "
		"'endnote', 'endnote-xml', 'medline', 'mods', 'nbib', 'ris', 'word2007'\n\n" );

	exit( EXIT_FAILURE );
}

int
lookup_format( const char *format )
{
	typedef struct flist_t { char *name; int code; } flist_t;
	flist_t formats[] = {
		{ "bibtex",      BIBL_BIBTEXIN },
		{ "biblatex",    BIBL_BIBLATEXIN },
		{ "copac",       BIBL_COPACIN },
		{ "ebi",         BIBL_EBIIN },
		{ "endnote",     BIBL_ENDNOTEIN },
		{ "endnote-xml", BIBL_ENDNOTEXMLIN },
		{ "medline",     BIBL_MEDLINEIN },
		{ "mods",        BIBL_MODSIN },
		{ "nbib",        BIBL_NBIBIN },
		{ "ris",         BIBL_RISIN },
		{ "word2007",    BIBL_WORDIN },
	};
	int i, nformats = sizeof( formats ) / sizeof( formats[0] );

	for ( i=0; i<nformats; ++i ) {
		if ( !strcasecmp( format, formats[i].name ) ) return formats[i].code;
	}

	fprintf( stderr, "%s: Cannot recognize format '%s'.\n", progname, format );
	fprintf( stderr, "%s: Valid format specifiers are 'bibtex', 'biblatex', 'copac', 'ebi', "
		"'endnote', 'endnote-xml', 'medline', 'mods', 'nbib', 'ris', 'word2007'\n", progname );
	fprintf( stderr, "%s: Exiting.\n", progname );
	exit( EXIT_FAILURE );
}

void
process_args( int *argc, char *argv[], int *format1, int *format2 )
{
	int i, j, done, subtract;
	char *f1, *f2;

	for ( i=0; i<*argc; ++i )
		if ( args_match( argv[i], "-h", "--help" ) ) help();

	for ( i=0; i<*argc; ++i )
		if ( args_match( argv[i], "-v", "--version" ) ) version();

	done = 0;
	i = 1;
	while ( i < *argc && !done ) {
		subtract = 0;
		if ( args_match( argv[i], "-f1", "--format1" ) ) {
			f1 = args_next( *argc, argv, i, progname, "-f1", "--format1" );
			*format1 = lookup_format( f1 );
			subtract = 2;
		}
		else if ( args_match( argv[i], "-f2", "--format2" ) ) {
			f2 = args_next( *argc, argv, i, progname, "-f1", "--format1" );
			*format2 = lookup_format( f2 );
			subtract = 2;
		}
		else if ( !strcmp( argv[i], "--" ) ) {
			done = 1;
			subtract = 1;
		}
		else if ( !strncmp( argv[i], "-", 1 ) ) {
			fprintf( stderr, "%s: Unrecognized command-line switch '%s'. Exiting.\n", progname, argv[i] );
			exit( EXIT_FAILURE );
		}
		if ( subtract ) {
			for ( j=i+subtract; j<*argc; ++j )
				argv[j-subtract] = argv[j];
			*argc -= subtract;
		} else i++;

	}
}

int
main( int argc, char *argv[] )
{
	int format1 = BIBL_MODSIN;
	int format2 = BIBL_MODSIN;
	param p1, p2;
	bibl b1, b2;
	int status;
	FILE *fp;

	process_args( &argc, argv, &format1, &format2 );

	if ( argc < 2 ) help();

	bibl_initparams( &p1, format1, BIBL_MODSOUT, progname );
	bibl_initparams( &p2, format2, BIBL_MODSOUT, progname );

	fp = fopen( argv[1], "r" );
	if ( !fp ) {
		fprintf( stderr, "%s: Cannot open %s\n", progname, argv[1] );
		return EXIT_SUCCESS;
	}

	bibl_init( &b1 );
	status = bibl_read( &b1, fp, argv[1], &p1 );
	fclose( fp );
	if ( status!=BIBL_OK ) {
		bibl_reporterr( status );
		return EXIT_FAILURE;
	}

	fp = fopen( argv[2], "r" );
	if ( !fp ) {
		fprintf( stderr, "%s: Cannot open %s\n", progname, argv[2] );
		return EXIT_SUCCESS;
	}

	bibl_init( &b2 );
	status = bibl_read( &b2, fp, argv[2], &p2 );
	fclose( fp );
	if ( status!=BIBL_OK ) {
		bibl_reporterr( status );
		return EXIT_FAILURE;
	}

	compare_bibliographies( &b1, argv[1], &b2, argv[2] );

	bibl_freeparams( &p1 );
	bibl_freeparams( &p2 );

	return EXIT_SUCCESS;
}
