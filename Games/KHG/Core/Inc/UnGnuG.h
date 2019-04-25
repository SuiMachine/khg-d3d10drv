/*=============================================================================
	UnGnuG.h: Unreal definitions for Gnu G++. Unfinished. Unsupported.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*----------------------------------------------------------------------------
	Hacks.
----------------------------------------------------------------------------*/

//!!
#define __INTEL_BYTE_ORDER__ 1
#define __INTEL__ 1
#undef ASM

/*
We've found a problem with your XXXClasses.h export format. You've
assumed that 'DWORD var:1;' will force DWORD alignment for the first
bitfield of a set of bits. Also that C++ will pad the fields to DWORD
width.

On gcc neither is true, gcc simply ignores the hint and packs everything
tightly. A Linux port is going to break because of this.

The simplest fix I have is to modify the Class.h format to insert
bitfield alignment statements like so:

typedef struct 
{
        int a:1;
        int :0; //align
        char b;
        int :0; //align
        int c:1;
}       test;

Basically wrap the start and end of each group of bits with 'int :0;'
The 'int :0;' syntax guarantees correct alignment, without it layout is
implementation dependent.

The alternative is explicit pad fields
-- 
Paul Shirley
*/

/*----------------------------------------------------------------------------
	Platform specifics types and defines.
----------------------------------------------------------------------------*/

// Undo any Windows defines.
#undef BYTE
#undef WORD
#undef DWORD
#undef INT
#undef FLOAT
#undef MAXBYTE
#undef MAXWORD
#undef MAXDWORD
#undef MAXINT
#undef VOID
#undef CDECL

// Make sure HANDLE is defined.
#define HANDLE DWORD
#define HINSTANCE DWORD

// Sizes.
enum {DEFAULT_ALIGNMENT = 8 }; // Default boundary to align memory allocations on.
enum {CACHE_LINE_SIZE   = 32}; // Cache line size.

// Optimization macros
//!!
#define DISABLE_OPTIMIZATION
#define ENABLE_OPTIMIZATION

// Function type macros.
//!!
#define DLL_IMPORT	/*!!*/
#define DLL_EXPORT  /*!!*/
#define VARARGS     /*!!*/
#define CDECL	    /*!!*/
#define STDCALL		/*!!*/
#define FORCEINLINE /* Force code to be inline */
#define ZEROARRAY 0 /* Zero-length arrays in structs */

// Variable arguments.
#define GET_VARARGS(msg,len,fmt) appGetVarArgs(msg,len,fmt)

// Compiler name.
#define COMPILER "Compiled with Gnu G++"

// Unsigned base types.
typedef unsigned char		BYTE;		// 8-bit  unsigned.
typedef unsigned short		_WORD;		// 16-bit unsigned.
typedef unsigned int		DWORD;		// 32-bit unsigned.
typedef unsigned long long	QWORD;		// 64-bit unsigned.

// Signed base types.
typedef	signed char			SBYTE;		// 8-bit  signed.
typedef signed short		SWORD;		// 16-bit signed.
typedef signed int  		INT;		// 32-bit signed.
typedef signed long long	SQWORD;		// 64-bit signed.

// Character types.
typedef char			    ANSICHAR;	// An ANSI character.
typedef unsigned short      UNICHAR;	// A unicode character.
typedef unsigned char		ANSICHARU;	// An ANSI character.
typedef unsigned short      UNICHARU;	// A unicode character.

// Other base types.
typedef signed int			UBOOL;		// Boolean 0 (false) or 1 (true).
typedef float				FLOAT;		// 32-bit IEEE floating point.
typedef double				DOUBLE;		// 64-bit IEEE double.
typedef unsigned int        SIZE_T;     // Corresponds to C SIZE_T.

//!!
typedef unsigned int size_t;
// Warnings to disable.
//!!

// Make sure characters are unsigned.
#ifdef __CHAR_UNSIGNED__
	#error "Bad compiler option: Characters must be signed"
#endif

// Strings.
#define LINE_TERMINATOR TEXT("\r\n")
#define PATH_SEPARATOR TEXT("\\")

// NULL.
#undef NULL
#define NULL 0

// Package implementation.
#define IMPLEMENT_PACKAGE_PLATFORM(pkgname) \
	extern "C" {HINSTANCE hInstance;}

// Platform support options.
#define PLATFORM_NEEDS_ARRAY_NEW 1
#define FORCE_ANSI_LOG           0

/*----------------------------------------------------------------------------
	Globals.
----------------------------------------------------------------------------*/

// System identification.
extern "C"
{
	extern HINSTANCE      hInstance;
	extern CORE_API UBOOL GIsMMX;
	extern CORE_API UBOOL GIsPentiumPro;
	extern CORE_API UBOOL GIsKatmai;
	extern CORE_API UBOOL GIsK6;
	extern CORE_API UBOOL GIs3DNow;
	extern CORE_API UBOOL GTimestamp;
}

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
