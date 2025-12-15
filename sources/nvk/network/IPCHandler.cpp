#include <nvk/network/IPCHandler.h>

namespace nv {

IPCHandler::IPCHandler(const String& pipeName)
    : _pipeName(pipeName), _pipeHandle(INVALID_HANDLE_VALUE), _connected(false),
      _timeout(DEFAULT_TIMEOUT) {

    if (!connect()) {
        THROW_MSG("Failed to connect to pipe {}", pipeName);
    }
}

IPCHandler::~IPCHandler() { disconnect(); }

auto IPCHandler::connect() -> bool {
    // Wait for the pipe to become available
    if (!WaitNamedPipeA(_pipeName.c_str(), _timeout)) {
        return false;
    }

    // Open the pipe
    _pipeHandle = CreateFileA(_pipeName.c_str(), GENERIC_READ | GENERIC_WRITE,
                              0, nullptr, OPEN_EXISTING, 0, nullptr);

    if (_pipeHandle == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Set pipe to message mode
    DWORD mode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(_pipeHandle, &mode, nullptr, nullptr)) {
        CloseHandle(_pipeHandle);
        _pipeHandle = INVALID_HANDLE_VALUE;
        return false;
    }

    _connected = true;
    return true;
}

void IPCHandler::disconnect() {
    if (_pipeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(_pipeHandle);
        _pipeHandle = INVALID_HANDLE_VALUE;
    }
    _connected = false;
}

auto IPCHandler::send_request(const String& request) -> String {
    if (!_connected || _pipeHandle == INVALID_HANDLE_VALUE) {
        THROW_MSG("IPC client not connected");
    }

    // Send the request
    DWORD bytesWritten = 0;
    BOOL writeSuccess =
        WriteFile(_pipeHandle, request.c_str(),
                  static_cast<DWORD>(request.length()), &bytesWritten, nullptr);

    if ((writeSuccess == 0) || bytesWritten != request.length()) {
        THROW_MSG("Failed to send IPC request");
    }

    // Read the response
    std::vector<char> buffer(BUFFER_SIZE);
    String response;
    DWORD bytesRead = 0;
    BOOL readSuccess = 0;

    do {
        readSuccess =
            ReadFile(_pipeHandle, buffer.data(),
                     static_cast<DWORD>(buffer.size()), &bytesRead, nullptr);

        if ((readSuccess != 0) && bytesRead > 0) {
            response.append(buffer.data(), bytesRead);
        }

        // Check if there's more data
        if (readSuccess == 0) {
            DWORD error = GetLastError();
            if (error == ERROR_MORE_DATA) {
                // Continue reading
                continue;
            }

            THROW_MSG("Failed to read IPC response");
        }

    } while (readSuccess == 0);

    return response;
}

} // namespace nv