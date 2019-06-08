/*
 * slist_test.c
 *
 * test slist functions
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "slist.h"

char progname[] = "slist_test";
char version[] = "0.3";

#define check( a, b ) { \
	if ( !(a) ) { \
		fprintf( stderr, "Failed %s (%s) in %s() line %d\n", #a, b, __FUNCTION__, __LINE__ );\
		return 1; \
	} \
}

#define check_len( a, b ) if ( !_check_len( a, b, __FUNCTION__, __LINE__ ) ) return 1;
int
_check_len( slist *a, int expected, const char *fn, int line )
{
	if ( a->n == expected ) return 1;
	fprintf( stderr, "Failed: %s() line %d: Expected slist length of %d, found %d\n", fn, line, expected, a->n );
	return 0;
}

#define check_entry( a, b, c ) if ( !_check_entry( a, b, c, __FUNCTION__, __LINE__ ) ) return 1;
int
_check_entry( slist *a, int n, const char *expected, const char *fn, int line )
{
	char *s;
	s = slist_cstr( a, n );
	if ( s==NULL && expected==NULL ) return 1;
	if ( s!=NULL && expected==NULL ) {
		fprintf( stderr, "Failed: %s() line %d: Expected slist element %d to be NULL, found '%s'\n",
			fn, line, n, s );
		return 0;
	}
	if ( s==NULL && expected!=NULL ) {
		fprintf( stderr, "Failed: %s() line %d: Expected slist element %d to be '%s', found NULL\n",
			fn, line, n, expected );
		return 0;
	}
	if ( !strcmp( s, expected ) ) return 1;
	fprintf( stderr, "Failed: %s() line %d: Expected slist element %d to be '%s', found '%s'\n",
		fn, line, n, expected, s );
	return 0;
}

#define check_add_result( a, b ) if ( !_check_add_result( a, b, __FUNCTION__, __LINE__ ) ) return 1;
int
_check_add_result( str *obtained, str *expected, const char *fn, int line )
{
	if ( obtained==NULL && expected!=NULL ) {
		fprintf( stderr, "Failed to add string: %s() line %d: Expected '%s'\n",
			fn, line, expected->data );
		return 0;
	}
	return 1;
}

#define check_addc_result( a, b ) if ( !_check_addc_result( a, b, __FUNCTION__, __LINE__ ) ) return 1;
int
_check_addc_result( str *obtained, char *expected, const char *fn, int line )
{
	if ( obtained==NULL && expected!=NULL ) {
		fprintf( stderr, "Failed to add string: %s() line %d: Expected '%s'\n",
			fn, line, expected );
		return 0;
	}
	return 1;
}

int
test_init( void )
{
	slist a;

	slist_init( &a );

	check_len( &a, 0 );
	check_entry( &a, -1, NULL );
	check_entry( &a,  0, NULL );
	check_entry( &a,  1, NULL );

	slist_free( &a );

	return 0;
}

int
test_add( void )
{
	str s, *t;
	slist a;
	str_init( &s );
	slist_init( &a );

	str_strcpyc( &s, "1" );
	t = slist_add( &a, &s );
	check_add_result( t, &s );
	check_len( &a, 1 );
	check_entry( &a, 0, "1" );
	check_entry( &a, 1, NULL );

	str_strcpyc( &s, "2" );
	t = slist_add( &a, &s );
	check_add_result( t, &s );
	check_len( &a, 2 );
	check_entry( &a, 0, "1" );
	check_entry( &a, 1, "2" );
	check_entry( &a, 2, NULL );

	slist_free( &a );
	str_free( &s );
	return 0;
}

int
test_addc( void )
{
	str *t;
	slist a;
	slist_init( &a );

	t = slist_addc( &a, "1" );
	check_addc_result( t, "1" );
	check_len( &a, 1 );
	check_entry( &a, 0, "1" );
	check_entry( &a, 1, NULL );

	t = slist_addc( &a, "2" );
	check_addc_result( t, "2" );
	check_len( &a, 2 );
	check_entry( &a, 0, "1" );
	check_entry( &a, 1, "2" );
	check_entry( &a, 2, NULL );

	slist_free( &a );
	return 0;
}

int
test_addvp( void )
{
	str s, *t;
	slist a;
	str_init( &s );
	slist_init( &a );

	t= slist_addvp( &a, SLIST_CHR, "1" );
	check_addc_result( t, "1" );
	check_len( &a, 1 );
	check_entry( &a, 0, "1" );
	check_entry( &a, 1, NULL );

	str_strcpyc( &s, "2" );
	t = slist_addvp( &a, SLIST_STR, &s );
	check_add_result( t, &s );
	check_len( &a, 2 );
	check_entry( &a, 0, "1" );
	check_entry( &a, 1, "2" );
	check_entry( &a, 2, NULL );

	str_free( &s );
	slist_free( &a );
	return 0;
}

int
test_add_all( void )
{
	str s, t;
	int i, j;
	slist a;

	str_initstrc( &s, "a" );
	str_initstrc( &t, "b" );
	slist_init( &a );

	for ( j=0; j<10; ++j ) {
		for ( i=0; i<10; ++i ) {
			slist_add_all( &a, &s, &t, NULL );
			check_len( &a, (i+1)*2 );
		}
		slist_empty( &a );
	}

	for ( i=0; i<a.n; ++i ) {
		if ( i % 2 == 0 ) {
			check_entry( &a, i, "a" );
		} else {
			check_entry( &a, i, "b" );
		}
	}

	str_free( &s );
	str_free( &t );
	slist_free( &a );

	return 0;
}

#define COUNT (10)
int
test_addc_all( void )
{
	char *u;
	slist a;
	int i;

	slist_init( &a );

	for ( i=0; i<COUNT; ++i ) {
		slist_addc_all( &a, "a", "b", NULL );
		check_len( &a, (i+1)*2 );
	}

	for ( i=0; i<a.n; ++i ) {
		u = slist_cstr( &a, i );
		if ( i % 2 == 0 ) {
			check( (!strcmp(u,"a")), "even entries should be 'a'" );
		} else {
			check( (!strcmp(u,"b")), "odd entries should be 'b'" );
		}
	}

	slist_free( &a );

	return 0;
}
#undef COUNT

#define COUNT (10)
int
test_addvp_all( void )
{
	str s, t;
	int i, j;
	char *u;
	slist a;

	str_init( &s );
	str_init( &t );
	slist_init( &a );

	str_strcpyc( &s, "amateurish" );
	str_strcpyc( &t, "boorish" );
	for ( j=0; j<COUNT; ++j ) {
		for ( i=0; i<2; ++i ) {
			if ( i%2==0 ) slist_addvp_all( &a, SLIST_CHR, "amateurish", "boorish", NULL );
			else          slist_addvp_all( &a, SLIST_STR, &s,  &t,  NULL );
		}
		check( ( a.n == (j+1)*4 ), "length should increase by 4 each time" );
	}

	for ( i=0; i<a.n; ++i ) {
		u = slist_cstr( &a, i );
		if ( i % 2 == 0 ) {
			check( (!strcmp(u,"amateurish")), "even entries should be 'amateurish'" );
		} else {
			check( (!strcmp(u,"boorish")), "odd entries should be 'boorish'" );
		}
	}

	str_free( &s );
	str_free( &t );
	slist_free( &a );

	return 0;
}
#undef COUNT

/*
 * str * slist_add_unique( slist *a, str *value );
 */
