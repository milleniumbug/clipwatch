#include "../include/clipwatch.h"

#ifndef _WIN32

#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define STATUS_STOPPED 1
#define STATUS_RUNNING 2
#define STATUS_TO_BE_DISPOSED 3

struct Clipwatch_ClipWatcher
{
    void *userData;

    void (*userDataDeleter)(void *);

    void (*clipboardEventHandler)(const char *, size_t, void *);

    const char *errorMessage;

    int status;

    pthread_t listeningThread;
};

static void *reallocate(void *buffer, size_t *length)
{
    size_t newLen = *length * 2 + 1;
    void *newBuf = realloc(buffer, newLen);
    if (newBuf)
    {
        *length = newLen;
    }

    return newBuf;
}

static void* Clipboard_ListeningWorker(void *rawHandle)
{
    Clipwatch_ClipWatcher *handle = (Clipwatch_ClipWatcher *) rawHandle;
    Display *disp = XOpenDisplay(NULL);
    if (!disp)
    {
        handle->errorMessage = "Can't open X display";
        return NULL;
    }

    Window root = DefaultRootWindow(disp);

    Atom clip = XInternAtom(disp, "CLIPBOARD", False);

    XEvent evt;

    XFixesSelectSelectionInput(disp, root, clip, XFixesSetSelectionOwnerNotifyMask);

    char *buffer = NULL;
    size_t length = 512;

    buffer = malloc(length);
    if (!buffer)
    {
        handle->errorMessage = "Could not allocate memory";
        return NULL;
    }

    handle->status = STATUS_RUNNING;

    while (handle->status == STATUS_RUNNING)
    {
        XNextEvent(disp, &evt);

        if (handle->status == STATUS_STOPPED)
        {
            continue;
        }

        if (handle->status == STATUS_TO_BE_DISPOSED)
        {
            break;
        }

        FILE *f;
        if ((f = popen("xsel -b", "r")) != NULL)
        {
            size_t readOffset = 0;
            size_t readLength = 0;
            while ((readLength = fread(buffer + readOffset, 1, length - readOffset, f)) != 0)
            {
                if (readOffset + readLength == length)
                {
                    void *newBuf = reallocate(buffer, &length);
                    if (!newBuf)
                    {
                        handle->errorMessage = "Could not allocate memory";
                        break;
                    }
                    buffer = newBuf;
                }
                readOffset += readLength;
            }
            pclose(f);
            handle->clipboardEventHandler(buffer, readOffset, handle->userData);
        }
    }

    XCloseDisplay(disp);
    free(buffer);

    return NULL;
}

Clipwatch_ClipWatcher *Clipwatch_Init(
        int pollingIntervalInMs,
        void (*clipboardEventHandler)(const char *, size_t, void *),
        void *userData,
        void (*userDataDeleter)(void *))
{
    (void) pollingIntervalInMs; // unused
    Clipwatch_ClipWatcher *handle = malloc(sizeof *handle);
    if (!handle)
    {
        return NULL;
    }

    handle->userData = userData;
    handle->userDataDeleter = userDataDeleter;
    handle->clipboardEventHandler = clipboardEventHandler;
    handle->errorMessage = NULL;
    handle->status = STATUS_STOPPED;

    int result_code = pthread_create(&handle->listeningThread, NULL, Clipboard_ListeningWorker, handle);

    return handle;
}

void Clipwatch_Release(
        Clipwatch_ClipWatcher *handle)
{
    if (!handle)
    {
        return;
    }
    handle->status = STATUS_TO_BE_DISPOSED;

    pthread_cancel(handle->listeningThread);
    if (handle->userDataDeleter)
    {
        handle->userDataDeleter(handle->userData);
    }
    free(handle);
}

void Clipwatch_Start(
        Clipwatch_ClipWatcher *handle)
{
    handle->status = STATUS_RUNNING;
}

void Clipwatch_Stop(
        Clipwatch_ClipWatcher *handle)
{
    handle->status = STATUS_STOPPED;
}

#endif