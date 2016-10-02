#include <stdio.h>

#include "cJSON2.h"


typedef int (*test_func)(void);

typedef struct
    {
    char const *    description;
    test_func       test_func;
    } test;

static int test_parse_false
    (
    void
    );

static int test_parse_null
    (
    void
    );

static int test_parse_true
    (
    void
    );

static test tests[] = 
    {/*     description,        test_func           */
        {   "Pasre false",      test_parse_false    },
        {   "Parse null",       test_parse_null     },
        {   "Parse true",       test_parse_true     },
    };

static char const * TEST_PASSED = "PASSED";
static char const * TEST_FAILED = "FAILED";


/**********************************************************
*	cJSON_test_suite
*
*	cJSON test suite
*
**********************************************************/
int main
    (
    void
    )
{
int             num_tests;
int             num_passed;
int             num_failed;
int             i;
int             did_pass;
char const *    test_result;

num_tests  = sizeof( tests ) / sizeof( tests[0] );
num_passed = 0;
num_failed = 0;

for( i = 0; i < num_tests; i++ )
    {
    did_pass = tests[i].test_func();

    if( did_pass )
        {
        test_result = TEST_PASSED;
        num_passed++;
        }
    else
        {
        test_result = TEST_FAILED;
        num_failed++;
        }

    printf( "Test %d %s: %s\n", (i + 1), test_result, tests[i].description );    
    }

printf( "%d PASSED, %d FAILED\n", num_passed, num_failed );
}


/**********************************************************
*	test_parse_false
*
*	Tests parsing a simple single false JSON value
*
**********************************************************/
static int test_parse_false
    (
    void
    )
{
int     did_pass;
cJSON * json;

json = cJSON_Parse( "false" );

did_pass = ( cJSON_False == json->type );

cJSON_Delete( json );

return did_pass;
}    


/**********************************************************
*	test_parse_null
*
*	Tests parsing a simple single null JSON value
*
**********************************************************/
static int test_parse_null
    (
    void
    )
{
int     did_pass;
cJSON * json;

json = cJSON_Parse( "null" );

did_pass = ( cJSON_Null == json->type );

cJSON_Delete( json );

return did_pass;
}


/**********************************************************
*	test_parse_true
*
*	Tests parsing a simple single true JSON value
*
**********************************************************/
static int test_parse_true
    (
    void
    )
{
int     did_pass;
cJSON * json;

json = cJSON_Parse( "true" );

did_pass = ( cJSON_True == json->type );

cJSON_Delete( json );

return did_pass;
} 