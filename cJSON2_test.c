#include <stdio.h>
#include <string.h>

#include "cJSON2.h"

#define cnt_of_array( _xs ) ( sizeof( _xs ) / sizeof( _xs[0] ) )

typedef int (*test_func)(void);

typedef struct
    {
    char const *    description;
    test_func       test_func;
    } test;

static int test_parse_array_empty
    (
    void
    );

static int test_parse_array_simple_values
    (
    void
    );

static int test_parse_false
    (
    void
    );

static int test_parse_null
    (
    void
    );

static int test_parse_object
    (
    void
    );

static int test_parse_object_empty
    (
    void
    );

static int test_parse_string
    (
    void
    );

static int test_parse_string_empty
    (
    void
    );

static int test_parse_true
    (
    void
    );

static test tests[] = 
    {/*     description,                    test_func                       */
        {   "Parse empty array",            test_parse_array_empty          },
        {   "Parse simple-valued array",    test_parse_array_simple_values  },
        {   "Parse false",                  test_parse_false                },
        {   "Parse null",                   test_parse_null                 },
        {   "Parse object",                 test_parse_object               },
        {   "Parse empty object",           test_parse_object_empty         },
        {   "Parse string",                 test_parse_string               },
        {   "Parse empty string",           test_parse_string_empty         },
        {   "Parse true",                   test_parse_true                 },
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
*	test_parse_array_empty
*
*	Tests parsing an empty JSON array.
*
**********************************************************/
static int test_parse_array_empty
    (
    void
    )
{
int     did_pass;
cJSON * json;

json = cJSON_Parse( "[]" );

did_pass = ( NULL != json );
did_pass = ( did_pass ) && ( cJSON_Array == json->type );
did_pass = ( did_pass ) && ( 0 == cJSON_GetArraySize( json ) );
did_pass = ( did_pass ) && ( NULL == cJSON_GetArrayItem( json, 0 ) );

cJSON_Delete( json );

return did_pass;
}


/**********************************************************
*	test_parse_array_simple_values
*
*	Tests parsing an JSON array containing simple values.
*
**********************************************************/
static int test_parse_array_simple_values
    (
    void
    )
{
int     did_pass;
cJSON * json;
int     idx;
cJSON * array_item;

const cJSON_ValueType exptd_values[] =
    {
    cJSON_Null,
    cJSON_False,
    cJSON_Array,
    cJSON_Object,
    cJSON_String,
    cJSON_True,
    cJSON_Array,
    };

json = cJSON_Parse( "[null, false, [], {}, \"hello\",  true, []]" );

did_pass = ( NULL != json );
did_pass = ( did_pass ) && ( cJSON_Array == json->type );
did_pass = ( did_pass ) && ( cJSON_GetArraySize( json ) == cnt_of_array( exptd_values ) );

for( idx = 0; ( did_pass ) && ( idx < cnt_of_array( exptd_values ) ); idx++ )
    {
    array_item = cJSON_GetArrayItem( json, idx );
    did_pass = ( did_pass ) && ( NULL != array_item );
    did_pass = ( did_pass ) && ( exptd_values[idx] == array_item->type );
    }

cJSON_Delete( json );

return did_pass;
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

did_pass = ( NULL != json );
did_pass = ( did_pass ) && ( cJSON_False == json->type );

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

did_pass = ( NULL != json );
did_pass = ( did_pass ) && ( cJSON_Null == json->type );

cJSON_Delete( json );

return did_pass;
}


/**********************************************************
*	test_parse_object
*
*	Tests parsing an object
*
**********************************************************/
static int test_parse_object
    (
    void
    )
{
int     did_pass;
cJSON * json;

json = cJSON_Parse( "{ \"hello\" : \"world\", \"array\": [], \"null\": null, \"true\" : true }" );
did_pass = ( NULL != json );
did_pass = ( did_pass ) && ( cJSON_Object == json->type );

cJSON_Delete( json );

return did_pass;
}


/**********************************************************
*	test_parse_object_empty
*
*	Tests parsing an empty object
*
**********************************************************/
static int test_parse_object_empty
    (
    void
    )
{
int     did_pass;
cJSON * json;

json = cJSON_Parse( "{}" );
did_pass = ( NULL != json );
did_pass = ( did_pass ) && ( cJSON_Object == json->type );

cJSON_Delete( json );

return did_pass;
}


/**********************************************************
*	test_parse_string
*
*	Tests parsing a simple string
*
**********************************************************/
static int test_parse_string
    (
    void
    )
{
int     did_pass;
cJSON * json;

json = cJSON_Parse( "\"hello\"" );
did_pass = ( NULL != json );
did_pass = ( did_pass ) && ( cJSON_String == json->type );
did_pass = ( did_pass ) && ( NULL != json->valuestring  );
did_pass = ( did_pass ) && ( strlen( "hello" ) == strlen( json->valuestring ) );
did_pass = ( did_pass ) && ( 0 == strcmp( "hello", json->valuestring ) );

cJSON_Delete( json );

return did_pass;
}


/**********************************************************
*	test_parse_string_empty
*
*	Tests parsing an empty string
*
**********************************************************/
static int test_parse_string_empty
    (
    void
    )
{
int     did_pass;
cJSON * json;

json = cJSON_Parse( "\"\"" );
did_pass = ( NULL != json );
did_pass = ( did_pass ) && ( cJSON_String == json->type );
did_pass = ( did_pass ) && ( NULL != json->valuestring  );
did_pass = ( did_pass ) && ( 0 == strcmp( "", json->valuestring ) );

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

did_pass = ( NULL != json );
did_pass = ( did_pass ) && ( cJSON_True == json->type );

cJSON_Delete( json );

return did_pass;
} 