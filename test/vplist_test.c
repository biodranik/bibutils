/*
 * vplist_test.c
 *
 * Copyright (c) 2014-2018
 *
 * Source code released under the GPL version 2
 *
 *
 * test vplist functions
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "vplist.h"

/*
 * typedef struct vplist {
 *    vplist_index n, max;
 *    void **data;
 * } vplist;
 */

const char progname[] = "vplist_test";
const char version[] = "0.1";

#define report_memerr( a ) { \
	fprintf( stderr, "Failed: %s() line %d: %s() did not return VPLIST_OK, memory error\n", __FUNCTION__, __LINE__, a ); \
	return 1; \
}

#define check_len( a, b ) if ( !_check_len( a, b, __FUNCTION__, __LINE__ ) ) return 1;
int
_check_len( vplist *a, int expected, const char *fn, int line )
{
	if ( a->n == expected ) return 1;
	fprintf( stderr, "Failed: %s() line %d: Expected list length of %d, found %d\n", fn, line, expected, a->n );
	return 0;
}

#define check_entry( a, b, c ) if ( !_check_entry( a, b, c, __FUNCTION__, __LINE__ ) ) return 1;
int
_check_entry( vplist *a, int n, const void *expected, const char *fn, int line )
{
	void *v;
	v = vplist_get( a, n );
	if ( v==NULL && expected==NULL ) return 1;
	if ( v!=NULL && expected==NULL ) {
		fprintf( stderr, "Failed: %s() line %d: Expected list element %d to be NULL, found %p '%s'\n",
			fn, line, n, v, (char*)v );
		return 0;
	}
	if ( v==NULL && expected!=NULL ) {
		fprintf( stderr, "Failed: %s() line %d: Expected list element %d to be %p '%s', found NULL\n",
			fn, line, n, expected, (char*)expected );
		return 0;
	}
	if ( v == expected ) return 1;
	fprintf( stderr, "Failed: %s() line %d: Expected list element %d to be %p '%s', found %p '%s'\n",
		fn, line, n, expected, (char*)expected, v, (char*)v );
	return 0;
}

#define check_isempty( a ) { \
	check_len( a, 0 ); \
	check_entry( a, -1, NULL ); \
	check_entry( a,  0, NULL ); \
	check_entry( a,  1, NULL ); \
}

/* vplist 'a' will hold strings "0", "1", "2" .... */
static int
build_vplist_a( vplist *a, char *s[], int lens )
{
	vplist_index i, j;
	char buf[256];
	int status;

	for ( i=0; i<lens; ++i ) {

		sprintf( buf, "%c", '0' + i );
		s[i] = strdup( buf );

		status = vplist_add( a, s[i] );
		if ( status!=VPLIST_OK ) report_memerr( "vplist_add" );

		check_len( a, i+1 );
		check_entry( a, -1, NULL );
		for ( j=0; j<=i; ++j )
			check_entry( a, j, s[j] );
		check_entry( a, i+1, NULL );

	}
	check_len( a, lens );

	return 0;
}

/* vplist 'b' will hold strings "a", "b", "c" .... */
static int
build_vplist_b( vplist *b, char *t[], int lent )
{
	vplist_index i, j;
	char buf[256];
	int status;

	for ( i=0; i<lent; ++i ) {

		sprintf( buf, "%c", 'a' + i );
		t[i] = strdup( buf );

		status = vplist_add( b, t[i] );
		if ( status!=VPLIST_OK ) report_memerr( "vplist_add" );

		check_len( b, i+1 );
		check_entry( b, -1, NULL );
		for ( j=0; j<=i; ++j )
			check_entry( b, j, t[j] );
		check_entry( b, i+1, NULL );

	}
	check_len( b, lent );

	return 0;
}

/*
 * void vplist_init( vplist *vpl );
 */
int
test_init( void )
{
	vplist a;

	vplist_init( &a );
	check_isempty( &a );
	vplist_free( &a );

	return 0;
}

/*
 * vplist * vplist_new( void );
 */
int
test_new( void )
{
	vplist *a;

	a = vplist_new();
	check_isempty( a );
	vplist_delete( &a );

	return 0;
}

/*
 * int vplist_add( vplist *vpl, void *v );
 *
 * returns VPLIST_OK or VPLIST_MEMERR
 */
