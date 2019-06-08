/*
 * str_test.c
 *
 * test str functions
 */

/* Need to add tests for...

const char *str_addutf8    ( str *s, const char *p );
void str_fprintf     ( FILE *fp, str *s );
int  str_fget        ( FILE *fp, char *buf, int bufsize, int *pbufpos,
                          str *outs );
int  str_fgetline    ( str *s, FILE *fp );
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "str.h"

char progname[] = "str_test";
char version[] = "0.3";

int
_inconsistent_len( str *s, unsigned long numchars, const char *fn, unsigned long line )
{
	if ( s->len > s->dim ) {
		fprintf(stdout,"%s line %lu: failed consistency check found s->len=%lu, s->max=%lu\n",fn,line,
			s->len, s->dim );
	}
	if ( s->data ) {
		if ( s->len != strlen( s->data ) ) {
			fprintf(stdout,"%s line %lu: failed consistency check found strlen=%d, s->len=%ld\n",fn,line,(int)strlen(s->data),s->len);
			return 1;
		}
	} else {
		if ( s->len != 0 ) {
			fprintf(stdout,"%s line %lu: failed consistency check found for unallocated string, s->len=%ld\n",fn,line,s->len);
			return 1;
		}
	}
	if ( s->len != numchars ) {
		fprintf(stdout,"%s line %lu: failed consistency check found %d, expected %lu\n",fn,line,(int)strlen(s->data),numchars);
		return 1;
	}
	return 0;
}

#define inconsistent_len( a, b ) _inconsistent_len( (a), (b), __FUNCTION__, __LINE__ )

int
_test_identity( str *s, const char *expected, const char *fn, unsigned long line )
{
	/* Unallocated strings are considered identical to empty strings */
	if ( expected[0]=='\0' ) {
		if ( s->data==NULL || s->data[0]=='\0' ) return 0;
		fprintf(stdout,"%s line %lu: failed identity check found '%s', expected ''\n",fn,line,s->data);
		return 1;
	}
	/* expected!="", so s->data must exist */
	if ( !s->data ) {
		fprintf(stdout,"%s line %lu: failed identity check, s->data unallocated, expected '%s'\n",fn,line,expected);
		return 1;
	}
	if ( strcmp( s->data, expected ) == 0 ) return 0;
	fprintf(stdout,"%s line %lu: failed identity check, found '%s', expected '%s'\n",fn,line,s->data,expected);
	return 1;
}

#define test_identity( a, b ) _test_identity( (a), (b), __FUNCTION__, __LINE__ )

#define string_mismatch( a, b, c ) ( test_identity( (a), (c) ) || inconsistent_len( (a), (b) ) )

static int
test_empty( str *s )
{
	int failed = 0;
	int numchars = 1000, i, j;

	str_empty( s );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	for ( i=0; i<numchars; ++i ) {
		for ( j=0; j<i; ++j )
			str_addchar( s, 'x' );
		str_empty( s );
		if ( string_mismatch( s, 0, "" ) ) failed++;
	}

	return failed;
}

static int
test_addchar( str *s )
{
	int failed = 0;
	int numshort = 5, numchars = 1000, i;

	/* ...appending '\0' characters won't increase length */
	str_empty( s );
	for ( i=0; i<numshort; ++i )
		str_addchar( s, '\0' );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	/* ...build "11111" with str_addchar */
	str_empty( s );
	for ( i=0; i<numshort; ++i )
		str_addchar( s, '1' );
	if ( string_mismatch( s, 5, "11111" ) ) failed++;

	/* ...build a bunch of random characters */
	str_empty( s );
	for ( i=0; i<numchars; ++i ) {
		str_addchar( s, ( i % 64 ) + 64);
	}
	if ( inconsistent_len( s, numchars ) ) failed++;

	return failed;
}

