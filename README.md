[![eHonnef - cpp-project](https://img.shields.io/static/v1?label=eHonnef&message=cpp-project&color=blue&logo=github)](https://github.com/eHonnef/cpp-project "Go to GitHub repo")
[![stars - cpp-project](https://img.shields.io/github/stars/eHonnef/cpp-project?style=social)](https://github.com/eHonnef/cpp-project)
[![forks - cpp-project](https://img.shields.io/github/forks/eHonnef/cpp-project?style=social)](https://github.com/eHonnef/cpp-project)
<div align="center">

[![Use this template](https://img.shields.io/badge/Generate-Use_this_template-2ea44f?style=for-the-badge)](https://github.com/eHonnef/cpp-project/generate)

</div>


# C++ Project starter

This is a modified fork from [Boiler plate for C++ projects](https://github.com/bsamseth/cpp-project), the basic features remains the same:

- Sources, headers and mains separated in distinct folders;
- Use of modern CMake for much easier compiling;
- Setup for tests using [doctest](https://github.com/onqtam/doctest);
- Continuous testing with GitHub Actions, with support for C++17;
- Code documentation with [Doxygen](http://www.stack.nl/~dimitri/doxygen/);

## Structure

Change the `PROJECT NAME` folder, or the `sample` folder in this example, and then use as your include folder. This way is better to 3rd party to include your project.

``` text
.
├── CMakeLists.txt
├── app
│   └── main.cpp
├── include
│   └── <PROJECT NAME>
│        └── example.h
├── src
│   └── example.cpp
└── tests
    ├── dummy.cpp
    └── main.cpp
```

Sources files go in [src/](src/), headers go in [include/\<PROJECT NAME>](include/sample/), main files go in [app/](app/) and tests go in [tests/](tests/) (compiled to `unit_tests` by default).

If you add a new executable, say `app/hello.cpp`, you only need to add the following two lines to [CMakeLists.txt](CMakeLists.txt):

```cmake
add_executable(hello app/hello.cpp)   # Name of exec. and location of file.
target_link_libraries(hello PRIVATE ${LIBRARY_NAME} ${lst_external})  # Link the executable to lib built from src/*.cpp (if it uses it).
```

You can find the example source code that builds the `main` executable in [app/main.cpp](app/main.cpp) under the `Build` section in [CMakeLists.txt](CMakeLists.txt).
If the executable you made does not use the library in [src/](src), then only the first line is needed.

## Building

Build by making a build directory (i.e. `build/`), run `cmake` in that dir, and then use `make` to build the desired target.

Example:

``` bash
> mkdir build && cd build
> cmake .. -DCMAKE_BUILD_TYPE=[Debug | Coverage | Release]
> make
> ./main
> make test      # Makes and runs the tests.
> make coverage  # Generate a coverage report.
> make doc       # Generate html documentation.
```

Also, you can simply load the [vscode workspace file](.vscode/workspace.code-workspace), then [vscode](https://code.visualstudio.com/) will handle everything (of course, if you have the [C++ environment](https://code.visualstudio.com/docs/cpp/cmake-linux) set in the editor). Just run:

```bash
code .vscode/workspace.code-workspace
```

## .gitignore

The [.gitignore](.gitignore) file is a copy of the [Github C++.gitignore file](https://github.com/github/gitignore/blob/master/C%2B%2B.gitignore),
with the addition of ignoring the build directory (`build/`).

## Setup

Change the configurations and the names in the [CMakeLists.txt](CMakeLists.txt) file. I commented a `<Change>` flag where you are suppose to or can change.

If you want to generate some badges to put on top of the readme of your project, you can use [MichaelCurrin's badge generator](https://michaelcurrin.github.io/badge-generator/#/).

### For single header libraries

Just set the flag `SINGLE_HEADER` to `ON`, and then you can remove the `src/` folder, you'll only use the `include/` folder.

### Using the GitHub template

Click the `Use this template` button to make a new repository from this template.

## What changes?

I removed the Travis-CI, Appveyor, Coveralls and Codecov. Personally, I don't use them, but you can include without a problem.

Also, I changed the CMakeFiles in the way that you can include using [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html), using the `MainProject` context. So this way is easier for 3rd parties to include your code... if it is the case.

With this modification there is no longer "insource" unit tests, but you can include without a problem. Keep in mind that it'll broke if your project is included as a subproject (or a dependency). Just include this lines to "enable" the feature:

```cpp
#ifdef ENABLE_DOCTEST_IN_LIBRARY
#include "doctest/doctest.h"
TEST_CASE("we can have tests in headers if we want") {
  Dummy d;
  CHECK(d.doSomething() == true);
}
#endif
```

And this in the main files:

```cpp
// Executables must have the following defined if the library contains
// doctest definitions. For builds with this disabled, e.g. code shipped to
// users, this can be left out.
#ifdef ENABLE_DOCTEST_IN_LIBRARY
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest/doctest.h"
#endif
```

## License

Released under [MIT](/LICENSE) by [@ehonnef](https://github.com/ehonnef).
