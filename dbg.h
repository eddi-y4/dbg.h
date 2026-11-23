/*
 * dbg.h
 *
 * A small C23 print debugging library.
 *
 * Main interface:
 *
 *     DBG_MSG(foramtted message)
 *     DBG_LOG(code)
 *     DBG_LOG_OPEN(code)
 *     DBG_TRACK(a, b, c)
 *
 * DBG_MSG(formatted message)
 *    Logs the source location and a per-log-site execution count,
 *    then prints message.
 *
 * DBG_LOG(code)
 *     Logs the source location and a per-log-site execution count,
 *     then executes code.
 *
 * DBG_LOG_OPEN(code)
 *     Same idea, but deliberately does not use a do/while statement
 *     wrapper. This allows the macro to participate in contexts where
 *     a declaration-like expansion is desired.
 *
 * DBG_TRACK(...)
 *     Inspects objects available at the call site. It reports their
 *     names, sizes, addresses, and raw object representations.
 *
 *     If a type has been registered with the formatting facility,
 *     its formatted value is also printed using:
 *
 * The raw representation makes DBG_TRACK useful even when the library
 * has no knowledge of a user-defined type.
 *
 * Define DBG_IMPLEMENTATION in exactly one translation unit before
 * including this header:
 *
 *     #define DBG_IMPLEMENTATION
 *     #include "dbg.h"
 *
 * Other translation units simply:
 *
 *     #include "dbg.h"
 *
 * __COUNTER__ is used when available to give each DBG_LOG invocation
 * a stable compile-time identity.
 */

#ifndef DBG_H
#define DBG_H

#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


/* ================================================================
 * Configuration
 * ================================================================ */

/*
 * Stream used by all debugging output.
 *
 * This is intentionally a variable rather than a hard-coded stderr
 * so the caller can redirect all debugging output:
 *
 *     stderr = fopen("debug.log", "w");
 */
//extern FILE *stderr;

//#define stderr stderr

/* ================================================================
 * Formatting
 * ================================================================
 * from fmt.h
 * TYPE_TABLE(X) is the type registry.
 *
 * Each entry has:
 *
 *     X(type, printf_format, view_expression)
 *
 * `v` in view_expression refers to the value being formatted.
 *
 * The table can be extended by defining DBG_USER_TYPE_TABLE(X)
 * before including this header.
 *
 * Example:
 *
 *     #define USER_TYPE_TABLE(X) \
 *         X(my_type, "%d", (v).value)
 *
 * The resulting generic formatter can then be used independently
 * of DBG_TRACK:
 *
 *     fprintf(stderr, TYPE_FMT(x), TYPE_VIEW(x));
 */


/* ================================================================
 * Objects
 * ================================================================ */

typedef struct {
    const char *file;
    unsigned    line;
    const char *function;
} dbg_location;


typedef struct {
    const char *name;
    const char *type_name;
    const void *address;
    size_t      size;
} dbg_object;


/* ================================================================
 * Tracking
 * ================================================================ */

void dbg_track(dbg_location location, const dbg_object *object);


/*
 * Print one object.
 *
 * The static type of x must still be available here, so the
 * formatting operation happens in the macro rather than inside
 * dbg_track(), where the object has become a void pointer.
 */

#define DBG__TYPE_NAME(x) _Generic((x), \
    char:               "char", \
    signed char:        "signed char", \
    unsigned char:      "unsigned char", \
    short:              "short", \
    unsigned short:     "unsigned short", \
    int:                 "int", \
    unsigned int:       "unsigned int", \
    long:               "long", \
    unsigned long:      "unsigned long", \
    long long:          "long long", \
    unsigned long long: "unsigned long long", \
    float:              "float", \
    double:             "double", \
    long double:        "long double", \
    _Bool:              "_Bool", \
    void *:             "void *", \
    char *:             "char *", \
    const char *:       "const char *", \
    default:            "<unknown type>" \
)


