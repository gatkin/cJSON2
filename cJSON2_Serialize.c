#include <string.h>

#include "cJSON2.h"
#include "cJSON2_private.h"

#define INITIAL_BUFFER_SIZE     ( 10 )

/****************************************
Private Types
****************************************/
typedef enum
    {
    SERIALIZE_STATE_VALUE,
    SERIALIZE_STATE_NEXT_ARRAY_VALUE,
    SERIALIZE_STATE_NEXT_OBJECT_VALUE,
    SERIALIZE_STATE_OBJECT_KEY,
    SERIALIZE_STATE_ERROR,
    SERIALIZE_STATE_COMPLETE,
    } serialize_state;

typedef struct
    {
    char *          buffer;         // Note: the buffer does not contain a null-terminator until parsing completes
    int             buffer_len;
    int             buffer_posn;    // The index of the next open position in the buffer
    cJSON_Hooks     hooks;
    cJSON const *   crnt_node;
    serialize_state state;
    } serialize_context;


/****************************************
Private Function Declarations
****************************************/
static void buffer_finalize
    (
    serialize_context * context
    );

static void buffer_grow
    (
    serialize_context * context,
    int                 growth_increment
    );

static void next_array_value
    (
    serialize_context * context
    );

static void next_object_value
    (
    serialize_context * context
    );

static void next_serialize_state
    (
    serialize_context * context
    );

static void serialize
    (
    serialize_context * context
    );

static void serialize_context_init
    (
    serialize_context * context
    );

static void serialize_array
    (
    serialize_context * context
    );

static void serialize_object
    (
    serialize_context * context
    );

static void serialize_object_key
    (
    serialize_context * context
    );

static void serialize_value
    (
    serialize_context * context
    );

static int string_add_to_buffer
    (
    serialize_context * context,
    char const *        string,
    int                 string_len
    );


/**********************************************************
*	cJSON_Print
*
*	Print JSON structure. If an error occurs, this returns
*	NULL. Otherwise, it is the caller's responsibility to
*	free the returned string.
*
**********************************************************/
char * cJSON_Print
    (
    cJSON const * json
    )
{
cJSON_Hooks default_hooks;

default_hooks.malloc_fn  = malloc;
default_hooks.realloc_fn = realloc;
default_hooks.free_fn    = free;

return cJSON_PrintWithHooks( json, &default_hooks );
}


/**********************************************************
*	cJSON_PrintWithHooks
*
*	Print JSON structure with the provided hooks. If an error
*	occurs, this returns NULL. Otherwise, it is the caller's
*	responsibility to free the returned string.
*
**********************************************************/
char * cJSON_PrintWithHooks
    (
    cJSON const *       json,
    cJSON_Hooks const * hooks
    )
{
char *              serialized_json;
serialize_context   context;

serialized_json = NULL;
serialize_context_init( &context );

context.hooks.malloc_fn  = hooks->malloc_fn;
context.hooks.realloc_fn = hooks->realloc_fn;
context.hooks.free_fn    = hooks->free_fn;
context.crnt_node        = json;
context.state            = SERIALIZE_STATE_VALUE;

if( NULL != json)
    {
    serialize( &context );
    serialized_json = context.buffer;
    }

return serialized_json;
}


/**********************************************************
*	buffer_finalize
*
*	Finalizes the provided context's buffer by ensuring it
*	is null-terminated and that it takes only the memory it
*	needs.
*
**********************************************************/
static void buffer_finalize
    (
    serialize_context * context
    )
{
int shrink_amount;

if( context->buffer_posn > context->buffer_len )
    {
    // Should never get here
    context->state = SERIALIZE_STATE_ERROR;
    }
else if( ( context->buffer_len == context->buffer_posn ) )
    {
    // Need one more space for the NULL terminator.
    buffer_grow( context, 1 );
    }
else if( context->buffer_len > context->buffer_posn )
    {
    // Need to shrink the buffer so that it is just large enough to hold
    // all characters plus the null-terminator.
    shrink_amount = context->buffer_len - ( context->buffer_posn + 1 );
    buffer_grow( context, -shrink_amount );
    }

// Finally, add the null-terminator
if( SERIALIZE_STATE_ERROR != context->state )
    {
    context->buffer[context->buffer_posn] = '\0';
    context->buffer_posn++;
    }
}