#define LENS (5)
int
test_add( void )
{
	vplist_index i;
	char *s[LENS];
	int status;
	vplist a;

	vplist_init( &a );
	check_isempty( &a );

	/* vplist 'a' will hold strings "0", "1", "2" .... */
	status = build_vplist_a( &a, s, LENS );
	if ( status!=0 ) return 1;

	for ( i=0; i<LENS; ++i )
		free( s[i] );

	vplist_free( &a );

	return 0;
}
#undef LENS

/*
 * int vplist_fill( vplist *vpl, vplist_index n, void *v );
 *
 * returns VPLIST_OK or VPLIST_MEMERR
 */
int
test_fill( void )
{
	vplist_index i, n = 5;
	int status;
	vplist a;

	vplist_init( &a );
	check_isempty( &a );

	status = vplist_fill( &a, n, NULL );
	if ( status!=VPLIST_OK ) report_memerr( "vplist_fill" );
	check_len( &a, n );

	for ( i=0; i<n; ++i ) {
		if ( vplist_get( &a, i ) != NULL ) {
			fprintf( stderr, "Failed: %s() line %d: vplist_get() returned %p, expected NULL\n",
				__FUNCTION__, __LINE__, vplist_get( &a, i ) );
	
			return 1;
		}
	}

	vplist_empty( &a );
	check_isempty( &a );

	status = vplist_fill( &a, n, (void*)&a );
	check_len( &a, n );

	for ( i=0; i<n; ++i ) {
		if ( vplist_get( &a, i ) != (void*)&a ) {
			fprintf( stderr, "Failed: %s() line %d: vplist_get() returned %p, expected %p\n",
				__FUNCTION__, __LINE__, vplist_get( &a, i ), &a );
	
			return 1;
		}
	}

	vplist_free( &a );

	return 0;
}

/*
 * int vplist_copy( vplist *to, vplist *from );
 *
 * returns VPLIST_OK or VPLIST_MEMERR
 */
#define LENS (5)
#define LENT (20)
int
test_copy( void )
{
	char *s[LENS], *t[LENT];
	vplist_index i, j;
	vplist a, b;
	int status;

	/* vplist 'a' will hold strings "0", "1", "2" .... */
	vplist_init( &a );
	check_isempty( &a );
	status = build_vplist_a( &a, s, LENS );
	if ( status!=0 ) return 1;

	/* vplist 'b' will hold strings "a", "b", "c" ... */
	vplist_init( &b );
	check_isempty( &b );
	status = build_vplist_b( &b, t, LENT );
	if ( status!=0 ) return 1;

	/* copy vplist 'a' to 'b' */
	status = vplist_copy( &b, &a );
	if ( status != VPLIST_OK ) report_memerr( "vplist_copy" );

	/* check that copy worked */
	check_len( &b, LENS );
	check_entry( &b, -1, NULL );
	for ( j=0; j<LENS; ++j )
		check_entry( &b, j, s[j] );
	check_entry( &b, LENS, NULL );

	for ( i=0; i<LENS; ++i )
		free( s[i] );

	for ( i=0; i<LENT; ++i )
		free( t[i] );

	vplist_free( &a );
	vplist_free( &b );

	return 0;
}
#undef LENS
#undef LENT

/*
 * int vplist_append( vplist *to, vplist *from );
 *
 * returns VPLIST_OK or VPLIST_MEMERR
 */
#define LENS (15)
#define LENT (3)
int
test_append( void )
{
	char *s[LENS], *t[LENT];
	vplist_index i;
	vplist a, b;
	int status;

	/* vplist 'a' will hold strings "0", "1", "2" .... */
	vplist_init( &a );
	check_isempty( &a );
	status = build_vplist_a( &a, s, LENS );
	if ( status!=0 ) return 1;

	/* vplist 'b' will hold strings "a", "b", "c" ... */
	vplist_init( &b );
	check_isempty( &b );
	status = build_vplist_b( &b, t, LENT );
	if ( status!=0 ) return 1;

	/* append vplist 'a' to 'b' */
	status = vplist_append( &b, &a );
	if ( status != VPLIST_OK ) report_memerr( "vplist_append" );

	/* check that vplist 'a' is unchanged */
	check_len( &a, LENS );
	check_entry( &a, -1, NULL );
	for ( i=0; i<LENS; ++i )
		check_entry( &a, i, s[i] );
	check_entry( &a, LENS, NULL );

	/* check that vplist 'b' has been appended to*/
	check_len( &b, LENS+LENT );
	check_entry( &b, -1, NULL );
	for ( i=0; i<LENT; ++i )
		check_entry( &b, i, t[i] );
	for ( i=0; i<LENS; ++i )
		check_entry( &b, i+LENT, s[i] );
	check_entry( &b, LENS+LENT, NULL );

	for ( i=0; i<LENS; ++i ) free( s[i] );

	for ( i=0; i<LENT; ++i ) free( t[i] );

	vplist_free( &a );
	vplist_free( &b );

	return 0;
}
#undef LENS
#undef LENT

