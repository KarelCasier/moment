#pragma once

#include <functional>
#include <mutex>
#include <vector>

#include <signals/NonCopyable.hpp>

/// The follwoing structs allow for variable number of placeholders in a std::bind call based off the number of
/// parameters.
/// [[[ PlaceholderTemplate ---------------------------------------------------

/// struct to aid in std::placeholder expansion
/// @note Used to convert a size_t to a std::placeholder value
template <std::size_t>
struct PlaceholderTemplate {
};

namespace std {
/// is_placeholder template specialization to convert an index to a placeholder type.
/// @note The off by one is since std::placeholders are 1 indexed not 0 indexed.
template <std::size_t N>
struct is_placeholder<PlaceholderTemplate<N>> : integral_constant<std::size_t, N + 1> {
};
} // namespace std

/// ]]] placeholderTemplate ---------------------------------------------------

namespace moment {

/// Generic template declarations
template <typename>
class Connection;

template <typename>
class Signal;

/// [[[ Signal ----------------------------------------------------------------

template <typename Ret, typename... Params>
class Signal<Ret(Params...)> : public NonCopyable {
public:
    using Slot = std::function<Ret(Params...)>;

    ~Signal();

    template <typename Obj, typename MemFunc>
    Connection<Ret(Params...)> connect(Obj* object, MemFunc Obj::*memFunc);
    Connection<Ret(Params...)> connect(Slot&& slot);

    bool disconnect(Connection<Ret(Params...)>& connection);

    template <typename... Args>
    void operator()(Args...);

private:
    using StateLock = std::lock_guard<std::mutex>;

    template <typename Obj, typename MemFunc, std::size_t... Indices>
    Connection<Ret(Params...)> connectBind(Obj* object, MemFunc Obj::*memFunc, std::index_sequence<Indices...>);
    Connection<Ret(Params...)> connectLocked(StateLock&, Slot&& slot);

    bool disconnectLocked(StateLock&, Connection<Ret(Params...)>& connection);

    mutable std::mutex _stateMutex;
    std::vector<Connection<Ret(Params...)>> _connections;
};

template <typename Ret, typename... Params>
inline Signal<Ret(Params...)>::~Signal()
{
    StateLock lock{_stateMutex};
    for (auto& connection : _connections) {
        connection.invalidate();
    }
    _connections.clear();
}

template <typename Ret, typename... Params>
template <typename... Args>
inline void Signal<Ret(Params...)>::operator()(Args... args)
{
    StateLock lock{_stateMutex};
    for (auto& connection : _connections) {
        connection.call(std::forward<Args>(args)...);
    }
}

template <typename Ret, typename... Params>
template <typename Obj, typename MemFunc>
inline Connection<Ret(Params...)> Signal<Ret(Params...)>::connect(Obj* object, MemFunc Obj::*memFunc)
{
    return connectBind(object, memFunc, std::make_index_sequence<sizeof...(Params)>{});
}

template <typename Ret, typename... Params>
template <typename Obj, typename MemFunc, std::size_t... Indices>
inline Connection<Ret(Params...)> Signal<Ret(Params...)>::connectBind(Obj* object,
                                                                      MemFunc Obj::*memFunc,
                                                                      std::index_sequence<Indices...>)
{
    return connect(std::bind(memFunc, object, PlaceholderTemplate<Indices>{}...));
}

template <typename Ret, typename... Params>
inline Connection<Ret(Params...)> Signal<Ret(Params...)>::connect(Slot&& slot)
{
    StateLock lock{_stateMutex};
    return connectLocked(lock, std::move(slot));
}

template <typename Ret, typename... Params>
inline bool Signal<Ret(Params...)>::disconnect(Connection<Ret(Params...)>& connection)
{
    StateLock lock{_stateMutex};
    return disconnectLocked(lock, connection);
}

template <typename Ret, typename... Params>
inline bool Signal<Ret(Params...)>::disconnectLocked(StateLock&, Connection<Ret(Params...)>& connection)
{
    auto I = std::find(std::begin(_connections), std::end(_connections), connection);
    if (I != std::end(_connections)) {
        connection.invalidate();
        _connections.erase(I);
        return true;
    }
    return false;
}

template <typename Ret, typename... Params>
inline Connection<Ret(Params...)> Signal<Ret(Params...)>::connectLocked(StateLock&, Slot&& slot)
{
    static uint32_t id = 0u;
    auto I = _connections.emplace_back(this, std::move(slot), id++);
    return I;
}

/// ]]] Signal ----------------------------------------------------------------

/// ]]] Connection ------------------------------------------------------------

template <typename Ret, typename... Params>
class Connection<Ret(Params...)> {
public:
    using SlotProto = Ret(Params...);
    using Slot = std::function<Ret(Params...)>;