int
test_add_unique( void )
{
	str s, t;
	int i, j;
	slist a;

	str_initstrc( &s, "amateurish" );
	str_initstrc( &t, "boorish" );
	slist_init( &a );

	for ( j=0; j<10; ++j ) {
		for ( i=0; i<10; ++i ) {
			slist_add_unique( &a, &s );
			check_len( &a, 1 );
		}
		slist_empty( &a );
	}

	for ( j=0; j<10; ++j ) {
		for ( i=0; i<10; ++i ) {
			slist_add_unique( &a, &s );
			slist_add_unique( &a, &t );
			check_len( &a, 2 );
		}
		slist_empty( &a );
	}

	str_free( &s );
	str_free( &t );
	slist_free( &a );

	return 0;
}

/*
 * str * slist_addc_unique( slist *a, const char *value );
 */
int
test_addc_unique( void )
{
	int i, j;
	slist a;

	slist_init( &a );

	for ( j=0; j<10; ++j ) {
		for ( i=0; i<10; ++i ) {
			slist_addc_unique( &a, "puerile" );
			check_len( &a, 1 );
		}
		slist_empty( &a );
	}

	for ( j=0; j<10; ++j ) {
		for ( i=0; i<10; ++i ) {
			slist_addc_unique( &a, "puerile" );
			slist_addc_unique( &a, "immature" );
			check_len( &a, 2 );
		}
		slist_empty( &a );
	}

	slist_free( &a );

	return 0;
}

/*
 * str * slist_addvp_unique( slist *a, unsigned char mode, void *vp );
 */
int
test_addvp_unique( void )
{
	str s, t;
	int i, j;
	slist a;

	slist_init( &a );
	str_init( &s );
	str_init( &t );

	str_strcpyc( &s, "puerile" );
	str_strcpyc( &t, "immature" );

	for ( j=0; j<10; ++j ) {
		for ( i=0; i<10; ++i ) {
			if ( i%2==0 ) slist_addvp_unique( &a, SLIST_CHR, "puerile" );
			else          slist_addvp_unique( &a, SLIST_STR, &s );
			check_len( &a, 1 );
		}
		slist_empty( &a );
	}

	for ( j=0; j<10; ++j ) {
		for ( i=0; i<10; ++i ) {
			if ( i%2==0 ) {
				slist_addvp_unique( &a, SLIST_CHR, "puerile" );
				slist_addvp_unique( &a, SLIST_CHR, "immature" );
			} else {
				slist_addvp_unique( &a, SLIST_STR, &s );
				slist_addvp_unique( &a, SLIST_STR, &t );
			}
			check_len( &a, 2 );
		}
		slist_empty( &a );
	}

	slist_free( &a );
	str_free( &s );
	str_free( &t );

	return 0;
}

int
test_addsorted( void )
{
	int status, i;
	slist a, b, *c;

	slist_init( &a );
	slist_init( &b );

	/* Check to see if sorted flag is initialized and reset with empty lists */
	check( (a.sorted!=0), "empty list a should be sorted" );
	slist_addc_all( &a, "1", "2", "10", "40", "0", "100", NULL );
	check( (a.sorted==0 ), "added elements aren't sorted" );
	slist_empty( &a );
	check_len( &a, 0 );
	check( (a.sorted!=0), "empty list a should be sorted" );

	/* Check to see if list_add_all() recognizes unsorted input */
	slist_addc_all( &a, "1", "2", "10", "40", "0", "100", NULL );
	check( (a.sorted==0 ), "added elements aren't sorted" );

	slist_sort( &a );
	check( (a.sorted!=0 ), "list_sort() should sort list" );

	/* Copy list entries from sorted list a to list b and check b sort status */
	for ( i=0; i<a.n; ++i )
		slist_add( &b, slist_str( &a, i ) );
	check_len( &a, b.n );
	check( (b.sorted!=0), "empty list b should be sorted" );

	/* Copy list with list_copy() and check sort status */
	slist_empty( &b );
	status = slist_copy( &b, &a );
	check( (status==SLIST_OK), "list_copy() return SLIST_OK on success" );
	check_len( &a, b.n );
	check( (b.sorted!=0), "empty list b should be sorted" );

	/* Copy list with list_dup() and check sort status */
	c = slist_dup( &a );
	if ( !c ) {
		fprintf( stderr, "%s: Memory error in %s() line %d\n", progname, __FUNCTION__, __LINE__ );
		return 1;
	}
	check_len( &a, c->n );
	check( (c->sorted!=0), "empty list b should be sorted" );
	slist_delete( c );

	/* Check to see if list_addc_all() recognizes sorted inserts */
	slist_empty( &a );
	check_len( &a, 0 );
	slist_addc_all( &a, "0", "1", "10", "100", "2", "40", NULL );
	check( (a.sorted!=0), "list a should be sorted" );

	slist_free( &a );
	slist_free( &b );
	return 0;
}

/*
 * int     slist_tokenize( slist *tokens, str *in, const char *delim, int merge_delim );
 */
