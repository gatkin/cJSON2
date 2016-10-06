#include <string.h>

#include "cJSON2.h"


/****************************************
Private Types
****************************************/
typedef enum
    {
    SERIALIZE_STATE_VALUE,
    SERIALIZE_STATE_ARRAY,
    SERIALIZE_STATE_OBJECT,
    SERIALIZE_STATE_ERROR,
    } serialize_state;

typedef struct
    {
    char *          buffer;
    int             buffer_len;
    int             buffer_posn;
    cJSON_Hooks     hooks;
    cJSON const *   crnt_node;
    serialize_state state;
    } serialize_context;


/****************************************
Private Function Declarations
****************************************/
static void serialize
    (
    serialize_context * context
    );

static void serialize_context_init
    (
    serialize_context * context
    );

static void serialize_value
    (
    serialize_context * context
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

default_hooks.free_fn   = free;
default_hooks.malloc_fn = malloc;

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

context.hooks.malloc_fn = hooks->malloc_fn;
context.hooks.free_fn   = hooks->free_fn;
context.crnt_node       = json;
context.state           = SERIALIZE_STATE_VALUE;

serialize( &context );

if( SERIALIZE_STATE_ERROR == context.state )
    {
    // Clean up if an error occurred.
    free( context.buffer );
    }
else
    {
    serialized_json = context.buffer;
    }

return serialized_json;
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


}