#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "cJSON2.h"
#include "cJSON2_private.h"

/****************************************
Private Types
****************************************/
typedef enum
    {
    PARSE_STATE_VALUE,
    PARSE_STATE_NEXT_ARRAY_VALUE,
    PARSE_STATE_NEXT_OBJECT_VALUE,
    PARSE_STATE_OBJECT_KEY,
    PARSE_STATE_ERROR,
    PARSE_STATE_COMPLETE,
    } parse_state;

typedef struct
    {
    char const *    json_str;
    char const *    crnt_posn;      /* Current position within JSON string  */
    cJSON_Hooks     hooks;
    cJSON *         root;
    cJSON *         crnt_node;
    parse_state     state;
    } parse_context;

/****************************************
Private Function Declarations
****************************************/
static void crnt_node_add_child
    (
    parse_context * context
    );

static void crnt_node_add_sibling
    (
    parse_context * context
    );

static cJSON * new_node
    (
    cJSON_Hooks const * hooks
    );

static void next_array_value
    (
    parse_context * context
    );

static void next_object_value
    (
    parse_context * context
    );

static void next_parse_state
    (
    parse_context * context
    );

static void parse
    (
    parse_context * context
    );

static void parse_array
    (
    parse_context * context
    );

static void parse_context_init
    (
    parse_context *context
    );

static void parse_number
    (
    parse_context * context
    );

static void parse_object
    (
    parse_context * context
    );

static void parse_object_key
    (
    parse_context * context
    );

static void parse_string
    (
    parse_context * context
    );

static void parse_value
    (
    parse_context * context
    );

static int string_extract_from_crnt_posn
    (
    parse_context * context,
    char **         extracted_string_out
    );

static const char * skip_whitespace
    (
    const char * string_in
    );


/****************************************
Public Functions
****************************************/

/**********************************************************
*	cJSON_Parse
*
*	Parse a JSON string with default hooks.
*
**********************************************************/
cJSON * cJSON_Parse
    (
    char const * json_str
    )
{
cJSON_Hooks default_hooks;

default_hooks.malloc_fn = malloc;
default_hooks.free_fn   = free;

return cJSON_ParseWithHooks( json_str, &default_hooks );
}


/**********************************************************
*	cJSON_ParseWithHooks
*
*	Parse JSON string with provided hooks. On error, this
*   returns NULL. Otherwise, the caller must free the resources
*   owned by the returned pointer by passing it to cJSON_Delete()
*   when they are done with it.
*
**********************************************************/
cJSON * cJSON_ParseWithHooks
    (
    char const *        json_str,
    cJSON_Hooks const * hooks
    )
{
parse_context context;

parse_context_init( &context );

context.json_str  = json_str;
context.crnt_posn = &json_str[0];
context.state     = PARSE_STATE_VALUE;

context.hooks.malloc_fn = hooks->malloc_fn;
context.hooks.free_fn   = hooks->free_fn;

context.root      = new_node( &context.hooks );
context.crnt_node = context.root;

if( NULL != context.root )
    {
    parse( &context );
    }

return context.root;
}


/**********************************************************
*	crnt_node_add_child
*
*	Adds a child to node the context's current node and sets
*	the context's current node to the newly created node.
*
**********************************************************/
static void crnt_node_add_child
    (
    parse_context * context
    )
{
cJSON * child;

child = new_node( &context->hooks );
if( NULL == child )
    {
    context->state = PARSE_STATE_ERROR;
    }
else
    {
    child->parent             = context->crnt_node;
    context->crnt_node->child = child;
    context->crnt_node        = child;
    }
}


/**********************************************************
*	crnt_node_add_sibling
*
*	Adds a sibling node to the context's current node and
*	sets the context's current node to the newly created
*	node.
*
**********************************************************/
static void crnt_node_add_sibling
    (
    parse_context * context
    )
{
cJSON * sibling;

sibling = new_node( &context->hooks );
if( NULL == sibling )
    {
    context->state = PARSE_STATE_ERROR;
    }
else
    {
    sibling->prev            = context->crnt_node;
    sibling->parent          = context->crnt_node->parent;
    context->crnt_node->next = sibling;
    context->crnt_node       = sibling;
    }
}


