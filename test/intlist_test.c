/*
 * intlist_test.c
 *
 * Copyright (c) 2013-2018
 *
 * Source code released under the GPL version 2
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "intlist.h"

char progname[] = "intlist_test";
char version[] = "0.1";

#define check( a, b ) { \
	if ( !(a) ) { \
		fprintf( stderr, "Failed %s (%s) in %s() line %d\n", #a, b, __FUNCTION__, __LINE__ );\
		return 1; \
	} \
}

#define check_len( a, b ) if ( !_check_len( a, b, __FUNCTION__, __LINE__ ) ) return 1;
int
_check_len( intlist *a, int expected, const char *fn, int line )
{
	if ( a->n == expected ) return 1;
	fprintf( stderr, "Failed: %s() line %d: Expected intlist length of %d, found %d\n", fn, line, expected, a->n );
	return 0;
}

#define check_entry( a, b, c ) if ( !_check_entry( a, b, c, __FUNCTION__, __LINE__ ) ) return 1;
int
_check_entry( intlist *a, int n, int expected, const char *fn, int line )
{
	int m;
	m = intlist_get( a, n );
	if ( m == expected ) return 1;
	fprintf( stderr, "Failed: %s() line %d: Expected intlist element %d to be %d, found %d\n",
		fn, line, n, expected, m );
	return 0;
}

/*
 * void      intlist_init( intlist *il );
 */
int
test_init( void )
{
	intlist il;

	intlist_init( &il );

	check_len( &il, 0 );

	intlist_free( &il );

	return 0;
}

/*
 * void      intlist_init_fill( intlist *il, int n, int value );
 */
#define COUNT (150)
int
test_init_fill( void )
{
	int i, status;
	intlist il;

	status = intlist_init_fill( &il, COUNT, 3121 );
	check( (status==INTLIST_OK), "intlist_init_fill() should return INTLIST_OK" );

	check_len( &il, COUNT );
	for ( i=0; i<COUNT; ++i )
		check_entry( &il, i, 3121 );

	intlist_free( &il );

	return 0;
}
#undef COUNT

/*
 * int       intlist_init_range( intlist *il, int low, int high, int step );
 */
int
test_init_range( void )
{
	intlist i1, i2, i3;
	int i, status;

	status = intlist_init_range( &i1, 0, 10, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );

	check_len( &i1, 10 );
	for ( i=0; i<10; ++i )
		check_entry( &i1, i, i );

	status = intlist_init_range( &i2, 0, 18, 5 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );

	check_len( &i2, 4 );
	for ( i=0; i<4; i++ )
		check_entry( &i2, i, i*5 );

	status = intlist_init_range( &i3, -10, -100, -10 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );

	check_len( &i3, 9 );
	for ( i=0; i<9; ++i )
		check_entry( &i3, i, -10 + -10*i );

	intlist_free( &i1 );
	intlist_free( &i2 );
	intlist_free( &i3 );

	return 0;
}

/*
 * intlist * intlist_new( void );
 */
int
test_new( void )
{
	intlist *il;

	il = intlist_new();
	check( (il!=NULL), "intlist_new() should not return NULL" );

	check_len( il, 0 );

	intlist_delete( il );

	return 0;
}

/*
 * intlist * intlist_new_fill( int n, int value );
 */
#define COUNT (1000)
int
test_new_fill( void )
{
	intlist *il;
	int i;

	il = intlist_new_fill( COUNT, 121 );
	check( (il!=NULL), "intlist_new_fill() should not return NULL" );

	check_len( il, COUNT );
	for ( i=0; i<COUNT; ++i )
		check_entry( il,  i, 121 );

	intlist_delete( il );

	return 0;
}
#undef COUNT

/*
 * intlist * intlist_new_range( int low, int high, int step );
 */
int
test_new_range( void )
{
	intlist *i1, *i2, *i3;
	int i;

	i1 = intlist_new_range( 0, 10, 1 );
	check( (i1!=NULL), "intlist_new_range() should not return NULL" );

	check_len( i1, 10 );
	for ( i=0; i<10; ++i )
		check_entry( i1, i, i );

	i2 = intlist_new_range( 0, 18, 5 );
	check( (i2!=NULL), "intlist_new_range() should not return NULL" );

	check_len( i2, 4 );
	for ( i=0; i<4; i++ )
		check_entry( i2, i, i*5 );

	i3 = intlist_new_range( -10, -100, -10 );
	check( (i3!=NULL), "intlist_new_range() should not return NULL" );

	check_len( i3, 9 );
	for ( i=0; i<9; ++i )
		check_entry( i3, i, -10 + -10*i );

	intlist_delete( i1 );
	intlist_delete( i2 );
	intlist_delete( i3 );

	return 0;
}