/**********************************************************
*	buffer_grow
*
*	Grows the provided context's buffer by the provided
 *	increment. If an error occurs, this will set the context's
*	state to SERIALIZE_STATE_ERROR and the context's buffer
*	will remain untouched.
*
**********************************************************/
static void buffer_grow
    (
    serialize_context * context,
    int                 growth_increment
    )
{
size_t  new_buffer_len;
char *  new_buffer;

new_buffer_len = context->buffer_len + growth_increment;
new_buffer = context->hooks.realloc_fn( context->buffer, new_buffer_len );

if( NULL == new_buffer )
    {
    context->state = SERIALIZE_STATE_ERROR;
    }
else
    {
    context->buffer     = new_buffer;
    context->buffer_len = new_buffer_len;
    }
}


/**********************************************************
*	next_array_value
*
*	Prepares to serialize the next value in an array
*
**********************************************************/
static void next_array_value
    (
    serialize_context * context
    )
{
if( !parent_node_is_array( context->crnt_node ) )
    {
    // We should never get here.
    context->state = SERIALIZE_STATE_ERROR;
    }
else if( NULL == context->crnt_node->next )
    {
    // We've come to the end of the array.
    string_add_to_buffer( context, "]", 1 );

    // Move back up to the containing array.
    context->crnt_node = context->crnt_node->parent;
    next_serialize_state( context );
    }
else
    {
    // More values in the array to serialize
    if( -1 != string_add_to_buffer( context, ",", 1 ) )
        {
        // Move on to serializing the next value in the array.
        context->crnt_node = context->crnt_node->next;
        context->state     = SERIALIZE_STATE_VALUE;
        }
    }
}


/**********************************************************
*	next_object_value
*
*	Sets up the provided context to serialize the next object
*	value.
*
**********************************************************/
static void next_object_value
    (
    serialize_context * context
    )
{
if( !parent_node_is_object( context->crnt_node ) )
    {
    // We should never get here.
    context->state = SERIALIZE_STATE_ERROR;
    }
else if( NULL == context->crnt_node->next )
    {
    // Done serializing the object.
    string_add_to_buffer( context, "}", 1 );

    // Move back up to the containing object.
    context->crnt_node = context->crnt_node->parent;
    next_serialize_state( context );
    }
else
    {
    // There are more values in the object to serialize.
    if( -1 != string_add_to_buffer( context, ",", 1 ) )
        {
        // Move on to serializing the next value in the object.
        context->crnt_node = context->crnt_node->next;
        context->state     = SERIALIZE_STATE_OBJECT_KEY;
        }
    }
}


/**********************************************************
*	next_serialize_state
*
*	Set the next serialize state. This should be called after
*	successfully completing parsing a value.
*
**********************************************************/
static void next_serialize_state
    (
    serialize_context * context
    )
{
if( SERIALIZE_STATE_ERROR == context->state )
    {
    // Don't touch the state, leave it as an error.
    }
else if( NULL == context->crnt_node->parent )
    {
    context->state = SERIALIZE_STATE_COMPLETE;
    }
else if( cJSON_Array == context->crnt_node->parent->type )
    {
    context->state = SERIALIZE_STATE_NEXT_ARRAY_VALUE;
    }
else if( cJSON_Object == context->crnt_node->parent->type )
    {
    context->state = SERIALIZE_STATE_NEXT_OBJECT_VALUE;
    }
else
    {
    context->state = SERIALIZE_STATE_ERROR;
    }
}


/**********************************************************
*	serialize
*
*	Top-level function to serialize an entire JSON tree.
*
**********************************************************/
static void serialize
    (
    serialize_context * context
    )
{
// Initialize the buffer to a default initial size.
context->buffer_len = INITIAL_BUFFER_SIZE;
context->buffer     = context->hooks.malloc_fn( INITIAL_BUFFER_SIZE );
if( NULL == context->buffer )
    {
    context->state = SERIALIZE_STATE_ERROR;
    }

while( ( SERIALIZE_STATE_ERROR != context->state ) && ( SERIALIZE_STATE_COMPLETE != context->state ) )
    {
    switch (context->state)
        {
        case SERIALIZE_STATE_VALUE:
            serialize_value( context );
            break;

        case SERIALIZE_STATE_OBJECT_KEY:
            serialize_object_key( context );
            break;

        case SERIALIZE_STATE_NEXT_ARRAY_VALUE:
            next_array_value( context );
            break;

        case SERIALIZE_STATE_NEXT_OBJECT_VALUE:
            next_object_value( context );
            break;

        default:
            // Shouldn't get here
            context->state = SERIALIZE_STATE_ERROR;
            break;
        }
    }

if( SERIALIZE_STATE_ERROR != context->state )
    {
    buffer_finalize( context );
    }

// Clean up on error
if( SERIALIZE_STATE_COMPLETE != context->state )
    {
    context->hooks.free_fn( context->buffer );
    context->buffer = NULL;
    }

}


