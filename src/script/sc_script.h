/*
===========================================================================
Copyright (C) 2008 Maurice Doison

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#ifndef _SCRIPT_SC_SCRIPT_H_
#define _SCRIPT_SC_SCRIPT_H_

#define MAX_TAG_LENGTH          16
#define MAX_FUNCTION_ARGUMENTS  16
#define MAX_NAMESPACE_DEPTH     8
#define MAX_NAMESPACE_LENGTH    16
#define MAX_PATH_LENGTH         64

#include "../game/g_local.h"

// Langages

typedef enum
{
  LANGAGE_INVALID,
  LANGAGE_C,
  LANGAGE_LUA,
  LANGAGE_PYTHON
} scLangage_t;

// Data Types

typedef enum
{
  TYPE_ANY=-1,
  TYPE_UNDEF,
  TYPE_INTEGER,
  TYPE_FLOAT,
  TYPE_STRING,
  TYPE_FUNCTION,
  TYPE_ARRAY,
  TYPE_HASH,
  TYPE_NAMESPACE,
} scDataType_t;

typedef long  scDataTypeInteger_t;
typedef float scDataTypeFloat_t;
typedef struct
{
  int   length;
  int   buflen;
  char  data;
  // following with datas...
} scDataTypeString_t;

typedef struct scDataTypeValue_s scDataTypeValue_t;
typedef struct scDataTypeArray_s scDataTypeArray_t;
typedef struct scDataTypeHash_s scDataTypeHash_t;
typedef struct scDataTypeHashEntry_s scDataTypeHashEntry_t;
typedef struct scDataTypeFunction_s scDataTypeFunction_t;
typedef struct scNamespace_s scNamespace_t;

struct scDataTypeValue_s
{
  scDataType_t    type;
  union
  {
    scDataTypeInteger_t   integer;
    scDataTypeFloat_t     floating;
    scDataTypeString_t    *string;
    scDataTypeFunction_t  *function;
    scDataTypeArray_t     *array;
    scDataTypeHash_t      *hash;
    scNamespace_t         *namespace;
  } data;
};

struct scDataTypeArray_s
{
  int                   size;
  int                   buflen;
  scDataTypeValue_t     data;
  // following with datas...
};

struct scDataTypeHash_s
{
  int                   size;
  int                   buflen;
  // TODO: a hash table should be a tree
  struct scDataTypeHashEntry_s
  {
    scDataTypeString_t  *key;
    scDataTypeValue_t   value;
  } data;
  // following with datas...
};

struct scDataTypeNamespace_s
{
  scDataTypeString_t    name[ MAX_NAMESPACE_DEPTH ];
};

struct scNamespace_s
{
  scNamespace_t         *parent;
  scDataTypeHash_t      *content;
};

typedef void (*scCRef_t)(scDataTypeValue_t*, scDataTypeValue_t*);

struct scDataTypeFunction_s
{
  scLangage_t           langage;
  scDataType_t          argument[ MAX_FUNCTION_ARGUMENTS + 1 ];
  scDataType_t          return_type;
  union
  {
    char                path[ MAX_PATH_LENGTH + 1 ];
    scCRef_t            ref;
  } data;
};

// Events data structure

typedef struct scnode_s scnode_t;
typedef struct schook_s schook_t;

struct scnode_s
{
  char                  name[ MAX_TAG_LENGTH + 1 ];
  int                   leaf;

  scnode_t              *before;
  scnode_t              *in;
  scnode_t              *after;

  scDataTypeFunction_t  *hook;

  scnode_t              *next;
  //scnode_t              *parent;
};

void SC_StringNew( scDataTypeString_t **string );
void SC_StringNewFromChar( scDataTypeString_t **string, const char* str );
void SC_Strcat( scDataTypeString_t **string, const scDataTypeString_t *src );
void SC_Strcpy( scDataTypeString_t **string, const scDataTypeString_t *src );
void SC_StringFree( scDataTypeString_t *string );

qboolean SC_ValueIsScalar( const scDataTypeValue_t *value );
void SC_ValueFree( scDataTypeValue_t *value );

void SC_ArrayNew( scDataTypeArray_t **array );
qboolean SC_ArrayGet( const scDataTypeArray_t *array, int index, scDataTypeValue_t *value );
void SC_ArraySet( scDataTypeArray_t **array, int index, scDataTypeValue_t *value );
qboolean SC_ArrayDelete( scDataTypeArray_t **array, int index );
void SC_ArrayClear( scDataTypeArray_t **array );
void SC_ArrayFree( scDataTypeArray_t *array );

void SC_HashNew( scDataTypeHash_t **hash );
qboolean SC_HashGet( const scDataTypeHash_t *hash, const char *key, scDataTypeValue_t *value );
qboolean SC_HashSet( scDataTypeHash_t **hash, const char *key, scDataTypeValue_t *value );
void SC_HashGetKeys( const scDataTypeHash_t *hash, scDataTypeArray_t **array );
qboolean SC_HashDelete( scDataTypeHash_t **hash, const char *key );
void SC_HashClear( scDataTypeHash_t **hash );
void SC_HashFree( scDataTypeHash_t *hash );

void SC_NamespaceInit( void );
qboolean SC_NamespaceGet( const char *path, scDataTypeValue_t *value );
qboolean SC_NamespaceSet( const char *path, scDataTypeValue_t *value );
qboolean SC_NamespaceDelete( const char *path );

void SC_FunctionNew( scDataTypeFunction_t **func );

// sc_main.c

void SC_Init( void );
void SC_Shutdown( void );
void SC_RunFunction( const scDataTypeFunction_t *func, scDataTypeValue_t *args, scDataTypeValue_t *ret );
int SC_RunScript( scLangage_t langage, const char *filename );
int SC_CallHooks( const char *path, gentity_t *entity );


// sc_c.c

typedef struct
{
  char                  name[ MAX_PATH_LENGTH + 1];
  scCRef_t              ref;
  scDataType_t          argument[ MAX_FUNCTION_ARGUMENTS + 1 ];
  scDataType_t          return_type;
} scLib_t;

void SC_AddLibrary( const char *namespace, scLib_t lib[] );

#endif

