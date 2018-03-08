/*
  Copyright (c) 2009-2017 Dave Gamble and cJSON contributors

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef cJSON__h
#define cJSON__h

#ifdef __cplusplus
extern "C"
{
#endif

/* project version */
#define CJSON_VERSION_MAJOR 1
#define CJSON_VERSION_MINOR 5
#define CJSON_VERSION_PATCH 0

#include <stddef.h>
#include <stdint.h>
/* cJSON Types: */
#define cJSON_Invalid 0
#define cJSON_False   1
#define cJSON_True    2
#define cJSON_NULL    4
#define cJSON_Number  8
#define cJSON_String  16
#define cJSON_Array   32
#define cJSON_Object  64
#define cJSON_Raw     128

#define cJSON_IsReference 256
#define cJSON_StringIsConst 512

/* The cJSON structure: */
typedef struct cJSON
{
    /* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
    struct cJSON*next;
    struct cJSON*prev;
    /* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
    struct cJSON*child;

    /* The type of the item, as above. */
    int type;

    /* The item's string, if type==cJSON_String  and type == cJSON_Raw */
    char *valuestring;
    /* writing to valueint is DEPRECATED, use cJSON_SetNumberValue instead */
    int64_t valueint;
    /* The item's number, if type==cJSON_Number */
    double valuedouble;

    /* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
    char *string;
} cJSON;

typedef struct cJSON_Hooks
{
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} cJSON_Hooks;

typedef int cJSON_bool;



/* Limits how deeply nested arrays/objects can be before cJSON rejects to parse them.
 * This is to prevent stack overflows. */
#ifndef CJSON_NESTING_LIMIT
#define CJSON_NESTING_LIMIT 1000
#endif

/* returns the version of cJSON as a string */
const char* cJSON_Version(void);

/* Supply malloc, realloc and free functions to cJSON*/
void cJSON_InitHooks(cJSON_Hooks* hooks);

/* Memory Management: the caller is always responsible to free the results from all variants of cJSON_Parse (with cJSON_Delete) and cJSON_Print (with stdlib free, cJSON_Hooks.free_fn, or cJSON_free as appropriate). The exception is cJSON_PrintPreallocated, where the caller has full responsibility of the buffer. */
/* Supply a block of JSON, and this returns a cJSON object you can interrogate. */
cJSON* cJSON_Parse(const char *value);
/* Render a cJSON entity to text for transfer/storage. */
char* cJSON_Print(const cJSON*item);
/* Render a cJSON entity to text for transfer/storage without any formatting. */
char* cJSON_PrintUnformatted(const cJSON*item);
/* Render a cJSON entity to text using a buffered strategy. prebuffer is a guess at the final size. guessing well reduces reallocation. fmt=0 gives unformatted, =1 gives formatted */
char* cJSON_PrintBuffered(const cJSON*item, int prebuffer, cJSON_bool fmt);
/* Render a cJSON entity to text using a buffer already allocated in memory with given length. Returns 1 on success and 0 on failure. */
/* NOTE: cJSON is not always 100% accurate in estimating how much memory it will use, so to be safe allocate 5 bytes more than you actually need */
cJSON_bool cJSON_PrintPreallocated(cJSON*item, char *buffer, const int length, const cJSON_bool format);
/* Delete a cJSON entity and all subentities. */
void cJSON_Delete(cJSON*c);

/* Returns the number of items in an array (or object). */
int cJSON_GetArraySize(const cJSON*array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
cJSON* cJSON_GetArrayItem(const cJSON*array, int index);
/* Get item "string" from object. Case insensitive. */
cJSON* cJSON_GetObjectItem(const cJSON* const object, const char * const string);
cJSON* cJSON_GetObjectItemCaseSensitive(const cJSON* const object, const char * const string);
cJSON_bool cJSON_HasObjectItem(const cJSON*object, const char *string);
/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when cJSON_Parse() returns 0. 0 when cJSON_Parse() succeeds. */
const char* cJSON_GetErrorPtr(void);

/* These functions check the type of an item */
cJSON_bool cJSON_IsInvalid(const cJSON* const item);
cJSON_bool cJSON_IsFalse(const cJSON* const item);
cJSON_bool cJSON_IsTrue(const cJSON* const item);
cJSON_bool cJSON_IsBool(const cJSON* const item);
cJSON_bool cJSON_IsNull(const cJSON* const item);
cJSON_bool cJSON_IsNumber(const cJSON* const item);
cJSON_bool cJSON_IsString(const cJSON* const item);
cJSON_bool cJSON_IsArray(const cJSON* const item);
cJSON_bool cJSON_IsObject(const cJSON* const item);
cJSON_bool cJSON_IsRaw(const cJSON* const item);

/* These calls create a cJSON item of the appropriate type. */
cJSON* cJSON_CreateNull(void);
cJSON* cJSON_CreateTrue(void);
cJSON* cJSON_CreateFalse(void);
cJSON* cJSON_CreateBool(cJSON_bool boolean);
cJSON* cJSON_CreateNumber(double num);
cJSON* cJSON_CreateString(const char *string);
/* raw json */
cJSON* cJSON_CreateRaw(const char *raw);
cJSON* cJSON_CreateArray(void);
cJSON* cJSON_CreateObject(void);

/* These utilities create an Array of count items. */
cJSON* cJSON_CreateIntArray(const int *numbers, int count);
cJSON* cJSON_CreateFloatArray(const float *numbers, int count);
cJSON* cJSON_CreateDoubleArray(const double *numbers, int count);
cJSON* cJSON_CreateStringArray(const char **strings, int count);

/* Append item to the specified array/object. */
void cJSON_AddItemToArray(cJSON*array, cJSON*item);
void cJSON_AddItemToObject(cJSON*object, const char *string, cJSON*item);
/* Use this when string is definitely const (i.e. a literal, or as good as), and will definitely survive the cJSON object.
 * WARNING: When this function was used, make sure to always check that (item->type & cJSON_StringIsConst) is zero before
 * writing to `item->string` */
void cJSON_AddItemToObjectCS(cJSON*object, const char *string, cJSON*item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing cJSON to a new cJSON, but don't want to corrupt your existing cJSON. */
void cJSON_AddItemReferenceToArray(cJSON*array, cJSON*item);
void cJSON_AddItemReferenceToObject(cJSON*object, const char *string, cJSON*item);

/* Remove/Detatch items from Arrays/Objects. */
cJSON* cJSON_DetachItemViaPointer(cJSON*parent, cJSON* const item);
cJSON* cJSON_DetachItemFromArray(cJSON*array, int which);
void cJSON_DeleteItemFromArray(cJSON*array, int which);
cJSON* cJSON_DetachItemFromObject(cJSON*object, const char *string);
cJSON* cJSON_DetachItemFromObjectCaseSensitive(cJSON*object, const char *string);
void cJSON_DeleteItemFromObject(cJSON*object, const char *string);
void cJSON_DeleteItemFromObjectCaseSensitive(cJSON*object, const char *string);

/* Update array items. */
void cJSON_InsertItemInArray(cJSON*array, int which, cJSON*newitem); /* Shifts pre-existing items to the right. */
cJSON_bool cJSON_ReplaceItemViaPointer(cJSON* const parent, cJSON* const item, cJSON* replacement);
void cJSON_ReplaceItemInArray(cJSON*array, int which, cJSON*newitem);
void cJSON_ReplaceItemInObject(cJSON*object,const char *string,cJSON*newitem);
void cJSON_ReplaceItemInObjectCaseSensitive(cJSON*object,const char *string,cJSON*newitem);

/* Duplicate a cJSON item */
cJSON* cJSON_Duplicate(const cJSON*item, cJSON_bool recurse);
/* Duplicate will create a new, identical cJSON item to the one you pass, in new memory that will
need to be released. With recurse!=0, it will duplicate any children connected to the item.
The item->next and ->prev pointers are always zero on return from Duplicate. */
/* Recursively compare two cJSON items for equality. If either a or b is NULL or invalid, they will be considered unequal.
 * case_sensitive determines if object keys are treated case sensitive (1) or case insensitive (0) */
cJSON_bool cJSON_Compare(const cJSON* const a, const cJSON* const b, const cJSON_bool case_sensitive);


/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
/* If you supply a ptr in return_parse_end and parsing fails, then return_parse_end will contain a pointer to the error. If not, then cJSON_GetErrorPtr() does the job. */
cJSON* cJSON_ParseWithOpts(const char *value, const char **return_parse_end, cJSON_bool require_null_terminated);

void cJSON_Minify(char *json);

/* Macros for creating things quickly. */
#define cJSON_AddNullToObject(object,name) cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object,name) cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object,name) cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddBoolToObject(object,name,b) cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define cJSON_AddNumberToObject(object,name,n) cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
#define cJSON_AddStringToObject(object,name,s) cJSON_AddItemToObject(object, name, cJSON_CreateString(s))
#define cJSON_AddRawToObject(object,name,s) cJSON_AddItemToObject(object, name, cJSON_CreateRaw(s))

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define cJSON_SetIntValue(object, number) ((object) ? (object)->valueint = (object)->valuedouble = (number) : (number))
/* helper for the cJSON_SetNumberValue macro */
double cJSON_SetNumberHelper(cJSON*object, double number);
#define cJSON_SetNumberValue(object, number) ((object != NULL) ? cJSON_SetNumberHelper(object, (double)number) : (number))

/* Macro for iterating over an array */
#define cJSON_ArrayForEach(element, array) for(element = (array != NULL) ? (array)->child : NULL; element != NULL; element = element->next)

/* malloc/free objects using the malloc/free functions that have been set with cJSON_InitHooks */
void * cJSON_malloc(size_t size);
void cJSON_free(void *object);

#ifdef __cplusplus
}
#endif

#endif
