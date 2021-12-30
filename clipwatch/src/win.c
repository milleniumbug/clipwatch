#define MODULE_API_EXPORTS

#include "../include/clipwatch.h"

#include <windows.h>
#include <process.h>
#include <combaseapi.h>

#define STATUS_STOPPED 1
#define STATUS_RUNNING 2
#define STATUS_TO_BE_DISPOSED 3

struct Clipwatch_ClipWatcher
{
    void* userData;
    void (*userDataDeleter)(void*);
    void (*clipboardEventHandler)(const char*, size_t, void*);
    const char* errorMessage;
    int status;
    uintptr_t threadHandle;
    HWND hwnd;
};

static char*
toUTF8(
    const wchar_t* src,
    size_t src_length,  /* = 0 */
    size_t* out_length  /* = NULL */
)
{
    if (!src)
    {
        return NULL;
    }

    if (src_length == 0) { src_length = wcslen(src); }
    int length = WideCharToMultiByte(CP_UTF8, 0, src, src_length,
        0, 0, NULL, NULL);
    char* output_buffer = (char*)malloc((length + 1) * sizeof(char));
    if (output_buffer) {
        WideCharToMultiByte(CP_UTF8, 0, src, src_length,
            output_buffer, length, NULL, NULL);
        output_buffer[length] = '\0';
    }
    if (out_length) { *out_length = length; }
    return output_buffer;
}

static LRESULT CALLBACK WindowsProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_CLIPBOARDUPDATE)
    {
        Clipwatch_ClipWatcher* handle = (Clipwatch_ClipWatcher*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        if (handle->status != STATUS_RUNNING)
        {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        if (!OpenClipboard(hwnd))
        {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        if (IsClipboardFormatAvailable(CF_UNICODETEXT))
        {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData != NULL)
            {
                wchar_t* text = GlobalLock(hData);
                if (text != NULL)
                {
                    size_t length;
                    char* translatedText = toUTF8(text, 0, &length);
                    handle->clipboardEventHandler(translatedText, length, handle->userData);
                    GlobalUnlock(hData);
                }
            }
        }

        CloseClipboard();
    }

    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

static unsigned __stdcall Clipboard_ListeningProc(void* rawHandle)
{
    Clipwatch_ClipWatcher* handle = (Clipwatch_ClipWatcher*)rawHandle;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    static const wchar_t* class_name = L"TheClipboardListener";
    WNDCLASSEXW wx = {0};
    wx.cbSize = sizeof(WNDCLASSEXW);
    wx.lpfnWndProc = WindowsProcedure;        // function which will handle messages
    wx.hInstance = GetModuleHandleW(NULL);
    wx.lpszClassName = class_name;
    if (!RegisterClassExW(&wx)) {
        _endthreadex(0);
        return 0;
    }
    HWND hWnd = CreateWindowExW(0, class_name, L"TheWindowName", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)handle);

    handle->hwnd = hWnd;

    MSG msg;
    BOOL bRet;

    bRet = AddClipboardFormatListener(hWnd);


    while ((bRet = GetMessage(&msg, hWnd, 0, 0)) != 0)
    {
        if (bRet == -1)
        {
            break;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    bRet = RemoveClipboardFormatListener(hWnd);

    _endthreadex(0);
    return 0;
}

CLIPWATCH_MODULE_API Clipwatch_ClipWatcher* Clipwatch_Init(
    int pollingIntervalInMs,
    void (*clipboardEventHandler)(const char*, size_t, void*),
    void* userData,
    void (*userDataDeleter)(void*))
{
    (void)pollingIntervalInMs; // unused
    Clipwatch_ClipWatcher* handle = malloc(sizeof *handle);
    if(!handle)
    {
        return NULL;
    }

    handle->userData = userData;
    handle->userDataDeleter = userDataDeleter;
    handle->clipboardEventHandler = clipboardEventHandler;
    handle->errorMessage = NULL;
    handle->status = STATUS_STOPPED;

    handle->threadHandle = _beginthreadex(NULL, 0, Clipboard_ListeningProc, handle, 0, NULL);

    return handle;
}

CLIPWATCH_MODULE_API void Clipwatch_Release(
    Clipwatch_ClipWatcher* handle)
{
    if (!handle)
    {
        return;
    }
    handle->status = STATUS_TO_BE_DISPOSED;
    DestroyWindow(handle->hwnd);

    if (handle->userDataDeleter)
    {
        handle->userDataDeleter(handle->userData);
    }
    free(handle);
}

CLIPWATCH_MODULE_API void Clipwatch_Start(
    Clipwatch_ClipWatcher* handle)
{
    handle->status = STATUS_RUNNING;
}

CLIPWATCH_MODULE_API void Clipwatch_Stop(
    Clipwatch_ClipWatcher* handle)
{
    handle->status = STATUS_STOPPED;
}