static int
test_strcatc( str *s )
{
	int failed = 0;
	int numshort = 5, numstrings = 1000, i;

	/* ...adding empty strings to an empty string shouldn't change length */
	str_empty( s );
	for ( i=0; i<numstrings; ++i )
		str_strcatc( s, "" );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	/* ...adding empty strings to a defined string shouldn't change string */
	str_strcpyc( s, "1" );
	for ( i=0; i<numstrings; ++i )
		str_strcatc( s, "" );
	if ( string_mismatch( s, 1, "1" ) ) failed++;

	/* ...build "1111" with str_strcatc */
	str_empty( s );
	for ( i=0; i<numshort; ++i )
		str_strcatc( s, "1" );
	if ( string_mismatch( s, numshort, "11111" ) ) failed++;

	/* ...build "xoxoxoxoxo" with str_strcatc */
	str_empty( s );
	for ( i=0; i<numshort; ++i )
		str_strcatc( s, "xo" );
	if ( string_mismatch( s, numshort*2, "xoxoxoxoxo" ) ) failed++;

	str_empty( s );
	for ( i=0; i<numstrings; ++i )
		str_strcatc( s, "1" );
	if ( inconsistent_len( s, numstrings ) ) failed++;

	str_empty( s );
	for ( i=0; i<numstrings; ++i )
		str_strcatc( s, "XXOO" );
	if ( inconsistent_len( s, numstrings*4 ) ) failed++;

	return failed;
}

static int
test_strcat( str *s )
{
	int numshort = 5, numstrings = 1000, i;
	int failed = 0;
	str t;

	str_init( &t );

	/* ...adding empty strings to an empty string shouldn't change length */
	str_empty( s );
	for ( i=0; i<numstrings; ++i )
		str_strcat( s, &t );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	/* ...adding empty strings to a defined string shouldn't change string */
	str_strcpyc( s, "1" );
	for ( i=0; i<numstrings; ++i )
		str_strcat( s, &t );
	if ( string_mismatch( s, 1, "1" ) ) failed++;

	/* ...build "1111" with str_strcat */
	str_empty( s );
	str_strcpyc( &t, "1" );
	for ( i=0; i<numshort; ++i )
		str_strcat( s, &t );
	if ( string_mismatch( s, numshort, "11111" ) ) failed++;

	/* ...build "xoxoxoxoxo" with str_strcat */
	str_empty( s );
	str_strcpyc( &t, "xo" );
	for ( i=0; i<numshort; ++i )
		str_strcat( s, &t );
	if ( string_mismatch( s, numshort*2, "xoxoxoxoxo" ) ) failed++;

	str_empty( s );
	str_strcpyc( &t, "1" );
	for ( i=0; i<numstrings; ++i )
		str_strcat( s, &t );
	if ( inconsistent_len( s, numstrings ) ) failed++;

	str_empty( s );
	str_strcpyc( &t, "XXOO" );
	for ( i=0; i<numstrings; ++i )
		str_strcat( s, &t );
	if ( inconsistent_len( s, numstrings*4 ) ) failed++;

	str_free( &t );

	return failed;
}

static int
test_strcpyc( str *s )
{
	int failed = 0;
	int numstrings = 1000, i;

	/* Copying null string should reset string */
	str_empty( s );
	for ( i=0; i<numstrings; ++i ) {
		str_strcpyc( s, "1" );
		str_strcpyc( s, "" );
		if ( string_mismatch( s, 0, "" ) ) failed++;
	}

	/* Many rounds of copying just "1" should give "1" */
	str_empty( s );
	for ( i=0; i<numstrings; ++i ) {
		str_strcpyc( s, "1" );
		if ( string_mismatch( s, 1, "1" ) ) failed++;
	}

	/* Many rounds of copying just "XXOO" should give "XXOO" */
	str_empty( s );
	for ( i=0; i<numstrings; ++i ) {
		str_strcpyc( s, "XXOO" );
		if ( string_mismatch( s, 4, "XXOO" ) ) failed++;
	}

	return failed;
}

static int
test_strcpy( str *s )
{
	int failed = 0;
	int numstrings = 1000, i;
	str t;

	str_init( &t );

	/* Copying null string should reset string */
	str_empty( s );
	for ( i=0; i<numstrings; ++i ) {
		str_strcpyc( s, "1" );
		str_strcpy( s, &t );
		if ( string_mismatch( s, 0, "" ) ) failed++;
	}

	/* Many rounds of copying just "1" should give "1" */
	str_empty( s );
	str_strcpyc( &t, "1" );
	for ( i=0; i<numstrings; ++i ) {
		str_strcpy( s, &t );
		if ( string_mismatch( s, t.len, t.data ) ) failed++;
	}

	/* Many rounds of copying just "XXOO" should give "XXOO" */
	str_empty( s );
	str_strcpyc( &t, "XXOO" );
	for ( i=0; i<numstrings; ++i ) {
		str_strcpy( s, &t );
		if ( string_mismatch( s, t.len, t.data ) ) failed++;
	}

	str_free( &t );

	return failed;
}

