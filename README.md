# moment

A header only c++14 signal library. See [Signal.hpp](moment/include/moment/Signal.hpp).

Allows users to define signals that can be connected to via labdas or member functions. When a signal is called it notifies all connected slots.

## quickstart

Defining a signal and attaching a lambda to it:

```cpp
moment::Signal<void(const std::string&)> sig;
auto connection = sig.connect([](const std::string& out) { std::cout << out << std::endl; });
sig("Hello World!");

connection.disconnect(); // sig.disconnect(connection);
sig("Hey World!");
```

Output:

    Hello World!

A little more complex now, we can have a signal in a class and bind that signal to a member function of another class:

```cpp
struct Emitter {
    void event() { onEvent(); }

    moment::Signal<void()> onEvent;
};

struct Receiver {
    void onEvent() { std::cout << "Event Occured!" << std::endl; }
};

Emitter emitter;
Receiver receiver;
emitter.onEvent.connect(&receiver, std::mem_fn<void()>(&Receiver::onEvent));
emitter.event();
```

Output:

    Event Occured!

see [moment/src/main.cpp](moment/src/main.cpp) for more usage examples.

## building

### macos

```sh
brew install CMake Ninja
brew install --with-toolchain llvm
```

#### build tests and examples

```sh
mkdir out/
cd out/
cmake .. -G Ninja
ninja
```

#### tests

After the build step:

```sh
ninja test
# or
ctest
```

#### run examples

```sh
./bin/moment
```