/*
 * int       intlist_add( intlist *il, int value );
 */
int
test_add( void )
{
	int status;
	intlist a;

	intlist_init( &a );

	status = intlist_add( &a, 1 );
	check( (status==INTLIST_OK), "intlist_add() should return INTLIST_OK" );
	check_len( &a, 1 );
	check_entry( &a, 0, 1 );

	status = intlist_add( &a, 2 );
	check( (status==INTLIST_OK), "intlist_add() should return INTLIST_OK" );
	check_len( &a, 2 );
	check_entry( &a, 0, 1 );
	check_entry( &a, 1, 2 );

	intlist_free( &a );

	return 0;
}

/*
 * int       intlist_add( intlist *il, int value );
 */
int
test_add_unique( void )
{
	int status;
	intlist a;

	intlist_init( &a );

	status = intlist_add_unique( &a, 100 );
	check( (status==INTLIST_OK), "intlist_add_unique() should return INTLIST_OK" );
	check_len( &a, 1 );
	check_entry( &a, 0, 100 );

	status = intlist_add_unique( &a, 100 );
	check( (status==INTLIST_OK), "intlist_add_unique() should return INTLIST_OK" );
	check_len( &a, 1 );
	check_entry( &a, 0, 100 );

	status = intlist_add_unique( &a, 200 );
	check( (status==INTLIST_OK), "intlist_add_unique() should return INTLIST_OK" );
	check_len( &a, 2 );
	check_entry( &a, 0, 100 );
	check_entry( &a, 1, 200 );

	intlist_free( &a );

	return 0;
}

/*
 * void      intlist_randomize( intlist *il );
 */
int
test_randomize( void )
{
	intlist a;
	int i, m;

	intlist_init_range( &a, 0, 100, 1 );
	check_len( &a, 100 );
	check_entry( &a, 0, 0 );
	check_entry( &a, 99, 99 );

	intlist_randomize( &a );

	/* ...can't check order, but can check that every value exists */
	check_len( &a, 100 );
	for ( i=0; i<99; ++i ) {
		m = intlist_find( &a, i );
		check( (m>=0&&m<99), "intlist_randomize() shouldn't remove values" );
	}

	intlist_free( &a );

	return 0;
}

/*
 * void      intlist_sort( intlist *il );
 */
int
test_sort( void )
{
	intlist a;
	int i;

	intlist_init_range( &a, 100, 0, -1 );
	check_len( &a, 100 );
	check_entry( &a, 0, 100 );
	check_entry( &a, 99, 1 );

	intlist_sort( &a );
	for ( i=0; i<100; ++i )
		check_entry( &a, i, i+1 );

	intlist_free( &a );
	return 0;
}

/*
 * int       intlist_fill( intlist *il, int n, int value );
 */
#define COUNT (1011)
int
test_fill( void )
{
	int i, status;
	intlist a;

	intlist_init( &a );

	status = intlist_fill( &a, COUNT, 51221 );
	check( (status==INTLIST_OK), "intlist_fill() should return INTLIST_OK" );
	check_len( &a, COUNT );
	for ( i=0; i<COUNT; ++i )
		check_entry( &a, i, 51221 );

	status = intlist_fill( &a, COUNT*2, 121 );
	check( (status==INTLIST_OK), "intlist_fill() should return INTLIST_OK" );
	check_len( &a, COUNT*2 );
	for ( i=0; i<COUNT*2; ++i )
		check_entry( &a, i, 121 );

	intlist_free( &a );

	return 0;
}
#undef COUNT

/*
 * int       intlist_fill_range( intlist *il, int low, int high, int step );
 */
#define COUNT (971)
int
test_fill_range( void )
{
	int i, status;
	intlist a;

	intlist_init( &a );

	status = intlist_fill_range( &a, 0, COUNT*2, 2 );
	check( (status==INTLIST_OK), "intlist_fill_range() should return INTLIST_OK" );
	check_len( &a, COUNT );
	for ( i=0; i<COUNT; ++i )
		check_entry( &a, i, i*2 );

	status = intlist_fill_range( &a, 0, COUNT, 1 );
	check( (status==INTLIST_OK), "intlist_fill_range() should return INTLIST_OK" );
	check_len( &a, COUNT );
	for ( i=0; i<COUNT; ++i )
		check_entry( &a, i, i );

	intlist_free( &a );

	return 0;
}
#undef COUNT

