/*
 * QuickJS C library
 * 
 * Copyright (c) 2017-2018 Fabrice Bellard
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
#ifndef QUICKJS_STORAGE_H
#define QUICKJS_STORAGE_H

#include "../quickjs.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32

#if defined(QJS_STATIC_LIB)
#  define QJS_DLLPORT
#elif defined(QJS_BUILD)
#  define QJS_API __declspec (dllexport)
#else
#  define QJS_DLLPORT __declspec (dllimport)
#endif
#else
#define QJS_DLLPORT
#endif

    QJS_API JSModuleDef *js_init_module_storage(JSContext *ctx, const char *module_name);

                                        
#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* QUICKJS_LIBC_H */
