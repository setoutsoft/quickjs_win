#ifndef QUICKJS_DEBUGGER_H
#define QUICKJS_DEBUGGER_H

#include "quickjs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JSDebuggerFunctionInfo {
    // same length as byte_code_buf.
    uint8_t *breakpoints;
    uint32_t dirty;
    int last_line_num;
} JSDebuggerFunctionInfo;

typedef struct JSDebuggerLocation {
    JSAtom filename;
    int line;
    int column;
} JSDebuggerLocation;

#define JS_DEBUGGER_STEP 1
#define JS_DEBUGGER_STEP_IN 2
#define JS_DEBUGGER_STEP_OUT 3
#define JS_DEBUGGER_STEP_CONTINUE 4

typedef int (*fun_trans_read)(void* udata, char* buffer, size_t length);
typedef int (*fun_trans_write)(void* udata, const char* buffer, size_t length);
typedef int (*fun_trans_peek)(void* udata);
typedef void (*fun_trans_close)(JSRuntime* rt, void* udata);

typedef struct JSDebuggerInfo {
    // JSContext that is used to for the JSON transport and debugger state.
    JSContext *ctx;
    JSContext *debugging_ctx;
 
    int attempted_connect;
    int attempted_wait;
    int peek_ticks;
    int should_peek;
    char *message_buffer;
    int message_buffer_length;
    int is_debugging;
    int is_paused;

    fun_trans_read transport_read;
    fun_trans_write transport_write;
    fun_trans_peek transport_peek;
    fun_trans_close transport_close;
    void *transport_udata;

    JSValue breakpoints;
    int exception_breakpoint;
    uint32_t breakpoints_dirty_counter;
    int stepping;
    JSDebuggerLocation step_over;
    int step_depth;
} JSDebuggerInfo;

QJS_API void js_debugger_new_context(JSContext *ctx);
QJS_API void js_debugger_free_context(JSContext *ctx);
QJS_API void js_debugger_check(JSContext *ctx, const uint8_t *pc);
QJS_API void js_debugger_exception(JSContext* ctx);
QJS_API void js_debugger_free(JSRuntime *rt, JSDebuggerInfo *info);


QJS_API void js_debugger_attach(
    JSContext* ctx, 
    fun_trans_read  transport_read,
    fun_trans_write transport_write,
    fun_trans_peek  transport_peek,
    fun_trans_close transport_close,
    void *udata
);
QJS_API void js_debugger_connect(JSContext *ctx, const char *address);
QJS_API void js_debugger_wait_connection(JSContext *ctx, const char* address);
QJS_API int js_debugger_is_transport_connected(JSRuntime* rt);

QJS_API JSValue js_debugger_file_breakpoints(JSContext *ctx, const char *path);
QJS_API void js_debugger_cooperate(JSContext *ctx);

// begin internal api functions
// these functions all require access to quickjs internal structures.

QJS_API JSDebuggerInfo *js_debugger_info(JSRuntime *rt);

// this may be able to be done with an Error backtrace,
// but would be clunky and require stack string parsing.
QJS_API uint32_t js_debugger_stack_depth(JSContext *ctx);
QJS_API JSValue js_debugger_build_backtrace(JSContext *ctx, const uint8_t *cur_pc);
QJS_API JSDebuggerLocation js_debugger_current_location(JSContext *ctx, const uint8_t *cur_pc);

// checks to see if a breakpoint exists on the current pc.
// calls back into js_debugger_file_breakpoints.
QJS_API int js_debugger_check_breakpoint(JSContext *ctx, uint32_t current_dirty, const uint8_t *cur_pc);

QJS_API JSValue js_debugger_local_variables(JSContext *ctx, int stack_index);
QJS_API JSValue js_debugger_closure_variables(JSContext *ctx, int stack_index);

// evaluates an expression at any stack frame. JS_Evaluate* only evaluates at the top frame.
QJS_API JSValue js_debugger_evaluate(JSContext *ctx, int stack_index, JSValue expression);

// end internal api functions

#ifdef __cplusplus
}
#endif

#endif