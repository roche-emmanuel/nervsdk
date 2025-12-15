#include <nvk/network/IPCHandler.h>

namespace nv {

IPCHandler::IPCHandler(const String& pipeName) : _pipeName(pipeName) {}

IPCHandler::~IPCHandler() { stop(); }

void IPCHandler::start() {
    _running = true;
    _readerThread = std::thread(&IPCHandler::run, this);
}

void IPCHandler::stop() {
    _running = false;
    CancelIoEx(_pipeHandle, nullptr);
    disconnect();
}

auto IPCHandler::create_pipe() -> bool {
    auto fullPipeName = format_string(R"(\\.\pipe\{})", _pipeName.c_str());

    _pipeHandle = CreateNamedPipe(
        fullPipeName.c_str(),                      // Use TCHAR directly
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED, // Read/write access
        PIPE_TYPE_MESSAGE |                        // Message-type pipe
            PIPE_READMODE_MESSAGE |                // Message-read mode
            PIPE_WAIT,                             // Blocking mode
        1,           // Max instances (1 client at a time)
        BUFFER_SIZE, // Output buffer size
        BUFFER_SIZE, // Input buffer size
        0,           // Default timeout
        nullptr      // Default security
    );

    if (_pipeHandle == INVALID_HANDLE_VALUE) {
        logERROR("Failed to create named pipe: {} (Error: {})", fullPipeName,
                 GetLastError());
        return false;
    }

    logDEBUG("Named pipe created: {}", fullPipeName);
    return true;
}

auto IPCHandler::connect() -> bool {
    logDEBUG("Waiting for IPC connection...");

    // Wait for client to connect
    OVERLAPPED Overlap = {};
    Overlap.hEvent = CreateEvent(nullptr, 1, 0, nullptr);

    _connected = ConnectNamedPipe(_pipeHandle, &Overlap);

    if (!_connected) {
        DWORD Err = GetLastError();
        if (Err == ERROR_IO_PENDING) // async wait
        {
            while (_running) {
                DWORD Res = WaitForSingleObject(Overlap.hEvent, 100);
                if (Res == WAIT_OBJECT_0) {
                    break;
                }
            }
        } else if (Err == ERROR_PIPE_CONNECTED) {
            // client connected before ConnectNamedPipe call
            SetEvent(Overlap.hEvent);
        } else {
            // error
            logERROR("ConnectNamedPipe failed: {}", GetLastError());
            CloseHandle(Overlap.hEvent);
            return false;
        }
    }

    if (_running) {
        _connected = true;
        logNOTE("IPC connected!");
    }

    return _connected;
}

void IPCHandler::disconnect() {
    if (_connected && _pipeHandle != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(_pipeHandle);
        _connected = false;
        logNOTE("Client disconnected.");
    }
}

void IPCHandler::set_on_data_received(DataCallback cb) {
    _onDataReceived = std::move(cb);
}

auto IPCHandler::send(const String& data) -> bool {
    if (!_connected || _pipeHandle == INVALID_HANDLE_VALUE) {
        logWARN("No client connected to send message to.");
        return false;
    }

    DWORD bytesWritten = 0;
    BOOL ok =
        WriteFile(_pipeHandle, data.data(), static_cast<DWORD>(data.size()),
                  &bytesWritten, nullptr);

    if ((ok == 0) || bytesWritten != data.size()) {
        int code = GetLastError();
        if (code == ERROR_NO_DATA) {
            logERROR("WriteFile failed: pipe closed at the other end.");
        } else {
            logERROR("WriteFile failed, error code: {}", code);
        }
        return false;
    }

    // Flush to ensure message is sent immediately
    FlushFileBuffers(_pipeHandle);

    logDEBUG("Sent {} bytes via IPC.", bytesWritten);
    return true;
}

void IPCHandler::run() {
    logDEBUG("Entering IPChandler thread.");
    if (!create_pipe()) {
        logERROR("Cannot create pipe instance.");
        return;
    }

    char buffer[BUFFER_SIZE];

    while (!_running) {
        // Wait for client connection
        if (!connect()) {
            sleep_s(1);
            continue;
        }

        // Read messages from connected client
        while (_running && _connected && _pipeHandle != INVALID_HANDLE_VALUE) {

            DWORD bytesRead = 0;

            BOOL success =
                ReadFile(_pipeHandle, buffer,
                         sizeof(buffer) - 1, // Leave room for null terminator
                         &bytesRead, nullptr);

            if (!success || bytesRead == 0) {
                if (GetLastError() == ERROR_BROKEN_PIPE) {
                    logNOTE("Client disconnected.");
                } else {
                    logERROR("ReadFile failed: {}", GetLastError());
                }

                disconnect();
                continue;
            }

            buffer[bytesRead] = '\0';

            logDEBUG("IPC received {} bytes", bytesRead);

            NVCHK(_onDataReceived != nullptr,
                  "No IPC data received callback assigned.");
            // Call callback directly on IPC thread:
            _onDataReceived(buffer);
        }
    }

    // Cleanup
    logDEBUG("IPC Server thread cleaning up...");
    if (_pipeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(_pipeHandle);
        _pipeHandle = INVALID_HANDLE_VALUE;
    }

    logDEBUG("Exiting IPCHandler::run()");
}

} // namespace nv
