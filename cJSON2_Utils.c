
#include "cJSON2_private.h"


/**********************************************************
*	parent_node_is_array
*
*	Returns 1 if the provided node's parent is an array.
*
**********************************************************/
int parent_node_is_array
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
int parent_node_is_object
    (
    cJSON const * node
    )
{
return ( ( NULL != node->parent) && ( cJSON_Object == node->parent->type ) );
}