int
test_tokenize( void )
{
	int status;
	slist a;
	str s;

	str_init( &s );
	slist_init( &a );

	str_strcpyc( &s, "1 2 3 4 5" );
	status = slist_tokenize( &a, &s, " \t", 0 );
	if ( status!=SLIST_OK ) {
		fprintf( stderr, "memory error at %s() line %d\n", __FUNCTION__, __LINE__ );
		goto out;
	}
	check( (a.n==5), "list a should have five elements" );
	check( (!strcmp(slist_cstr(&a,0),"1")), "first element should be '1'" );
	check( (!strcmp(slist_cstr(&a,1),"2")), "second element should be '2'" );
	check( (!strcmp(slist_cstr(&a,2),"3")), "third element should be '3'" );
	check( (!strcmp(slist_cstr(&a,3),"4")), "fourth element should be '4'" );
	check( (!strcmp(slist_cstr(&a,4),"5")), "fifth element should be '5'" );
	slist_empty( &a );

	str_strcpyc( &s, "1\t2\t3\t4\t5" );
	status = slist_tokenize( &a, &s, " \t", 1 );
	if ( status!=SLIST_OK ) {
		fprintf( stderr, "Memory error at %s() line %d\n", __FUNCTION__, __LINE__ );
		goto out;
	}
	check( (a.n==5), "list a should have five elements" );
	check( (!strcmp(slist_cstr(&a,0),"1")), "first element should be '1'" );
	check( (!strcmp(slist_cstr(&a,1),"2")), "second element should be '2'" );
	check( (!strcmp(slist_cstr(&a,2),"3")), "third element should be '3'" );
	check( (!strcmp(slist_cstr(&a,3),"4")), "fourth element should be '4'" );
	check( (!strcmp(slist_cstr(&a,4),"5")), "fifth element should be '5'" );
	slist_empty( &a );

	str_strcpyc( &s, "1  2 3 4" );
	status = slist_tokenize( &a, &s, " \t", 0 );
	if ( status!=SLIST_OK ) {
		fprintf( stderr, "Memory error at %s() line %d\n", __FUNCTION__, __LINE__ );
		goto out;
	}
	check( (a.n==5), "list a should have five elements" );
	check( (!strcmp(slist_cstr(&a,0),"1")), "first element should be '1'" );
	check( (!strcmp(slist_cstr(&a,1),"")), "second element should be ''" );
	check( (!strcmp(slist_cstr(&a,2),"2")), "third element should be '2'" );
	check( (!strcmp(slist_cstr(&a,3),"3")), "fourth element should be '3'" );
	check( (!strcmp(slist_cstr(&a,4),"4")), "fifth element should be '4'" );
	slist_empty( &a );

	str_strcpyc( &s, "1  2 3 4" );
	status = slist_tokenize( &a, &s, " \t", 1 );
	if ( status!=SLIST_OK ) {
		fprintf( stderr, "Memory error at %s() line %d\n", __FUNCTION__, __LINE__ );
		goto out;
	}
	check( (a.n==4), "list a should have four elements" );
	check( (!strcmp(slist_cstr(&a,0),"1")), "first element should be '1'" );
	check( (!strcmp(slist_cstr(&a,1),"2")), "second element should be '2'" );
	check( (!strcmp(slist_cstr(&a,2),"3")), "third element should be '3'" );
	check( (!strcmp(slist_cstr(&a,3),"4")), "fourth element should be '4'" );
	slist_empty( &a );

out:
	str_free( &s );
	slist_free( &a );

	return 0;
}

/*
 * int     slist_tokenizec( slist *tokens, char *p, const char *delim, int merge_delim );
 */
int
test_tokenizec( void )
{
	int status;
	slist a;

	slist_init( &a );

	status = slist_tokenizec( &a, "1 2 3 4 5", " \t", 0 );
	if ( status!=SLIST_OK ) {
		fprintf( stderr, "memory error at %s() line %d\n", __FUNCTION__, __LINE__ );
		goto out;
	}
	check( (a.n==5), "list a should have five elements" );
	check( (!strcmp(slist_cstr(&a,0),"1")), "first element should be '1'" );
	check( (!strcmp(slist_cstr(&a,1),"2")), "second element should be '2'" );
	check( (!strcmp(slist_cstr(&a,2),"3")), "third element should be '3'" );
	check( (!strcmp(slist_cstr(&a,3),"4")), "fourth element should be '4'" );
	check( (!strcmp(slist_cstr(&a,4),"5")), "fifth element should be '5'" );
	slist_empty( &a );

	status = slist_tokenizec( &a, "1\t2\t3\t4\t5", " \t", 1 );
	if ( status!=SLIST_OK ) {
		fprintf( stderr, "Memory error at %s() line %d\n", __FUNCTION__, __LINE__ );
		goto out;
	}
	check( (a.n==5), "list a should have five elements" );
	check( (!strcmp(slist_cstr(&a,0),"1")), "first element should be '1'" );
	check( (!strcmp(slist_cstr(&a,1),"2")), "second element should be '2'" );
	check( (!strcmp(slist_cstr(&a,2),"3")), "third element should be '3'" );
	check( (!strcmp(slist_cstr(&a,3),"4")), "fourth element should be '4'" );
	check( (!strcmp(slist_cstr(&a,4),"5")), "fifth element should be '5'" );
	slist_empty( &a );

	status = slist_tokenizec( &a, "1  2 3 4", " \t", 0 );
	if ( status!=SLIST_OK ) {
		fprintf( stderr, "Memory error at %s() line %d\n", __FUNCTION__, __LINE__ );
		goto out;
	}
	check( (a.n==5), "list a should have five elements" );
	check( (!strcmp(slist_cstr(&a,0),"1")), "first element should be '1'" );
	check( (!strcmp(slist_cstr(&a,1),"")), "second element should be ''" );
	check( (!strcmp(slist_cstr(&a,2),"2")), "third element should be '2'" );
	check( (!strcmp(slist_cstr(&a,3),"3")), "fourth element should be '3'" );
	check( (!strcmp(slist_cstr(&a,4),"4")), "fifth element should be '4'" );
	slist_empty( &a );

	status = slist_tokenizec( &a, "1  2 3 4", " \t", 1 );
	if ( status!=SLIST_OK ) {
		fprintf( stderr, "Memory error at %s() line %d\n", __FUNCTION__, __LINE__ );
		goto out;
	}
	check( (a.n==4), "list a should have four elements" );
	check( (!strcmp(slist_cstr(&a,0),"1")), "first element should be '1'" );
	check( (!strcmp(slist_cstr(&a,1),"2")), "second element should be '2'" );
	check( (!strcmp(slist_cstr(&a,2),"3")), "third element should be '3'" );
	check( (!strcmp(slist_cstr(&a,3),"4")), "fourth element should be '4'" );
	slist_empty( &a );

out:
	slist_free( &a );

	return 0;
}

/*
 * void    slist_empty( slist *a );
 */
int
test_empty( void )
{
	str s, *t;
	slist a;
	str_init( &s );
	slist_init( &a );

	str_strcpyc( &s, "1" );
	t = slist_add( &a, &s );
	check_add_result( t, &s );
	check_len( &a, 1 );
	check_entry( &a, 0, "1" );
	check_entry( &a, 1, NULL );

	str_strcpyc( &s, "2" );
	t = slist_add( &a, &s );
	check_add_result( t, &s );
	check_len( &a, 2 );
	check_entry( &a, 0, "1" );
	check_entry( &a, 1, "2" );
	check_entry( &a, 2, NULL );

	slist_empty( &a );
	check_len( &a, 0 );
	check_entry( &a, 0, NULL );

	slist_free( &a );
	str_free( &s );
	return 0;
}

