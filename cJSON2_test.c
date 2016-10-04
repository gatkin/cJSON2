#include <math.h>
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

typedef struct
    {
    char const *    json;
    int             should_parse_succeed;
    double          exptd_value;
    } parse_number_test_case;

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

static int test_parse_number
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

test tests[] =
    {/*     description,                    test_func                       */
    {   "Parse empty array",            test_parse_array_empty          },
    {   "Parse simple-valued array",    test_parse_array_simple_values  },
    {   "Parse false",                  test_parse_false                },
    {   "Parse null",                   test_parse_null                 },
    {   "Parse number",                 test_parse_number               },
    {   "Parse object",                 test_parse_object               },
    {   "Parse empty object",           test_parse_object_empty         },
    {   "Parse string",                 test_parse_string               },
    {   "Parse empty string",           test_parse_string_empty         },
    {   "Parse true",                   test_parse_true                 },
    };

num_tests  = cnt_of_array( tests );
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
    cJSON_Number,
    cJSON_True,
    cJSON_Array,
    };

json = cJSON_Parse( "[null, false, [], {}, \"hello\", 1.0,  true, []]" );

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
*	test_parse_number
*
*	Tests parsing numbers
*
**********************************************************/
static int test_parse_number
    (
    void
    )
{
parse_number_test_case test_cases[] =
    {/*     JSON,       should_parse_succeed,   exptd_value     */
        {   "1.0",      1,                      1.0             },
        {   "1",        1,                      1.0             },
        {   "1.7e3",    1,                      1.7e3           },
        {   "NaN",      1,                      NAN             },
        {   "Infinity", 1,                      INFINITY        },
        {   "-1.0",     1,                      -1.0            },
        {   "-hello",   0,                      0.0             },
        {   "1.hello",  0,                      0.0             },
        {   "1,0hello", 0,                      0.0             }
    };

cJSON * json;
int     i;
int     did_pass;

did_pass = 1;

for( i = 0; ( did_pass ) && ( i < cnt_of_array( test_cases ) ); i++ )
    {
    json = cJSON_Parse( test_cases[i].json );

    if( test_cases[i].should_parse_succeed )
        {
        did_pass = ( NULL != json );
        did_pass = ( did_pass ) && ( cJSON_Number == json->type );
        did_pass = ( did_pass ) && ( test_cases[i].exptd_value == json->valuedouble );
        }
    else
        {
        did_pass = ( NULL == json );
        }

    cJSON_Delete( json );
    }

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