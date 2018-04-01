#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string>

#include <moment/Signal.hpp>

namespace {

using namespace testing;

/// Helper class to test lambdas which are not mockable in gtest
class ICallback {
public:
    virtual ~ICallback() = default;
    virtual void voidCallback() = 0;
    virtual void intAndStringCallback(int, std::string) = 0;
};

class MockCallback : public ICallback {
public:
    MOCK_METHOD0(voidCallback, void());
    MOCK_METHOD2(intAndStringCallback, void(int, std::string));
};

class MemberCallbacks {
public:
    void expectVoidCallback() { EXPECT_CALL(mockCallback, voidCallback()); }
    void expectIntAndStrngCallback(int arg1, std::string arg2)
    {
        EXPECT_CALL(mockCallback, intAndStringCallback(Eq(arg1), Eq(arg2)));
    }

    void voidCallback() { mockCallback.voidCallback(); }
    void intAndStringCallback(int arg1, std::string arg2) { mockCallback.intAndStringCallback(arg1, arg2); }

private:
    StrictMock<MockCallback> mockCallback;
};

} // namespace

namespace moment {

using namespace testing;

TEST(Signal, lambdaNoParams)
{
    /// Arrange
    Signal<void()> sig{};
    StrictMock<MockCallback> callback{};
    sig.connect([&callback]() { callback.voidCallback(); });

    /// Act
    EXPECT_CALL(callback, voidCallback());
    sig();

    /// Assert
}

TEST(Signal, lambdaWithParams)
{
    /// Arrange
    Signal<void(int, std::string)> sig{};
    StrictMock<MockCallback> callback{};
    sig.connect([&callback](int i, std::string s) { callback.intAndStringCallback(i, s); });
    auto arg1 = 5;
    auto arg2 = std::string{"Test"};

    /// Act
    EXPECT_CALL(callback, intAndStringCallback(Eq(arg1), Eq(arg2)));
    sig(arg1, arg2);

    /// Assert
}

TEST(Signal, memberFunctionNoParams)
{
    /// Arrange
    Signal<void()> sig{};
    MemberCallbacks callback{};
    sig.connect(&callback, &MemberCallbacks::voidCallback);

    /// Act
    callback.expectVoidCallback();
    sig();

    /// Assert
}

TEST(Signal, memberFunctionWithParams)
{
    /// Arrange
    Signal<void(int, std::string)> sig{};
    MemberCallbacks callback{};
    sig.connect(&callback, &MemberCallbacks::intAndStringCallback);
    auto arg1 = 5;
    auto arg2 = std::string{"Test"};

    /// Act
    callback.expectIntAndStrngCallback(arg1, arg2);
    sig(arg1, arg2);

    /// Assert
}

} // namespace moment

/// End Tests

int main(int argc, char** argv)
{
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