/*
 * int vplist_insert_list( vplist *vpl, vlist_index pos, vplist *add );
 *
 * returns VPLIST_OK or VPLIST_MEMERR
 */
#define LENS (5)
#define INSERTPOS (3)
#define LENT (26)
int
test_insert_list( void )
{
	char *s[LENS], *t[LENT];
	vplist_index i;
	vplist a, b;
	int status;

	/* vplist 'a' will hold strings "0", "1", "2" .... */
	vplist_init( &a );
	check_isempty( &a );
	status = build_vplist_a( &a, s, LENS );
	if ( status!=0 ) return 1;

	vplist_init( &b );
	check_isempty( &b );

	/* check insertion of empty vplist -- a should be unchanged*/
	status = vplist_insert_list( &a, INSERTPOS, &b );
	if ( status != VPLIST_OK ) report_memerr( "vplist_insert_list" );
	check_len( &a, LENS );
	for ( i=0; i<LENS; ++i ) check_entry( &a, i, s[i] );
	check_entry( &a, LENS, NULL );

	/* vplist 'b' will hold strings "a", "b", "c" ... */
	status = build_vplist_b( &b, t, LENT );
	if ( status != 0 ) return 1;

	/* insert vplist 'b' into 'a' */
	status = vplist_insert_list( &a, INSERTPOS, &b );
	if ( status != VPLIST_OK ) report_memerr( "vplist_insert_list" );
	check_len( &a, LENS + LENT );
	for ( i=0; i<INSERTPOS; ++i ) check_entry( &a, i, s[i] );
	for ( i=INSERTPOS+LENT; i<LENS+LENT; ++i ) check_entry( &a, i, s[i-LENT] );
	for ( i=INSERTPOS; i<INSERTPOS+LENT; ++i ) check_entry( &a, i, t[i-INSERTPOS] );

	for ( i=0; i<LENS; ++i )
		free( s[i] );

	for ( i=0; i<LENT; ++i )
		free( t[i] );

	vplist_free( &a );
	vplist_free( &b );

	return 0;
}
#undef LENS
#undef INSERTPOS
#undef LENT

/*
 * void * vplist_get( vplist *vpl, vplist_index n );
 */
#define LENS (5)
int
test_get( void )
{
	vplist_index i;
	char *s[LENS];
	int status;
	vplist a;
	void *v;

	/* vplist 'a' will hold strings "0", "1", "2" .... */
	vplist_init( &a );
	check_isempty( &a );
	status = build_vplist_a( &a, s, LENS );
	if ( status!=0 ) return 1;

	for ( i=0; i<LENS; ++i ) {
		v = vplist_get( &a, i );
		if ( v != s[i] ) {
			fprintf( stderr, "Failed: %s() line %d: vplist_get() returned %p '%s', expected %p '%s'\n",
				__FUNCTION__, __LINE__, vplist_get( &a, i ), (char*)vplist_get( &a, i ),
				s[i], (char*)s[i] );
			return 1;
		}
	}

	for ( i=0; i<LENS; ++i ) free( s[i] );

	vplist_free( &a );

	return 0;
}
#undef LENS

/*
 * void vplist_set( vplist *vpl, vplist_index n, void *v );
 */
#define LENS (5)
int
test_set( void )
{
	char *s[LENS], *t[LENS], buf[256];
	vplist_index i, j;
	int status;
	vplist a;

	/* vplist 'a' will hold strings "0", "1", "2" .... */
	vplist_init( &a );
	check_isempty( &a );
	status = build_vplist_a( &a, s, LENS );
	if ( status!=0 ) return 1;

	/* t[] will hold strings "a", "b", "c", ... */
	for ( i=0; i<LENS; ++i ) {
		sprintf( buf, "%c", 'a' + i );
		t[i] = strdup( buf );
	}

	for ( i=0; i<LENS; ++i ) {
		vplist_set( &a, i, t[i] );
		check_len( &a, LENS );
		check_entry( &a, -1, NULL );
		for ( j=0; j<i+1; ++j )
			check_entry( &a, j, t[j] );
		for ( j=i+1; j<LENS; ++j )
			check_entry( &a, j, s[j] );
	}

	for ( i=0; i<LENS; ++i ) {
		free( s[i] );
		free( t[i] );
	}
	vplist_free( &a );

	return 0;
}
#undef LENS