static int
test_segcpy( str *s )
{
	char segment[]="0123456789";
	char *start=&(segment[2]), *end=&(segment[5]);
	int numstrings = 1000, i;
	str t, u;
	int failed = 0;

	str_init( &t );
	str_init( &u );

	str_empty( s );
	str_segcpy( s, start, start );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_segcpy( &t, start, start );
	if ( string_mismatch( &t, 0, "" ) ) failed++;

	str_segcpy( &u, start, end );
	if ( string_mismatch( &u, 3, "234" ) ) failed++;

	str_empty( s );
	for ( i=0; i<numstrings; ++i ) {
		str_segcpy( s, start, end );
		if ( string_mismatch( s, 3, "234" ) ) failed++;
	}

	str_free( &t );
	str_free( &u );

	return failed;
}

static int
test_indxcpy( str *s )
{
	char segment[]="0123456789";
	int numstrings = 10, i;
	str t, u;
	int failed = 0;

	str_init( &t );
	str_init( &u );

	str_empty( s );
	str_indxcpy( s, segment, 2, 2 );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_indxcpy( &t, segment, 2, 2 );
	if ( string_mismatch( &t, 0, "" ) ) failed++;

	str_indxcpy( &u, segment, 2, 5 );
	if ( string_mismatch( &u, 3, "234" ) ) failed++;

	str_empty( s );
	for ( i=0; i<numstrings; ++i ) {
		str_indxcpy( s, segment, 2, 5 );
		if ( string_mismatch( s, 3, "234" ) ) failed++;
	}

	str_free( &t );
	str_free( &u );

	return failed;
}

/* void str_copyposlen  ( str *s, str *in, unsigned long pos, unsigned long len ); */
static int
test_copyposlen( str *s )
{
	str t;
	int failed = 0;

	str_init( &t );

	str_copyposlen( s, &t, 1, 5 );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_strcpyc( &t, "0123456789" );

	str_copyposlen( s, &t, 1, 5 );
	if ( string_mismatch( s, 5, "12345" ) ) failed++;

	str_free( &t );

	return failed;
}

static int
test_indxcat( str *s )
{
	char segment[]="0123456789";
	int numstrings = 3, i;
	str t, u;
	int failed = 0;

	str_init( &t );
	str_init( &u );

	str_empty( s );
	str_indxcat( s, segment, 2, 2 );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_indxcat( &t, segment, 2, 2 );
	if ( string_mismatch( &t, 0, "" ) ) failed++;

	str_indxcat( &u, segment, 2, 5 );
	if ( string_mismatch( &u, 3, "234" ) ) failed++;

	str_empty( s );
	for ( i=0; i<numstrings; ++i )
		str_indxcat( s, segment, 2, 5 );
	if ( string_mismatch( s, 9, "234234234" ) ) failed++;

	str_free( &t );
	str_free( &u );

	return failed;
}

static int
test_segcat( str *s )
{
	char segment[]="0123456789";
	char *start=&(segment[2]), *end=&(segment[5]);
	int numstrings = 1000, i;
	int failed = 0;
	str t, u;

	str_init( &t );
	str_init( &u );

	str_empty( s );
	str_segcpy( s, start, start );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_segcpy( &t, start, start );
	if ( string_mismatch( &t, 0, "" ) ) failed++;

	str_segcpy( &u, start, end );
	if ( string_mismatch( &u, 3, "234" ) ) failed++;

	str_empty( s );
	for ( i=0; i<numstrings; ++i )
		str_segcat( s, start, end );
	if ( inconsistent_len( s, 3*numstrings ) ) failed++;

	str_free( &t );
	str_free( &u );

	return failed;
}

static int
test_prepend( str *s )
{
	int failed = 0;

	str_empty( s );
	str_prepend( s, "" );
	if ( string_mismatch( s, 0, "" ) ) failed++;
	str_prepend( s, "asdf" );
	if ( string_mismatch( s, 4, "asdf" ) ) failed++;

	str_strcpyc( s, "567890" );
	str_prepend( s, "01234" );
	if ( string_mismatch( s, 11, "01234567890" ) ) failed++;

	return failed;
}

