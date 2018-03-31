#include <iostream>
#include <sstream>
#include <cassert>
#include <vector>
#include <functional>

template <typename Proto>
class Connection;

template <typename T>
bool compareSharedPtr(const std::shared_ptr<T>& lhs, const std::shared_ptr<T>& rhs)
{
    if (lhs == rhs) {
        return true;
    }
    if (lhs && rhs) {
        return *lhs == *rhs;
    }
    return false;
}

template <typename Proto>
class Signal {
public:
    using Callback = std::function<Proto>;

    ~Signal();

    Connection<Proto> connect(Callback&& callback);
    bool disconnect(Connection<Proto>& connection);

    template <typename... Args>
    void operator()(Args...);

private:
    using StateLock = std::lock_guard<std::mutex>;

    bool disconnectLocked(StateLock&, Connection<Proto>& connection);

    mutable std::mutex _stateMutex;
    std::vector<Connection<Proto>> _connections;
};

template <typename Proto>
Signal<Proto>::~Signal()
{
    StateLock lock{_stateMutex};
    for (auto& connection : _connections) {
        disconnectLocked(lock, connection);
    }
}

template <typename Proto>
template <typename... Args>
void Signal<Proto>::operator()(Args... args)
{
    StateLock lock{_stateMutex};
    for (auto& connection : _connections) {
        connection.call(std::forward<Args>(args)...);
    }
}

template <typename Proto>
Connection<Proto> Signal<Proto>::connect(Callback&& callback)
{
    StateLock lock{_stateMutex};
    static uint32_t id = 0u;
    auto I = _connections.emplace_back(this, std::move(callback), id++);
    return I;
}

template <typename Proto>
bool Signal<Proto>::disconnect(Connection<Proto>& connection)
{
    StateLock lock{_stateMutex};
    return disconnectLocked(lock, connection);
}

template <typename Proto>
bool Signal<Proto>::disconnectLocked(StateLock&, Connection<Proto>& connection)
{
    auto I = std::find(std::begin(_connections), std::end(_connections), connection);
    if (I != std::end(_connections)) {
        connection.invalidate();
        _connections.erase(I);
        return true;
    }
    return false;
}

//////////////////////////////////

template <typename Proto>
class Connection {
public:
    bool operator==(const Connection&) const;
    Connection(const Connection& other);
    Connection(Connection&& other);
    Connection& operator=(const Connection& other);
    Connection& operator=(Connection&& other);

    bool disconnect();
    bool valid() const;

    Connection(Signal<Proto>* signal, std::function<Proto> callback, uint32_t id);
private:
    friend class Signal<Proto>;

    template <typename... Args>
    void call(Args... args);

    void invalidate();

    class SharedConnectionData {
    public:
        SharedConnectionData(std::function<Proto>&& callback);

        template <typename... Args>
        void call(Args... args);

        bool valid() const;

        void invalidate();

    private:
        using StateLock = std::lock_guard<std::mutex>;

        mutable std::mutex _stateMutex;
        bool _valid{true};
        std::function<Proto> _callback;
    };

    std::shared_ptr<SharedConnectionData> _sharedConnectionData;
    Signal<Proto>* _signal;
    uint32_t _id;
};

template <typename Proto>
Connection<Proto>::Connection(Signal<Proto>* signal, std::function<Proto> callback, uint32_t id)
: _sharedConnectionData{std::make_shared<SharedConnectionData>(std::move(callback))}
, _signal(signal)
, _id{id}
{
}

template <typename Proto>
bool Connection<Proto>::operator==(const Connection& other) const
{
    return _id == other._id;
}

template <typename Proto>
Connection<Proto>::Connection(const Connection& other)
: _sharedConnectionData{other._sharedConnectionData}
, _signal{other._signal}
, _id{other._id}
{
}

template <typename Proto>
Connection<Proto>::Connection(Connection&& other)
: _sharedConnectionData{std::move(other._sharedConnectionData)}
, _signal{other._signal}
, _id{other._id}
{
}
template <typename Proto>
Connection<Proto>& Connection<Proto>::operator=(const Connection& other)
{
    _sharedConnectionData = other._sharedConnectionData;
    _signal = other._signal;
    _id = other._id;
    return *this;
}

template <typename Proto>
Connection<Proto>& Connection<Proto>::operator=(Connection&& other)
{
    _sharedConnectionData = std::move(other._sharedConnectionData);
    _signal = other._signal;
    _id = other._id;
    return *this;
}

template <typename Proto>
bool Connection<Proto>::disconnect()
{
    return _signal->disconnect(*this);
}

template <typename Proto>
bool Connection<Proto>::valid() const
{
    return _sharedConnectionData->valid();
}

template <typename Proto>
template <typename... Args>
void Connection<Proto>::call(Args... args)
{
    _sharedConnectionData->call(std::forward<Args>(args)...);
}

template <typename Proto>
void Connection<Proto>::invalidate()
{
    _sharedConnectionData->invalidate();
}

template <typename Proto>
Connection<Proto>::SharedConnectionData::SharedConnectionData(std::function<Proto>&& callback)
: _callback{std::move(callback)}
{
}

template <typename Proto>
template <typename... Args>
void Connection<Proto>::SharedConnectionData::call(Args... args)
{
    StateLock lock{_stateMutex};
    _callback(std::forward<Args>(args)...);
}

template <typename Proto>
bool Connection<Proto>::SharedConnectionData::valid() const
{
    StateLock lock{_stateMutex};
    return _valid;
}

template <typename Proto>
void Connection<Proto>::SharedConnectionData::invalidate()
{
    StateLock lock{_stateMutex};
    _valid = false;
}

int main(int /*argv*/, char** /*argc*/)
{
    Signal<void()> sig;
    auto connection = sig.connect([](){ std::cout << "Callback" << std::endl; });
    sig();
    return 0;
}
