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

    std::cout << "Started at stopped state" << "\n";
    std::this_thread::sleep_for(5s);

    for(int i = 0; i < 3; i++)
    {
        Clipwatch_Start(handle.get());
        std::cout << "Started" << "\n";
        std::this_thread::sleep_for(4s);

        Clipwatch_Stop(handle.get());
        std::cout << "Stopped" << "\n";
        std::this_thread::sleep_for(4s);
    }
}