static int
test_pad( str *s )
{
	int failed = 0;

	str_empty( s );
	str_pad( s, 10, '-' );
	if ( string_mismatch( s, 10, "----------" ) ) failed++;

	str_strcpyc( s, "012" );
	str_pad( s, 10, '-' );
	if ( string_mismatch( s, 10, "012-------" ) ) failed++;

	str_strcpyc( s, "0123456789" );
	str_pad( s, 10, '-' );
	if ( string_mismatch( s, 10, "0123456789" ) ) failed++;

	str_strcpyc( s, "01234567890" );
	str_pad( s, 10, '-' );
	if ( string_mismatch( s, 11, "01234567890" ) ) failed++;

	return failed;
}

static int
test_makepath( str *s )
{
	int failed = 0;

	str_empty( s );
	str_makepath( s, "", "", '/' );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_makepath( s, "", "file1.txt", '/' );
	if ( string_mismatch( s, 9, "file1.txt" ) ) failed++;

	str_makepath( s, "/home/user", "", '/' );
	if ( string_mismatch( s, 11, "/home/user/" ) ) failed++;

	str_makepath( s, "/home/user", "file1.txt", '/' );
	if ( string_mismatch( s, 20, "/home/user/file1.txt" ) ) failed++;

	str_makepath( s, "/home/user/", "", '/' );
	if ( string_mismatch( s, 11, "/home/user/" ) ) failed++;

	str_makepath( s, "/home/user/", "file1.txt", '/' );
	if ( string_mismatch( s, 20, "/home/user/file1.txt" ) ) failed++;

	return failed;
}

static int
test_findreplace( str *s )
{
	char segment[]="0123456789";
	int numstrings = 1000, i;
	int failed = 0;

	for ( i=0; i<numstrings; ++i ) {
		str_strcpyc( s, segment );
		str_findreplace( s, "234", "" );
	}
	if ( string_mismatch( s, 7, "0156789" ) ) failed++;

	for ( i=0; i<numstrings; ++i ) {
		str_strcpyc( s, segment );
		str_findreplace( s, "234", "223344" );
	}
	if ( string_mismatch( s, 13, "0122334456789" ) ) failed++;

	return failed;
}

static int
test_mergestrs( str *s )
{
	int failed = 0;

	str_empty( s );

	/* don't add any anything */
	str_mergestrs( s, NULL );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	/* add just one string */
	str_mergestrs( s, "01", NULL );
	if ( string_mismatch( s, 2, "01" ) ) failed++;

	/* add multiple strings */
	str_mergestrs( s, "01", "23", "45", "67", "89", NULL );
	if ( string_mismatch( s, 10, "0123456789" ) ) failed++;

	return failed;
}