/*
 * int       intlist_find( intlist *il, int searchvalue );
 */
#define COUNT (315)
int
test_find( void )
{
	int i, m, status;
	intlist a;

	status = intlist_init_range( &a, 0, COUNT, 1 );
	check( (status==INTLIST_OK), "intlist_fill_range() should return INTLIST_OK" );
	check_len( &a, COUNT );

	for ( i=0; i<COUNT; ++i ) {
		m = intlist_find( &a, i );
		check( (m>=0&&m<=COUNT), "intlist_find() should find valid entries" );
	}

	for ( i=-100;i<0; ++i ) {
		m = intlist_find( &a, i );
		check( (m<0||m>=COUNT), "intlist_find() should not find invalid entries" );
	}

	for ( i=COUNT;i<COUNT+100; ++i ) {
		m = intlist_find( &a, i );
		check( (m<0||m>=COUNT), "intlist_find() should not find invalid entries" );
	}

	intlist_free( &a );

	return 0;
}

/*
 * int       intlist_find_or_add( intlist *il, int searchvalue );
 */
int
test_find_or_add( void )
{
	int i, m, n;
	intlist a;

	intlist_init_range( &a, 0, 150, 10 );
	check_len( &a, 15 );
	check_entry( &a, 0, 0 );
	check_entry( &a, 14, 140 );

	for ( i=0; i<150; i+=10 ) {
		m = intlist_find_or_add( &a, i );
		check( (m>=0&&m<15), "intlist_find_or_add() should find existing entries" );
	}

	n = a.n;
	for ( i=5; i<155; i+=10 ) {
		n++;
		m = intlist_find( &a, i );
		check( (m<0||m>=n), "intlist_find() should not find missing entries" );
		m = intlist_find_or_add( &a, i );
		check_len( &a, n );
		check_entry( &a, n-1, i );
		check( (m==n-1), "intlist_find_or_add() should find added entries" );
	}
	check_len( &a, 30 );

	intlist_free( &a );

	return 0;
}

/*
 * void      intlist_empty( intlist *il );
 */
int
test_empty( void )
{
	intlist a;
	int status;

	status = intlist_init_fill( &a, 100, 0 );
	check( (status==INTLIST_OK), "intlist_init_fill() should return INTLIST_OK" );
	check_len( &a, 100 );

	intlist_empty( &a );
	check_len( &a, 0 );

	status = intlist_fill( &a, 150, 10 );
	check( (status==INTLIST_OK), "intlist_fill() should return INTLIST_OK" );
	check_len( &a, 150 );

	intlist_empty( &a );
	check_len( &a, 0 );

	intlist_free( &a );

	return 0;
}

/*
 * int       intlist_copy( intlist *to, intlist *from );
 */
int
test_copy( void )
{
	int i, status;
	intlist a, b;

	status = intlist_init_fill( &a, 110, 11 );
	check( (status==INTLIST_OK), "intlist_init_fill() should return INTLIST_OK" );
	check_len( &a, 110 );
	for ( i=0; i<110; ++i )
		check_entry( &a, i, 11 );

	status = intlist_init_range( &b, 0, 60, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );
	check_len( &b, 60 );
	for ( i=0; i<60; ++i )
		check_entry( &b, i, i );

	status = intlist_copy( &b, &a );
	check( (status==INTLIST_OK), "intlist_copy() should return INTLIST_OK" );
	check_len( &a, 110 );
	for ( i=0; i<110; ++i )
		check_entry( &a, i, 11 );
	check_len( &b, 110 );
	for ( i=0; i<110; ++i )
		check_entry( &b, i, 11 );

	intlist_free( &a );
	intlist_free( &b );

	return 0;
}

/*
 * intlist * intlist_dup( intlist *from );
 */
int
test_dup( void )
{
	intlist a, *b;
	int i, status;

	status = intlist_init_range( &a, 0, 60, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );
	check_len( &a, 60 );
	for ( i=0; i<60; ++i )
		check_entry( &a, i, i );

	b = intlist_dup( &a );
	check( (b!=NULL), "intlist_dup() should not return NULL" );
	check_len( b, 60 );
	for ( i=0; i<60; ++i )
		check_entry( b, i, i );

	intlist_free( &a );
	intlist_delete( b );

	return 0;
}

/*
 * int intlist_get( intlist *il, int pos );
 */
