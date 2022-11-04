# template
A template for assignments and projects.
## Setup
```
mkdir cmake-build-debug
```

## Source Code Additions
Add your source files to the CMakeLists.txt:

```
set(HEADER_LIST [files])
set(SOURCE_LIST [files])
```

```
cmake -S . -B cmake-build-debug
```
or:

```
cmake -S . -B cmake-build-debug
```

## Build 
Examples:
```
cmake --build cmake-build-debug
cmake --build cmake-build-debug --clean-first
cmake --build cmake-build-debug --target docs
cmake --build cmake-build-debug --target format
```