static int
test_cpytodelim( str *s )
{
	char str0[]="\0";
	char str1[]="Col1\tCol2\tCol3\n";
	char str2[]="Col1 Col2 Col3";
	char *q;
	int failed = 0;

	q = str_cpytodelim( s, str0, "\t", 0 );
	if ( string_mismatch( s, 0, "" ) ) failed++;
	if ( *q!='\0' ) {
		fprintf( stdout, "%s line %d: str_cpytodelim() returned '%c', expected '\\t'\n", __FUNCTION__, __LINE__, *q );
		failed++;
	}

	q = str_cpytodelim( s, str1, "\t", 0 );
	if ( string_mismatch( s, 4, "Col1" ) ) failed++;
	if ( *q!='\t' ) {
		fprintf( stdout, "%s line %d: str_cpytodelim() returned '%c', expected '\\t'\n", __FUNCTION__, __LINE__, *q );
		failed++;
	}

	q = str_cpytodelim( s, str1, " \t", 0 );
	if ( string_mismatch( s, 4, "Col1" ) ) failed++;
	if ( *q!='\t' ) {
		fprintf( stdout, "%s line %d: str_cpytodelim() returned '%c', expected '\\t'\n", __FUNCTION__, __LINE__, *q );
		failed++;
	}

	q = str_cpytodelim( s, str1, "\t", 1 );
	if ( string_mismatch( s, 4, "Col1" ) ) failed++;
	if ( *q!='C' ) {
		fprintf( stdout, "%s line %d: str_cpytodelim() returned '%c', expected 'C'\n", __FUNCTION__, __LINE__, *q );
		failed++;
	}

	q = str_cpytodelim( s, str1, "\n", 0 );
	if ( string_mismatch( s, strlen(str1)-1, "Col1\tCol2\tCol3" ) ) failed++;
	if ( *q!='\n' ) {
		fprintf( stdout, "%s line %d: str_cpytodelim() returned '%c', expected '\\n'\n", __FUNCTION__, __LINE__, *q );
		failed++;
	}

	q = str_cpytodelim( s, str1, "\r", 0 );
	if ( string_mismatch( s, strlen(str1), "Col1\tCol2\tCol3\n" ) ) failed++;
	if ( *q!='\0' ) {
		fprintf( stdout, "%s line %d: str_cpytodelim() returned '%c', expected '\\n'\n", __FUNCTION__, __LINE__, *q );
		failed++;
	}

	q = str_cpytodelim( s, str2, " ", 0 );
	if ( string_mismatch( s, 4, "Col1" ) ) failed++;
	if ( *q!=' ' ) {
		fprintf( stdout, "%s line %d: str_cpytodelim() returned '%c', expected '\\t'\n", __FUNCTION__, __LINE__, *q );
		failed++;
	}

	q = str_cpytodelim( s, str2, "\t", 0 );
	if ( string_mismatch( s, strlen(str2), str2 ) ) failed++;
	if ( *q!='\0' ) {
		fprintf( stdout, "%s line %d: str_cpytodelim() returned '%c', expected '\\t'\n", __FUNCTION__, __LINE__, *q );
		failed++;
	}

	return failed;
}

/* char *str_caytodelim  ( str *s, char *p, const char *delim, unsigned char finalstep ); */
static int
test_cattodelim( str *s )
{
	char str1[] = "1 1 1 1 1 1 1";
	int failed = 0, i, n = 2;
	char *q;

	str_empty( s );
	for ( i=0; i<n; ++i ) {
		q = str_cattodelim( s, str1, " ", 0 );
		if ( *q!=' ' ) {
			fprintf( stdout, "%s line %d: str_cattodelim() returned '%c', expected ' '\n", __FUNCTION__, __LINE__, *q );
			failed++;
		}
	}
	if ( string_mismatch( s, n, "11" ) ) failed++;

	str_empty( s );
	q = str1;
	while ( *q ) {
		q = str_cattodelim( s, q, " ", 1 );
		if ( *q!='1' && *q!='\0' ) {
			fprintf( stdout, "%s line %d: str_cattodelim() returned '%c', expected '1' or '\\0' \n", __FUNCTION__, __LINE__, *q );
			failed++;
		}
	}
	if ( string_mismatch( s, 7, "1111111" ) ) failed++;

	return failed;
}

static int
test_strdup( void )
{
	char str1[] = "In Isbel's case and mine own. Service is no heritage: and I think I shall never have the blessing of God till I have issue o' my body; for they say barnes are blessings.";
	char str2[] = "Here once again we sit, once again crown'd, And looked upon, I hope, with cheerful eyes.";
	int failed = 0;
	str *dup;

	dup = str_strdupc( "" );
	if ( dup==NULL ) {
		fprintf( stdout, "%s line %d: str_strdup() returned NULL\n", __FUNCTION__, __LINE__ );
		failed++;
	} else {
		if ( string_mismatch( dup, 0, "" ) ) failed++;
		str_delete( dup );
	}

	dup = str_strdupc( str1 );
	if ( dup==NULL ) {
		fprintf( stdout, "%s line %d: str_strdup() returned NULL\n", __FUNCTION__, __LINE__ );
		failed++;
	} else {
		if ( string_mismatch( dup, strlen(str1), str1 ) ) failed++;
		str_delete( dup );
	}

	dup = str_strdupc( str2 );
	if ( dup==NULL ) {
		fprintf( stdout, "%s line %d: str_strdup() returned NULL\n", __FUNCTION__, __LINE__ );
		failed++;
	} else {
		if ( string_mismatch( dup, strlen(str2), str2 ) ) failed++;
		str_delete( dup );
	}
	return failed;
}