/*
 * slist * slist_new( void );
 */
int
test_new( void )
{
	char buf[1000];
	slist *a;
	str *tmp;
	int i;

	a = slist_new();
	if ( !a ) {
		fprintf( stderr, "Memory error at %s() line %d\n", __FUNCTION__, __LINE__ );
		return 0;
	}
	check_len( a, 0 );
	check_entry( a, 0, NULL );

	for ( i=0; i<100; ++i ) {
		sprintf( buf, "Test%d", i );
		tmp = slist_addc( a, buf );
		if ( !tmp ) {
			fprintf( stderr, "Memory error at %s() line %d\n", __FUNCTION__, __LINE__ );
			goto out;
		}
	}
	check_len( a, 100 );
	for ( i=0; i<100; ++i ) {
		sprintf( buf, "Test%d", i );
		check_entry( a, i, buf );
	}
	check_entry( a, 101, NULL );

out:
	slist_delete( a );

	return 0;
}

/*
 * slist * slist_dup( slist *a );
 */
int
test_dup( void )
{
	char buf[1000];
	slist a, *dupa;
	str *tmp;
	int i;

	slist_init( &a );

	for ( i=0; i<100; ++i ) {
		sprintf( buf, "Test%d", i );
		tmp = slist_addc( &a, buf );
		if ( !tmp ) {
			fprintf( stderr, "Memory error 1 at %s() line %d\n", __FUNCTION__, __LINE__ );
			goto out;
		}
	}

	dupa = slist_dup( &a );
	if ( !dupa ) {
		fprintf( stderr, "Memory error 2 at %s() line %d\n", __FUNCTION__, __LINE__ );
		goto out;
	}
	check_len( dupa, 100 );
	for ( i=0; i<100; ++i ) {
		sprintf( buf, "Test%d", i );
		check_entry( dupa, i, buf );
	}
	check_entry( dupa, 101, NULL );

	slist_delete( dupa );

out:
	slist_free( &a );

	return 0;
}

/*
 * int slist_copy( slist *to, slist *from );
 */
int
test_copy( void )
{
	int i, status, ret = 0;
	char buf[1000];
	slist a, copya;
	str *tmp;

	/* Build and test list to be copied */
	slist_init( &a );
	for ( i=0; i<100; ++i ) {
		sprintf( buf, "ToBeCopied%d", i );
		tmp = slist_addc( &a, buf );
		if ( !tmp ) {
			fprintf( stderr, "Memory error at %s() line %d\n", __FUNCTION__, __LINE__ );
			goto out;
		}
	
	}
	check_len( &a, 100 );
	for ( i=0; i<100; ++i ) {
		sprintf( buf, "ToBeCopied%d", i );
		check_entry( &a, i, buf );
	}
	check_entry( &a, 101, NULL );

	/* Build and test list to be overwritten */
	slist_init( &copya );
	for ( i=0; i<10; ++i ) {
		sprintf( buf, "ToBeOverwritten%d", i );
		slist_addc( &copya, buf );
	}
	check_len( &copya, 10 );
	for ( i=0; i<10; ++i ) {
		sprintf( buf, "ToBeOverwritten%d", i );
		check_entry( &copya, i, buf );
	}
	check_entry( &copya, 10, NULL );

	/* Copy and check copy */
	status = slist_copy( &copya, &a );
	if ( status!=SLIST_OK ) {
		fprintf( stderr, "Memory error at %s() line %d\n", __FUNCTION__, __LINE__ );
		ret = 1;
		goto out;
	}
	check_len( &copya, 100 );
	for ( i=0; i<100; ++i ) {
		sprintf( buf, "ToBeCopied%d", i );
		check_entry( &copya, i, buf );
	}
	check_entry( &copya, 100, NULL );

out:
	slist_free( &a );
	slist_free( &copya );

	return ret;
}

/*
 * int slist_append( slist *a, slist *toadd );
 */
