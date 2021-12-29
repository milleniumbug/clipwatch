#ifndef CLIPWATCH_H_HEADER_DEFINED
#define CLIPWATCH_H_HEADER_DEFINED

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* dll support */
#ifdef _WIN32
#  ifdef MODULE_API_EXPORTS
#    define CLIPWATCH_MODULE_API __declspec(dllexport)
#  else
#    define CLIPWATCH_MODULE_API __declspec(dllimport)
#  endif
#else
#  define CLIPWATCH_MODULE_API
#endif

struct Clipwatch_ClipWatcher;

typedef struct Clipwatch_ClipWatcher Clipwatch_ClipWatcher;

// pollingIntervalInMs is only used as a fallback
// when clipboard watching is not available
CLIPWATCH_MODULE_API Clipwatch_ClipWatcher *Clipwatch_Init(
        int pollingIntervalInMs,
        void (*clipboardEventHandler)(const char *, size_t, void *),
        void *userData,
        void (*userDataDeleter)(void *));

CLIPWATCH_MODULE_API void Clipwatch_Release(
        Clipwatch_ClipWatcher *handle);

CLIPWATCH_MODULE_API void Clipwatch_Start(
        Clipwatch_ClipWatcher *handle);

CLIPWATCH_MODULE_API void Clipwatch_Stop(
        Clipwatch_ClipWatcher *handle);

#ifdef __cplusplus
}
#endif

#endif
