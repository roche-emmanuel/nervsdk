#ifndef IPCHANDLER_H_
#define IPCHANDLER_H_

#include <nvk_base.h>

namespace nv {

class IPCHandler : public RefObject {
  public:
    explicit IPCHandler(const String& pipeName);
    ~IPCHandler() override;

    // Send a request and get response
    auto send_request(const String& request) -> String;

    // Check if connected
    [[nodiscard]] auto is_connected() const -> bool { return _connected; }

    // Set timeout for operations (milliseconds)
    void set_timeout(DWORD timeout) { _timeout = timeout; }

  private:
    auto connect() -> bool;
    void disconnect();

    String _pipeName;
    HANDLE _pipeHandle;
    bool _connected;
    DWORD _timeout;

    static constexpr DWORD DEFAULT_TIMEOUT = 5000; // 5 seconds
    static constexpr size_t BUFFER_SIZE = 65536;   // 64 KB
};

} // namespace nv

#endif // IPCHANDLER_H_
