# Logger thread C++

This is a logger thread to log the program event. It uses my [thread wrapper](https://github.com/eHonnef/thread-cpp).

It can manage multiple files, also it bufferize the strings before writting to file to minimize the IO load.

## Usage

Very simple, just import it and be happy :)

You can tweak some stuff in the [LoggerDaemon file](include/LoggerThread/LoggerDaemon.cc):

- [c_nBufferSize](include/LoggerThread/LoggerDaemon.cc#L28): Set the buffer size, the [common agreement seems like to be 4096 bytes](https://stackoverflow.com/questions/1862982/c-sharp-filestream-optimal-buffer-size-for-writing-large-files);
- [c_nMaxIterations](include/LoggerThread/LoggerDaemon.cc#L186): Set the max iterations without writting to file before writting to file, in other words, if there is 50 iterations without writting to file, then it'll force write the buffer;
- [c_nThreadRateMs](include/LoggerThread/LoggerDaemon.cc#L187): Set the thread processing rate;

Then you can add a log file using the [AddLogFile](include/LoggerThread/LoggerDaemon.cc#L228) function, it'll register the file and return the file index inside the thread. So, when you are writting a log you call the `Write` functions with this index to indicate the file to be written.

Checkout [main](app/main.cpp) for an usage example.

## Include in my project

Using CMake's `FetchContent`:

```cmake
FetchContent_Declare(LoggerThread
  GIT_REPOSITORY "https://github.com/eHonnef/logger-thread"
  GIT_TAG "master"
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(LoggerThread)
target_link_libraries(${PROJECT_NAME} PUBLIC LoggerThread)
```

And then you can include the header:

```cpp
#include <LoggerThread/LoggerDaemon.cc>
```
