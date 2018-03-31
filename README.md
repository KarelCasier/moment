# moment

A header only c++14 signal library.

Allows users to define signals that can be connected to via labdas or member functions. When a signal is called it notifies all connected slots.

## quickstart

see [moment/src/main.cpp](moment/src/main.cpp) for usage examples.

## building

### macos

```sh
brew install CMake Ninja
brew install llvm
```

#### build

```sh
mkdir out/
cd out/
cmake .. -G Ninja
ninja
```

#### run

```sh
./bin/moment
```