/**********************************************************
*	new_node
*
*	Creates, initializes, and returns a new node. It is the
*   caller's responsibility to free the returned pointer.
*
**********************************************************/
static cJSON * new_node
    (
    cJSON_Hooks const * hooks
    )
{
cJSON * node;

node = (cJSON*)hooks->malloc_fn( sizeof( *node ) );

if( NULL != node )
    {
    memset( node, 0, sizeof( *node ) );
    }

return node;
}


/**********************************************************
*	next_array_value
*
*	Prepares the context to parse the next array value if
*	if there is one
*
**********************************************************/
static void next_array_value
    (
    parse_context * context
    )
{
context->crnt_posn = skip_whitespace( context->crnt_posn );

if( !parent_node_is_array( context->crnt_node ) )
    {
    // We should never get here
    context->state = PARSE_STATE_ERROR;
    }
else if( ']' == context->crnt_posn[0] )
    {
    // We've come to the end of an array. Move past the ']'.
    context->crnt_posn++;

    // Step up a level back to the containing array.
    context->crnt_node = context->crnt_node->parent;
    next_parse_state( context );
    }
else if( ',' ==  context->crnt_posn[0] )
    {
    // We found another value in the array, move past the ',' and prepare to parse the
    // next value in the array.
    context->crnt_posn++;

    crnt_node_add_sibling( context );
    if( PARSE_STATE_ERROR != context->state )
        {
        context->state = PARSE_STATE_VALUE;
        }
    }
else
    {
    // Invalid input
    context->state = PARSE_STATE_ERROR;
    }
}


/**********************************************************
*	next_object_value
*
*	Prepares the context to parse the next object value if
*	if there is one
*
**********************************************************/
static void next_object_value
    (
    parse_context * context
    )
{
context->crnt_posn = skip_whitespace( context->crnt_posn );

if( !parent_node_is_object( context->crnt_node ) )
    {
    // We should never get here.
    context->state = PARSE_STATE_ERROR;
    }
else if( '}' == context->crnt_posn[0] )
    {
    // We've reached the end of an object. Move past the closing '}'.
    context->crnt_posn++;

    // Step up one level to the containing object.
    context->crnt_node = context->crnt_node->parent;
    next_parse_state( context );
    }
else if( ',' == context->crnt_posn[0] )
    {
    // We've found another key/value pair in the object, move past the ','
    // and prepare to parse the next value in the object.
    context->crnt_posn++;

    crnt_node_add_sibling( context );
    if( PARSE_STATE_ERROR != context->state )
        {
        context->state = PARSE_STATE_OBJECT_KEY;
        }
    }
else
    {
    // Invalid input
    context->state = PARSE_STATE_ERROR;
    }
}


/**********************************************************
*	next_parse_state
*
*	Returns the next parse state. Should be called after
*   Successfully parsing a value.
*
**********************************************************/
static void next_parse_state
    (
    parse_context * context
    )
{
if( PARSE_STATE_ERROR == context->state )
    {
    // Shouldn't get here. If we do it's an error, so leave the
    // state alone.
    }
else if( ( NULL == context->crnt_node->parent ) && ( '\0' == context->crnt_posn[0] ) )
    {
    // We just finished parsing the top-level value so we're done. Hurray!
    context->state = PARSE_STATE_COMPLETE;
    }
else if( parent_node_is_array( context->crnt_node ) )
    {
    // The value we finished parsing is contained within an array
    context->state = PARSE_STATE_NEXT_ARRAY_VALUE;
    }
else if( parent_node_is_object( context->crnt_node ) )
    {
    // The value we finished parsing is contained within an object
    context->state = PARSE_STATE_NEXT_OBJECT_VALUE;
    }
else
    {
    context->state = PARSE_STATE_ERROR;
    }
}


/**********************************************************
*	parse
*
*	Top-level parse function
*
**********************************************************/
static void parse
    (
    parse_context * context
    )
{
while( ( PARSE_STATE_COMPLETE != context->state ) && ( PARSE_STATE_ERROR != context->state ) )
    {
    switch ( context->state )
        {
        case PARSE_STATE_VALUE:
            parse_value( context );
            break;

        case PARSE_STATE_OBJECT_KEY:
            parse_object_key( context );
            break;

        case PARSE_STATE_NEXT_ARRAY_VALUE:
            next_array_value( context );
            break;

        case PARSE_STATE_NEXT_OBJECT_VALUE:
            next_object_value( context );
            break;

        default:
            // Shouldn't get here.
            context->state = PARSE_STATE_ERROR;
            break;
        }
    }

if( PARSE_STATE_ERROR == context->state )
    {
    cJSON_DeleteWithHooks( context->root, &context->hooks );
    context->root = NULL;
    }
}    


