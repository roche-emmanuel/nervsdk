#ifndef IPCBASE_H_
#define IPCBASE_H_

#include <nvk_common.h>

#include <nvk/base/Signal.h>
namespace nv {

// Base class with shared IPC functionality
class IPCBase : public RefObject {
  public:
    explicit IPCBase(const String& pipeName);
    ~IPCBase() override;

    // Send data
    auto send(const String& data) -> bool;

    // Connection state
    [[nodiscard]] auto is_connected() const -> bool { return _connected; }

    // Timeout (milliseconds)
    void set_timeout(DWORD timeout) { _timeout = timeout; }

    Signal<> connected;
    Signal<> disconnected;
    Signal<const String&> dataReceived;

    void start();
    void stop();

  protected:
    // Virtual methods for subclasses to implement
    virtual auto establish_connection() -> bool = 0;
    virtual void cleanup_connection() = 0;

    // Shared helper methods
    void disconnect();
    void run();

    String _pipeName;
    HANDLE _pipeHandle{INVALID_HANDLE_VALUE};
    HANDLE _readEvent{INVALID_HANDLE_VALUE};  // Add this
    HANDLE _writeEvent{INVALID_HANDLE_VALUE}; // Add this
    DWORD _timeout{5000};

    std::atomic<bool> _connected{false};
    std::atomic<bool> _running{false};

    std::thread _readerThread;

    static constexpr size_t BUFFER_SIZE = 65536;
};

// Server implementation
class IPCServer : public IPCBase {
  public:
    explicit IPCServer(const String& pipeName);

  protected:
    auto establish_connection() -> bool override;
    void cleanup_connection() override;

  private:
    auto create_pipe() -> bool;
    auto wait_for_connection() -> bool;
};

// Client implementation
class IPCClient : public IPCBase {
  public:
    explicit IPCClient(const String& pipeName);

    // Reconnection interval (seconds)
    void set_reconnect_interval(int seconds) { _reconnectInterval = seconds; }

  protected:
    auto establish_connection() -> bool override;
    void cleanup_connection() override;

  private:
    auto connect_to_server() -> bool;
    int _reconnectInterval{1}; // seconds
};

} // namespace nv

#endif // IPCBASE_H_