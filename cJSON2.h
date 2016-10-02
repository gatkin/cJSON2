#ifndef cJSON_h
#define cJSON_h

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>

/****************************************
Types
****************************************/
typedef enum {
    cJSON_False,
    cJSON_True,
    cJSON_Null,
    cJSON_Number,
    cJSON_String,
    cJSON_Array,
    cJSON_Object
} JSON_ValueType;


typedef struct cJSON {
   struct cJSON * prev;
   struct cJSON * next;
   struct cJSON * parent;
   struct cJSON * child; 
   
   JSON_ValueType type;
   
   char *   valuestring;
   int      valueint;
   double   valuedouble;
   
   char * string;
} cJSON;

typedef struct cJSON_Hooks {
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} cJSON_Hooks;


/****************************************
Functions
****************************************/
void cJSON_Delete
    (
    cJSON * json
    );

void cJSON_DeleteWithHooks
    (
    cJSON *             json,
    cJSON_Hooks const * hooks
    );

cJSON * cJSON_Parse
    (
    char const *        json_str
    );

cJSON * cJSON_ParseWithHooks
    (
    char const *        json_str,
    cJSON_Hooks const * hooks
    );

#ifdef __cplusplus
}
#endif

#endif