/*
 * void vplist_swap( vplist *vpl, vplist_index n1, vplist_index n2 );
 */
#define LENS (5)
int
test_swap( void )
{
	vplist_index i;
	char *s[LENS];
	int status;
	vplist a;

	/* vplist 'a' will hold strings "0", "1", "2" .... */
	vplist_init( &a );
	check_isempty( &a );
	status = build_vplist_a( &a, s, LENS );
	if ( status!=0 ) return 1;

	vplist_swap( &a, 0, 4 );
	check_entry( &a, 0, s[4] );
	check_entry( &a, 4, s[0] );
	check_len( &a, 5 );

	vplist_swap( &a, 1, 3 );
	check_entry( &a, 1, s[3] );
	check_entry( &a, 3, s[1] );
	check_len( &a, 5 );

	vplist_swap( &a, 2, 2 );
	check_entry( &a, 2, s[2] );
	check_len( &a, 5 );

	for ( i=0; i<LENS; ++i )
		free( s[i] );

	vplist_free( &a );

	return 0;
}
#undef LENS

/*
 * vplist_index vplist_find( vplist *vpl, void *v );
 */
#define LENS (3)
#define LENT (5)
int
test_find( void )
{
	char *s[LENS], *t[LENT], buf[256];
	vplist_index i, n;
	int status;
	vplist a;

	/* vplist 'a' will hold strings "0", "1", "2" .... */
	vplist_init( &a );
	check_isempty( &a );
	status = build_vplist_a( &a, s, LENS );
	if ( status!=0 ) return 1;

	/* t[] will hold strings "a", "b", "c", ... */
	for ( i=0; i<LENT; ++i ) {
		sprintf( buf, "%c", 'a' + i );
		t[i] = strdup( buf );
	}

	for ( i=0; i<LENS; ++i ) {
		n = vplist_find( &a, s[i] );
		if ( n!=i ) {
			fprintf( stderr, "Failed: %s() line %d: vplist_find() returned %d, expected %d\n",
				__FUNCTION__, __LINE__, n, i );
			return 1;
		}
	}

	for ( i=0; i<LENT; ++i ) {
		n = vplist_find( &a, t[i] );
		if ( n!=-1 ) {
			fprintf( stderr, "Failed: %s() line %d: vplist_find() returned %d, expected -1\n",
				__FUNCTION__, __LINE__, n );
			return 1;
		}
	}

	for ( i=0; i<LENS; ++i ) free( s[i] );
	for ( i=0; i<LENT; ++i ) free( t[i] );
	vplist_free( &a );

	return 0;

}
#undef LENS
#undef LENT

/*
 * void vplist_remove( vplist *vpl, vplist_index n );
 */
#define LENS (5)
int
test_remove( void )
{
	vplist_index i;
	char *s[LENS];
	int status;
	vplist a;

	/* vplist 'a' will hold strings "0", "1", "2" .... */
	vplist_init( &a );
	check_isempty( &a );
	status = build_vplist_a( &a, s, LENS );
	if ( status!=0 ) return 1;

	vplist_remove( &a, 2 );
	check_len( &a, 4 );
	check_entry( &a, 0, s[0] );
	check_entry( &a, 1, s[1] );
	check_entry( &a, 2, s[3] );
	check_entry( &a, 3, s[4] );

	vplist_remove( &a, 0 );
	check_len( &a, 3 );
	check_entry( &a, 0, s[1] );
	check_entry( &a, 1, s[3] );
	check_entry( &a, 2, s[4] );

	vplist_remove( &a, 2 );
	check_len( &a, 2 );
	check_entry( &a, 0, s[1] );
	check_entry( &a, 1, s[3] );

	vplist_remove( &a, 1 );
	check_len( &a, 1 );
	check_entry( &a, 0, s[1] );

	vplist_remove( &a, 0 );
	check_isempty( &a );

	for ( i=0; i<LENS; ++i )
		free( s[i] );

	vplist_free( &a );

	return 0;
}
#undef LENS

/*
 * void vplist_removevp( vplist *vpl, void *v );
 */
