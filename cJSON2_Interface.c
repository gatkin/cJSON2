/*
 * Contains publicly-scoped interface functions.
 */

#include <string.h>

#include "cJSON2.h"

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