int
test_get( void )
{
	int i, m, status;
	intlist a;

	status = intlist_init_range( &a, 0, 60, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );
	check_len( &a, 60 );
	for ( i=0; i<60; ++i ) {
		m = intlist_get( &a, i );
		check( (m==i), "intlist_get() should return value of the entry" );
	}

	intlist_free( &a );

	return 0;
}

/*
 * int intlist_set( intlist *il, int pos );
 */
int
test_set( void )
{
	int i, m, status;
	intlist a;

	status = intlist_init_range( &a, 0, 60, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );
	check_len( &a, 60 );

	for ( i=0; i<30; ++i ) {
		status = intlist_set( &a, i, -10 );
		check( (status==INTLIST_OK), "intlist_set() should return INTLIST_OK" );
		check_len( &a, 60 );
	}
		

	for ( i=0; i<60; ++i ) {
		m = intlist_get( &a, i );
		if ( i<30 ) {
			check( (m==-10), "intlist_get() should return value of the entry" );
		} else {
			check( (m==i), "intlist_get() should return value of the entry" );
		}
	}

	intlist_free( &a );

	return 0;
}

/*
 * int       intlist_append( intlist *to, intlist *from );
 */
int
test_append( void )
{
	int i, status;
	intlist a, b;

	status = intlist_init_range( &a, 0, 60, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );
	check_len( &a, 60 );

	status = intlist_init_range( &b, 60, 90, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );
	check_len( &b, 30 );

	status = intlist_append( &a, &b );
	check( (status==INTLIST_OK), "intlist_append() should return INTLIST_OK" );
	check_len( &a, 90 );

	for ( i=0; i<a.n; ++i )
		check_entry( &a, i, i );

	intlist_free( &a );
	intlist_free( &b );

	return 0;
}

/*
 * int       intlist_append_unique( intlist *to, intlist *from );
 */
int
test_append_unique( void )
{
	intlist a, b, c;
	int i, status;

	status = intlist_init_range( &a, 0, 60, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );
	check_len( &a, 60 );

	status = intlist_init_range( &b, 0, 60, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );
	check_len( &b, 60 );

	status = intlist_init_range( &c, 60, 90, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );
	check_len( &c, 30 );

	status = intlist_append_unique( &a, &b );
	check( (status==INTLIST_OK), "intlist_append_unique() should return INTLIST_OK" );
	check_len( &a, 60 );
	for ( i=0; i<a.n; ++i )
		check_entry( &a, i, i );
	check_len( &b, 60 );
	for ( i=0; i<b.n; ++i )
		check_entry( &b, i, i );

	status = intlist_append_unique( &a, &c );
	check( (status==INTLIST_OK), "intlist_append_unique() should return INTLIST_OK" );
	check_len( &a, 90 );
	for ( i=0; i<a.n; ++i )
		check_entry( &a, i, i );
	check_len( &c, 30 );
	for ( i=0; i<c.n; ++i )
		check_entry( &c, i, 60+i );

	intlist_free( &a );
	intlist_free( &b );
	intlist_free( &c );

	return 0;
}

/*
 * int       intlist_remove( intlist *il, int searchvalue );
 */
int
test_remove( void )
{
	int i, status;
	intlist a;

	status = intlist_init_fill( &a, 100, 1 );
	check( (status==INTLIST_OK), "intlist_init_fill() should return INTLIST_OK" );
	check_len( &a, 100 );

	/* ...try to remove non-existant elements */
	for ( i=0; i<100; ++i ) {
		if ( i==1 ) continue;
		status = intlist_remove( &a, i );
		check( (status==INTLIST_VALUE_MISSING), "intlist_remove() should return INTLIST_VALUE_MISSING" );
	}
	check_len( &a, 100 );

	/* ...try to remove existing elements (intlist_remove does one at a time) */
	for ( i=0; i<100; ++i ) {
		status = intlist_remove( &a, 1 );
		check_len( &a, 99-i );
		check( (status==0), "intlist_remove() should return position of first element to be removed" );
	}
	check_len( &a, 0 );

	intlist_free( &a );

	return 0;
}

/*
 * int intlist_remove_pos( intlist *il, int pos );
 */