#define LENS (5)
int
test_removevp( void )
{
	int status, ret, failed = 0;
	vplist_index i;
	char *s[LENS];
	vplist a;

	/* vplist 'a' will hold strings "0", "1", "2" .... */
	vplist_init( &a );
	check_isempty( &a );
	status = build_vplist_a( &a, s, LENS );
	if ( status!=0 ) return 1;

	/* this vplist_removevp() call should leave vpl unchanged */
	ret = vplist_removevp( &a, (void*) &a );
	if ( ret!=0 ) {
		fprintf( stderr, "Failed: %s() line %d: Expected vplist_removevp() to return %d, got %d\n", __FUNCTION__, __LINE__, 0, ret );
		failed++;
	}
	check_len( &a, 5 );
	check_entry( &a, 0, s[0] );
	check_entry( &a, 1, s[1] );
	check_entry( &a, 2, s[2] );
	check_entry( &a, 3, s[3] );
	check_entry( &a, 4, s[4] );

	ret = vplist_removevp( &a, (void*) s[2] );
	if ( ret!=1 ) {
		fprintf( stderr, "Failed: %s() line %d: Expected vplist_removevp() to return %d, got %d\n", __FUNCTION__, __LINE__, 1, ret );
		failed++;
	}
	check_len( &a, 4 );
	check_entry( &a, 0, s[0] );
	check_entry( &a, 1, s[1] );
	check_entry( &a, 2, s[3] );
	check_entry( &a, 3, s[4] );

	ret = vplist_removevp( &a, (void*) s[0] );
	if ( ret!=1 ) {
		fprintf( stderr, "Failed: %s() line %d: Expected vplist_removevp() to return %d, got %d\n", __FUNCTION__, __LINE__, 1, ret );
		failed++;
	}
	check_len( &a, 3 );
	check_entry( &a, 0, s[1] );
	check_entry( &a, 1, s[3] );
	check_entry( &a, 2, s[4] );

	ret = vplist_removevp( &a, (void*) s[4] );
	if ( ret!=1 ) {
		fprintf( stderr, "Failed: %s() line %d: Expected vplist_removevp() to return %d, got %d\n", __FUNCTION__, __LINE__, 1, ret );
		failed++;
	}
	check_len( &a, 2 );
	check_entry( &a, 0, s[1] );
	check_entry( &a, 1, s[3] );

	ret = vplist_removevp( &a, (void*) s[3] );
	if ( ret!=1 ) {
		fprintf( stderr, "Failed: %s() line %d: Expected vplist_removevp() to return %d, got %d\n", __FUNCTION__, __LINE__, 1, ret );
		failed++;
	}
	check_len( &a, 1 );
	check_entry( &a, 0, s[1] );

	ret = vplist_removevp( &a, (void*) s[1] );
	if ( ret!=1 ) {
		fprintf( stderr, "Failed: %s() line %d: Expected vplist_removevp() to return %d, got %d\n", __FUNCTION__, __LINE__, 1, ret );
		failed++;
	}
	check_isempty( &a );

	for ( i=0; i<LENS; ++i )
		free( s[i] );

	vplist_free( &a );

	return failed;
}
#undef LENS

/*
 * void vplist_remove_range( vplist *vpl, vplist_index start, vplist_index endplusone );
 */
#define LENS (5)
int
test_remove_range( void )
{
	int status, failed = 0;
	vplist_index i;
	char *s[LENS];
	vplist a;

	/* vplist 'a' will hold strings "0", "1", "2" .... */
	vplist_init( &a );
	check_isempty( &a );
	status = build_vplist_a( &a, s, LENS );
	if ( status!=0 ) return 1;

	vplist_remove_range( &a, 1, 3 );
	check_len( &a, 3 );
	check_entry( &a, 0, s[0] );
	check_entry( &a, 1, s[3] );
	check_entry( &a, 2, s[4] );

	vplist_remove_range( &a, 0, 1 );
	check_len( &a, 2 );
	check_entry( &a, 0, s[3] );
	check_entry( &a, 1, s[4] );

	vplist_remove_range( &a, 0, 2 );
	check_isempty( &a );

	for ( i=0; i<LENS; ++i )
		free( s[i] );

	vplist_free( &a );

	return failed;
}
#undef LENS

int
main( int argc, char *argv[] )
{
	int failed = 0;

	failed += test_init();
	failed += test_new();

	failed += test_add();
	failed += test_fill();
	failed += test_copy();
	failed += test_append();
	failed += test_insert_list();

	failed += test_get();
	failed += test_set();

	failed += test_swap();

	failed += test_find();

	failed += test_remove();
	failed += test_removevp();
	failed += test_remove_range();

	if ( !failed ) {
		printf( "%s: PASSED\n", progname );
		return EXIT_SUCCESS;
	} else {
		printf( "%s: FAILED\n", progname );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