static int
test_toupper( str *s )
{
	char str1[] = "abcde_ABCDE_12345";
	char str2[] = "0123456789";
	int failed = 0;

	str_empty( s );
	str_toupper( s );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_strcpyc( s, str1 );
	str_toupper( s );
	if ( string_mismatch( s, strlen(str1), "ABCDE_ABCDE_12345" ) ) failed++;

	str_strcpyc( s, str2 );
	str_toupper( s );
	if ( string_mismatch( s, strlen(str2), str2 ) ) failed++;

	return failed;
}

static int
test_tolower( str *s )
{
	char str1[] = "abcde_ABCDE_12345";
	char str2[] = "0123456789";
	int failed = 0;

	str_empty( s );
	str_tolower( s );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_strcpyc( s, str1 );
	str_tolower( s );
	if ( string_mismatch( s, strlen(str1), "abcde_abcde_12345" ) ) failed++;

	str_strcpyc( s, str2 );
	str_tolower( s );
	if ( string_mismatch( s, strlen(str2), str2 ) ) failed++;

	return failed;
}

static int
test_trimws( str *s )
{
	char str1[] = "      ksjadfk    lajskfjds      askdjflkj   ";
	char str2[] = "        ";
	int failed = 0;

	str_empty( s );
	str_trimstartingws( s );
	if ( string_mismatch( s, 0, "" ) ) failed++;
	str_trimendingws( s );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_strcpyc( s, str2 );
	str_trimstartingws( s );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_strcpyc( s, str2 );
	str_trimendingws( s );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_strcpyc( s, str1 );
	str_trimstartingws( s );
	if ( string_mismatch( s, strlen("ksjadfk    lajskfjds      askdjflkj   "), "ksjadfk    lajskfjds      askdjflkj   " ) ) failed++;
	str_trimendingws( s );
	if ( string_mismatch( s, strlen("ksjadfk    lajskfjds      askdjflkj"), "ksjadfk    lajskfjds      askdjflkj" ) ) failed++;

	str_strcpyc( s, str1 );
	str_trimendingws( s );
	if ( string_mismatch( s, strlen("      ksjadfk    lajskfjds      askdjflkj"), "      ksjadfk    lajskfjds      askdjflkj" ) ) failed++;
	str_trimstartingws( s );
	if ( string_mismatch( s, strlen("ksjadfk    lajskfjds      askdjflkj"), "ksjadfk    lajskfjds      askdjflkj" ) ) failed++;

	str_empty( s );
	str_stripws( s );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_strcpyc( s, "0123456789" );
	str_stripws( s );
	if ( string_mismatch( s, 10, "0123456789" ) ) failed++;

	str_strcpyc( s, str1 );
	str_stripws( s );
	if ( string_mismatch( s, strlen("ksjadfklajskfjdsaskdjflkj"), "ksjadfklajskfjdsaskdjflkj" ) ) failed++;

	return failed;
}

static int
test_reverse( str *s )
{
	int failed = 0;

	/* empty string */
	str_strcpyc( s, "" );
	str_reverse( s );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	/* string with even number of elements */
	str_strcpyc( s, "0123456789" );
	str_reverse( s );
	if ( string_mismatch( s, 10, "9876543210" ) ) failed++;
	str_reverse( s );
	if ( string_mismatch( s, 10, "0123456789" ) ) failed++;

	/* string with odd number of elements */
	str_strcpyc( s, "123456789" );
	str_reverse( s );
	if ( string_mismatch( s, 9, "987654321" ) ) failed++;
	str_reverse( s );
	if ( string_mismatch( s, 9, "123456789" ) ) failed++;

	return failed;
}

