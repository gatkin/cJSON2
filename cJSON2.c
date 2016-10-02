#include <stdio.h>
#include <string.h>

#include "cJSON2.h"

/****************************************
Private Types
****************************************/
typedef enum
    {
    PARSE_STATE_VALUE,
    PARSE_STATE_NEXT_ARRAY_VALUE,
    PARSE_STATE_OBJECT,
    PARSE_STATE_NEXT_OBJECT_VALUE,
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
Private Helper Macros
****************************************/
#define node_is_array( _node ) ( ( NULL != _node ) && ( cJSON_Array == _node->type ) )

#define is_number( _crnt_char ) ( ( _crnt_char == '-' ) || ( ( '0' <= _crnt_char ) && ( _crnt_char <= '9' ) ) )

/****************************************
Private Function Declarations
****************************************/
static void context_init
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

static void parse_number
    (
    parse_context * context
    );

static void parse_object
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

static const char * skip_whitespace
    (
    const char * string_in
    );


/****************************************
Public Functions
****************************************/

/**********************************************************
*	cJSON_Delete
*
*	Clean up resouces owned by a cJSON object.
*
**********************************************************/
void cJSON_Delete
    (
    cJSON * json
    )
{
// TODO: Actually implemet. This currently leaks resources.
free( json );
}


/**********************************************************
*	cJSON_DeleteWithHooks
*
*	Clean up resouces owned by a cJSON object using the
*   provided hooks.
*
**********************************************************/
void cJSON_DeleteWithHooks
    (
    cJSON *             json,
    cJSON_Hooks const * hooks
    )
{
// TODO:
hooks->free_fn( json );
}

/**********************************************************
*	cJSON_GetArrayItem
*
*	Returns the item at the specified index in the provided
*   array. Returns NULL if the provided object is not an array
*   or the index exceeds the bounds of the array.
*
**********************************************************/
cJSON * cJSON_GetArrayItem
    (
    cJSON const *   json_array,
    int             index
    )
{
int     crnt_idx;
cJSON * node;

node     = NULL;
crnt_idx = 0;

if( index < 0 )
    {
    return NULL;
    }
if( NULL == json_array )
    {
    return NULL;
    }
else if ( cJSON_Array != json_array->type )
    {
    return NULL;
    }

node = json_array->child;
while( ( NULL != node ) && ( crnt_idx < index ) )
    {
    node = node->next;
    crnt_idx++;
    }

if( crnt_idx != index )
    {
    // The provided index exceeded the bounds of the array
    node = NULL;
    }

return node;
}

/**********************************************************
*	cJSON_GetArraySize
*
*	Returns the size of the provided array, -1 if the provided
*   object is not an array.
*
**********************************************************/
int cJSON_GetArraySize
    (
    cJSON const *   json_array
    )
{
int     size;
cJSON * crnt_node;

size = 0;

if( NULL == json_array )
    {
    return -1;
    }
else if( cJSON_Array != json_array->type )
    {
    return -1;
    }

crnt_node = json_array->child;

while( NULL != crnt_node )
    {
    size++;
    crnt_node = crnt_node->next;
    }

return size;
}


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
parse_context   context;
int             success;

context_init( &context );

context.json_str  = json_str;
context.crnt_posn = &json_str[0];
context.state     = PARSE_STATE_VALUE;

context.hooks.malloc_fn = hooks->malloc_fn;
context.hooks.free_fn   = hooks->free_fn;

context.root      = new_node( &context.hooks );
context.crnt_node = context.root;
success = ( NULL != context.root );

if( success )
    {
    parse( &context );
    }

return context.root;
}


