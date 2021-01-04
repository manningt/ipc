#ifdef __cplusplus
extern "C" {
#endif

#ifndef mylog_h
#define mylog_h

#include <stdlib.h>

#define LOG_CONFIG_PATH "/home/pi/zlog.conf"
#define LOG_STRING_LENGTH 81

//#define USE_ZLOG
#ifdef USE_ZLOG
#include "zlog.h"
//NOTE: I didn't know how to 'wrap' the zlog_category_t structure

// These macros allow changing to a different log facility (or no logging) with changing the code
#define LOG_INIT(...) zlog_init(__VA_ARGS__)
#define LOG_FINI(...) zlog_fini(__VA_ARGS__)
#define LOG_GET_CATEGORY(...) zlog_get_category(__VA_ARGS__)
#define LOG_DEBUG(...) zlog_debug(__VA_ARGS__)
#define LOG_INFO(...) zlog_info(__VA_ARGS__)
#define LOG_WARN(...) zlog_warn(__VA_ARGS__)
#define LOG_ERROR(...) zlog_error(__VA_ARGS__)
#define LOG_FATAL(...) zlog_fatal(__VA_ARGS__)

#define LOG_INIT_CATEGORY(category_name) \
    char *log_string = (char*)malloc(LOG_STRING_LENGTH * sizeof(char)); \
    zlog_category_t *module_category; \
    char *module_category_name = (char*)malloc(32 * sizeof(char)); \
    strncpy(module_category_name, __FILENAME__, (strlen(__FILENAME__)-2)); \
    module_category = LOG_GET_CATEGORY(module_category_name); \
    free(module_category_name); \
    if (!module_category) { \
        printf("get category '%s' failed\n", module_category_name); \
        LOG_FINI(); \
    }

#else
#define LOG_INIT(...)
#define LOG_FINI(...)
#define LOG_GET_CATEGORY(...) zlog_get_category(__VA_ARGS__)
#define LOG_DEBUG(category, message) printf("DEBUG: %s\n", message)
#define LOG_INFO(category, message) printf("INFO: %s\n", message)
#define LOG_WARNING(category, message) printf("WARN: %s\n", message)
#define LOG_ERROR(category, message) printf("ERROR: %s\n", message)
#define LOG_FATAL(category, message) printf("FATAL: %s\n", message)

#endif //USE_ZLOG

#endif //mylog_h


#ifdef __cplusplus
}
#endif
