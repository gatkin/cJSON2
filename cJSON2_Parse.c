#include <ctype.h>
#include <errno.h>
#include <math.h>
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

static int __inline parent_node_is_array
    (
    cJSON const * node
    );

static int __inline parent_node_is_object
    (
    cJSON const * node
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
*	cJSON_Delete
*
*	Clean up resources owned by a cJSON object.
*
**********************************************************/
void cJSON_Delete
    (
    cJSON * json
    )
{
cJSON_Hooks default_hooks;

default_hooks.free_fn   = free;
default_hooks.malloc_fn = malloc;

cJSON_DeleteWithHooks( json, &default_hooks );
}


/**********************************************************
*	cJSON_DeleteWithHooks
*
*	Clean up resources owned by a cJSON object using the
*   provided hooks.
*
**********************************************************/
void cJSON_DeleteWithHooks
    (
    cJSON *             json,
    cJSON_Hooks const * hooks
    )
{
cJSON * crnt_node;
cJSON * next_node;

crnt_node = json;

while( NULL != crnt_node )
    {
    // First delete all of a node's children before deleting the node
    if( NULL != crnt_node->child )
        {
        next_node = crnt_node->child;
        }
    else
        {
        // Move on to this node's sibling first.
        if( NULL != crnt_node->next )
            {
            next_node = crnt_node->next;
            }
        else
            {
            // Finally move back to this node's parent.
            next_node = crnt_node->parent;

            // Set the parent's child pointer to NULL so it can know that all
            // of its children have been clean up.
            if( NULL != next_node )
                {
                next_node->child = NULL;
                }
            }

        // Safe to completely free the whole node now.
        hooks->free_fn( crnt_node->string );
        hooks->free_fn( crnt_node->valuestring );
        hooks->free_fn( crnt_node );
        }

    crnt_node = next_node;
    }

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
*	cJSON_GetObjectItem
*
*	Looks up an item in an object with the provided key. If
*	no item with the key is found, or if the provided JSON
*	is not an object, this returns NULL.
*
**********************************************************/
cJSON * cJSON_GetObjectItem
    (
    cJSON const *   json_object,
    char const *    key
    )
{
cJSON * found_item;
cJSON * crnt_item;

found_item = NULL;

if( NULL == json_object )
    {
    return NULL;
    }
else if( cJSON_Object != json_object->type )
    {
    return NULL;
    }
else if( NULL == key )
    {
    return NULL;
    }

crnt_item = json_object->child;
while( ( NULL != crnt_item ) && ( NULL == found_item ) )
    {
    if( 0 == strcmp( key, crnt_item->string ) )
        {
        found_item = crnt_item;
        }
    else
        {
        crnt_item = crnt_item->next;
        }
    }

return found_item;
}


/**********************************************************
*	cJSON_ObjectHasItem
*
*	Returns 1 if the provided object contains an item with
*	the provided key, 0 otherwise.
*
**********************************************************/
int cJSON_ObjectHasItem
    (
    cJSON const *   json_object,
    char const *    key
    )
{
return ( NULL != cJSON_GetObjectItem( json_object, key ) );
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

if( ( ']' == context->crnt_posn[0] ) && ( parent_node_is_array( context->crnt_node ) ) )
    {
    // We've come to the end of an array. Move past the ']'.
    context->crnt_posn++;

    // Step up a level back to the containing array.
    context->crnt_node = context->crnt_node->parent;
    next_parse_state( context );
    }
else if( ( ',' ==  context->crnt_posn[0] ) && ( parent_node_is_array( context->crnt_node ) ) )
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

if( ( '}' == context->crnt_posn[0] ) && ( parent_node_is_object( context->crnt_node ) ) )
    {
    // We've reached the end of an object. Move past the closing '}'.
    context->crnt_posn++;

    // Step up one level to the containing object.
    context->crnt_node = context->crnt_node->parent;
    next_parse_state( context );
    }
else if( ( ',' == context->crnt_posn[0] ) && ( parent_node_is_object( context->crnt_node ) ) )
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
*	parent_node_is_array
*
*	Returns 1 if the provided node's parent is an array.
*
**********************************************************/
static int __inline parent_node_is_array
    (
    cJSON const * node
    )
{
return ( ( NULL != node->parent) && ( cJSON_Array == node->parent->type ) );
}



/**********************************************************
*	parent_node_is_array
*
*	Returns 1 if the provided node's parent is an object.
*
**********************************************************/
static int __inline parent_node_is_object
    (
    cJSON const * node
    )
{
return ( ( NULL != node->parent) && ( cJSON_Object == node->parent->type ) );
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