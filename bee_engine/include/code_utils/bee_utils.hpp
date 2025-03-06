#pragma once
// This file is included by the precompiled header,
// but was kept as separate for including in header files

//Delete Macros

#define NON_COPYABLE(name)                      \
	name(const name& other) = delete;           \
	name& operator=(const name& other) = delete \

#define NON_MOVABLE(name)                       \
	name(name&& other) = delete;                \
	name& operator=(name&& other) = delete      \

//Assert macros

#if defined(BEE_DEBUG) && defined(BEE_PLATFORM_PC)

#include <cassert>
#define BEE_ASSERT(cond) assert(cond)

#elif defined(BEE_DEBUG)

#include <agc.h>
#define BEE_ASSERT(cond) SCE_AGC_ASSERT(cond)

#else

#define BEE_ASSERT(cond)

#endif // BEE_DEBUG

//Debug only

#if defined(BEE_DEBUG)
#define BEE_DEBUG_ONLY(expr) expr
#else
#define BEE_DEBUG_ONLY(expr)
#endif