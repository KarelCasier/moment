#pragma once

#include <functional>
#include <mutex>
#include <vector>
#include <iostream>

/// The follwoing structs allow for a variable number of placeholders in a
/// std::bind call based off the number of parameters to the signal.
/// [[[ PlaceholderTemplate ---------------------------------------------------

namespace moment {

/// struct to aid in std::placeholder expansion
/// @note Used to convert a size_t to a std::placeholder value
template <std::size_t>
struct PlaceholderTemplate {
};

} // namespace moment

namespace std {

/// is_placeholder template specialization to convert an index to a placeholder type.
/// @note The off by one is due to std::placeholders being 1 indexed rather 0 indexed.
template <std::size_t N>
struct is_placeholder<moment::PlaceholderTemplate<N>> : integral_constant<std::size_t, N + 1> {
};

} // namespace std

/// ]]] PlaceholderTemplate ---------------------------------------------------

namespace moment {

/// Generic template declarations. Signal and Connection are template specializations to allow for
/// Signal<void(int, int)> vs Signal<void, int, int>

template <typename>
class Signal;

template <typename>
class Connection;

/// [[[ Signal ----------------------------------------------------------------

/// A signal class that defines a callable function that will notify all connected slots.
/// @note Return values will be ignored
template <typename Ret, typename... Params>
class Signal<Ret(Params...)> {
public:
    using SlotProto = Ret(Params...);
    using Slot = std::function<SlotProto>;

    ~Signal();
    Signal() = default;

    /// Movable
    Signal(Signal&& other);
    Signal& operator=(Signal&& other);

    /// Non-copyable
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;

    /// Connect a member function via c function ptr to this signal.
    /// @tparam Obj The object type.
    /// @tparam MemFunc The member fuction type.
    /// @param object The object to bind to.
    /// @param memFunc The member function to bind to.
    /// @returns The connection created.
    template <typename Obj, typename MemFunc>
    Connection<Ret(Params...)> connect(Obj* object, MemFunc Obj::*memFunc);

    /// Connect a member function via std::mem_fn to this signal.
    /// @tparam Obj The object type.
    /// @tparam MemFunc The member fuction type.
    /// @param object The object to bind to.
    /// @param memFunc The member function to bind to.
    /// @returns The connection created.
    template <typename Obj, typename MemFunc>
    Connection<Ret(Params...)> connect(Obj* object, MemFunc&& memFunc);

    /// Connect a slot to this signal.
    /// @param slot The slot to connect the signal to.
    /// @returns The connection created.
    Connection<Ret(Params...)> connect(Slot&& slot);

    /// Disonnect a connection from this signal.
    /// @param connection The connection to disconnect from the signal.
    /// @returns True if disconnected, false otherwise.
    bool disconnect(Connection<SlotProto>& connection);

    /// Disonnect all connections from this signal.
    /// @returns True if disconnected, false otherwise.
    void disconnect();

    /// Emit the signal.
    /// @tparam Args The arguments to the slot
    template <typename... Args>
    void operator()(Args...);

private:
    using StateLock = std::lock_guard<std::mutex>;
    using ConnectionData = typename Connection<SlotProto>::ConnectionData;
    using SharedConnectionData = std::shared_ptr<ConnectionData>;

    /// Connect a member function
    template <typename Obj, typename MemFunc, std::size_t... Indices>
    Connection<SlotProto> connectBind(Obj* object, MemFunc Obj::*memFunc, std::index_sequence<Indices...>);
    /// Connect a member function
    template <typename Obj, typename MemFunc>
    Connection<SlotProto> connectBind(Obj* object, MemFunc&& memFunc);
    /// Connect a slot to this signal under a lock.
    /// @returns The connection created.
    Connection<SlotProto> connectLocked(const StateLock&, Slot&& slot);
    /// Disconnect a slot from this signal under a lock.
    /// @returns True if disconnected, false otherwise.
    bool disconnectLocked(const StateLock&, Connection<SlotProto>& connection);
    /// Disconnect all slots from this signal under a lock.
    void disconnectAllLocked(const StateLock&);
    /// Calls @p func with each connection.
    void forEachConnection(const StateLock&, std::function<void(ConnectionData&)>&& func);

