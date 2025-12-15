#ifndef IPCHANDLER_H_
#define IPCHANDLER_H_

#include <nvk_base.h>

#include <atomic>
#include <functional>
#include <thread>
#include <vector>

namespace nv {

class IPCHandler : public RefObject {
  public:
    using DataCallback = std::function<void(const String&)>;

    explicit IPCHandler(const String& pipeName);
    ~IPCHandler() override;

    // Send data (fire-and-forget or request/response)
    auto send(const String& data) -> bool;

    // Connection state
    [[nodiscard]] auto is_connected() const -> bool { return _connected; }

    // Timeout (milliseconds)
    void set_timeout(DWORD timeout) { _timeout = timeout; }

    // Event: called from reader thread
    void set_on_data_received(DataCallback cb);

    void start();
    void stop();

  private:
    auto connect() -> bool;
    void disconnect();

    void run();
    auto create_pipe() -> bool;

    String _pipeName;
    HANDLE _pipeHandle{INVALID_HANDLE_VALUE};
    DWORD _timeout{5000};

    std::atomic<bool> _connected{false};
    std::atomic<bool> _running{false};

    std::thread _readerThread;
    DataCallback _onDataReceived;

    static constexpr size_t BUFFER_SIZE = 65536;
};

} // namespace nv

#endif // IPCHANDLER_H_
