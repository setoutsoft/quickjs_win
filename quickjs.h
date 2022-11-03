/*
 * QuickJS Javascript Engine
 *
 * Copyright (c) 2017-2020 Fabrice Bellard
 * Copyright (c) 2017-2020 Charlie Gordon
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef QUICKJS_H
#define QUICKJS_H

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include "quickjs-version.h"
#include "quickjs-api.h"
#ifdef __cplusplus
extern "C" {
#endif


#if _WIN32
#define JS_STRICT_NAN_BOXING 1
#endif

#if defined(__GNUC__) || defined(__clang__)
  #define js_likely(x)          __builtin_expect(!!(x), 1)
  #define js_unlikely(x)        __builtin_expect(!!(x), 0)
  #define js_force_inline       inline __attribute__((always_inline))
  //#define __js_printf_like(A, B)   __attribute__((format(printf, (A), (B))))
  #define __js_printf_like(A, B) /*doesn't work, why?*/
#else
  #define js_likely(x)     (x)
  #define js_unlikely(x)   (x)
  #define js_force_inline  __forceinline
  #define __js_printf_like(A, B) /* */
#endif

#define JS_BOOL int

typedef struct JSRuntime JSRuntime;
typedef struct JSContext JSContext;
typedef struct JSObject JSObject;
typedef struct JSClass JSClass;
typedef uint32_t JSClassID;
typedef uint32_t JSAtom;

#ifdef CONFIG_STORAGE
typedef enum JS_PERSISTENT_STATUS {
  JS_NOT_PERSISTENT = 0,
  JS_PERSISTENT_DORMANT = 1,
  JS_PERSISTENT_LOADED = 2,
  JS_PERSISTENT_MODIFIED = 3
} JS_PERSISTENT_STATUS;
#endif

#if INTPTR_MAX >= INT64_MAX
#define JS_PTR64
#define JS_PTR64_DEF(a) a
#else
#define JS_PTR64_DEF(a)
#endif

#ifndef JS_PTR64
#define JS_NAN_BOXING
#endif

typedef struct JSRefCountHeader {
    int ref_count;
} JSRefCountHeader;

#define JS_FLOAT64_NAN NAN

#if defined(JS_STRICT_NAN_BOXING) 

  // This schema defines strict NAN boxing for both 32 and 64 versions 

  // This is a method of storing values in the IEEE 754 double-precision
  // floating-point number. double type is 64-bit, comprised of 1 sign bit, 11
  // exponent bits and 52 mantissa bits:
  //    7         6        5        4        3        2        1        0
  // seeeeeee|eeeemmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm|mmmmmmmm
  //

  // s0000000|0000tttt|vvvvvvvv|vvvvvvvv|vvvvvvvv|vvvvvvvv|vvvvvvvv|vvvvvvvv
  // NaN marker   |tag|  48-bit placeholder for values: pointers, strings
  // all bits 0   | 4 |  
  // for non float|bit|  

  // Doubles contain non-zero in NaN marker field and are stored with bits inversed 

  // JS_UNINITIALIZED is strictly uint64_t(0)

  enum {

    JS_TAG_UNINITIALIZED = 0,
    JS_TAG_INT = 1,
    JS_TAG_BOOL = 2,
    JS_TAG_NULL = 3,
    JS_TAG_UNDEFINED = 4,
    JS_TAG_CATCH_OFFSET = 5,
    JS_TAG_EXCEPTION = 6,
    JS_TAG_FLOAT64 = 7,

    /* all tags with a reference count have 0b1000 bit */
    JS_TAG_OBJECT = 8,
    JS_TAG_FUNCTION_BYTECODE = 9, /* used internally */
    JS_TAG_MODULE = 10, /* used internally */
    JS_TAG_STRING = 11,
    JS_TAG_SYMBOL = 12,
    JS_TAG_BIG_FLOAT = 13,
    JS_TAG_BIG_INT = 14,
    JS_TAG_BIG_DECIMAL = 15,

  };

  typedef uint64_t JSValue;

  #define JSValueConst JSValue

  #define JS_VALUE_GET_TAG(v) (((v)>0xFFFFFFFFFFFFFull)? (unsigned)JS_TAG_FLOAT64 : (unsigned)((v) >> 48))

  #define JS_VALUE_GET_INT(v)  (int)(v)
  #define JS_VALUE_GET_BOOL(v) (int)(v)
  #ifdef JS_PTR64
  #define JS_VALUE_GET_PTR(v)  ((void *)((intptr_t)(v) & 0x0000FFFFFFFFFFFFull))
  #else
  #define JS_VALUE_GET_PTR(v)  ((void *)(intptr_t)(v))
  #endif

  #define JS_MKVAL(tag, val) (((uint64_t)(0xF & tag) << 48) | (uint32_t)(val))
  #define JS_MKPTR(tag, ptr) (((uint64_t)(0xF & tag) << 48) | ((uint64_t)(ptr) & 0x0000FFFFFFFFFFFFull))

  #define JS_NAN JS_MKVAL(JS_TAG_FLOAT64,0)
  #define JS_INFINITY_NEGATIVE JS_MKVAL(JS_TAG_FLOAT64,1)
  #define JS_INFINITY_POSITIVE JS_MKVAL(JS_TAG_FLOAT64,2)

  QJS_API double JS_VALUE_GET_FLOAT64(JSValue v);

  QJS_API JSValue __JS_NewFloat64(JSContext* ctx, double d);

  //#define JS_TAG_IS_FLOAT64(tag) ((tag & 0x7ff0) != 0)
  #define JS_TAG_IS_FLOAT64(tag) (tag == JS_TAG_FLOAT64)

  /* same as JS_VALUE_GET_TAG, but return JS_TAG_FLOAT64 with NaN boxing */
  /* Note: JS_VALUE_GET_TAG already normalized in this packaging schema*/
  #define JS_VALUE_GET_NORM_TAG(v) JS_VALUE_GET_TAG(v)

  #define JS_VALUE_IS_NAN(v) (v == JS_NAN)

  #define JS_VALUE_HAS_REF_COUNT(v) ((JS_VALUE_GET_TAG(v) & 0xFFF8) == 0x8)

#else // !JS_STRICT_NAN_BOXING

enum {
    /* all tags with a reference count are negative */
    JS_TAG_FIRST       = -11, /* first negative tag */
    JS_TAG_BIG_DECIMAL = -11,
    JS_TAG_BIG_INT     = -10,
    JS_TAG_BIG_FLOAT   = -9,
    JS_TAG_SYMBOL      = -8,
    JS_TAG_STRING      = -7,
    JS_TAG_MODULE      = -3, /* used internally */
    JS_TAG_FUNCTION_BYTECODE = -2, /* used internally */
    JS_TAG_OBJECT      = -1,

    JS_TAG_INT         = 0,
    JS_TAG_BOOL        = 1,
    JS_TAG_NULL        = 2,
    JS_TAG_UNDEFINED   = 3,
    JS_TAG_UNINITIALIZED = 4,
    JS_TAG_CATCH_OFFSET = 5,
    JS_TAG_EXCEPTION   = 6,
    JS_TAG_FLOAT64     = 7,
    /* any larger tag is FLOAT64 if JS_NAN_BOXING */
};

#ifdef CONFIG_CHECK_JSVALUE
/* JSValue consistency : it is not possible to run the code in this
   mode, but it is useful to detect simple reference counting
   errors. It would be interesting to modify a static C analyzer to
   handle specific annotations (clang has such annotations but only
   for objective C) */