/**********************************************************
*	serialize_context_init
*
*	Initializes provided print context
*
**********************************************************/
static void serialize_context_init
    (
    serialize_context *context
    )
{
context->buffer      = NULL;
context->buffer_len  = 0;
context->buffer_posn = 0;
context->crnt_node   = NULL;
context->state       = SERIALIZE_STATE_ERROR;
memset( &context->hooks, 0, sizeof( context->hooks ) );
}


/**********************************************************
*	serialize_array
*
*	Sets up the provided context to serialize an array
*
**********************************************************/
static void serialize_array
    (
    serialize_context * context
    )
{
if( NULL == context->crnt_node->child )
    {
    // This is an empty array.
    string_add_to_buffer( context, "[]", 2 );
    next_serialize_state( context );
    }
else
    {
    // This array has values.
    if( -1 != string_add_to_buffer( context, "[", 1 ) )
        {
        // Move on to serializing this array's children
        context->crnt_node = context->crnt_node->child;
        context->state = SERIALIZE_STATE_VALUE;
        }
    }
}


/**********************************************************
*	serialize_object
*
*	Sets up the provided context to serialize an array.
*
**********************************************************/
static void serialize_object
    (
    serialize_context * context
    )
{
if( NULL == context->crnt_node->child )
    {
    // This is an empty object.
    string_add_to_buffer( context, "{}", 2 );
    next_serialize_state( context );
    }
else
    {
    // This object has values
    if( -1 != string_add_to_buffer( context, "{", 1 ) )
        {
        // Move onto this object's children.
        context->crnt_node = context->crnt_node->child;
        context->state     = SERIALIZE_STATE_OBJECT_KEY;
        }
    }
}


/**********************************************************
*	serialize_object_key
*
*	Serializes an object key and prepares the provided
*	context to serialize the corresponding value.
*
**********************************************************/
static void serialize_object_key
    (
    serialize_context * context
    )
{
int success;

if( NULL == context->crnt_node->string )
    {
    // Shouldn't get here.
    context->state = SERIALIZE_STATE_ERROR;
    }
else
    {
    // Add the value's key, surrounded by quotes, to the buffer.
    success = ( -1 != string_add_to_buffer( context, "\"", 1 ) );
    success = ( success ) && ( -1 != string_add_to_buffer( context, context->crnt_node->string, strlen( context->crnt_node->string ) ) );
    success = ( success ) && ( -1 != string_add_to_buffer( context, "\"", 1 ) );
    success = ( success ) && ( -1 != string_add_to_buffer( context, ":", 1 ) );

    if( success )
        {
        context->state = SERIALIZE_STATE_VALUE;
        }
    }
}


/**********************************************************
*	serialize_value
*
*	Serializes a JSON value
*
**********************************************************/
static void serialize_value
    (
    serialize_context * context
    )
{
switch ( context->crnt_node->type )
    {
    case cJSON_True:
        string_add_to_buffer( context, "true", 4 );
        next_serialize_state( context );
        break;

    case cJSON_False:
        string_add_to_buffer(context, "false", 5 );
        next_serialize_state( context );
        break;

    case cJSON_Null:
        string_add_to_buffer( context, "null", 4 );
        next_serialize_state( context );
        break;

    case cJSON_Number:
        // TODO:
        context->state = SERIALIZE_STATE_ERROR;
        break;

    case cJSON_String:
        string_add_to_buffer( context, context->crnt_node->string, strlen( context->crnt_node->string ) );
        next_serialize_state( context );
        break;

    case cJSON_Array:
        serialize_array( context );
        break;

    case cJSON_Object:
        serialize_object( context );
        break;

    default:
        // Shouldn't get here
        context->state = SERIALIZE_STATE_ERROR;
    }
}


/**********************************************************
*	string_add_to_buffer
*
*	Adds the provided string to the end of the provided
*	serialize context's buffer. If the context's buffer
*	does not have enough room to hold the string, this will
*	reallocate the buffer to be twice as large. If an error
*	occurs attempting to reallocate the buffer, this will
*	set the provided context's state to SERIALIZE_STATE_ERROR
*	and return -1. Otherwise this will return 1 and not change
*	the provided context's state.
*
**********************************************************/
static int string_add_to_buffer
    (
    serialize_context * context,
    char const *        string,
    int                 string_len
    )
{
int rcode;

// Double the buffer if it is not large enough to hold the provided string.
if( ( context->buffer_posn + string_len ) > context->buffer_len )
    {
    buffer_grow( context, 2 * context->buffer_len );
    }

if( SERIALIZE_STATE_ERROR == context->state )
    {
    rcode = -1;
    }
else
    {
    memcpy( &context->buffer[context->buffer_posn], string, string_len );
    context->buffer_posn += string_len;
    rcode = 1;
    }

return rcode;
}