/**********************************************************
*	context_init
*
*	Initialize parse context.
*
**********************************************************/
static void context_init
    (
    parse_context * context
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
cJSON * next_array_value;

context->crnt_posn = skip_whitespace( context->crnt_posn );

if( ( ']' == context->crnt_posn[0] ) && ( node_is_array( context->crnt_node->parent ) ) )
    {
    // We've come to the end of an array. Move past the ']'.
    context->crnt_posn++;

    // Step up a level back to the containing array.
    context->crnt_node = context->crnt_node->parent;
    next_parse_state( context );
    }
else if( ( ',' ==  context->crnt_posn[0] ) && ( node_is_array( context->crnt_node->parent ) ) )
    {
    // We found another value in the array, move past the ','
    context->crnt_posn++;

    next_array_value = new_node( &context->hooks );
    if( NULL == next_array_value )
        {
        context->state = PARSE_STATE_ERROR;
        }
    else
        {
        next_array_value->parent = context->crnt_node->parent;
        next_array_value->prev   = context->crnt_node;
        context->crnt_node->next = next_array_value;
        context->crnt_node       = next_array_value;
        context->state           = PARSE_STATE_VALUE;
        }
    }
else if( ( cJSON_Array == context->crnt_node->type ) && ( NULL == context->crnt_node->child ) )
    {
    // We've come to the first value in an array
    next_array_value = new_node( &context->hooks );
    if( NULL == next_array_value )
        {
        context->state = PARSE_STATE_ERROR;
        }
    else
        {
        // Step down a level to parse all values in the array.
        next_array_value->parent  = context->crnt_node;
        context->crnt_node->child = next_array_value;
        context->crnt_node        = next_array_value;
        context->state            = PARSE_STATE_VALUE;
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
// TODO:
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
    // Shouldn't get here.
    }
else if( NULL == context->crnt_node->parent )
    {
    context->state = PARSE_STATE_COMPLETE;
    }
else if( cJSON_Array == context->crnt_node->parent->type )
    {
    context->state = PARSE_STATE_NEXT_ARRAY_VALUE;
    }
else if( cJSON_Object == context->crnt_node->parent->type )
    {
    context->state = PARSE_STATE_NEXT_OBJECT_VALUE;
    }
else
    {
    // Shouldn't get here.
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

// Check whether this is an empty array.
context->crnt_posn = skip_whitespace( context->crnt_posn );

if( ']' == context->crnt_posn[0] )
    {
    // Move past the closing bracket of this empty array
    context->crnt_posn++;
    next_parse_state( context );
    }
else
    {
    // This array contains values
    context->state = PARSE_STATE_NEXT_ARRAY_VALUE;
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
else if( '\"' == context->crnt_posn[0] )
    {
    parse_string( context );
    }
else if( is_number( context->crnt_posn[0] ) )
    {
    parse_number( context );
    }
else if( '[' == context->crnt_posn[0] )
    {
    parse_array( context );
    }
else if( '{' == context->crnt_posn[0] )
    {
    parse_object( context );
    }
else
    {
    // Invalid input
    context->state = PARSE_STATE_ERROR;
    }
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
// TODO
context->crnt_node->type = cJSON_Number;
context->crnt_node->valuedouble = 1.0;
context->crnt_node->valueint = 1;
next_parse_state( context );
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

// Check whether this is an empty object
context->crnt_posn = skip_whitespace( context->crnt_posn );
if( '}' == context->crnt_posn[0] )
    {
    // Move past the closing brace of this empty object
    context->crnt_posn++;
    next_parse_state( context );
    }
else
    {
    // This object contains values.
    context->state = PARSE_STATE_NEXT_OBJECT_VALUE;
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
int             length;
char const *    crnt_char_ptr;

length = 0;
context->crnt_node->type = cJSON_String;

if( '\"' != context->crnt_posn[0] )
    {
    // Not a string
    context->state = PARSE_STATE_ERROR;
    return;
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
    return;
    }

// Allocate space to hold the string, including the NULL terminator
context->crnt_node->valuestring = (char*)context->hooks.malloc_fn( length + 1 );
if( NULL == context->crnt_node->valuestring )
    {
    context->state = PARSE_STATE_ERROR;
    return;
    }
else
    {
    snprintf( context->crnt_node->valuestring, length + 1, "%s", &context->crnt_posn[1] );
    context->crnt_posn += length + 2;
    next_parse_state( context );
    }
}


/**********************************************************
*	skip_whitespace
*
*	Utility function that returns a pointer to the first
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

for( i = 0; ( string_in[i] != '\0' ) && ( (unsigned char)string_in[i] <= ' ' ); i++ )
    ;

return &string_in[i];
}