typedef struct __JSValue *JSValue;
typedef const struct __JSValue *JSValueConst;

#define JS_VALUE_GET_TAG(v) (int)((uintptr_t)(v) & 0xf)
/* same as JS_VALUE_GET_TAG, but return JS_TAG_FLOAT64 with NaN boxing */
#define JS_VALUE_GET_NORM_TAG(v) JS_VALUE_GET_TAG(v)
#define JS_VALUE_GET_INT(v) (int)((intptr_t)(v) >> 4)
#define JS_VALUE_GET_BOOL(v) JS_VALUE_GET_INT(v)
#define JS_VALUE_GET_FLOAT64(v) (double)JS_VALUE_GET_INT(v)
#define JS_VALUE_GET_PTR(v) (void *)((intptr_t)(v) & ~0xf)

#define JS_MKVAL(tag, val) (JSValue)(intptr_t)(((val) << 4) | (tag))
#define JS_MKPTR(tag, p) (JSValue)((intptr_t)(p) | (tag))

#define JS_TAG_IS_FLOAT64(tag) ((unsigned)(tag) == JS_TAG_FLOAT64)

#define JS_NAN JS_MKVAL(JS_TAG_FLOAT64, 1)

static inline JSValue __JS_NewFloat64(JSContext *ctx, double d)
{
    return JS_MKVAL(JS_TAG_FLOAT64, (int)d);
}

static inline JS_BOOL JS_VALUE_IS_NAN(JSValue v)
{
    return 0;
}
    
#elif defined(JS_NAN_BOXING)

typedef uint64_t JSValue;

#define JSValueConst JSValue

#define JS_VALUE_GET_TAG(v) (int)((v) >> 32)
#define JS_VALUE_GET_INT(v) (int)(v)
#define JS_VALUE_GET_BOOL(v) (int)(v)
#define JS_VALUE_GET_PTR(v) (void *)(intptr_t)(v)

#define JS_MKVAL(tag, val) (((uint64_t)(tag) << 32) | (uint32_t)(val))
#define JS_MKPTR(tag, ptr) (((uint64_t)(tag) << 32) | (uintptr_t)(ptr))

#define JS_FLOAT64_TAG_ADDEND (0x7ff80000 - JS_TAG_FIRST + 1) /* quiet NaN encoding */

double JS_VALUE_GET_FLOAT64(JSValue v);

#define JS_NAN (0x7ff8000000000000 - ((uint64_t)JS_FLOAT64_TAG_ADDEND << 32))

JSValue __JS_NewFloat64(JSContext* ctx, double d);

#define JS_TAG_IS_FLOAT64(tag) ((unsigned)((tag) - JS_TAG_FIRST) >= (JS_TAG_FLOAT64 - JS_TAG_FIRST))

/* same as JS_VALUE_GET_TAG, but return JS_TAG_FLOAT64 with NaN boxing */
static inline int JS_VALUE_GET_NORM_TAG(JSValue v)
{
    uint32_t tag;
    tag = JS_VALUE_GET_TAG(v);
    if (JS_TAG_IS_FLOAT64(tag))
        return JS_TAG_FLOAT64;
    else
        return tag;
}

static inline JS_BOOL JS_VALUE_IS_NAN(JSValue v)
{
    uint32_t tag;
    tag = JS_VALUE_GET_TAG(v);
    return tag == (JS_NAN >> 32);
}
    
#else /* !JS_NAN_BOXING */

typedef union JSValueUnion {
    int32_t int32;
    double float64;
    void *ptr;
} JSValueUnion;

typedef struct JSValue {
    JSValueUnion u;
    int64_t tag;
} JSValue;

#define JSValueConst JSValue

#define JS_VALUE_GET_TAG(v) ((int32_t)(v).tag)
/* same as JS_VALUE_GET_TAG, but return JS_TAG_FLOAT64 with NaN boxing */
#define JS_VALUE_GET_NORM_TAG(v) JS_VALUE_GET_TAG(v)
#define JS_VALUE_GET_INT(v) ((v).u.int32)
#define JS_VALUE_GET_BOOL(v) ((v).u.int32)
#define JS_VALUE_GET_FLOAT64(v) ((v).u.float64)
#define JS_VALUE_GET_PTR(v) ((v).u.ptr)

#define JS_MKVAL(tag, val) (JSValue){ (JSValueUnion){ .int32 = val }, tag }
#define JS_MKPTR(tag, p) (JSValue){ (JSValueUnion){ .ptr = p }, tag }

#define JS_TAG_IS_FLOAT64(tag) ((unsigned)(tag) == JS_TAG_FLOAT64)

#define JS_NAN (JSValue){ .u.float64 = JS_FLOAT64_NAN, JS_TAG_FLOAT64 }

static inline JSValue __JS_NewFloat64(JSContext *ctx, double d)
{
    JSValue v;
    v.tag = JS_TAG_FLOAT64;
    v.u.float64 = d;
    return v;
}

static inline JS_BOOL JS_VALUE_IS_NAN(JSValue v)
{
    union {
        double d;
        uint64_t u64;
    } u;
    if (v.tag != JS_TAG_FLOAT64)
        return 0;
    u.d = v.u.float64;
    return (u.u64 & 0x7fffffffffffffff) > 0x7ff0000000000000;
}

#endif /* !JS_NAN_BOXING */

  #define JS_VALUE_HAS_REF_COUNT(v) ((unsigned)JS_VALUE_GET_TAG(v) >= (unsigned)JS_TAG_FIRST)

#endif /* !JS_STRICT_NAN_BOXING */

#define JS_VALUE_IS_BOTH_INT(v1, v2) ((JS_VALUE_GET_TAG(v1) | JS_VALUE_GET_TAG(v2)) == 0)
#define JS_VALUE_IS_BOTH_FLOAT(v1, v2) (JS_TAG_IS_FLOAT64(JS_VALUE_GET_TAG(v1)) && JS_TAG_IS_FLOAT64(JS_VALUE_GET_TAG(v2)))

#define JS_VALUE_GET_OBJ(v) ((JSObject *)JS_VALUE_GET_PTR(v))
#define JS_VALUE_GET_STRING(v) ((JSString *)JS_VALUE_GET_PTR(v))


/* special values */
#define JS_NULL      JS_MKVAL(JS_TAG_NULL, 0)
#define JS_UNDEFINED JS_MKVAL(JS_TAG_UNDEFINED, 0)
#define JS_FALSE     JS_MKVAL(JS_TAG_BOOL, 0)
#define JS_TRUE      JS_MKVAL(JS_TAG_BOOL, 1)
#define JS_EXCEPTION JS_MKVAL(JS_TAG_EXCEPTION, 0)
#define JS_UNINITIALIZED JS_MKVAL(JS_TAG_UNINITIALIZED, 0)

/* flags for object properties */
#define JS_PROP_CONFIGURABLE  (1 << 0)
#define JS_PROP_WRITABLE      (1 << 1)
#define JS_PROP_ENUMERABLE    (1 << 2)
#define JS_PROP_C_W_E         (JS_PROP_CONFIGURABLE | JS_PROP_WRITABLE | JS_PROP_ENUMERABLE)
#define JS_PROP_LENGTH        (1 << 3) /* used internally in Arrays */
#define JS_PROP_TMASK         (3 << 4) /* mask for NORMAL, GETSET, VARREF, AUTOINIT */
#define JS_PROP_NORMAL         (0 << 4)
#define JS_PROP_GETSET         (1 << 4)
#define JS_PROP_VARREF         (2 << 4) /* used internally */
#define JS_PROP_AUTOINIT       (3 << 4) /* used internally */