int
test_remove_pos( void )
{
	int status;
	intlist a;

	status = intlist_init_range( &a, 0, 5, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );
	check_len( &a, 5 );
	check_entry( &a, 0, 0 );
	check_entry( &a, 1, 1 );
	check_entry( &a, 2, 2 );
	check_entry( &a, 3, 3 );
	check_entry( &a, 4, 4 );

	/* ...first element */
	status = intlist_remove_pos( &a, 0 );
	check( (status==INTLIST_OK), "intlist_remove_pos() should return INTLIST_OK" );
	check_len( &a, 4 );
	check_entry( &a, 0, 1 );
	check_entry( &a, 1, 2 );
	check_entry( &a, 2, 3 );
	check_entry( &a, 3, 4 );

	/* ...last element */
	status = intlist_remove_pos( &a, 3 );
	check( (status==INTLIST_OK), "intlist_remove_pos() should return INTLIST_OK" );
	check_len( &a, 3 );
	check_entry( &a, 0, 1 );
	check_entry( &a, 1, 2 );
	check_entry( &a, 2, 3 );

	/* ...middle element */
	status = intlist_remove_pos( &a, 1 );
	check( (status==INTLIST_OK), "intlist_remove_pos() should return INTLIST_OK" );
	check_len( &a, 2 );
	check_entry( &a, 0, 1 );
	check_entry( &a, 1, 3 );

	/* ...last elements */
	status = intlist_remove_pos( &a, 1 );
	check( (status==INTLIST_OK), "intlist_remove_pos() should return INTLIST_OK" );
	check_len( &a, 1 );
	check_entry( &a, 0, 1 );
	status = intlist_remove_pos( &a, 0 );
	check( (status==INTLIST_OK), "intlist_remove_pos() should return INTLIST_OK" );
	check_len( &a, 0 );

	intlist_free( &a );

	return 0;
}

/*
 * float     intlist_median( intlist *il );
 */
int
test_median( void )
{
	intlist a, b, c, d;
	float median;
	int status;

	status = intlist_init_fill( &a, 5, 1 );
	check( (status==INTLIST_OK), "intlist_init_fill() should return INTLIST_OK" );

	median = intlist_median( &a );
	check( (fabs(median-1.0)<1e-7), "intlist_median() should be 1" );

	status = intlist_init_range( &b, 0, 5, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );

	median = intlist_median( &b );
	check( (fabs(median-2.0)<1e-7), "intlist_median() should be 2" );

	status = intlist_init_range( &c, 0, 6, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );

	median = intlist_median( &c );
	check( (fabs(median-2.5)<1e-7), "intlist_median() should be 2.5" );

	status = intlist_init_range( &d, 5, -1, -1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );

	median = intlist_median( &d );
	check( (fabs(median-2.5)<1e-7), "intlist_median() should be 2.5" );

	intlist_free( &a );
	intlist_free( &b );
	intlist_free( &c );
	intlist_free( &d );

	return 0;
}

/*
 * float     intlist_mean( intlist *il );
 */
int
test_mean( void )
{
	intlist a, b, c, d;
	float mean;
	int status;

	status = intlist_init_fill( &a, 5, 1 );
	check( (status==INTLIST_OK), "intlist_init_fill() should return INTLIST_OK" );

	mean = intlist_mean( &a );
	check( (fabs(mean-1.0)<1e-7), "intlist_mean() should be 1" );

	status = intlist_init_range( &b, 0, 5, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );

	mean = intlist_mean( &b );
	check( (fabs(mean-2.0)<1e-7), "intlist_mean() should be 2" );

	status = intlist_init_range( &c, 0, 6, 1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );

	mean = intlist_mean( &c );
	check( (fabs(mean-2.5)<1e-7), "intlist_mean() should be 2.5" );

	status = intlist_init_range( &d, 5, -1, -1 );
	check( (status==INTLIST_OK), "intlist_init_range() should return INTLIST_OK" );

	mean = intlist_mean( &d );
	check( (fabs(mean-2.5)<1e-7), "intlist_mean() should be 2.5" );

	intlist_free( &a );
	intlist_free( &b );
	intlist_free( &c );
	intlist_free( &d );

	return 0;
}

int
main( int argc, char *argv[] )
{
	int failed = 0;

	failed += test_init();
	failed += test_init_fill();
	failed += test_init_range();

	failed += test_new();
	failed += test_new_fill();
	failed += test_new_range();

	failed += test_add();
	failed += test_add_unique();

	failed += test_randomize();
	failed += test_sort();

	failed += test_fill();
	failed += test_fill_range();

	failed += test_find();
	failed += test_find_or_add();

	failed += test_empty();

	failed += test_copy();
	failed += test_dup();

	failed += test_get();
	failed += test_set();

	failed += test_append();
	failed += test_append_unique();

	failed += test_remove();
	failed += test_remove_pos();

	failed += test_median();
	failed += test_mean();

	if ( !failed ) {
		printf( "%s: PASSED\n", progname );
		return EXIT_SUCCESS;
	} else {
		printf( "%s: FAILED\n", progname );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