    bool operator==(const Connection&) const;
    Connection(const Connection& other);
    Connection(Connection&& other);
    Connection& operator=(const Connection& other);
    Connection& operator=(Connection&& other);

    bool disconnect();
    bool valid() const;

    Connection(Signal<SlotProto>* signal, Slot&& slot, uint32_t id);

private:
    friend class Signal<SlotProto>;

    template <typename... Args>
    void call(Args... args);

    void invalidate();

    class SharedConnectionData {
    public:
        SharedConnectionData(Slot&& slot);

        template <typename... Args>
        void call(Args... args);

        bool valid() const;

        void invalidate();

    private:
        using StateLock = std::lock_guard<std::mutex>;

        mutable std::mutex _stateMutex;
        bool _valid{true};
        Slot _slot;
    };

    std::shared_ptr<SharedConnectionData> _sharedConnectionData;
    Signal<SlotProto>* _signal;
    uint32_t _id;
};

template <typename Ret, typename... Params>
inline Connection<Ret(Params...)>::Connection(Signal<SlotProto>* signal, Slot&& slot, uint32_t id)
: _sharedConnectionData{std::make_shared<SharedConnectionData>(std::move(slot))}
, _signal(signal)
, _id{id}
{
}

template <typename Ret, typename... Params>
inline bool Connection<Ret(Params...)>::operator==(const Connection& other) const
{
    return _id == other._id;
}

template <typename Ret, typename... Params>
inline Connection<Ret(Params...)>::Connection(const Connection& other)
: _sharedConnectionData{other._sharedConnectionData}
, _signal{other._signal}
, _id{other._id}
{
}

template <typename Ret, typename... Params>
inline Connection<Ret(Params...)>::Connection(Connection&& other)
: _sharedConnectionData{std::move(other._sharedConnectionData)}
, _signal{other._signal}
, _id{other._id}
{
}

template <typename Ret, typename... Params>
inline Connection<Ret(Params...)>& Connection<Ret(Params...)>::operator=(const Connection& other)
{
    _sharedConnectionData = other._sharedConnectionData;
    _signal = other._signal;
    _id = other._id;
    return *this;
}

template <typename Ret, typename... Params>
inline Connection<Ret(Params...)>& Connection<Ret(Params...)>::operator=(Connection&& other)
{
    _sharedConnectionData = std::move(other._sharedConnectionData);
    _signal = other._signal;
    _id = other._id;
    return *this;
}

template <typename Ret, typename... Params>
inline bool Connection<Ret(Params...)>::disconnect()
{
    return _signal->disconnect(*this);
}

template <typename Ret, typename... Params>
inline bool Connection<Ret(Params...)>::valid() const
{
    return _sharedConnectionData->valid();
}

template <typename Ret, typename... Params>
template <typename... Args>
inline void Connection<Ret(Params...)>::call(Args... args)
{
    _sharedConnectionData->call(std::forward<Args>(args)...);
}

template <typename Ret, typename... Params>
inline void Connection<Ret(Params...)>::invalidate()
{
    _sharedConnectionData->invalidate();
}

template <typename Ret, typename... Params>
inline Connection<Ret(Params...)>::SharedConnectionData::SharedConnectionData(Slot&& slot)
: _slot{std::move(slot)}
{
}

template <typename Ret, typename... Params>
template <typename... Args>
inline void Connection<Ret(Params...)>::SharedConnectionData::call(Args... args)
{
    StateLock lock{_stateMutex};
    _slot(std::forward<Args>(args)...);
}

template <typename Ret, typename... Params>
inline bool Connection<Ret(Params...)>::SharedConnectionData::valid() const
{
    StateLock lock{_stateMutex};
    return _valid;
}

template <typename Ret, typename... Params>
inline void Connection<Ret(Params...)>::SharedConnectionData::invalidate()
{
    StateLock lock{_stateMutex};
    _valid = false;
}

/// [[[ Connection ------------------------------------------------------------

} // namespace moment
