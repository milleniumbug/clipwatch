#include "../include/clipwatch.h"

#define _GNU_SOURCE
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define STATUS_STOPPED 1
#define STATUS_RUNNING 2
#define STATUS_TO_BE_DISPOSED 3
#define STATUS_FAILURE 4

struct Clipwatch_ClipWatcher
{
    void *userData;

    void (*userDataDeleter)(void *);

    void (*clipboardEventHandler)(const char *, size_t, void *);

    char errorMessage[512];

    int status;

    pthread_t listeningThread;

    int readPipeFd;
    int writePipeFd;
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

static int Clipboard_SendClipboardContentsToCallback(Clipwatch_ClipWatcher *handle, char** buffer, size_t *length)
{
    FILE *f;
    if ((f = popen("xsel -b", "r")) != NULL)
    {
        size_t readOffset = 0;
        size_t readLength = 0;
        while ((readLength = fread(*buffer + readOffset, 1, (*length) - readOffset, f)) != 0)
        {
            if (readOffset + readLength == (*length))
            {
                void *newBuf = reallocate(*buffer, length);
                if (!newBuf)
                {
                    strcpy(handle->errorMessage, "Could not allocate memory");
                    return -1;
                }
                *buffer = newBuf;
            }
            readOffset += readLength;
        }
        pclose(f);
        handle->clipboardEventHandler(*buffer, readOffset, handle->userData);
    }

    return 0;
}

static void* Clipboard_ListeningWorker(void *rawHandle)
{
    Clipwatch_ClipWatcher *handle = (Clipwatch_ClipWatcher *) rawHandle;
    Display *display = XOpenDisplay(NULL);
    if (!display)
    {
        strcpy(handle->errorMessage, "Can't open X display");
        handle->status = STATUS_FAILURE;
        return NULL;
    }

    Window rootWindow = DefaultRootWindow(display);

    Atom clipboardAtom = XInternAtom(display, "CLIPBOARD", False);

    XEvent evt;

    XFixesSelectSelectionInput(display, rootWindow, clipboardAtom, XFixesSetSelectionOwnerNotifyMask);

    size_t length = 512;
    char *buffer = malloc(length);

    if (!buffer)
    {
        strcpy(handle->errorMessage, "Could not initialize buffer");
        return NULL;
    }

    int x11_fd = ConnectionNumber(display);
    fd_set in_fds;

    XFlush(display);

    while (1)
    {
        FD_ZERO(&in_fds);
        FD_SET(x11_fd, &in_fds);
        FD_SET(handle->readPipeFd, &in_fds);
        int maxFd = -1;
        maxFd = maxFd < x11_fd ? x11_fd : maxFd;
        maxFd = maxFd < handle->readPipeFd ? handle->readPipeFd : maxFd;

        int readyFds = select(maxFd + 1, &in_fds, NULL, NULL, NULL);
        if(readyFds == -1)
        {
            strerror_r(errno, handle->errorMessage, sizeof handle->errorMessage);
            break;
        }
        if(FD_ISSET(x11_fd, &in_fds))
        {
            while(XPending(display))
            {
                XNextEvent(display, &evt);
            }
        }
        if(FD_ISSET(handle->readPipeFd, &in_fds))
        {
            int status;
            if(read(handle->readPipeFd, &status, sizeof status) != sizeof status)
            {
                handle->status = STATUS_FAILURE;
            }
            else
            {
                handle->status = status;
            }
        }

        if (handle->status == STATUS_STOPPED)
        {
            continue;
        }

        if (handle->status == STATUS_TO_BE_DISPOSED)
        {
            break;
        }

        if(handle->status == STATUS_RUNNING && FD_ISSET(x11_fd, &in_fds))
        {
            int result = Clipboard_SendClipboardContentsToCallback(handle, &buffer, &length);
            if (result == -1)
            {
                break;
            }
        }
    }

    XCloseDisplay(display);
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
        goto handle_alloc_failure;
    }

    handle->userData = userData;
    handle->userDataDeleter = userDataDeleter;
    handle->clipboardEventHandler = clipboardEventHandler;
    handle->errorMessage[0] = '\0';
    handle->status = STATUS_STOPPED;

    int result_code;

    int pipefd[2];
    result_code = pipe2(pipefd, O_CLOEXEC);
    if (result_code == -1)
    {
        goto pipe_init_failure;
    }
    handle->readPipeFd = pipefd[0];
    handle->writePipeFd = pipefd[1];


    result_code = pthread_create(&handle->listeningThread, NULL, Clipboard_ListeningWorker, handle);
    if (result_code == -1)
    {
        goto thread_init_failure;
    }

    return handle;

    thread_init_failure:
    close(handle->readPipeFd);
    close(handle->writePipeFd);
    pipe_init_failure:
    free(handle);
    handle_alloc_failure:
    return NULL;
}

void Clipwatch_Release(
        Clipwatch_ClipWatcher *handle)
{
    if (!handle)
    {
        return;
    }
    const int status = STATUS_TO_BE_DISPOSED;
    write(handle->writePipeFd, &status, sizeof(handle->status));

    void* result;
    pthread_join(handle->listeningThread, &result);
    if (handle->userDataDeleter)
    {
        handle->userDataDeleter(handle->userData);
    }
    free(handle);
}

void Clipwatch_Start(
        Clipwatch_ClipWatcher *handle)
{
    const int status = STATUS_RUNNING;
    write(handle->writePipeFd, &status, sizeof(handle->status));
}

void Clipwatch_Stop(
        Clipwatch_ClipWatcher *handle)
{
    const int status = STATUS_STOPPED;
    write(handle->writePipeFd, &status, sizeof(handle->status));
}