static int
test_trim( str *s )
{
	char str1[] = "123456789";
	char str2[] = "987654321";
	int failed = 0;

	str_strcpyc( s, str1 );
	str_trimbegin( s, 0 );
	if ( string_mismatch( s, 9, str1 ) ) failed++;
	str_trimend( s, 0 );
	if ( string_mismatch( s, 9, str1 ) ) failed++;

	str_strcpyc( s, str1 );
	str_trimbegin( s, 1 );
	if ( string_mismatch( s, 8, "23456789" ) ) failed++;

	str_strcpyc( s, str1 );
	str_trimbegin( s, 4 );
	if ( string_mismatch( s, 5, "56789" ) ) failed++;

	str_strcpyc( s, str1 );
	str_trimbegin( s, 9 );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	str_strcpyc( s, str2 );
	str_trimend( s, 1 );
	if ( string_mismatch( s, 8, "98765432" ) ) failed++;

	str_strcpyc( s, str2 );
	str_trimend( s, 6 );
	if ( string_mismatch( s, 3, "987" ) ) failed++;

	str_strcpyc( s, str2 );
	str_trimend( s, 9 );
	if ( string_mismatch( s, 0, "" ) ) failed++;

	return failed;
}

static int
test_case( str *s )
{
	int failed = 0;

	str_strcpyc( s, "asdfjalskjfljasdfjlsfjd" );
	if ( !str_is_lowercase( s ) ) {
		fprintf( stdout, "%s line %d: str_is_lowercase('%s') returned false\n", __FUNCTION__, __LINE__, s->data );
		failed++;
	}
	if ( str_is_uppercase( s ) ) {
		fprintf( stdout, "%s line %d: str_is_uppercase('%s') returned true\n", __FUNCTION__, __LINE__, s->data );
		failed++;
	}
	if ( str_is_mixedcase( s ) ) {
		fprintf( stdout, "%s line %d: str_is_mixedcase('%s') returned true\n", __FUNCTION__, __LINE__, s->data );
		failed++;
	}

	str_strcpyc( s, "ASDFJALSKJFLJASDFJLSFJD" );
	if ( str_is_lowercase( s ) ) {
		fprintf( stdout, "%s line %d: str_is_lowercase('%s') returned true\n", __FUNCTION__, __LINE__, s->data );
		failed++;
	}
	if ( !str_is_uppercase( s ) ) {
		fprintf( stdout, "%s line %d: str_is_uppercase('%s') returned false\n", __FUNCTION__, __LINE__, s->data );
		failed++;
	}
	if ( str_is_mixedcase( s ) ) {
		fprintf( stdout, "%s line %d: str_is_mixedcase('%s') returned true\n", __FUNCTION__, __LINE__, s->data );
		failed++;
	}

	str_strcpyc( s, "ASdfjalsKJFLJASdfjlsfjd" );
	if ( str_is_lowercase( s ) ) {
		fprintf( stdout, "%s line %d: str_is_lowercase('%s') returned true\n", __FUNCTION__, __LINE__, s->data );
		failed++;
	}
	if ( str_is_uppercase( s ) ) {
		fprintf( stdout, "%s line %d: str_is_uppercase('%s') returned true\n", __FUNCTION__, __LINE__, s->data );
		failed++;
	}
	if ( !str_is_mixedcase( s ) ) {
		fprintf( stdout, "%s line %d: str_is_mixedcase('%s') returned false\n", __FUNCTION__, __LINE__, s->data );
		failed++;
	}

	return failed;
}

static int
test_strcmp( str *s )
{
	int failed = 0;
	str t;

	str_init( &t );

	str_empty( s );
	if ( str_strcmp( s, s ) ) {
		fprintf( stdout, "%s line %d: str_strcmp(s,s) returned non-zero\n", __FUNCTION__, __LINE__ );
		failed++;
	}
	if ( str_strcmp( s, &t ) ) {
		fprintf( stdout, "%s line %d: str_strcmp(s,t) returned non-zero\n", __FUNCTION__, __LINE__ );
		failed++;
	}

	str_strcpyc( s, "lakjsdlfjdskljfklsjf" );
	if ( str_strcmp( s, s ) ) {
		fprintf( stdout, "%s line %d: str_strcmp(s,s) returned non-zero\n", __FUNCTION__, __LINE__ );
		failed++;
	}
	if ( !str_strcmp( s, &t ) ) {
		fprintf( stdout, "%s line %d: str_strcmp(s,t) returned zero\n", __FUNCTION__, __LINE__ );
		failed++;
	}

	str_strcpy( &t, s );
	if ( str_strcmp( s, s ) ) {
		fprintf( stdout, "%s line %d: str_strcmp(s,s) returned non-zero\n", __FUNCTION__, __LINE__ );
		failed++;
	}
	if ( str_strcmp( s, &t ) ) {
		fprintf( stdout, "%s line %d: str_strcmp(s,t) returned non-zero\n", __FUNCTION__, __LINE__ );
		failed++;
	}

	str_free( &t );

	return failed;
}

