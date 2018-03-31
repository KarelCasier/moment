#include <iostream>

#include <signals/Signal.hpp>

class Test {
public:
    void callback(int x) {
        std::cout << __func__ << " " << x << std::endl;
    }

};

int main(int /*argv*/, char** /*argc*/)
{
    moment::Signal<void(int)> sig;
    auto connection = sig.connect([](int x){ std::cout << "Callback: " << x << std::endl; });
    sig.connect([](int x){ std::cout << "Callback2: " << x << std::endl; });
    Test test{};
    sig.connect(&test, &Test::callback);
    sig(5);
    std::cout << "here" << std::endl;
    return 0;
}
