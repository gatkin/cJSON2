#ifndef cJSON2_h
#define cJSON2_h

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
} cJSON_ValueType;


typedef struct cJSON {
   struct cJSON * prev;
   struct cJSON * next;
   struct cJSON * parent;
   struct cJSON * child; 
   
   cJSON_ValueType type;
   
   char *   valuestring;
   int      valueint;
   double   valuedouble;
   
   char * string;
} cJSON;

typedef struct cJSON_Hooks {
      void *(*malloc_fn)(size_t size);
      void *(*realloc_fn)(void *ptr, size_t size);
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

cJSON * cJSON_GetArrayItem
    (
    cJSON const *   json_array,
    int             index
    );

int cJSON_GetArraySize
    (
    cJSON const *   json_array
    );

cJSON * cJSON_GetObjectItem
    (
    cJSON const *   json_object,
    char const *    key
    );

int cJSON_ObjectHasItem
    (
    cJSON const *   json_object,
    char const *    key
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

char * cJSON_Print
    (
    cJSON const * json
    );

char * cJSON_PrintWithHooks
    (
    cJSON const *       json,
    cJSON_Hooks const * hooks
    );

#ifdef __cplusplus
}
#endif

#endif