#define DBG_PRINT_VAR(x) \
    do { \
        fprintf(stderr, "%s = ", #x); \
        _Generic((x), \
            char:               fprintf(stderr, "%c\n", (x)), \
            signed char:        fprintf(stderr, "%hhd\n", (x)), \
            unsigned char:      fprintf(stderr, "%hhu\n", (x)), \
            short:              fprintf(stderr, "%hd\n", (x)), \
            unsigned short:     fprintf(stderr, "%hu\n", (x)), \
            int:                fprintf(stderr, "%d\n", (x)), \
            unsigned int:       fprintf(stderr, "%u\n", (x)), \
            long:               fprintf(stderr, "%ld\n", (x)), \
            unsigned long:      fprintf(stderr, "%lu\n", (x)), \
            long long:          fprintf(stderr, "%lld\n", (x)), \
            unsigned long long: fprintf(stderr, "%llu\n", (x)), \
            float:              fprintf(stderr, "%f\n", (x)), \
            double:             fprintf(stderr, "%f\n", (x)), \
            long double:        fprintf(stderr, "%Lf\n", (x)), \
            _Bool:              fprintf(stderr, "%s\n", (x) ? "true" : "false"), \
            char *:             fprintf(stderr, "\"%s\"\n", (x)), \
            const char *:       fprintf(stderr, "\"%s\"\n", (x)), \
            void *:             fprintf(stderr, "%p\n", (x)), \
            const void *:       fprintf(stderr, "%p\n", (x)), \
            default:            fprintf(stderr, "<unsupported type>\n") \
        ); \
    } while (0)



#define DBG_TRACK_ONE(x)                                         \
    do {                                                         \
        dbg_object dbg_object_ = {                               \
            .name      = #x,                                     \
            .type_name = DBG__TYPE_NAME(x),                       \
            .address   = &(x),                                  \
            .size      = sizeof(x)                              \
        };                                                       \
                                                                 \
        dbg_track(                                               \
            (dbg_location){                                      \
                .file     = __FILE__,                            \
                .line     = __LINE__,                            \
                .function = __func__                             \
            },                                                    \
            &dbg_object_                                          \
        );                                                       \
        DBG_PRINT_VAR(x);        \
    } while (0)


/*
 * Recursive variadic expansion.
 *
 * C23's __VA_OPT__ makes this substantially cleaner than older
 * preprocessor implementations.
 */
#define DBG_TRACK_A(x, ...) \
    DBG_TRACK_ONE(x) \
    __VA_OPT__(; DBG_TRACK_B(__VA_ARGS__))

#define DBG_TRACK_B(x, ...) \
    DBG_TRACK_ONE(x) \
    __VA_OPT__(; DBG_TRACK_A(__VA_ARGS__))

#define DBG_TRACK(...) \
    DBG_TRACK_A(__VA_ARGS__)


/* ================================================================
 * Logging
 * ================================================================ */

void dbg_log(
    dbg_location location,
    unsigned long long site,
    unsigned long long count,
    const char *code
);


/*
 * Token concatenation.
 */
#define DBG_CAT_I(a, b) a ## b
#define DBG_CAT(a, b)   DBG_CAT_I(a, b)


/*
 * __COUNTER__ gives each macro invocation a unique compile-time
 * identity.
 *
 * It is deliberately captured once and then passed through another
 * macro layer so that every use of the identity refers to the same
 * number.
 */
#ifdef __COUNTER__
#define DBG_MSG(...) \
    DBG_MSG_COUNTER(__COUNTER__, __VA_ARGS__)

#define DBG_MSG_COUNTER(id, ...)                                \
    do {                                                         \
        static unsigned long long DBG_CAT(dbg_msg_count_, id);  \
        unsigned long long dbg_count =                           \
            ++DBG_CAT(dbg_msg_count_, id);                      \
                                                                 \
        fprintf(                                                 \
            stderr,                                          \
            "%s:%u %s() [%llu, #%llu] ",                        \
            __FILE__,                                            \
            __LINE__,                                            \
            __func__,                                            \
            (unsigned long long)(id),                            \
            dbg_count                                            \
        );                                                       \
                                                                 \
        fprintf(stderr, __VA_ARGS__);                        \
        fputc('\n', stderr);                                 \
    } while (0)

#    define DBG_LOG(code) \
        DBG_LOG_COUNTER(__COUNTER__, code)

#    define DBG_LOG_COUNTER(id, code)                            \
        do {                                                     \
            static unsigned long long                           \
                DBG_CAT(dbg_log_count_, id);                     \
                                                                 \
            DBG_CAT(dbg_log_count_, id)++;                      \
                                                                 \
            dbg_log(                                             \
                (dbg_location){                                  \
                    .file     = __FILE__,                        \
                    .line     = __LINE__,                        \
                    .function = __func__                         \
                },                                               \
                (unsigned long long)(id),                        \
                DBG_CAT(dbg_log_count_, id),                      \
                #code                                            \
            );                                                   \
                                                                 \
            code                                                \
			fprintf(stderr, "SUCCESS\n");                         \
        } while (0);\


/*
 * Open form.
 *
 * Deliberately no do/while wrapper.
 */
#    define DBG_LOG_OPEN(code) \
        DBG_LOG_OPEN_COUNTER(__COUNTER__, code)

#    define DBG_LOG_OPEN_COUNTER(id, code)                       \
	do {                                                    \
		static unsigned long long                               \
		DBG_CAT(dbg_log_count_, id);                         \
		DBG_CAT(dbg_log_count_, id)++;                           \
		dbg_log(                                                 \
				(dbg_location){                                      \
				.file     = __FILE__,                            \
				.line     = __LINE__,                            \
				.function = __func__                             \
				},                                                   \
				(unsigned long long)(id),                            \
				DBG_CAT(dbg_log_count_, id),                        \
#code                                                \
				);                                                       \
	}while(0);                                                \
code                                                         \
	do {                                                   \
		fprintf(stderr, "SUCCESS\n");                         \
	} while(0);


#else

/*
 * Portable fallback.
 *
 * __LINE__ is not globally unique, but is stable for a particular
 * source location.
 */
#    define DBG_LOG(code) \
        DBG_LOG_LINE(__LINE__, code)

#    define DBG_LOG_LINE(line, code)                             \
        do {                                                     \
            static unsigned long long                           \
                DBG_CAT(dbg_log_count_, line);                   \
                                                                 \
            DBG_CAT(dbg_log_count_, line)++;                     \
                                                                 \
            dbg_log(                                             \
                (dbg_location){                                  \
                    .file     = __FILE__,                        \
                    .line     = __LINE__,                        \
                    .function = __func__                         \
                },                                               \
                (unsigned long long)(line),                      \
                #code                                            \
            );                                                   \
                                                                 \
            code;                                                \
        } while (0)


#    define DBG_LOG_OPEN(code) \
        DBG_LOG_OPEN_LINE(__LINE__, code)

#    define DBG_LOG_OPEN_LINE(line, code)                        \
        static unsigned long long                               \
            DBG_CAT(dbg_log_count_, line);                       \
        DBG_CAT(dbg_log_count_, line)++;                         \
        dbg_log(                                                 \
            (dbg_location){                                      \
                .file     = __FILE__,                            \
                .line     = __LINE__,                            \
                .function = __func__                             \
            },                                                   \
            (unsigned long long)(line),                          \
            #code                                                \
        );                                                       \
        code

#endif


/* ================================================================
 * Implementation
 * ================================================================ */

#ifdef DBG_IMPLEMENTATION


//FILE *stderr = NULL;


/*
 * Initialize lazily so that merely including the header is enough
 * to get useful behavior.
 */
static FILE *
dbg_stream(void) {
    return stderr ? stderr : stderr;
}


/*
 * Print raw object representation.
 */
static void dbg_print_bytes(const void *address, size_t size) {
    const unsigned char *bytes = address;

    fprintf(stderr, "    bytes:");

    for (size_t i = 0; i < size; ++i)
        fprintf(stderr, " %02X", bytes[i]);

    fputc('\n', stderr);
}


/*
 * Object inspection.
 *
 * Formatting of the actual value is intentionally NOT performed
 * here because the original static C type is no longer available.
 */
void dbg_track(dbg_location location, const dbg_object *object) {
    FILE *stream = stderr;

    fprintf(
        stream,
        "%s:%u %s(): %s\n",
        location.file,
        location.line,
        location.function,
        object->name
    );

    fprintf(
        stream,
        "    type:    %s\n"
        "    size:    %zu\n"
        "    address: %p\n",
        object->type_name,
        object->size,
        object->address
    );

    dbg_print_bytes(object->address, object->size);
}


/*
 * Log execution event.
 *
 * The per-site counter itself lives at the macro call site. This
 * function receives the site's current identity and is responsible
 * only for presenting the event.
 */

void dbg_log(
    dbg_location location,
    unsigned long long site,
    unsigned long long count,
    const char *code
) {
    fprintf(
        stderr,
        "%s:%u %s() [log %llu, #%llu]: %s\n",
        location.file,
        location.line,
        location.function,
        site,
        count,
        code
    );
}

#endif /* DBG_IMPLEMENTATION */

#endif /* DBG_H */