/**********************************************************
*	parse_array
*
*	Parses a JSON array
*
**********************************************************/
static void parse_array
    (
    parse_context * context
    )
{
context->crnt_node->type = cJSON_Array;

// Move past the opening '[' of the array.
context->crnt_posn++;

// Move to the first value of the array
context->crnt_posn = skip_whitespace( context->crnt_posn );

if( ']' == context->crnt_posn[0] )
    {
    // This is an empty array. Move past the closing
    // bracket of this empty array.
    context->crnt_posn++;
    next_parse_state( context );
    }
else
    {
    // This array contains values, prepare to parse the
    // first value of the array
    crnt_node_add_child( context );
    if( PARSE_STATE_ERROR != context->state )
        {
        context->state = PARSE_STATE_VALUE;
        }
    }
}


/**********************************************************
*	parse_context_init
*
*	Initialize parse context.
*
**********************************************************/
static void parse_context_init
    (
    parse_context *context
    )
{
context->json_str     = NULL;
context->crnt_posn    = NULL;
context->root         = NULL;
context->crnt_node    = NULL;
context->state        = PARSE_STATE_ERROR;
memset( &context->hooks, 0, sizeof( context->hooks ) );
}


/**********************************************************
*	parse_number
*
*	Parse JSON number
*
**********************************************************/
static void parse_number
    (
    parse_context * context
    )
{
double parsed_value;
char * next_posn;

parsed_value = strtod( context->crnt_posn, &next_posn );

if( ( HUGE_VAL == parsed_value ) || ( -HUGE_VAL == parsed_value ) )
    {
    // Overflow
    context->state = PARSE_STATE_ERROR;
    }
else if( ( 0.0 == parsed_value ) && ( ERANGE == errno ) )
    {
    // Underflow
    context->state = PARSE_STATE_ERROR;
    }
else if( ( 0.0 == parsed_value ) && ( next_posn == context->crnt_posn ) )
    {
    // No conversion was performed
    context->state = PARSE_STATE_ERROR;
    }
else
    {
    // Successful parse
    context->crnt_node->type        = cJSON_Number;
    context->crnt_node->valuedouble = parsed_value;
    context->crnt_node->valueint    = (int)parsed_value;
    context->crnt_posn              = next_posn;
    next_parse_state( context );
    }
}


/**********************************************************
*	parse_object
*
*	Parses a JSON object.
*
**********************************************************/
static void parse_object
    (
    parse_context * context
    )
{
context->crnt_node->type = cJSON_Object;

// Move past the opening '}'
context->crnt_posn++;

// Move to the first value of the object
context->crnt_posn = skip_whitespace( context->crnt_posn );

if( '}' == context->crnt_posn[0] )
    {
    // This is an empty object, move past its closing brace
    context->crnt_posn++;
    next_parse_state( context );
    }
else
    {
    // Prepare to parse this object's first value
    crnt_node_add_child( context );
    if( PARSE_STATE_ERROR != context->state )
        {
        context->state = PARSE_STATE_OBJECT_KEY;
        }
    }
}


/**********************************************************
*	parse_object_key
*
*	Parse an object key
*
**********************************************************/
static void parse_object_key
    (
    parse_context * context
    )
{
int is_valid_key;

is_valid_key = string_extract_from_crnt_posn( context, &context->crnt_node->string );

if( is_valid_key )
    {
    // Now that we got the key, look for the ':' character
    context->crnt_posn = skip_whitespace( context->crnt_posn );

    if( ':' == context->crnt_posn[0] )
        {
        // Move past the ':'
        context->crnt_posn++;
        context->state = PARSE_STATE_VALUE;
        }
    else
        {
        context->state = PARSE_STATE_ERROR;
        }
    }
}


/**********************************************************
*	parse_string
*
*	Parse JSON string.
*
*   TODO: Does not allow control characters or unicode values
*         and does not prevent against buffer overflows.
*
**********************************************************/
static void parse_string
    (
    parse_context * context
    )
{
int is_valid_string;

context->crnt_node->type = cJSON_String;

is_valid_string = string_extract_from_crnt_posn( context, &context->crnt_node->valuestring );

if( is_valid_string )
    {
    next_parse_state( context );
    }
}


