#ifndef HL_TYPE_H
#define HL_TYPE_H

typedef void    HL_VOID;
typedef char    HL_BOOL;


typedef char    HL_CHAR;
typedef char    HL_S8;
typedef short   HL_S16;
typedef int     HL_S32;
typedef long    HL_S64;

typedef unsigned char   HL_U8;
typedef unsigned short  HL_U16;
typedef unsigned int    HL_U32;
typedef unsigned long long HL_U64;


typedef         HL_VOID *       HL_PVOID;
typedef         HL_CHAR *       HL_PCHAR;
typedef         HL_S8   *       HL_PS8;
typedef         HL_S16  *       HL_PS16;
typedef         HL_S32  *       HL_PS32;
typedef         HL_S64  *       HL_PS64;
typedef         HL_U8   *       HL_PU8;
typedef         HL_U16  *       HL_PU16;
typedef         HL_U32  *       HL_PU32;
typedef         HL_U64  *       HL_PU64;

#define HL_TRUE     1
#define HL_FALSE    0

#define HL_SUCCESS  0

#endif // HL_TYPE_H
