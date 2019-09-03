#pragma once

#include <stdint.h>
// TODO(Leonardo): move these!
#include <windows.h>
#include <intrin.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float r32;
typedef double r64;

typedef i32 b32;

typedef size_t memory_index;
typedef uintptr_t unm;
typedef intptr_t snm;


union Vec2
{
    struct
    {
        r32 x;
        r32 y;
    };
    
    r32 E[2];
    
    inline Vec2& operator *= ( r32 c );
    inline Vec2& operator += ( Vec2 v );
    inline Vec2& Vec2::operator -= ( Vec2 v );
};

union Vec3
{
    struct
    {
        r32 x;
        r32 y;
        r32 z;
    };
    
    struct
    {
        r32 r;
        r32 g;
        r32 b;
    };
    
    struct
    {
        Vec2 xy;
        r32 ignored;
    };
    
    struct
    {
        r32 ignored;
        Vec2 yz;
    };
    
    r32 E[3];
    
    inline Vec3& operator *= ( r32 c );
    inline Vec3& operator += ( Vec3 v );
    inline Vec3& Vec3::operator -= ( Vec3 v );
};

union Vec4
{
    struct
    {
        r32 x;
        r32 y;
        r32 z;
        r32 w;
    };
    
    struct
    {
        union
        {
            Vec3 rgb;
            struct
            {
                r32 r;
                r32 g;
                r32 b;
            };
        };
        r32 a;
    };
    
    struct
    {
        Vec3 xyz;
        r32 ignoredw;
    };
    
    struct
    {
        Vec2 xy;
        r32 ignored_0;
        r32 ignored_1;
    };
    
    struct
    {
        r32 ignored_2;
        Vec2 yz;
        r32 ignored_3;
    };
    
    struct
    {
        r32 ignored_4;
        r32 ignored_5;
        Vec2 zw;
    };
    
    r32 E[4];
    
    inline Vec4& operator *= ( r32 c );
    inline Vec4& operator += ( Vec4 v );
    inline Vec4& Vec4::operator -= ( Vec4 v );
};



#if FORGIVENESS_SLOW
#define Assert( expression ) if(!(expression)) {*((int*) 0) = 0;}
#else
#define Assert( expression )
#endif

#define InvalidCodePath Assert(!"noooo");
#define InvalidDefaultCase default: { InvalidCodePath; }

#if FORGIVENESS_INTERNAL
#define NotImplemented InvalidCodePath
#else
#define NotImplemented NotImplemented_ImplementIt
#endif


#define introspection(...)
#define printTable(...)
#define printFlags(...)
#define printLabels(...)
#define MetaDefault(...)

#include <float.h>
#include <limits.h>
#include <stdio.h>

#if !defined(internal)
#define internal static
#endif
#define global_variable static
#define local_persist static

#define U32FromPointer(pointer) ((u32) (memory_index) (pointer))
#define PointerFromU32(type, value) (type*) ((memory_index) value)
#define OffsetOf(type, name) (u32) (&((type*) 0)->name)

introspection() struct Rect2
{
    Vec2 min;
    Vec2 max;
};

introspection() struct Rect3
{
    Vec3 min;
    Vec3 max;
};

struct m4x4
{
    // NOTE(Leonardo): stored as row-major! E[ROW][COLUMS] gives E[ROW][COLUMN]
    r32 E[4][4];
};

#define PI32 3.14f
#define TAU32 6.28f

#define KiloBytes(value) ((value) * 1024)
#define MegaBytes(value) ((value) * 1024 * 1024)
#define GigaBytes(value) ((value) * 1024 * 1024 * 1024)
#define TeraBytes(value) ((value) * 1024 * 1024 * 1024 * 1024)

#define Max(A , B) (((A) > (B)) ? (A) : (B))
#define Min(A , B) (((A) < (B)) ? (A) : (B))

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))

#define AlignPow2(value, n)  (((value)  + ((n) - 1)) & ~((n) - 1))  
#define Align16(value) AlignPow2(value, 16)
#define Align8(value) AlignPow2(value, 8)
#define Align4(value) AlignPow2(value, 4)

#define R32_MAX FLT_MAX
#define R32_MIN -FLT_MAX
#define I32_MAX INT_MAX 
#define I32_MIN INT_MIN
#define U32_MAX UINT_MAX
#define U16_MAX 0xffff
#define I16_MAX SHRT_MAX
#define I16_MIN SHRT_MIN

//
//
//
#define DLLIST_INSERT(sentinel, element) \
(element)->next = (sentinel)->next; \
(element)->prev = (sentinel); \
(element)->prev->next = (element); \
(element)->next->prev = (element);

#define DLLIST_REMOVE(element) \
if((element)->next)\
{\
    (element)->prev->next = (element)->next; \
    (element)->next->prev = (element)->prev;\
    (element)->next = 0;\
    (element)->prev = 0;\
}