/* flags for JS_DefineProperty */
#define JS_PROP_HAS_SHIFT        8
#define JS_PROP_HAS_CONFIGURABLE (1 << 8)
#define JS_PROP_HAS_WRITABLE     (1 << 9)
#define JS_PROP_HAS_ENUMERABLE   (1 << 10)
#define JS_PROP_HAS_GET          (1 << 11)
#define JS_PROP_HAS_SET          (1 << 12)
#define JS_PROP_HAS_VALUE        (1 << 13)

/* throw an exception if false would be returned
   (JS_DefineProperty/JS_SetProperty) */
#define JS_PROP_THROW            (1 << 14)
/* throw an exception if false would be returned in strict mode
   (JS_SetProperty) */
#define JS_PROP_THROW_STRICT     (1 << 15)

#define JS_PROP_NO_ADD           (1 << 16) /* internal use */
#define JS_PROP_NO_EXOTIC        (1 << 17) /* internal use */

#define JS_DEFAULT_STACK_SIZE (256 * 1024)

/* JS_Eval() flags */
#define JS_EVAL_TYPE_GLOBAL   (0 << 0) /* global code (default) */
#define JS_EVAL_TYPE_MODULE   (1 << 0) /* module code */
#define JS_EVAL_TYPE_DIRECT   (2 << 0) /* direct call (internal use) */
#define JS_EVAL_TYPE_INDIRECT (3 << 0) /* indirect call (internal use) */
#define JS_EVAL_TYPE_MASK     (3 << 0)

#define JS_EVAL_FLAG_STRICT   (1 << 3) /* force 'strict' mode */
#define JS_EVAL_FLAG_STRIP    (1 << 4) /* force 'strip' mode */
/* compile but do not run. The result is an object with a
   JS_TAG_FUNCTION_BYTECODE or JS_TAG_MODULE tag. It can be executed
   with JS_EvalFunction(). */
#define JS_EVAL_FLAG_COMPILE_ONLY (1 << 5)
/* don't include the stack frames before this eval in the Error() backtraces */
#define JS_EVAL_FLAG_BACKTRACE_BARRIER (1 << 6)