/**********************************************************
*	parse_value
*
*	Parses the next JSON value in a JSON string.
*
**********************************************************/
static void parse_value
    (
    parse_context * context
    )
{
context->crnt_posn = skip_whitespace( context->crnt_posn );

if( NULL == context->crnt_posn )
    {
    context->state = PARSE_STATE_ERROR;
    }
else if( '\"' == context->crnt_posn[0] )
    {
    parse_string( context );
    }

else if( '[' == context->crnt_posn[0] )
    {
    parse_array( context );
    }
else if( '{' == context->crnt_posn[0] )
    {
    parse_object( context );
    }
else if( 0 == strncmp( context->crnt_posn, "null", 4 ) )
    {
    context->crnt_node->type = cJSON_Null;
    context->crnt_posn += 4;
    next_parse_state( context );
    }
else if( 0 == strncmp( context->crnt_posn, "false", 5 ) )
    {
    context->crnt_node->type = cJSON_False;
    context->crnt_posn += 5;
    next_parse_state( context );
    }
else if( 0 == strncmp( context->crnt_posn, "true", 4 ) )
    {
    context->crnt_node->type = cJSON_True;
    context->crnt_posn += 4;
    next_parse_state( context );
    }
else if( 0 == strncmp( "-Infinity", context->crnt_posn, 9 ) )
    {
    context->crnt_node->type        = cJSON_Number;
    context->crnt_node->valuedouble = -INFINITY;
    context->crnt_posn += 9;
    next_parse_state( context );
    }
else if( ( '-' == context->crnt_posn[0] ) || ( isdigit( context->crnt_posn[0] ) ) )
    {
    parse_number( context );
    }
else if( 0 == strncmp( "NaN", context->crnt_posn, 3 ) )
    {
    context->crnt_node->type        = cJSON_Number;
    context->crnt_node->valuedouble = NAN;
    context->crnt_posn += 3;
    next_parse_state( context );
    }
else if( 0 == strncmp( "Infinity", context->crnt_posn, 8 ) )
    {
    context->crnt_node->type        = cJSON_Number;
    context->crnt_node->valuedouble = INFINITY;
    context->crnt_posn += 8;
    next_parse_state( context );
    }
else
    {
    // Invalid input
    context->state = PARSE_STATE_ERROR;
    }
}


/**********************************************************
*	string_extract_from_crnt_posn
*
*   Extracts string from current position in JSON string.
*	If an error occurs, or if the provided JSON is invalid,
*	this returns 0 and sets extracted_string_out to NULL.
*	Otherwise this returns 1 and populates extracted_string_out
*
**********************************************************/
static int string_extract_from_crnt_posn
    (
    parse_context * context,
    char **         extracted_string_out
    )
{
int             length;
char const *    crnt_char_ptr;

length = 0;
*extracted_string_out = NULL;

context->crnt_posn = skip_whitespace( context->crnt_posn );

if( '\"' != context->crnt_posn[0] )
    {
    // Not a string
    context->state = PARSE_STATE_ERROR;
    return 0;
    }

// Move to the first character past the opening "
crnt_char_ptr = &context->crnt_posn[1];

while( ( '\0' != *crnt_char_ptr ) && ( '\"' != *crnt_char_ptr ) )
    {
    crnt_char_ptr++;
    length++;
    }

if( '\"' != *crnt_char_ptr )
    {
    // Invalid input
    context->state = PARSE_STATE_ERROR;
    return 0;
    }

// Allocate space to hold the string, including the NULL terminator
*extracted_string_out = (char*)context->hooks.malloc_fn( length + 1 );
if( NULL == *extracted_string_out )
    {
    context->state = PARSE_STATE_ERROR;
    return 0;
    }
else
    {
    snprintf( *extracted_string_out, length + 1, "%s", &context->crnt_posn[1] );
    context->crnt_posn += length + 2;
    }

return 1;
}


/**********************************************************
*	skip_whitespace
*
*   Utility function that returns a pointer to the first
*   non-whitespace character encountered in the provided
*   string or the null-terminiator if the entire string is
*   whitespace.
*
**********************************************************/
static const char * skip_whitespace
    (
    const char * string_in
    )
{
int i;

if( NULL == string_in )
    {
    return NULL;
    }

for( i = 0; ( string_in[i] != '\0' ) && ( isspace( string_in[i] ) ); i++ )
    ;

return &string_in[i];
}