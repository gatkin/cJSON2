/*
 * Contains declarations for private cJSON utility functions and should NOT be included by client code
 */

#ifndef cJSON2_private_h
#define cJSON2_private_h

#ifdef __cplusplus
extern "C"
{
#endif


#include "cJSON2.h"

int parent_node_is_array
    (
    cJSON const * node
    );

int parent_node_is_object
    (
    cJSON const * node
    );


#ifdef __cplusplus
}
#endif

#endif