static int
test_match( str *s )
{
	int failed = 0;

	str_empty( s );
	if ( str_match_first( s, '0' ) ) {
		fprintf( stdout, "%s line %d: str_match_first() returned non-zero\n", __FUNCTION__, __LINE__ );
		failed++;
	}
	str_strcpyc( s, "012345" );
	if ( !str_match_first( s, '0' ) ) {
		fprintf( stdout, "%s line %d: str_match_first() returned zero\n", __FUNCTION__, __LINE__ );
		failed++;
	}
	if ( !str_match_end( s, '5' ) ) {
		fprintf( stdout, "%s line %d: str_match_end() returned zero\n", __FUNCTION__, __LINE__ );
		failed++;
	}

	return failed;
}

static int
test_char( str *s )
{
	unsigned long i;
	str t, u;
	int failed = 0;

	str_init( &t );
	str_init( &u );

	str_empty( s );
	for ( i=0; i<5; ++i ) {
		if ( str_char( s, i ) != '\0' ) {
			fprintf( stdout, "%s line %d: str_char() did not return '\\0'\n", __FUNCTION__, __LINE__ );
			failed++;
		}
		if ( str_revchar( s, i ) != '\0' ) {
			fprintf( stdout, "%s line %d: str_revchar() did not return '\\0'\n", __FUNCTION__, __LINE__ );
			failed++;
		}
	}

	str_strcpyc( s, "0123456789" );
	for ( i=0; i<s->len; ++i ) {
		str_addchar( &t, str_char( s, i ) );
		str_addchar( &u, str_revchar( s, i ) );
	}

	if ( string_mismatch( &t, s->len, s->data ) ) failed++;

	str_reverse( s );
	if ( string_mismatch( &u, s->len, s->data ) ) failed++;

	str_free( &t );
	str_free( &u );

	return failed;
}

static int
test_swapstrings( str *s )
{
	int failed = 0;
	str t;

	str_init( &t );

	str_strcpyc( &t, "0123456789" );
	str_strcpyc( s,  "abcde" );

	str_swapstrings( s, &t );
	if ( string_mismatch( &t, 5, "abcde" ) ) failed++;
	if ( string_mismatch( s, 10, "0123456789" ) ) failed++;

	str_swapstrings( s, &t );
	if ( string_mismatch( s, 5, "abcde" ) ) failed++;
	if ( string_mismatch( &t, 10, "0123456789" ) ) failed++;

	str_free( &t );

	return failed;
}

int
main ( int argc, char *argv[] )
{
	int failed = 0;
	int ntest = 2;
	int i;
	str s;
	str_init( &s );

	/* ...core functions */
	for ( i=0; i<ntest; ++i )
		failed += test_empty( &s );

	/* ...adding functions */
	for ( i=0; i<ntest; ++i)
		failed += test_addchar( &s );
	for ( i=0; i<ntest; ++i)
		failed += test_strcatc( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_strcat( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_segcat( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_indxcat( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_cattodelim( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_prepend( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_pad( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_mergestrs( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_makepath( &s );

	/* ...copying functions */
	for ( i=0; i<ntest; ++i)
		failed += test_strcpyc( &s );
	for ( i=0; i<ntest; ++i)
		failed += test_strcpy( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_cpytodelim( &s );
	for ( i=0; i<ntest; ++i)
		failed += test_segcpy( &s );
	for ( i=0; i<ntest; ++i)
		failed += test_indxcpy( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_copyposlen( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_strdup();

	/* ...utility functions */
	for ( i=0; i<ntest; ++i)
		failed += test_findreplace( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_reverse( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_toupper( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_tolower( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_trimws( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_trim( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_case( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_strcmp( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_char( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_swapstrings( &s );
	for ( i=0; i<ntest; ++i )
		failed += test_match( &s );

	str_free( &s );

	if ( !failed ) {
		printf( "%s: PASSED\n", progname );
		return EXIT_SUCCESS;
	} else {
		printf( "%s: FAILED\n", progname );
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