    mutable std::mutex _stateMutex;
    std::vector<SharedConnectionData> _connections;
    bool _valid{true};
};

template <typename Ret, typename... Params>
inline Signal<Ret(Params...)>::~Signal()
{
    StateLock lock{_stateMutex};
    disconnectAllLocked(lock);
}

template <typename Ret, typename... Params>
inline Signal<Ret(Params...)>::Signal(Signal&& other)
{
    StateLock thisLock{_stateMutex};
    StateLock otherLock{other._stateMutex};
    disconnectAllLocked(thisLock);
    _connections = std::move(other._connections);
    other._connections.clear();
    other._valid = false;
    _valid = true;
    forEachConnection(thisLock, [this](auto& connection) { connection.updateSignal(this); });
}

template <typename Ret, typename... Params>
inline Signal<Ret(Params...)>& Signal<Ret(Params...)>::operator=(Signal&& other)
{
    StateLock thisLock{_stateMutex};
    StateLock otherLock{other._stateMutex};
    disconnectAllLocked(thisLock);
    _connections = std::move(other._connections);
    other._connections.clear();
    other._valid = false;
    _valid = true;
    forEachConnection(thisLock, [this](auto& connection) { connection.updateSignal(this); });
    return *this;
}

template <typename Ret, typename... Params>
template <typename... Args>
inline void Signal<Ret(Params...)>::operator()(Args... args)
{
    StateLock lock{_stateMutex};
    assert(_valid);
    forEachConnection(lock, [tup{std::make_tuple(std::forward<Args>(args)...)}](auto& connection) {
        std::apply([&connection](auto const&... args) { connection.call(args...); }, tup);
    });
}

template <typename Ret, typename... Params>
template <typename Obj, typename MemFunc>
inline Connection<Ret(Params...)> Signal<Ret(Params...)>::connect(Obj* object, MemFunc Obj::*memFunc)
{
    return connectBind(object, memFunc, std::make_index_sequence<sizeof...(Params)>{});
}

template <typename Ret, typename... Params>
template <typename Obj, typename MemFunc>
inline Connection<Ret(Params...)> Signal<Ret(Params...)>::connect(Obj* object, MemFunc&& memFunc)
{
    return connectBind(object, std::move(memFunc));
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
inline void Signal<Ret(Params...)>::disconnect()
{
    StateLock lock{_stateMutex};
    disconnectAllLocked(lock);
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
template <typename Obj, typename MemFunc>
inline Connection<Ret(Params...)> Signal<Ret(Params...)>::connectBind(Obj* object, MemFunc&& memFunc)
{
    return connect([object, memFunc{std::move(memFunc)}](Params... params) { memFunc(object, params...); });
}

template <typename Ret, typename... Params>
inline bool Signal<Ret(Params...)>::disconnectLocked(const StateLock&, Connection<Ret(Params...)>& connection)
{
    assert(_valid);
    const auto I = std::find(std::begin(_connections), std::end(_connections), connection.sharedData());
    if (I != std::end(_connections)) {
        (*I)->invalidate();
        _connections.erase(I);
        return true;
    }
    return false;
}

template <typename Ret, typename... Params>
inline Connection<Ret(Params...)> Signal<Ret(Params...)>::connectLocked(const StateLock&, Slot&& slot)
{
    assert(_valid);
    static std::atomic<uint64_t> id{0u};
    auto connection = ConnectionData::buildConnection(this, std::move(slot), id++);
    _connections.push_back(connection);
    return {connection};
}

template <typename Ret, typename... Params>
inline void Signal<Ret(Params...)>::disconnectAllLocked(const StateLock& lock)
{
    forEachConnection(lock, [](auto& connection) { connection.invalidate(); });
    _connections.clear();
}

template <typename Ret, typename... Params>
inline void Signal<Ret(Params...)>::forEachConnection(const StateLock&, std::function<void(ConnectionData&)>&& func)
{
    for (auto I = std::rbegin(_connections); I != std::rend(_connections); ++I) {
        func(**I);
    }
}

/// ]]] Signal ----------------------------------------------------------------

/// [[[ Connection ------------------------------------------------------------

/// A connection class that references a signal-slot connection.
template <typename Ret, typename... Params>
class Connection<Ret(Params...)> {
public:
    using SlotProto = Ret(Params...);
    using Slot = std::function<SlotProto>;

    bool operator==(const Connection&) const;

    /// Disconnect this connection from the signal.
    /// @returns True if the connection was disconnect, false otherwise.
    bool disconnect();

    /// Get the validity of the connection
    /// @returns true if the connection is valid, false otherwise.
    bool valid() const;

private:
    friend class Signal<SlotProto>;
    class ConnectionData;

    /// Construct a connection from scratch
    Connection(Signal<SlotProto>* signal, Slot&& slot, uint32_t id);

    /// Construct a connection from shared data
    Connection(std::shared_ptr<ConnectionData> sharedConnectionData);

    /// Call the slot.
    template <typename... Args>
    void call(Args... args);

    /// Invalidate the connection.
    void invalidate();

    /// Get the connection id.
    uint32_t id() const;

    /// Get the shared data.
    const std::shared_ptr<ConnectionData> sharedData() const;

    std::shared_ptr<ConnectionData> _sharedConnectionData;
};

template <typename Ret, typename... Params>
inline bool Connection<Ret(Params...)>::operator==(const Connection& other) const
{
    return id() == other.id();
}

template <typename Ret, typename... Params>
inline bool Connection<Ret(Params...)>::disconnect()
{
    return _sharedConnectionData->disconnect();
}

template <typename Ret, typename... Params>
inline bool Connection<Ret(Params...)>::valid() const
{
    return _sharedConnectionData->valid();
}

template <typename Ret, typename... Params>
inline Connection<Ret(Params...)>::Connection(Signal<SlotProto>* signal, Slot&& slot, uint32_t id)
: _sharedConnectionData{ConnectionData::buildConnection(signal, std::move(slot), id)}
{
}

template <typename Ret, typename... Params>
inline Connection<Ret(Params...)>::Connection(std::shared_ptr<ConnectionData> sharedConnectionData)
: _sharedConnectionData{std::move(sharedConnectionData)}
{
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
inline uint32_t Connection<Ret(Params...)>::id() const
{
    return _sharedConnectionData->id();
}

template <typename Ret, typename... Params>
inline const std::shared_ptr<typename Connection<Ret(Params...)>::ConnectionData> Connection<
    Ret(Params...)>::sharedData() const
{
    return _sharedConnectionData;
}

/// ]]] Connection ------------------------------------------------------------

/// [[[ Connection::ConnectionData --------------------------------------

/// Class containing data shared between equivalint connections
template <typename Ret, typename... Params>
class Connection<Ret(Params...)>::ConnectionData : public std::enable_shared_from_this<ConnectionData> {
public:
    /// Connection data should only be held as a shared_ptr (due to the use of shared_from_this)
    static std::shared_ptr<ConnectionData> buildConnection(Signal<SlotProto>* signal, Slot&& slot, uint32_t id);

    /// Non-copyable / Non-movable
    ConnectionData(const ConnectionData&) = delete;
    ConnectionData(ConnectionData&&) = delete;
    ConnectionData& operator=(const ConnectionData&) = delete;
    ConnectionData& operator=(ConnectionData&&) = delete;

    /// Call the slot.
    template <typename... Args>
    void call(Args... args);

    /// Get the validity of the connection.
    bool valid() const;

    /// Invalidate the connection.
    void invalidate();

    /// Get the id of the connection.
    uint32_t id() const;

    /// Disconnect from the signal.
    bool disconnect();

    /// Update the signal in the event that it has moved.
    void updateSignal(Signal<SlotProto>* signal);

private:
    using StateLock = std::lock_guard<std::mutex>;

    ConnectionData(Signal<SlotProto>* signal, Slot&& slot, uint32_t id);

    mutable std::mutex _stateMutex;
    Signal<SlotProto>* _signal;
    bool _valid{true};
    Slot _slot;
    uint32_t _id;
};

template <typename Ret, typename... Params>
inline auto Connection<Ret(Params...)>::ConnectionData::buildConnection(Signal<SlotProto>* signal,
                                                                        Slot&& slot,
                                                                        uint32_t id) -> std::shared_ptr<ConnectionData>
{
    /// Can't use make_shared as the construtor is private. May convert to the struct work around in the future.
    return std::shared_ptr<ConnectionData>(new ConnectionData(signal, std::move(slot), id));
}

template <typename Ret, typename... Params>
inline Connection<Ret(Params...)>::ConnectionData::ConnectionData(Signal<SlotProto>* signal, Slot&& slot, uint32_t id)
: _signal{signal}
, _slot{std::move(slot)}
, _id{id}
{
}

template <typename Ret, typename... Params>
template <typename... Args>
inline void Connection<Ret(Params...)>::ConnectionData::call(Args... args)
{
    StateLock lock{_stateMutex};
    assert(_valid);
    _slot(std::forward<Args>(args)...);
}

template <typename Ret, typename... Params>
inline bool Connection<Ret(Params...)>::ConnectionData::valid() const
{
    StateLock lock{_stateMutex};
    return _valid;
}

template <typename Ret, typename... Params>
inline void Connection<Ret(Params...)>::ConnectionData::invalidate()
{
    StateLock lock{_stateMutex};
    _valid = false;
}

template <typename Ret, typename... Params>
inline uint32_t Connection<Ret(Params...)>::ConnectionData::id() const
{
    StateLock lock{_stateMutex};
    return _id;
}

template <typename Ret, typename... Params>
inline void Connection<Ret(Params...)>::ConnectionData::updateSignal(Signal<SlotProto>* signal)
{
    StateLock lock{_stateMutex};
    _signal = signal;
}

template <typename Ret, typename... Params>
inline bool Connection<Ret(Params...)>::ConnectionData::disconnect()
{
    auto connection = Connection{this->shared_from_this()};
    return _signal->disconnect(connection);
}

/// ]]] Connection::ConnectionData --------------------------------------

} // namespace moment