typedef JSValue JSCFunction(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
typedef JSValue JSCFunctionMagic(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic);
typedef JSValue JSCFunctionData(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic, JSValue *func_data);

typedef struct JSMallocState {
    size_t malloc_count;
    size_t malloc_size;
    size_t malloc_limit;
    void *opaque; /* user opaque */
} JSMallocState;

typedef struct JSMallocFunctions {
    void *(*js_malloc)(JSMallocState *s, size_t size);
    void (*js_free)(JSMallocState *s, void *ptr);
    void *(*js_realloc)(JSMallocState *s, void *ptr, size_t size);
    size_t (*js_malloc_usable_size)(const void *ptr);
} JSMallocFunctions;

typedef struct JSGCObjectHeader JSGCObjectHeader;

QJS_API JSRuntime *JS_NewRuntime(void);
/* info lifetime must exceed that of rt */
QJS_API void JS_SetRuntimeInfo(JSRuntime *rt, const char *info);
QJS_API void JS_SetMemoryLimit(JSRuntime *rt, size_t limit);
QJS_API void JS_SetGCThreshold(JSRuntime *rt, size_t gc_threshold);
/* use 0 to disable maximum stack size check */
QJS_API void JS_SetMaxStackSize(JSRuntime *rt, size_t stack_size);
/* should be called when changing thread to update the stack top value
   used to check stack overflow. */
QJS_API void JS_UpdateStackTop(JSRuntime *rt);
QJS_API JSRuntime *JS_NewRuntime2(const JSMallocFunctions *mf, void *opaque);
QJS_API void JS_FreeRuntime(JSRuntime *rt);
QJS_API void *JS_GetRuntimeOpaque(JSRuntime *rt);
QJS_API void JS_SetRuntimeOpaque(JSRuntime *rt, void *opaque);
QJS_API typedef void JS_MarkFunc(JSRuntime *rt, JSGCObjectHeader *gp);
QJS_API void JS_MarkValue(JSRuntime *rt, JSValueConst val, JS_MarkFunc *mark_func);
QJS_API void JS_RunGC(JSRuntime *rt);
QJS_API JS_BOOL JS_IsLiveObject(JSRuntime *rt, JSValueConst obj);

QJS_API JSContext *JS_NewContext(JSRuntime *rt);
QJS_API void JS_FreeContext(JSContext *s);
QJS_API JSContext *JS_DupContext(JSContext *ctx);
QJS_API void *JS_GetContextOpaque(JSContext *ctx);
QJS_API void JS_SetContextOpaque(JSContext *ctx, void *opaque);
QJS_API JSRuntime *JS_GetRuntime(JSContext *ctx);
QJS_API void JS_SetClassProto(JSContext *ctx, JSClassID class_id, JSValue obj);
QJS_API JSValue JS_GetClassProto(JSContext *ctx, JSClassID class_id);
QJS_API JSValue JS_GetClassName(JSContext *ctx, JSClassID class_id);


/* the following functions are used to select the intrinsic object to
   save memory */
QJS_API JSContext *JS_NewContextRaw(JSRuntime *rt);
QJS_API void JS_AddIntrinsicBaseObjects(JSContext *ctx);
QJS_API void JS_AddIntrinsicDate(JSContext *ctx);
QJS_API void JS_AddIntrinsicEval(JSContext *ctx);
QJS_API void JS_AddIntrinsicStringNormalize(JSContext *ctx);
QJS_API void JS_AddIntrinsicRegExpCompiler(JSContext *ctx);
QJS_API void JS_AddIntrinsicRegExp(JSContext *ctx);
QJS_API void JS_AddIntrinsicJSON(JSContext *ctx);
QJS_API void JS_AddIntrinsicProxy(JSContext *ctx);
QJS_API void JS_AddIntrinsicMapSet(JSContext *ctx);
QJS_API void JS_AddIntrinsicTypedArrays(JSContext *ctx);
QJS_API void JS_AddIntrinsicPromise(JSContext *ctx);
QJS_API void JS_AddIntrinsicBigInt(JSContext *ctx);
QJS_API void JS_AddIntrinsicBigFloat(JSContext *ctx);
QJS_API void JS_AddIntrinsicBigDecimal(JSContext *ctx);
/* enable operator overloading */
QJS_API void JS_AddIntrinsicOperators(JSContext *ctx);
/* enable "use math" */
QJS_API void JS_EnableBignumExt(JSContext *ctx, JS_BOOL enable);

QJS_API JSValue js_string_codePointRange(JSContext *ctx, JSValueConst this_val,
                                 int argc, JSValueConst *argv);

QJS_API void *js_malloc_rt(JSRuntime *rt, size_t size);
QJS_API void js_free_rt(JSRuntime *rt, void *ptr);
QJS_API void *js_realloc_rt(JSRuntime *rt, void *ptr, size_t size);
QJS_API size_t js_malloc_usable_size_rt(JSRuntime *rt, const void *ptr);
QJS_API void *js_mallocz_rt(JSRuntime *rt, size_t size);

QJS_API void *js_malloc(JSContext *ctx, size_t size);
QJS_API void js_free(JSContext *ctx, void *ptr);
QJS_API void *js_realloc(JSContext *ctx, void *ptr, size_t size);
QJS_API size_t js_malloc_usable_size(JSContext *ctx, const void *ptr);
QJS_API void *js_realloc2(JSContext *ctx, void *ptr, size_t size, size_t *pslack);
QJS_API void *js_mallocz(JSContext *ctx, size_t size);
QJS_API char *js_strdup(JSContext *ctx, const char *str);
QJS_API char *js_strndup(JSContext *ctx, const char *s, size_t n);

typedef struct JSMemoryUsage {
    int64_t malloc_size, malloc_limit, memory_used_size;
    int64_t malloc_count;
    int64_t memory_used_count;
    int64_t atom_count, atom_size;
    int64_t str_count, str_size;
    int64_t obj_count, obj_size;
    int64_t prop_count, prop_size;
    int64_t shape_count, shape_size;
    int64_t js_func_count, js_func_size, js_func_code_size;
    int64_t js_func_pc2line_count, js_func_pc2line_size;
    int64_t c_func_count, array_count;
    int64_t fast_array_count, fast_array_elements;
    int64_t binary_object_count, binary_object_size;
} JSMemoryUsage;

QJS_API void JS_ComputeMemoryUsage(JSRuntime *rt, JSMemoryUsage *s);
QJS_API void JS_DumpMemoryUsage(FILE *fp, const JSMemoryUsage *s, JSRuntime *rt);

/* atom support */
#define JS_ATOM_NULL 0

QJS_API JSAtom JS_NewAtomLen(JSContext *ctx, const char *str, size_t len);
QJS_API JSAtom JS_NewAtom(JSContext *ctx, const char *str);
QJS_API JSAtom JS_NewAtomUInt32(JSContext *ctx, uint32_t n);
QJS_API const char *JS_AtomGetStr(JSContext *ctx, char *buf, int buf_size, JSAtom atom);
QJS_API JSAtom JS_DupAtom(JSContext *ctx, JSAtom v);
QJS_API void JS_FreeAtom(JSContext *ctx, JSAtom v);
QJS_API JSValue JS_AtomToValue(JSContext *ctx, JSAtom atom);
QJS_API JSValue JS_AtomToString(JSContext *ctx, JSAtom atom);
QJS_API const char *JS_AtomToCString(JSContext *ctx, JSAtom atom);
QJS_API JSAtom JS_ValueToAtom(JSContext *ctx, JSValueConst val);

/* object class support */

typedef struct JSPropertyEnum {
    JS_BOOL is_enumerable;
    JSAtom atom;
} JSPropertyEnum;

QJS_API void js_free_prop_enum(JSContext *ctx, JSPropertyEnum *tab, uint32_t len);

typedef struct JSPropertyDescriptor {
    int flags;
    JSValue value;
    JSValue getter;
    JSValue setter;
} JSPropertyDescriptor;

#define JS_PROCEED_WITH_DEFAULT 12345

typedef struct JSClassExoticMethods {
    /* Return -1 if exception (can only happen in case of Proxy object),
       FALSE if the property does not exists, TRUE if it exists. If 1 is
       returned, the property descriptor 'desc' is filled if != NULL. */
    int (*get_own_property)(JSContext *ctx, JSPropertyDescriptor *desc,
                             JSValueConst obj, JSAtom prop);
    /* '*ptab' should hold the '*plen' property keys. Return 0 if OK,
       -1 if exception. The 'is_enumerable' field is ignored.
    */
    int (*get_own_property_names)(JSContext *ctx, JSPropertyEnum **ptab,
                                  uint32_t *plen,
                                  JSValueConst obj);
    /* return < 0 if exception, or TRUE/FALSE */
    int (*delete_property)(JSContext *ctx, JSValueConst obj, JSAtom prop);
    /* return < 0 if exception or TRUE/FALSE */
    int (*define_own_property)(JSContext *ctx, JSValueConst this_obj,
                               JSAtom prop, JSValueConst val,
                               JSValueConst getter, JSValueConst setter,
                               int flags);
    /* The following methods can be emulated with the previous ones,
       so they are usually not needed */
    /* return < 0 if exception or TRUE/FALSE or JS_PROCEED_WITH_DEFAULT */
    int (*has_property)(JSContext *ctx, JSValueConst obj, JSAtom atom);
    JSValue (*get_property)(JSContext *ctx, JSValueConst obj, JSAtom atom,
                            JSValueConst receiver);
    /* return < 0 if exception or TRUE/FALSE */
    int (*set_property)(JSContext *ctx, JSValueConst obj, JSAtom atom,
                        JSValueConst value, JSValueConst receiver, int flags);
} JSClassExoticMethods;

typedef void JSClassFinalizer(JSRuntime *rt, JSValue val);
typedef void JSClassGCMark(JSRuntime *rt, JSValueConst val,
                           JS_MarkFunc *mark_func);
#define JS_CALL_FLAG_CONSTRUCTOR (1 << 0)
typedef JSValue JSClassCall(JSContext *ctx, JSValueConst func_obj,
                            JSValueConst this_val, int argc, JSValueConst *argv,
                            int flags);

typedef struct JSClassDef {
    const char *class_name;
    JSClassFinalizer *finalizer;
    JSClassGCMark *gc_mark;
    /* if call != NULL, the object is a function. If (flags &
       JS_CALL_FLAG_CONSTRUCTOR) != 0, the function is called as a
       constructor. In this case, 'this_val' is new.target. A
       constructor call only happens if the object constructor bit is
       set (see JS_SetConstructorBit()). */
    JSClassCall *call;
    /* XXX: suppress this indirection ? It is here only to save memory
       because only a few classes need these methods */
    JSClassExoticMethods *exotic;
} JSClassDef;

QJS_API JSClassID JS_NewClassID(JSClassID *pclass_id);
QJS_API JSClassID JS_GetClassID(JSValueConst v);
QJS_API int JS_NewClass(JSRuntime *rt, JSClassID class_id, const JSClassDef *class_def);
QJS_API int JS_IsRegisteredClass(JSRuntime *rt, JSClassID class_id);

/* value handling */

static js_force_inline JSValue JS_NewBool(JSContext *ctx, JS_BOOL val)
{
    return JS_MKVAL(JS_TAG_BOOL, (val != 0));
}

static js_force_inline JSValue JS_NewInt32(JSContext *ctx, int32_t val)
{
    return JS_MKVAL(JS_TAG_INT, val);
}

static js_force_inline JSValue JS_NewCatchOffset(JSContext *ctx, int32_t val)
{
    return JS_MKVAL(JS_TAG_CATCH_OFFSET, val);
}

static js_force_inline JSValue JS_NewInt64(JSContext *ctx, int64_t val)
{
    JSValue v;
    if (val == (int32_t)val) {
        v = JS_NewInt32(ctx, (int32_t)val);
    } else {
        v = __JS_NewFloat64(ctx, (double)val);
    }
    return v;
}

static js_force_inline JSValue JS_NewUint32(JSContext *ctx, uint32_t val)
{
    JSValue v;
    if (val <= 0x7fffffff) {
        v = JS_NewInt32(ctx, val);
    } else {
        v = __JS_NewFloat64(ctx, val);
    }
    return v;
}

QJS_API JSValue JS_NewBigInt64(JSContext *ctx, int64_t v);
QJS_API JSValue JS_NewBigUint64(JSContext *ctx, uint64_t v);

static js_force_inline JSValue JS_NewFloat64(JSContext *ctx, double d)
{
    JSValue v;
    int32_t val;
    union {
        double d;
        uint64_t u;
    } u, t;
    u.d = d;
    val = (int32_t)d;
    t.d = val;
    /* -0 cannot be represented as integer, so we compare the bit
        representation */
    if (u.u == t.u) {
        v = JS_MKVAL(JS_TAG_INT, val);
    } else {
        v = __JS_NewFloat64(ctx, d);
    }
    return v;
}

QJS_API JSValue JS_NewDate(JSContext* ctx, double ms_1970);
QJS_API JS_BOOL JS_IsDate(JSContext *ctx, JSValueConst obj, double* ms_since_1970);

static inline JS_BOOL JS_IsNumber(JSValueConst v)
{
    int tag = JS_VALUE_GET_TAG(v);
    return tag == JS_TAG_INT || JS_TAG_IS_FLOAT64(tag);
}

static inline JS_BOOL JS_IsBigInt(JSContext *ctx, JSValueConst v)
{
    int tag = JS_VALUE_GET_TAG(v);
    return tag == JS_TAG_BIG_INT;
}

static inline JS_BOOL JS_IsBigFloat(JSValueConst v)
{
    int tag = JS_VALUE_GET_TAG(v);
    return tag == JS_TAG_BIG_FLOAT;
}

static inline JS_BOOL JS_IsBigDecimal(JSValueConst v)
{
    int tag = JS_VALUE_GET_TAG(v);
    return tag == JS_TAG_BIG_DECIMAL;
}

static inline JS_BOOL JS_IsBool(JSValueConst v)
{
    return JS_VALUE_GET_TAG(v) == JS_TAG_BOOL;
}

static inline JS_BOOL JS_IsNull(JSValueConst v)
{
    return JS_VALUE_GET_TAG(v) == JS_TAG_NULL;
}

static inline JS_BOOL JS_IsUndefined(JSValueConst v)
{
    return JS_VALUE_GET_TAG(v) == JS_TAG_UNDEFINED;
}

static inline JS_BOOL JS_IsException(JSValueConst v)
{
    return js_unlikely(JS_VALUE_GET_TAG(v) == JS_TAG_EXCEPTION);
}

static inline JS_BOOL JS_IsUninitialized(JSValueConst v)
{
    return js_unlikely(JS_VALUE_GET_TAG(v) == JS_TAG_UNINITIALIZED);
}

static inline JS_BOOL JS_IsString(JSValueConst v)
{
    return JS_VALUE_GET_TAG(v) == JS_TAG_STRING;
}

static inline JS_BOOL JS_IsSymbol(JSValueConst v)
{
    return JS_VALUE_GET_TAG(v) == JS_TAG_SYMBOL;
}

static inline JS_BOOL JS_IsObject(JSValueConst v)
{
    return JS_VALUE_GET_TAG(v) == JS_TAG_OBJECT;
}

QJS_API int JS_IsObjectPlain(JSContext *ctx, JSValueConst val); /* plain JS object, that is not function nor array nor anything else */

QJS_API JSValue JS_Throw(JSContext *ctx, JSValue obj);
QJS_API JSValue JS_GetException(JSContext *ctx);
QJS_API JS_BOOL JS_IsError(JSContext *ctx, JSValueConst val);
QJS_API void JS_ResetUncatchableError(JSContext *ctx);
QJS_API JSValue JS_NewError(JSContext *ctx);
QJS_API JSValue __js_printf_like(2, 3) JS_ThrowSyntaxError(JSContext *ctx, const char *fmt, ...);
QJS_API JSValue __js_printf_like(2, 3) JS_ThrowTypeError(JSContext *ctx, const char *fmt, ...);
QJS_API JSValue __js_printf_like(2, 3) JS_ThrowReferenceError(JSContext *ctx, const char *fmt, ...);
QJS_API JSValue __js_printf_like(2, 3) JS_ThrowRangeError(JSContext *ctx, const char *fmt, ...);
QJS_API JSValue __js_printf_like(2, 3) JS_ThrowInternalError(JSContext *ctx, const char *fmt, ...);
QJS_API JSValue JS_ThrowOutOfMemory(JSContext *ctx);
QJS_API void JS_FreeValue(JSContext* ctx, JSValue v);
QJS_API void JS_FreeValueRT(JSRuntime* rt, JSValue v);

QJS_API JSValue JS_DupValue(JSContext* ctx, JSValueConst v);
QJS_API JSValue JS_DupValueRT(JSRuntime* rt, JSValueConst v);

QJS_API int JS_ToBool(JSContext *ctx, JSValueConst val); /* return -1 for JS_EXCEPTION */
QJS_API int JS_ToInt32(JSContext *ctx, int32_t *pres, JSValueConst val);
static inline int JS_ToUint32(JSContext *ctx, uint32_t *pres, JSValueConst val)
{
    return JS_ToInt32(ctx, (int32_t*)pres, val);
}
QJS_API int JS_ToInt64(JSContext *ctx, int64_t *pres, JSValueConst val);
QJS_API int JS_ToIndex(JSContext *ctx, uint64_t *plen, JSValueConst val);
QJS_API int JS_ToFloat64(JSContext *ctx, double *pres, JSValueConst val);
/* return an exception if 'val' is a Number */
QJS_API int JS_ToBigInt64(JSContext *ctx, int64_t *pres, JSValueConst val);
/* same as JS_ToInt64() but allow BigInt */
QJS_API int JS_ToInt64Ext(JSContext *ctx, int64_t *pres, JSValueConst val);

QJS_API JSValue JS_NewStringLen(JSContext *ctx, const char *str1, size_t len1);
QJS_API JSValue JS_NewString(JSContext *ctx, const char *str);
QJS_API JSValue JS_NewAtomString(JSContext *ctx, const char *str);
QJS_API JSValue JS_ToString(JSContext *ctx, JSValueConst val);
QJS_API JSValue JS_ToPropertyKey(JSContext *ctx, JSValueConst val);
QJS_API const char *JS_ToCStringLen2(JSContext *ctx, size_t *plen, JSValueConst val1, JS_BOOL cesu8);
static inline const char *JS_ToCStringLen(JSContext *ctx, size_t *plen, JSValueConst val1)
{
    return JS_ToCStringLen2(ctx, plen, val1, 0);
}
static inline const char *JS_ToCString(JSContext *ctx, JSValueConst val1)
{
    return JS_ToCStringLen2(ctx, NULL, val1, 0);
}
QJS_API void JS_FreeCString(JSContext *ctx, const char *ptr);

QJS_API JSValue JS_NewObjectProtoClass(JSContext *ctx, JSValueConst proto, JSClassID class_id);
QJS_API JSValue JS_NewObjectClass(JSContext *ctx, int class_id);
QJS_API JSValue JS_NewObjectProto(JSContext *ctx, JSValueConst proto);
QJS_API JSValue JS_NewObject(JSContext *ctx);

QJS_API JS_BOOL JS_IsFunction(JSContext* ctx, JSValueConst val);
QJS_API JS_BOOL JS_IsConstructor(JSContext* ctx, JSValueConst val);
QJS_API JS_BOOL JS_SetConstructorBit(JSContext *ctx, JSValueConst func_obj, JS_BOOL val);
QJS_API JS_BOOL JS_IsFunctionOfThisRealm(JSContext *ctx, JSValueConst val);

QJS_API JS_BOOL JS_AreFunctionsOfSameOrigin(JSContext *ctx, JSValue f1, JSValue f2);

QJS_API JSValue JS_GetUserClassName(JSContext *ctx, JSValueConst obj);

QJS_API JSValue JS_NewArray(JSContext *ctx);
QJS_API int JS_IsArray(JSContext *ctx, JSValueConst val);
/* isArray and has 'tag' property */
QJS_API int     JS_IsTuple(JSContext *ctx, JSValueConst val);
QJS_API JSValue JS_GetTupleTag(JSContext *ctx, JSValueConst val);

QJS_API JSValue JS_NewFastArray(JSContext *ctx, int argc, JSValueConst *argv);
/* Access an Array's internal JSValue array if available */
QJS_API int     JS_GetFastArray(JSContext *ctx, JSValueConst obj, JSValue **arrpp, uint32_t *countp);

QJS_API JSValue JS_GetPropertyInternal(JSContext *ctx, JSValueConst obj,
                               JSAtom prop, JSValueConst receiver,
                               JS_BOOL throw_ref_error);
static js_force_inline JSValue JS_GetProperty(JSContext *ctx, JSValueConst this_obj,
                                              JSAtom prop)
{
    return JS_GetPropertyInternal(ctx, this_obj, prop, this_obj, 0);
}
QJS_API JSValue JS_GetPropertyStr(JSContext *ctx, JSValueConst this_obj,
                          const char *prop);
QJS_API JSValue JS_GetPropertyUint32(JSContext *ctx, JSValueConst this_obj,
                             uint32_t idx);

/* get .length property */
QJS_API int JS_GetPropertyLength(JSContext *ctx, int64_t *plength, JSValueConst obj);

QJS_API int JS_SetPropertyInternal(JSContext *ctx, JSValueConst this_obj,
                           JSAtom prop, JSValue val,
                           int flags);
static inline int JS_SetProperty(JSContext *ctx, JSValueConst this_obj,
                                 JSAtom prop, JSValue val)
{
    return JS_SetPropertyInternal(ctx, this_obj, prop, val, JS_PROP_THROW);
}
QJS_API int JS_SetPropertyUint32(JSContext *ctx, JSValueConst this_obj,
                         uint32_t idx, JSValue val);
QJS_API int JS_SetPropertyInt64(JSContext *ctx, JSValueConst this_obj,
                        int64_t idx, JSValue val);
QJS_API int JS_SetPropertyStr(JSContext *ctx, JSValueConst this_obj,
                      const char *prop, JSValue val);
QJS_API int JS_HasProperty(JSContext *ctx, JSValueConst this_obj, JSAtom prop);
QJS_API int JS_IsExtensible(JSContext *ctx, JSValueConst obj);
QJS_API int JS_PreventExtensions(JSContext *ctx, JSValueConst obj);
QJS_API int JS_DeleteProperty(JSContext *ctx, JSValueConst obj, JSAtom prop, int flags);
QJS_API int JS_SetPrototype(JSContext *ctx, JSValueConst obj, JSValueConst proto_val);
QJS_API JSValue JS_GetPrototype(JSContext *ctx, JSValueConst val);
QJS_API JSValue JS_GetPrototypeOfDate(JSContext *ctx);

QJS_API int JS_CopyDataProperties(JSContext *ctx, JSValueConst target, JSValueConst source, JSValueConst excluded, int setprop);

#define JS_GPN_STRING_MASK  (1 << 0)
#define JS_GPN_SYMBOL_MASK  (1 << 1)
#define JS_GPN_PRIVATE_MASK (1 << 2)
/* only include the enumerable properties */
#define JS_GPN_ENUM_ONLY    (1 << 4)
/* set theJSPropertyEnum.is_enumerable field */
#define JS_GPN_SET_ENUM     (1 << 5)

QJS_API int JS_GetOwnPropertyNames(JSContext *ctx, JSPropertyEnum **ptab,
                           uint32_t *plen, JSValueConst obj, int flags);
QJS_API int JS_GetOwnProperty(JSContext *ctx, JSPropertyDescriptor *desc,
                      JSValueConst obj, JSAtom prop);

QJS_API JSValue JS_Call(JSContext *ctx, JSValueConst func_obj, JSValueConst this_obj,
                int argc, JSValueConst *argv);
QJS_API JSValue JS_Invoke(JSContext *ctx, JSValueConst this_val, JSAtom atom,
                  int argc, JSValueConst *argv);
QJS_API JSValue JS_CallConstructor(JSContext *ctx, JSValueConst func_obj,
                           int argc, JSValueConst *argv);
QJS_API JSValue JS_CallConstructor2(JSContext *ctx, JSValueConst func_obj,
                            JSValueConst new_target,
                            int argc, JSValueConst *argv);
QJS_API JS_BOOL JS_DetectModule(const char *input, size_t input_len);
/* 'input' must be zero terminated i.e. input[input_len] = '\0'. */
QJS_API JSValue JS_Eval(JSContext *ctx, const char *input, size_t input_len,
                const char *filename, int eval_flags);
QJS_API JSValue JS_Eval2(JSContext *ctx, const char *input, size_t input_len,
                const char *filename, int eval_flags, int line_no);

QJS_API JSValue JS_EvalFunction(JSContext *ctx, JSValue fun_obj);
/* same as JS_Eval() but with an explicit 'this_obj' parameter */
QJS_API JSValue JS_EvalThis(JSContext *ctx, JSValueConst this_obj,
                    const char *input, size_t input_len,
                    const char *filename, int eval_flags);
QJS_API JSValue JS_GetGlobalObject(JSContext *ctx);
QJS_API int JS_IsInstanceOf(JSContext *ctx, JSValueConst val, JSValueConst obj);
QJS_API int JS_DefineProperty(JSContext *ctx, JSValueConst this_obj,
                      JSAtom prop, JSValueConst val,
                      JSValueConst getter, JSValueConst setter, int flags);
QJS_API int JS_DefinePropertyValue(JSContext *ctx, JSValueConst this_obj,
                           JSAtom prop, JSValue val, int flags);
QJS_API int JS_DefinePropertyValueUint32(JSContext *ctx, JSValueConst this_obj,
                                 uint32_t idx, JSValue val, int flags);
QJS_API int JS_DefinePropertyValueStr(JSContext *ctx, JSValueConst this_obj,
                              const char *prop, JSValue val, int flags);
QJS_API int JS_DefinePropertyGetSet(JSContext *ctx, JSValueConst this_obj,
                            JSAtom prop, JSValue getter, JSValue setter,
                            int flags);
QJS_API void JS_SetOpaque(JSValue obj, void *opaque);
QJS_API void *JS_GetOpaque(JSValueConst obj, JSClassID class_id);
QJS_API void *JS_GetOpaque2(JSContext *ctx, JSValueConst obj, JSClassID class_id);
QJS_API JSClassID JS_GetClassID(JSValueConst obj);

/* 'buf' must be zero terminated i.e. buf[buf_len] = '\0'. */
QJS_API JSValue JS_ParseJSON(JSContext *ctx, const char *buf, size_t buf_len,
                     const char *filename);
#define JS_PARSE_JSON_EXT (1 << 0) /* allow extended JSON */
QJS_API JSValue JS_ParseJSON2(JSContext *ctx, const char *buf, size_t buf_len,
                      const char *filename, int flags);
QJS_API JSValue JS_JSONStringify(JSContext *ctx, JSValueConst obj,
                         JSValueConst replacer, JSValueConst space0);

typedef void JSFreeArrayBufferDataFunc(JSRuntime *rt, void *opaque, void *ptr);
QJS_API JSValue JS_NewArrayBuffer(JSContext *ctx, uint8_t *buf, size_t len,
                          JSFreeArrayBufferDataFunc *free_func, void *opaque,
                          JS_BOOL is_shared);
QJS_API JSValue JS_NewArrayBufferCopy(JSContext *ctx, const uint8_t *buf, size_t len);
QJS_API void JS_DetachArrayBuffer(JSContext *ctx, JSValueConst obj);
QJS_API uint8_t *JS_GetArrayBuffer(JSContext *ctx, size_t *psize, JSValueConst obj);
QJS_API JSValue JS_GetTypedArrayBuffer(JSContext *ctx, JSValueConst obj,
                               size_t *pbyte_offset,
                               size_t *pbyte_length,
                               size_t *pbytes_per_element);
typedef struct {
    void *(*sab_alloc)(void *opaque, size_t size);
    void (*sab_free)(void *opaque, void *ptr);
    void (*sab_dup)(void *opaque, void *ptr);
    void *sab_opaque;
} JSSharedArrayBufferFunctions;
QJS_API void JS_SetSharedArrayBufferFunctions(JSRuntime *rt,
                                      const JSSharedArrayBufferFunctions *sf);

QJS_API JSValue JS_NewPromiseCapability(JSContext *ctx, JSValue *resolving_funcs);

/* is_handled = TRUE means that the rejection is handled */
typedef void JSHostPromiseRejectionTracker(JSContext *ctx, JSValueConst promise,
                                           JSValueConst reason,
                                           JS_BOOL is_handled, void *opaque);
QJS_API void JS_SetHostPromiseRejectionTracker(JSRuntime *rt, JSHostPromiseRejectionTracker *cb, void *opaque);
QJS_API void JS_SetHostUnhandledPromiseRejectionTracker(JSRuntime *rt, JSHostPromiseRejectionTracker *cb, void *opaque);

/* return != 0 if the JS code needs to be interrupted */
typedef int JSInterruptHandler(JSRuntime *rt, void *opaque);
QJS_API void JS_SetInterruptHandler(JSRuntime *rt, JSInterruptHandler *cb, void *opaque);
/* if can_block is TRUE, Atomics.wait() can be used */
QJS_API void JS_SetCanBlock(JSRuntime *rt, JS_BOOL can_block);
/* set the [IsHTMLDDA] internal slot */
QJS_API void JS_SetIsHTMLDDA(JSContext *ctx, JSValueConst obj);

typedef struct JSModuleDef JSModuleDef;

/* return the module specifier (allocated with js_malloc()) or NULL if
   exception */
typedef char *JSModuleNormalizeFunc(JSContext *ctx,
                                    const char *module_base_name,
                                    const char *module_name, void *opaque);
typedef JSModuleDef *JSModuleLoaderFunc(JSContext *ctx,
                                        const char *module_name, void *opaque);
typedef void JSModuleUnloaderFunc(JSContext* ctx,void *so_handler);

/* module_normalize = NULL is allowed and invokes the default module
   filename normalizer */
QJS_API void JS_SetModuleLoaderFunc(JSRuntime *rt,
                            JSModuleNormalizeFunc *module_normalize,
                            JSModuleLoaderFunc *module_loader, JSModuleUnloaderFunc * module_unloader, void *opaque);
/* return the import.meta object of a module */
QJS_API JSValue JS_GetImportMeta(JSContext *ctx, JSModuleDef *m);
QJS_API JSAtom JS_GetModuleName(JSContext *ctx, JSModuleDef *m);

/* JS Job support */

typedef JSValue JSJobFunc(JSContext *ctx, int argc, JSValueConst *argv);
QJS_API int JS_EnqueueJob(JSContext *ctx, JSJobFunc *job_func, int argc, JSValueConst *argv);

QJS_API JS_BOOL JS_IsJobPending(JSRuntime *rt);
QJS_API int JS_ExecutePendingJob(JSRuntime *rt, JSContext **pctx);
QJS_API void JS_ExecuteTimer(JSContext* ctx);
/* Object Writer/Reader (currently only used to handle precompiled code) */
#define JS_WRITE_OBJ_BYTECODE  (1 << 0) /* allow function/module */
#define JS_WRITE_OBJ_BSWAP     (1 << 1) /* byte swapped output */
#define JS_WRITE_OBJ_SAB       (1 << 2) /* allow SharedArrayBuffer */
#define JS_WRITE_OBJ_REFERENCE (1 << 3) /* allow object references to
                                           encode arbitrary object
                                           graph */
QJS_API uint8_t *JS_WriteObject(JSContext *ctx, size_t *psize, JSValueConst obj,
                        int flags);
QJS_API uint8_t *JS_WriteObject2(JSContext *ctx, size_t *psize, JSValueConst obj,
                         int flags, uint8_t ***psab_tab, size_t *psab_tab_len);

#define JS_READ_OBJ_BYTECODE  (1 << 0) /* allow function/module */
#define JS_READ_OBJ_ROM_DATA  (1 << 1) /* avoid duplicating 'buf' data */
#define JS_READ_OBJ_SAB       (1 << 2) /* allow SharedArrayBuffer */
#define JS_READ_OBJ_REFERENCE (1 << 3) /* allow object references */
QJS_API JSValue JS_ReadObject(JSContext *ctx, const uint8_t *buf, size_t buf_len, int flags);
QJS_API JSValue JS_ReadObject2(JSContext *ctx, const uint8_t *buf, size_t buf_len, int flags, size_t* remnants_len);

/* load the dependencies of the module 'obj'. Useful when JS_ReadObject()
   returns a module. */
QJS_API int JS_ResolveModule(JSContext *ctx, JSValueConst obj);

/* only exported for os.Worker() */
QJS_API JSAtom JS_GetScriptOrModuleName(JSContext *ctx, int n_stack_levels);
/* only exported for os.Worker() */
QJS_API JSModuleDef *JS_RunModule(JSContext *ctx, const char *basename,
                          const char *filename);

QJS_API JSValue JS_GetModuleExportItemStr(JSContext *ctx, JSModuleDef *m, const char *name);
QJS_API JSValue JS_GetModuleExportItem(JSContext *ctx, JSModuleDef *m, JSAtom atom);

/* C function definition */
typedef enum JSCFunctionEnum {  /* XXX: should rename for namespace isolation */
    JS_CFUNC_generic,
    JS_CFUNC_generic_magic,
    JS_CFUNC_constructor,
    JS_CFUNC_constructor_magic,
    JS_CFUNC_constructor_or_func,
    JS_CFUNC_constructor_or_func_magic,
    JS_CFUNC_f_f,
    JS_CFUNC_f_f_f,
    JS_CFUNC_getter,
    JS_CFUNC_setter,
    JS_CFUNC_getter_magic,
    JS_CFUNC_setter_magic,
    JS_CFUNC_iterator_next,
} JSCFunctionEnum;

typedef union JSCFunctionType {
    JSCFunction *generic;
    JSValue (*generic_magic)(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic);
    JSCFunction *constructor;
    JSValue (*constructor_magic)(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv, int magic);
    JSCFunction *constructor_or_func;
    double (*f_f)(double);
    double (*f_f_f)(double, double);
    JSValue (*getter)(JSContext *ctx, JSValueConst this_val);
    JSValue (*setter)(JSContext *ctx, JSValueConst this_val, JSValueConst val);
    JSValue (*getter_magic)(JSContext *ctx, JSValueConst this_val, int magic);
    JSValue (*setter_magic)(JSContext *ctx, JSValueConst this_val, JSValueConst val, int magic);
    JSValue (*iterator_next)(JSContext *ctx, JSValueConst this_val,
                             int argc, JSValueConst *argv, int *pdone, int magic);
} JSCFunctionType;

QJS_API JSValue JS_NewCFunction2(JSContext *ctx, JSCFunction *func,
                         const char *name,
                         int length, JSCFunctionEnum cproto, int magic);
QJS_API JSValue JS_NewCFunctionData(JSContext *ctx, JSCFunctionData *func,
                            int length, int magic, int data_len,
                            JSValueConst *data);

static inline JSValue JS_NewCFunction(JSContext *ctx, JSCFunction *func, const char *name,
                                      int length)
{
    return JS_NewCFunction2(ctx, func, name, length, JS_CFUNC_generic, 0);
}

static inline JSValue JS_NewCFunctionMagic(JSContext *ctx, JSCFunctionMagic *func,
                                           const char *name,
                                           int length, JSCFunctionEnum cproto, int magic)
{
    return JS_NewCFunction2(ctx, (JSCFunction *)func, name, length, cproto, magic);
}
QJS_API void JS_SetConstructor(JSContext *ctx, JSValueConst func_obj,
                       JSValueConst proto);

/* C property definition */

typedef struct JSCFunctionListEntry {
    const char *name;
    uint8_t prop_flags;
    uint8_t def_type;
    int16_t magic;
    union {
        struct {
            uint8_t length; /* XXX: should move outside union */
            uint8_t cproto; /* XXX: should move outside union */
            JSCFunctionType cfunc;
        } func;
        struct {
            JSCFunctionType get;
            JSCFunctionType set;
        } getset;
        struct {
            const char *name;
            int base;
        } alias;
        struct {
            const struct JSCFunctionListEntry *tab;
            int len;
        } prop_list;
        const char *str;
        int32_t i32;
        int64_t i64;
        double f64;
    } u;
} JSCFunctionListEntry;

#define JS_DEF_CFUNC          0
#define JS_DEF_CGETSET        1
#define JS_DEF_CGETSET_MAGIC  2
#define JS_DEF_PROP_STRING    3
#define JS_DEF_PROP_INT32     4
#define JS_DEF_PROP_INT64     5
#define JS_DEF_PROP_DOUBLE    6
#define JS_DEF_PROP_UNDEFINED 7
#define JS_DEF_OBJECT         8
#define JS_DEF_ALIAS          9

/* Note: c++ does not like nested designators */
#define JS_CFUNC_DEF(name, length, func1) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, .u = { .func = { length, JS_CFUNC_generic, { .generic = func1 } } } }
#define JS_CFUNC_MAGIC_DEF(name, length, func1, magic) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, magic, .u = { .func = { length, JS_CFUNC_generic_magic, { .generic_magic = func1 } } } }
#define JS_CFUNC_SPECIAL_DEF(name, length, cproto, func1) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, 0, .u = { .func = { length, JS_CFUNC_ ## cproto, { .cproto = func1 } } } }
#define JS_ITERATOR_NEXT_DEF(name, length, func1, magic) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_CFUNC, magic, .u = { .func = { length, JS_CFUNC_iterator_next, { .iterator_next = func1 } } } }
#define JS_CGETSET_DEF(name, fgetter, fsetter) { name, JS_PROP_CONFIGURABLE, JS_DEF_CGETSET, 0, .u = { .getset = { .get = { .getter = fgetter }, .set = { .setter = fsetter } } } }
#define JS_CGETSET_MAGIC_DEF(name, fgetter, fsetter, magic) { name, JS_PROP_CONFIGURABLE, JS_DEF_CGETSET_MAGIC, magic, .u = { .getset = { .get = { .getter_magic = fgetter }, .set = { .setter_magic = fsetter } } } }
#define JS_PROP_STRING_DEF(name, cstr, prop_flags) { name, prop_flags, JS_DEF_PROP_STRING, 0, .u = { .str = cstr } }
#define JS_PROP_INT32_DEF(name, val, prop_flags) { name, prop_flags, JS_DEF_PROP_INT32, 0, .u = { .i32 = val } }
#define JS_PROP_INT64_DEF(name, val, prop_flags) { name, prop_flags, JS_DEF_PROP_INT64, 0, .u = { .i64 = val } }
#define JS_PROP_DOUBLE_DEF(name, val, prop_flags) { name, prop_flags, JS_DEF_PROP_DOUBLE, 0, .u = { .f64 = val } }
#define JS_PROP_UNDEFINED_DEF(name, prop_flags) { name, prop_flags, JS_DEF_PROP_UNDEFINED, 0, .u = { .i32 = 0 } }
#define JS_OBJECT_DEF(name, tab, len, prop_flags) { name, prop_flags, JS_DEF_OBJECT, 0, .u = { .prop_list = { tab, len } } }
#define JS_ALIAS_DEF(name, from) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_ALIAS, 0, .u = { .alias = { from, -1 } } }
#define JS_ALIAS_BASE_DEF(name, from, base) { name, JS_PROP_WRITABLE | JS_PROP_CONFIGURABLE, JS_DEF_ALIAS, 0, .u = { .alias = { from, base } } }

QJS_API void JS_SetPropertyFunctionList(JSContext *ctx, JSValueConst obj,
                                const JSCFunctionListEntry *tab,
                                int len);

/* C module definition */

typedef int JSModuleInitFunc(JSContext *ctx, JSModuleDef *m);

QJS_API JSModuleDef *JS_NewCModule(JSContext *ctx, const char *name_str,
                           JSModuleInitFunc *func);
/* can only be called before the module is instantiated */
QJS_API int JS_AddModuleExport(JSContext *ctx, JSModuleDef *m, const char *name_str);
QJS_API int JS_AddModuleExportList(JSContext *ctx, JSModuleDef *m,
                           const JSCFunctionListEntry *tab, int len);
/* can only be called after the module is instantiated */
QJS_API int JS_SetModuleExport(JSContext *ctx, JSModuleDef *m, const char *export_name,
                       JSValue val);
QJS_API int JS_SetModuleExportList(JSContext *ctx, JSModuleDef *m,
                           const JSCFunctionListEntry *tab, int len);

#ifdef CONFIG_DEBUGGER

typedef JS_BOOL JSDebuggerCheckLineNoF(JSContext *ctx, JSAtom file_name, uint32_t line_no, const uint8_t *pc);

QJS_DLLPORT void JS_SetBreakpointHandler(JSContext *ctx, JSDebuggerCheckLineNoF* line_hit_handler);
QJS_DLLPORT void JS_SetDebuggerMode(JSContext *ctx, int onoff);

QJS_DLLPORT uint32_t js_debugger_stack_depth(JSContext *ctx);
QJS_DLLPORT JSValue  js_debugger_build_backtrace(JSContext *ctx, const uint8_t *cur_pc);
QJS_DLLPORT JSValue  js_debugger_closure_variables(JSContext *ctx, int stack_index);
QJS_DLLPORT JSValue  js_debugger_local_variables(JSContext *ctx, int stack_index);

#endif

QJS_API void*    js_debugger_get_object_id(JSValue val);

#ifdef _WIN32
QJS_API int js_prepare_waitlist(JSContext* ctx, HANDLE* handles, int length, int* rwsize, int* msgSize, uint32_t*waitTime);
QJS_API void js_handle_waitresult(JSContext* ctx, int ret, int osrw_cnt, int msg_cnt);

#endif

#undef js_force_inline

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* QUICKJS_H */