#define DLLIST_ISEMPTY(sentinel) ((sentinel)->next == (sentinel))

#define DLLIST_INSERT_AS_LAST(sentinel, element) \
(element)->next = (sentinel); \
(element)->prev = (sentinel)->prev; \
(element)->prev->next = (element); \
(element)->next->prev = (element);


#define DLLIST_INIT(sentinel) \
(sentinel)->next = (sentinel);\
(sentinel)->prev = (sentinel);

#define FREELIST_ALLOC(result, firstFreePtr, allocationCode) if((result) = (firstFreePtr)) { (firstFreePtr) = (result)->nextFree; } else{ (result) = allocationCode; } Assert((result) != (firstFreePtr)); 
#define FREELIST_DEALLOC(result, firstFreePtr) Assert((result) != (firstFreePtr)); if((result)) { (result)->nextFree = (firstFreePtr); (firstFreePtr) = (result); }
#define FREELIST_INSERT(newFirst, firstPtr) Assert((firstPtr) != (newFirst)); (newFirst)->next = (firstPtr); (firstPtr) = (newFirst);

#define FREELIST_INSERT_AS_LAST(newLast, firstPtr, lastPtr) \
if(lastPtr){(lastPtr)->next = newLast; lastPtr = newLast;} else{ (firstPtr) = (lastPtr) = dest; }


#define FREELIST_FREE(listPtr, type, firstFreePtr) for(type* element = (listPtr); element;) { Assert(element != element->next); type* nextElement = element->next; FREELIST_DEALLOC(element, firstFreePtr); element = nextElement; } (listPtr) = 0;
#define FREELIST_COPY(destList, type, toCopy, firstFree, pool, ...){ type* currentElement_ = (toCopy); while(currentElement_) { type* elementToCopy_; FREELIST_ALLOC(elementToCopy_, (firstFree), PushStruct((pool), type)); ##__VA_ARGS__; *elementToCopy_ = *currentElement_; FREELIST_INSERT(elementToCopy_, (destList)); currentElement_ = currentElement_->next; } }
//
//
//

#define ZeroStruct(s) ZeroSize(sizeof(s), &(s))
#define ZeroArray(s, count) ZeroSize(count * sizeof(s), &(s))
inline void ZeroSize(u64 size, void* memory)
{
    u8* dest = (u8*) memory;
    while(size--)
    {
        *dest++ = 0;
    }
}

inline void* Copy(u64 size, void* destPointer, void* sourcePointer)
{
    u8* dest = (u8*) destPointer;
    u8* source = (u8*) sourcePointer;
    while(size--)
    {
        *dest++ = *source++;
    }
    return destPointer;
}

struct TicketMutex
{
    u64 volatile ticket;
    u64 volatile serving;
};

#if _MSC_VER
inline u32 AtomicCompareExchangeUint32(u32* toCheck, u32 newValue, u32 oldValue)
{
    u32 result = InterlockedCompareExchange((LONG volatile*) toCheck, newValue, oldValue);
    return result;
}

inline void BusyWait(u32* lock)
{
    while(AtomicCompareExchangeUint32(lock, 1, 0) == 1){}
}

inline u32 AtomicIncrementU32(u32 volatile* value, u32 addend)
{
    // NOTE(Leonardo): return the value that was there _Before_ the add!
    u32 result = (InterlockedExchangeAdd((long*) value, addend));
    return result;
}

inline u64 AtomicIncrementU64(u64 volatile* value, u64 addend)
{
    // NOTE(Leonardo): return the value that was there _Before_ the add!
    u64 result = (InterlockedExchangeAdd64((__int64*) value, addend));
    return result;
}

inline u64 AtomicExchangeU64(u64 volatile* value, u64 changeTo )
{
    u64 result = InterlockedExchange64((__int64*) value, changeTo);
    return result;
}

inline u32 GetThreadID()
{
    u8* threadLocalStorage = (u8*) __readgsqword(0x30);
    u32 threadID = *(u32*) (threadLocalStorage + 0x48);
    return threadID;
}
#define CompletePastWritesBeforeFutureWrites _WriteBarrier(); _mm_sfence(); 
#define CompletePastReadsBeforeFutureReads _ReadBarrier(); 

#elif COMPILER_LLVM
// TODO(Leonardo): intrinsics for other compilers!
#else
#endif

inline void BeginTicketMutex(TicketMutex* mutex)
{
    u64 ticket = AtomicIncrementU64(&mutex->ticket, 1);
    while(ticket != mutex->serving);{_mm_pause();}
}

inline void EndTicketMutex(TicketMutex* mutex)
{
    AtomicIncrementU64(&mutex->serving, 1);
}
