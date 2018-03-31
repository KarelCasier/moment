#pragma once

#include <functional>
#include <mutex>
#include <vector>

#include <signals/NonCopyable.hpp>

namespace moment {

template <typename Proto>
class Connection;

/// [[[ Signal ----------------------------------------------------------------

template <typename Proto>
class Signal : public NonCopyable {
public:
    using Slot = std::function<Proto>;

    ~Signal();

    Connection<Proto> connect(Slot&& slot);
    bool disconnect(Connection<Proto>& connection);

    template <typename... Args>
    void operator()(Args...);

private:
    using StateLock = std::lock_guard<std::mutex>;

    bool disconnectLocked(StateLock&, Connection<Proto>& connection);
    Connection<Proto> connectLocked(StateLock&, Slot&& slot);

    mutable std::mutex _stateMutex;
    std::vector<Connection<Proto>> _connections;
};

template <typename Proto>
inline Signal<Proto>::~Signal()
{
    StateLock lock{_stateMutex};
    for (auto& connection : _connections) {
        connection.invalidate();
    }
    _connections.clear();
}

template <typename Proto>
template <typename... Args>
inline void Signal<Proto>::operator()(Args... args)
{
    StateLock lock{_stateMutex};
    for (auto& connection : _connections) {
        connection.call(std::forward<Args>(args)...);
    }
}

template <typename Proto>
inline Connection<Proto> Signal<Proto>::connect(Slot&& slot)
{
    StateLock lock{_stateMutex};
    return connectLocked(lock, std::move(slot));
}

template <typename Proto>
inline bool Signal<Proto>::disconnect(Connection<Proto>& connection)
{
    StateLock lock{_stateMutex};
    return disconnectLocked(lock, connection);
}

template <typename Proto>
inline bool Signal<Proto>::disconnectLocked(StateLock&, Connection<Proto>& connection)
{
    auto I = std::find(std::begin(_connections), std::end(_connections), connection);
    if (I != std::end(_connections)) {
        connection.invalidate();
        _connections.erase(I);
        return true;
    }
    return false;
}

template <typename Proto>
inline Connection<Proto> Signal<Proto>::connectLocked(StateLock&, Slot&& slot)
{
    static uint32_t id = 0u;
    auto I = _connections.emplace_back(this, std::move(slot), id++);
    return I;
}

/// ]]] Signal ----------------------------------------------------------------

/// ]]] Connection ------------------------------------------------------------

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
inline Connection<Proto>::Connection(Signal<Proto>* signal, std::function<Proto> callback, uint32_t id)
: _sharedConnectionData{std::make_shared<SharedConnectionData>(std::move(callback))}
, _signal(signal)
, _id{id}
{
}

template <typename Proto>
inline bool Connection<Proto>::operator==(const Connection& other) const
{
    return _id == other._id;
}

template <typename Proto>
inline Connection<Proto>::Connection(const Connection& other)
: _sharedConnectionData{other._sharedConnectionData}
, _signal{other._signal}
, _id{other._id}
{
}

template <typename Proto>
inline Connection<Proto>::Connection(Connection&& other)
: _sharedConnectionData{std::move(other._sharedConnectionData)}
, _signal{other._signal}
, _id{other._id}
{
}
template <typename Proto>
inline Connection<Proto>& Connection<Proto>::operator=(const Connection& other)
{
    _sharedConnectionData = other._sharedConnectionData;
    _signal = other._signal;
    _id = other._id;
    return *this;
}

template <typename Proto>
inline Connection<Proto>& Connection<Proto>::operator=(Connection&& other)
{
    _sharedConnectionData = std::move(other._sharedConnectionData);
    _signal = other._signal;
    _id = other._id;
    return *this;
}

template <typename Proto>
inline bool Connection<Proto>::disconnect()
{
    return _signal->disconnect(*this);
}

template <typename Proto>
inline bool Connection<Proto>::valid() const
{
    return _sharedConnectionData->valid();
}

template <typename Proto>
template <typename... Args>
inline void Connection<Proto>::call(Args... args)
{
    _sharedConnectionData->call(std::forward<Args>(args)...);
}

template <typename Proto>
inline void Connection<Proto>::invalidate()
{
    _sharedConnectionData->invalidate();
}

template <typename Proto>
inline Connection<Proto>::SharedConnectionData::SharedConnectionData(std::function<Proto>&& callback)
: _callback{std::move(callback)}
{
}

template <typename Proto>
template <typename... Args>
inline void Connection<Proto>::SharedConnectionData::call(Args... args)
{
    StateLock lock{_stateMutex};
    _callback(std::forward<Args>(args)...);
}

template <typename Proto>
inline bool Connection<Proto>::SharedConnectionData::valid() const
{
    StateLock lock{_stateMutex};
    return _valid;
}

template <typename Proto>
inline void Connection<Proto>::SharedConnectionData::invalidate()
{
    StateLock lock{_stateMutex};
    _valid = false;
}

/// [[[ Connection ------------------------------------------------------------

} // namespace moment
