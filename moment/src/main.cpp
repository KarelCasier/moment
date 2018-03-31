#include <iostream>

#include <moment/Signal.hpp>

class Emmiter {
public:
    ~Emmiter() { onDestroy(); }

    moment::Signal<void()> onDestroy;
};

class Receiver {
public:
    void emmiterDestroyed() { std::cout << "Emmiter::OnDestroy signal called." << std::endl; }
};

int main(int /*argv*/, char** /*argc*/)
{
    ///////////////////////////
    // Signal without params //
    ///////////////////////////
    moment::Signal<void()> signal;
    auto connection = signal.connect([]() { std::cout << "Hello World!" << std::endl; });
    signal(); // Labda is called

    connection.disconnect();
    signal(); // Lambda is not called

    ////////////////////////
    // Signal with params //
    ////////////////////////
    moment::Signal<void(std::string, std::string, int)> signalWithParam;
    signalWithParam.connect([](std::string left, std::string right, int times) {
        for (auto i{0}; i < times; ++i) {
            std::cout << left << " " << right << std::endl;
        }
    });
    signalWithParam.connect([](std::string right, std::string left, int times) {
        for (auto i{0}; i < times; ++i) {
            std::cout << left << " " << right << std::endl;
        }
    });
    signalWithParam("Left", "Right", 2); // Calls both lambdas

    ////////////////////////
    // Signal and classes //
    ////////////////////////
    Receiver receiver{};
    {
        Emmiter emmiter{};

        // Connect a lambda
        emmiter.onDestroy.connect([]() { std::cout << "Emmiter::OnDestroy signal called." << std::endl; });

        // Connect a member function
        emmiter.onDestroy.connect(&receiver, &Receiver::emmiterDestroyed);

    } // calls Emitter::onDestroy() thus calling both the lambda and the member function

    return 0;
}
