#include "../include/clipwatch.h"
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <string_view>

struct ClipwatchDeleter
{
    void operator()(Clipwatch_ClipWatcher *handle) const
    {
        Clipwatch_Release(handle);
    }
};

void clipboardEventHandler(const char *text, size_t length, void *userData)
{
    std::cout << "EVENT: " << std::string_view(text, length) << "\n";
}

int main()
{
    using namespace std::chrono_literals;

    std::unique_ptr<Clipwatch_ClipWatcher, ClipwatchDeleter> handle(Clipwatch_Init(
            2000,
            clipboardEventHandler,
            NULL,
            NULL));

    Clipwatch_Start(handle.get());

    std::this_thread::sleep_for(20000s);
}