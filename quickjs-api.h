#pragma once

#ifdef _WIN32

#if defined(QJS_STATIC_LIB)
#  define QJS_DLLPORT
#elif defined(QJS_BUILD)
#  define QJS_API __declspec (dllexport)
#else
#  define QJS_API __declspec (dllimport)
#endif
#else
#define QJS_API
#endif