int
test_append( void )
{
	int status;
	slist a, c;

	slist_init( &a );
	slist_init( &c );

	status = slist_addc_all( &a, "amateurish", "boorish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	check_len( &a, 2 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, NULL );

	status = slist_addc_all( &c, "churlish", "dull", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	check_len( &c, 2 );
	check_entry( &c, 0, "churlish" );
	check_entry( &c, 1, "dull" );
	check_entry( &c, 2, NULL );

	status = slist_append( &a, &c );
	check( (status==SLIST_OK), "slist_append() should return SLIST_OK" );

	check_len( &a, 4 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, "churlish" );
	check_entry( &a, 3, "dull" );
	check_entry( &a, 4, NULL );

	check_len( &c, 2 );
	check_entry( &c, 0, "churlish" );
	check_entry( &c, 1, "dull" );
	check_entry( &c, 2, NULL );

	slist_free( &a );
	slist_free( &c );

	return 0;
}

/*
 * int slist_append_unique( slist *a, slist *toadd );
 */
int
test_append_unique( void )
{
	int status;
	slist a, c;

	slist_init( &a );
	slist_init( &c );

	status = slist_addc_all( &a, "amateurish", "boorish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	check_len( &a, 2 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, NULL );

	status = slist_addc_all( &c, "churlish", "boorish", NULL );
	check( (status==SLIST_OK), "slist_append_unique() should return SLIST_OK" );

	check_len( &c, 2 );
	check_entry( &c, 0, "churlish" );
	check_entry( &c, 1, "boorish" );
	check_entry( &c, 2, NULL );

	status = slist_append_unique( &a, &c );
	check( (status==SLIST_OK), "slist_append_unique() should return SLIST_OK" );

	check_len( &a, 3 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, "churlish" );
	check_entry( &a, 3, NULL );

	check_len( &c, 2 );
	check_entry( &c, 0, "churlish" );
	check_entry( &c, 1, "boorish" );
	check_entry( &c, 2, NULL );

	status = slist_append_unique( &a, &c );
	check( (status==SLIST_OK), "slist_append_unique() should return SLIST_OK" );

	check_len( &a, 3 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, "churlish" );
	check_entry( &a, 3, NULL );

	check_len( &c, 2 );
	check_entry( &c, 0, "churlish" );
	check_entry( &c, 1, "boorish" );
	check_entry( &c, 2, NULL );

	slist_free( &a );
	slist_free( &c );

	return 0;
}

/*
 * int slist_remove( slist *a, int n );
 */
int
test_remove( void )
{
	int status;
	slist a;

	slist_init( &a );

	status = slist_addc_all( &a, "amateurish", "boorish", "churlish", "dull", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	check_len( &a, 4 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, "churlish" );
	check_entry( &a, 3, "dull" );
	check_entry( &a, 4, NULL );

	status = slist_remove( &a, 2 );
	check( (status==SLIST_OK), "slist_remove() should return SLIST_OK" );

	check_len( &a, 3 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, "dull" );
	check_entry( &a, 3, NULL );

	status = slist_remove( &a, 1 );
	check( (status==SLIST_OK), "slist_remove() should return SLIST_OK" );

	check_len( &a, 2 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "dull" );
	check_entry( &a, 2, NULL );

	status = slist_remove( &a, 100 );
	check( (status==SLIST_ERR_BADPARAM), "slist_remove() should return SLIST_ERR_BADPARAM" );

	check_len( &a, 2 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "dull" );
	check_entry( &a, 2, NULL );

	slist_free( &a );

	return 0;
}

/*
 * void slist_swap( slist *a, int n1, int n2 );
 */
int
test_swap( void )
{
	int status;
	slist a;

	slist_init( &a );

	status = slist_addc_all( &a, "dull", "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	check_len( &a, 4 );
	check_entry( &a, 0, "dull" );
	check_entry( &a, 1, "churlish" );
	check_entry( &a, 2, "boorish" );
	check_entry( &a, 3, "amateurish" );
	check_entry( &a, 4, NULL );

	slist_swap( &a, 0, 3 );

	check_len( &a, 4 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "churlish" );
	check_entry( &a, 2, "boorish" );
	check_entry( &a, 3, "dull" );
	check_entry( &a, 4, NULL );

	slist_swap( &a, 1, 2 );

	check_len( &a, 4 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, "churlish" );
	check_entry( &a, 3, "dull" );
	check_entry( &a, 4, NULL );

	slist_swap( &a, 1, 2 );

	check_len( &a, 4 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "churlish" );
	check_entry( &a, 2, "boorish" );
	check_entry( &a, 3, "dull" );
	check_entry( &a, 4, NULL );

	slist_swap( &a, 0, 3 );

	check_len( &a, 4 );
	check_entry( &a, 0, "dull" );
	check_entry( &a, 1, "churlish" );
	check_entry( &a, 2, "boorish" );
	check_entry( &a, 3, "amateurish" );
	check_entry( &a, 4, NULL );

	slist_free( &a );

	return 0;
}

/*
 * void slist_sort( slist *a );
 */
int
test_sort( void )
{
	int status;
	slist a;

	slist_init( &a );

	slist_sort( &a );

	check_len( &a, 0 );

	status = slist_addc_all( &a, "dull", "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	check_len( &a, 4 );
	check_entry( &a, 0, "dull" );
	check_entry( &a, 1, "churlish" );
	check_entry( &a, 2, "boorish" );
	check_entry( &a, 3, "amateurish" );
	check_entry( &a, 4, NULL );

	slist_sort( &a );

	check_len( &a, 4 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, "churlish" );
	check_entry( &a, 3, "dull" );
	check_entry( &a, 4, NULL );

	slist_empty( &a );

	status = slist_addc_all( &a, "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	check_len( &a, 3 );
	check_entry( &a, 0, "churlish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, "amateurish" );
	check_entry( &a, 3, NULL );

	slist_sort( &a );

	check_len( &a, 3 );
	check_entry( &a, 0, "amateurish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, "churlish" );
	check_entry( &a, 3, NULL );

	slist_free( &a );

	return 0;
}

/*
 * str* list_str( list *a, int n );
 */
int
test_get( void )
{
	int status;
	str *s;
	slist a;

	slist_init( &a );

	status = slist_addc_all( &a, "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	s = slist_str( &a, -1 );
	check( (s==NULL), "element -1 should be NULL" );

	s = slist_str( &a, 0 );
	check( (s!=NULL && !strcmp( str_cstr( s ), "churlish") ), "element 0 should be 'churlish'" );

	s = slist_str( &a, 1 );
	check( (s!=NULL && !strcmp( str_cstr( s ), "boorish") ), "element 1 should be 'boorish'" );

	s = slist_str( &a, 2 );
	check( (s!=NULL && !strcmp( str_cstr( s ), "amateurish") ), "element 2 should be 'amateurish'" );

	s = slist_str( &a, 3 );
	check( (s==NULL), "element 3 should be NULL" );

	slist_free( &a );

	return 0;
}

/*
 * char* slist_cstr( list *a, int n );
 */
int
test_getc( void )
{
	int status;
	char *s;
	slist a;

	slist_init( &a );

	status = slist_addc_all( &a, "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	s = slist_cstr( &a, -1 );
	check( (s==NULL), "element -1 should be NULL" );

	s = slist_cstr( &a, 0 );
	check( (s!=NULL && !strcmp( s, "churlish") ), "element 0 should be 'churlish'" );

	s = slist_cstr( &a, 1 );
	check( (s!=NULL && !strcmp( s, "boorish") ), "element 1 should be 'boorish'" );

	s = slist_cstr( &a, 2 );
	check( (s!=NULL && !strcmp( s, "amateurish") ), "element 2 should be 'amateurish'" );

	s = slist_cstr( &a, 3 );
	check( (s==NULL), "element 3 should be NULL" );

	slist_free( &a );

	return 0;
}

/*
 * str* slist_set( list *a, int n, str *s );
 */
int
test_set( void )
{
	int status;
	str s, *t;
	slist a;

	slist_init( &a );
	str_init( &s );

	str_strcpyc( &s, "puerile" );

	status = slist_addc_all( &a, "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	t = slist_set( &a, -1, &s );
	check( (t==NULL), "element -1 should be NULL" );

	t = slist_set( &a, 3, &s );
	check( (t==NULL), "element 3 should be NULL" );

	t = slist_set( &a, 1, &s );
	check( (t!=NULL && !strcmp(str_cstr(t),"puerile")), "slist_set() should return 'puerile'" );

	check_len( &a, 3 );
	check_entry( &a, 0, "churlish" );
	check_entry( &a, 1, "puerile" );
	check_entry( &a, 2, "amateurish" );

	t = slist_set( &a, 0, &s );
	check( (t!=NULL && !strcmp(str_cstr(t),"puerile")), "slist_set() should return 'puerile'" );

	check_len( &a, 3 );
	check_entry( &a, 0, "puerile" );
	check_entry( &a, 1, "puerile" );
	check_entry( &a, 2, "amateurish" );

	t = slist_set( &a, 2, &s );
	check( (t!=NULL && !strcmp(str_cstr(t),"puerile")), "slist_set() should return 'puerile'" );

	check_len( &a, 3 );
	check_entry( &a, 0, "puerile" );
	check_entry( &a, 1, "puerile" );
	check_entry( &a, 2, "puerile" );

	slist_free( &a );
	str_free( &s );

	return 0;
}

/*
 * str* slist_setc( list *a, int n, const char *s );
 */
int
test_setc( void )
{
	int status;
	str *t;
	slist a;

	slist_init( &a );

	status = slist_addc_all( &a, "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	t = slist_setc( &a, -1, "puerile" );
	check( (t==NULL), "element -1 should be NULL" );

	t = slist_setc( &a, 3, "puerile" );
	check( (t==NULL), "element 3 should be NULL" );

	t = slist_setc( &a, 1, "puerile" );
	check( (t!=NULL && !strcmp(str_cstr(t),"puerile")), "slist_setc() should return 'puerile'" );

	check_len( &a, 3 );
	check_entry( &a, 0, "churlish" );
	check_entry( &a, 1, "puerile" );
	check_entry( &a, 2, "amateurish" );

	t = slist_setc( &a, 0, "puerile" );
	check( (t!=NULL && !strcmp(str_cstr(t),"puerile")), "slist_setc() should return 'puerile'" );

	check_len( &a, 3 );
	check_entry( &a, 0, "puerile" );
	check_entry( &a, 1, "puerile" );
	check_entry( &a, 2, "amateurish" );

	t = slist_setc( &a, 2, "puerile" );
	check( (t!=NULL && !strcmp(str_cstr(t),"puerile")), "slist_setc() should return 'puerile'" );

	check_len( &a, 3 );
	check_entry( &a, 0, "puerile" );
	check_entry( &a, 1, "puerile" );
	check_entry( &a, 2, "puerile" );

	slist_free( &a );

	return 0;
}

/*
 * int slist_find( list *a, str *searchstr );
 */
int
test_find( void )
{
	int n, status;
	slist a;
	str s;

	slist_init( &a );
	str_init( &s );

	status = slist_addc_all( &a, "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	str_strcpyc( &s, "dull" );
	n = slist_find( &a, &s );
	check( (!slist_wasfound(&a,n)), "slist_find() should not find 'dull'" );
	check( (slist_wasnotfound(&a,n)), "slist_find() should not find 'dull'" );

	str_strcpyc( &s, "churlish" );
	n = slist_find( &a, &s );
	check( (slist_wasfound(&a,n)), "slist_find() should find 'churlish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_find() should find 'churlish'" );
	check( (n==0), "'churlish' should be at element 0" );

	str_strcpyc( &s, "boorish" );
	n = slist_find( &a, &s );
	check( (slist_wasfound(&a,n)), "slist_find() should find 'boorlish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_find() should find 'boorlish'" );
	check( (n==1), "'boorish' should be at element 1" );

	str_strcpyc( &s, "amateurish" );
	n = slist_find( &a, &s );
	check( (slist_wasfound(&a,n)), "slist_find() should find 'amateurish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_find() should find 'amateurish'" );
	check( (n==2), "'amateurish' should be at element 2" );

	slist_free( &a );
	str_free( &s );

	return 0;
}

/*
 * int slist_findc( list *a, const char *searchstr );
 */
int
test_findc( void )
{
	int n, status;
	slist a;

	slist_init( &a );

	status = slist_addc_all( &a, "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	n = slist_findc( &a, "dull" );
	check( (slist_wasnotfound(&a,n)), "slist_findc() should not find 'dull'" );
	check( (!slist_wasfound(&a,n)), "slist_findc() should not find 'dull'" );

	n = slist_findc( &a, "churlish" );
	check( (slist_wasfound(&a,n)), "slist_findc() should find 'churlish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findc() should find 'churlish'" );
	check( (n==0), "'churlish' should be at element 0" );

	n = slist_findc( &a, "boorish" );
	check( (slist_wasfound(&a,n)), "slist_findc() should find 'boorlish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findc() should find 'boorlish'" );
	check( (n==1), "'boorish' should be at element 1" );

	n = slist_findc( &a, "amateurish" );
	check( (slist_wasfound(&a,n)), "slist_findc() should find 'amateurish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findc() should find 'amateurish'" );
	check( (n==2), "'amateurish' should be at element 2" );

	slist_free( &a );

	return 0;
}

/*
 * int slist_findnocase( list *a, const char *searchstr );
 */
int
test_findnocase( void )
{
	int n, status;
	slist a;
	str s;

	slist_init( &a );
	str_init( &s );

	status = slist_addc_all( &a, "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	str_strcpyc( &s, "dull" );
	n = slist_findnocase( &a, &s );
	check( (slist_wasnotfound(&a,n)), "slist_findnocase() should not find 'dull'" );
	check( (!slist_wasfound(&a,n)), "slist_findnocase() should not find 'dull'" );

	str_strcpyc( &s, "churlish" );
	n = slist_findnocase( &a, &s );
	check( (slist_wasfound(&a,n)), "slist_findnocase() should find 'churlish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocase() should find 'churlish'" );
	check( (n==0), "'churlish' should be at element 0" );

	str_strcpyc( &s, "CHURlish" );
	n = slist_findnocase( &a, &s );
	check( (slist_wasfound(&a,n)), "slist_findnocase() should find 'CHURlish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocase() should find 'CHURlish'" );
	check( (n==0), "'churlish' should be at element 0" );

	str_strcpyc( &s, "churLISH" );
	n = slist_findnocase( &a, &s );
	check( (slist_wasfound(&a,n)), "slist_findnocase() should find 'churLISH'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocase() should find 'churLISH'" );
	check( (n==0), "'churlish' should be at element 0" );

	str_strcpyc( &s, "boorish" );
	n = slist_findnocase( &a, &s );
	check( (slist_wasfound(&a,n)), "slist_findnocase() should find 'boorish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocase() should find 'boorish'" );
	check( (n==1), "'boorish' should be at element 1" );

	str_strcpyc( &s, "Boorish" );
	n = slist_findnocase( &a, &s );
	check( (slist_wasfound(&a,n)), "slist_findnocase() should find 'Boorish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocase() should find 'Boorish'" );
	check( (n==1), "'Boorish' should be at element 1" );

	str_strcpyc( &s, "BOORISH" );
	n = slist_findnocase( &a, &s );
	check( (slist_wasfound(&a,n)), "slist_findnocase() should find 'BOORISH'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocase() should find 'BOORISH'" );
	check( (n==1), "'BOORISH' should be at element 1" );

	str_strcpyc( &s, "aMaTeUrIsH" );
	n = slist_findnocase( &a, &s );
	check( (slist_wasfound(&a,n)), "slist_findnocase() should find 'aMaTeUrIsH'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocase() should find 'aMaTeUrIsH'" );
	check( (n==2), "'aMaTeUrIsH' should be at element 2" );

	slist_free( &a );
	str_free( &s );

	return 0;
}

/*
 * int slist_findnocasec( list *a, const char *searchstr );
 */
int
test_findnocasec( void )
{
	int n, status;
	slist a;

	slist_init( &a );

	status = slist_addc_all( &a, "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	n = slist_findnocasec( &a, "dull" );
	check( (slist_wasnotfound(&a,n)), "slist_findnocasec() should not find 'dull'" );
	check( (!slist_wasfound(&a,n)), "slist_findnocasec() should not find 'dull'" );

	n = slist_findnocasec( &a, "churlish" );
	check( (slist_wasfound(&a,n)), "slist_findnocasec() should find 'churlish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocasec() should find 'churlish'" );
	check( (n==0), "'churlish' should be at element 0" );

	n = slist_findnocasec( &a, "CHURlish" );
	check( (slist_wasfound(&a,n)), "slist_findnocasec() should find 'CHURlish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocasec() should find 'CHURlish'" );
	check( (n==0), "'churlish' should be at element 0" );

	n = slist_findnocasec( &a, "churLISH" );
	check( (slist_wasfound(&a,n)), "slist_findnocasec() should find 'churLISH'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocasec() should find 'churLISH'" );
	check( (n==0), "'churlish' should be at element 0" );

	n = slist_findnocasec( &a, "boorish" );
	check( (slist_wasfound(&a,n)), "slist_findnocasec() should find 'boorish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocasec() should find 'boorish'" );
	check( (n==1), "'boorish' should be at element 1" );

	n = slist_findnocasec( &a, "Boorish" );
	check( (slist_wasfound(&a,n)), "slist_findnocasec() should find 'Boorish'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocasec() should find 'Boorish'" );
	check( (n==1), "'Boorish' should be at element 1" );

	n = slist_findnocasec( &a, "BOORISH" );
	check( (slist_wasfound(&a,n)), "slist_findnocasec() should find 'BOORISH'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocasec() should find 'BOORISH'" );
	check( (n==1), "'BOORISH' should be at element 1" );

	n = slist_findnocasec( &a, "aMaTeUrIsH" );
	check( (slist_wasfound(&a,n)), "slist_findnocasec() should find 'aMaTeUrIsH'" );
	check( (!slist_wasnotfound(&a,n)), "slist_findnocasec() should find 'aMaTeUrIsH'" );
	check( (n==2), "'aMaTeUrIsH' should be at element 2" );

	slist_free( &a );

	return 0;
}

/*
 * int slist_match_entry( slist *a, int n, const char *s );
 */
int
test_match_entry( void )
{
	int n, status;
	slist a;

	slist_init( &a );

	status = slist_addc_all( &a, "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	n = slist_match_entry( &a, 0, "churlish" );
	check( (n), "'churlish' should match entry 0" );
	n = slist_match_entry( &a, 0, "boorish" );
	check( (n==0), "'boorlish' should not match entry 0" );
	n = slist_match_entry( &a, 0, "amateurish" );
	check( (n==0), "'amateurish' should not match entry 0" );
	n = slist_match_entry( &a, 0, "dull" );
	check( (n==0), "'dull' should not match entry 0" );

	n = slist_match_entry( &a, 1, "churlish" );
	check( (n==0), "'churlish' should not match entry 1" );
	n = slist_match_entry( &a, 1, "boorish" );
	check( (n), "'boorlish' should match entry 1" );
	n = slist_match_entry( &a, 1, "amateurish" );
	check( (n==0), "'amateurish' should not match entry 1" );
	n = slist_match_entry( &a, 1, "dull" );
	check( (n==0), "'dull' should not match entry 1" );

	n = slist_match_entry( &a, 2, "churlish" );
	check( (n==0), "'churlish' should not match entry 2" );
	n = slist_match_entry( &a, 2, "boorish" );
	check( (n==0), "'boorlish' should not match entry 2" );
	n = slist_match_entry( &a, 2, "amateurish" );
	check( (n), "'amateurish' should match entry 2" );
	n = slist_match_entry( &a, 2, "dull" );
	check( (n==0), "'dull' should not match entry 2" );

	n = slist_match_entry( &a, 3, "churlish" );
	check( (n==0), "'churlish' should not match entry 3" );
	n = slist_match_entry( &a, 3, "boorish" );
	check( (n==0), "'boorlish' should not match entry 3" );
	n = slist_match_entry( &a, 3, "amateurish" );
	check( (n==0), "'amateurish' should not match entry 3" );
	n = slist_match_entry( &a, 3, "dull" );
	check( (n==0), "'dull' should not match entry 3" );

	slist_free( &a );

	return 0;
}

/*
 * void slist_trimend( slist *a, int n );
 */
int
test_trimend( void )
{
	int status;
	slist a;

	slist_init( &a );

	status = slist_addc_all( &a, "churlish", "boorish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	check_len( &a, 3 );
	check_entry( &a, 0, "churlish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, "amateurish" );
	check_entry( &a, 3, NULL );

	slist_trimend( &a, 1 );

	check_len( &a, 2 );
	check_entry( &a, 0, "churlish" );
	check_entry( &a, 1, "boorish" );
	check_entry( &a, 2, NULL );

	slist_trimend( &a, 2 );

	check_len( &a, 0 );
	check_entry( &a, 0, NULL );

	slist_free( &a );

	return 0;
}

/*
extern int     list_fill( list *a, const char *filename, unsigned char skip_blank_lines );
extern int     list_fillfp( list *a, FILE *fp, unsigned char skip_blank_lines );
*/

int
test_fill( void )
{
	char filename[512];
	unsigned long val;
	int status;
	FILE *fp;
	slist a;

	val = ( unsigned long ) getpid();
	sprintf( filename, "test_slist.%lu", val );

	fp = fopen( filename, "w" );
	if ( !fp ) {
		fprintf( stderr, "%s: Could not open file %s\n", progname, filename );
		return 1;
	}

	fprintf( fp, "Line 1\n" );
	fprintf( fp, "Line 2\n" );
	fprintf( fp, "\n" );
	fprintf( fp, "Line 4\n" );
	fprintf( fp, "\n" );
	fprintf( fp, "Line 6\n" );

	fclose( fp );

	slist_init( &a );

	status = slist_fill( &a, filename, 0 );
	if ( status!=SLIST_OK ) {
		fprintf( stderr, "%s: Could not slist_fill() %s\n", progname, filename );
		return 1;
	}
	check_len( &a, 6 );
	check_entry( &a, 0, "Line 1" );
	check_entry( &a, 1, "Line 2" );
	check_entry( &a, 2, "" );
	check_entry( &a, 3, "Line 4" );
	check_entry( &a, 4, "" );
	check_entry( &a, 5, "Line 6" );

	slist_empty( &a );

	status = slist_fill( &a, filename, 1 );
	if ( status!=SLIST_OK ) {
		fprintf( stderr, "%s: Could not slist_fill() %s\n", progname, filename );
		return 1;
	}

	check_len( &a, 4 );
	check_entry( &a, 0, "Line 1" );
	check_entry( &a, 1, "Line 2" );
	check_entry( &a, 2, "Line 4" );
	check_entry( &a, 3, "Line 6" );

	slist_free( &a );

	status = unlink( filename );
	if ( status!=0 )
		fprintf( stderr, "%s: Error unlink failed for %s\n", progname, filename );

	return 0;
}

/*
 * void    slist_dump( slist *a, FILE *fp, int newline );
 */
int
test_dump( void )
{
	char filename[512];
	unsigned long val;
	int status;
	slist a, b;
	FILE *fp;

	val = ( unsigned long ) getpid();
	sprintf( filename, "test_slist.%lu", val );

	fp = fopen( filename, "w" );
	if ( !fp ) {
		fprintf( stderr, "%s: Could not open file %s\n", progname, filename );
		return 1;
	}

	slist_init( &a );

	status = slist_addc_all( &a, "dull", "boorish", "churlish", "amateurish", NULL );
	check( (status==SLIST_OK), "slist_addc_all() should return SLIST_OK" );

	slist_dump( &a, fp, 1 );
	fclose( fp );

	slist_free( &a );

	slist_init( &b );

	status = slist_fill( &b, filename, 1 );
	check( (status==SLIST_OK), "slist_fill() should return SLIST_OK" );
	check_len( &b, 4 );
	check_entry( &b, 0, "dull" );
	check_entry( &b, 1, "boorish" );
	check_entry( &b, 2, "churlish" );
	check_entry( &b, 3, "amateurish" );
	check_entry( &b, 4, NULL );

	status = unlink( filename );
	if ( status!=0 )
		fprintf( stderr, "%s: Error unlink failed for %s\n", progname, filename );

	slist_free( &b );

	return 0;
}

/*
 * void slists_init( slist *a, ... );
 * void slists_free( slist *a, ... );
 * void slists_empty( slist *a, ... );
 */
int
test_lists( void )
{
	char buf[1000];
	slist a, b, c;
	int i;

	slists_init( &a, &b, &c, NULL );
	check_len( &a, 0 );
	check_len( &b, 0 );
	check_len( &c, 0 );
	check_entry( &a, 0, NULL );
	check_entry( &b, 0, NULL );
	check_entry( &c, 0, NULL );

	for ( i=0; i<10; ++i ) {
		sprintf( buf, "a_entry%d\n", i );
		slist_addc( &a, buf );
	}
	for ( i=0; i<100; ++i ) {
		sprintf( buf, "b_entry%d\n", i );
		slist_addc( &b, buf );
	}
	for ( i=0; i<1000; ++i ) {
		sprintf( buf, "c_entry%d\n", i );
		slist_addc( &c, buf );
	}
	check_len( &a, 10 );
	check_len( &b, 100 );
	check_len( &c, 1000 );
	check_entry( &a, 10, NULL );
	check_entry( &b, 100, NULL );
	check_entry( &c, 1000, NULL );
	for ( i=0; i<10; ++i ) {
		sprintf( buf, "a_entry%d\n", i );
		check_entry( &a, i, buf );
	}
	for ( i=0; i<100; ++i ) {
		sprintf( buf, "b_entry%d\n", i );
		check_entry( &b, i, buf );
	}
	for ( i=0; i<1000; ++i ) {
		sprintf( buf, "c_entry%d\n", i );
		check_entry( &c, i, buf );
	}

	slists_empty( &a, &b, &c, NULL );
	check_len( &a, 0 );
	check_len( &b, 0 );
	check_len( &c, 0 );
	check_entry( &a, 0, NULL );
	check_entry( &b, 0, NULL );
	check_entry( &c, 0, NULL );

	slists_free( &a, &b, &c, NULL );

	return 0;
}

/*
 * unsigned long slist_get_maxlen( slist *a );
 */
int
test_get_maxlen( void )
{
	unsigned long n;
	slist a;
	str *t;

	slist_init( &a );

	t = slist_addc( &a, "churlish" );
	check( (t!=NULL), "slist_addc() should not return NULL" );

	n = slist_get_maxlen( &a );
	check( (n==strlen("churlish")), "slist_get_maxlen() should return length of 'churlish'" );

	t = slist_addc( &a, "boorish" );
	check( (t!=NULL), "slist_addc() should not return NULL" );

	n = slist_get_maxlen( &a );
	check( (n==strlen("churlish")), "slist_get_maxlen() should return length of 'churlish'" );

	t = slist_addc( &a, "amateurish" );
	check( (t!=NULL), "slist_addc() should not return NULL" );

	n = slist_get_maxlen( &a );
	check( (n==strlen("amateurish")), "slist_get_maxlen() should return length of 'amateurish'" );

	slist_free( &a );

	return 0;
}

int
main( int argc, char *argv[] )
{
	int failed = 0;

	failed += test_init();

	failed += test_add();
	failed += test_addc();
	failed += test_addvp();

	failed += test_add_all();
	failed += test_addc_all();
	failed += test_addvp_all();

	failed += test_add_unique();
	failed += test_addc_unique();
	failed += test_addvp_unique();

	failed += test_addsorted();

	failed += test_swap();

	failed += test_tokenize();
	failed += test_tokenizec();

	failed += test_empty();
	failed += test_new();
	failed += test_dup();
	failed += test_copy();
	failed += test_append();
	failed += test_append_unique();
	failed += test_remove();
	failed += test_sort();

	failed += test_get();
	failed += test_getc();

	failed += test_set();
	failed += test_setc();

	failed += test_find();
	failed += test_findc();
	failed += test_findnocase();
	failed += test_findnocasec();
	failed += test_match_entry();

	failed += test_fill();

	failed += test_trimend();

	failed += test_lists();

	failed += test_get_maxlen();

	failed += test_dump();

	if ( !failed ) {
		printf( "%s: PASSED\n", progname );
		return EXIT_SUCCESS;
	} else {
		printf( "%s: FAILED\n", progname );
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
