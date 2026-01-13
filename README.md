# AutoNex

## A template library collection in C++20 providing modern and flexible implementation of state-machine, event dispatcher, object pool, timer, etc. for creating engines

This library is a work in progress. New functionalities will be added via separate branches until they get merged with the master branch.

### How to use the library

Copy the [`include`](./include/) folder into your project. Add the folder to your project's include directory. Then you can use `#include "autonex/<library_name>.hpp"` to use any library.

### Dependencies
As of now, the library does not have any external dependency. Only C++20 and its STL are required.

### Testing
While the functionalities of the libraries are tested, I cannot promise that bugs or memory leaks are non-existent. The tests are initially performed in Visual Studio 2022 with both MSVC and Clang. Then they are tested in Linux systems with GCC.