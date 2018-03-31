#include <iostream>

#include <moment/Signal.hpp>

class Emmiter {
public:
    ~Emmiter() { onDestroy(); }

    void print(int x) { onPrint(x); }

    moment::Signal<void()> onDestroy;
    moment::Signal<void(int)> onPrint;
};

class Receiver {
public:
    void onEmmiterDestroyed() { std::cout << "Emmiter::OnDestroy signal called." << std::endl; }

    void onPrint() { /* Never called */ }
    void onPrint(int x) { std::cout << x << std::endl; }
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
        emmiter.onDestroy.connect(&receiver, &Receiver::onEmmiterDestroyed);
    } // calls Emitter::onDestroy() thus calling both the lambda and the member function

    /////////////////////////////////////////////////////////////////////////////
    // Signal and classes                                                      //
    // connecting to a signal when a member function has overloaded parameters //
    /////////////////////////////////////////////////////////////////////////////
    Emmiter emmiter{};
    // Connect a member function that has overloads
    emmiter.onPrint.connect(&receiver, std::mem_fn<void(int)>(&Receiver::onPrint));
    emmiter.print(42); // Calls receiver.onPrint(int)

    return 0;
}
