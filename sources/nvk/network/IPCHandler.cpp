#include <nvk/network/IPCHandler.h>

namespace nv {

// ============================================================================
// IPCBase - Shared Implementation
// ============================================================================

IPCBase::IPCBase(const String& pipeName)
    : _pipeName(pipeName),
      _readEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr)),
      _writeEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr)) {
    // Create reusable events

    if (_readEvent == INVALID_HANDLE_VALUE ||
        _writeEvent == INVALID_HANDLE_VALUE) {
        logERROR("Failed to create IPC events");
        if (_readEvent != INVALID_HANDLE_VALUE)
            CloseHandle(_readEvent);
        if (_writeEvent != INVALID_HANDLE_VALUE)
            CloseHandle(_writeEvent);
        THROW_MSG("Failed to create IPC events");
    }
}

IPCBase::~IPCBase() {
    stop();
    // Clean up events
    if (_readEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(_readEvent);
        _readEvent = INVALID_HANDLE_VALUE;
    }
    if (_writeEvent != INVALID_HANDLE_VALUE) {
        CloseHandle(_writeEvent);
        _writeEvent = INVALID_HANDLE_VALUE;
    }
}

void IPCBase::start() {
    _running = true;
    _readerThread = std::thread(&IPCBase::run, this);
}

void IPCBase::stop() {
    if (_running) {
        _running = false;
        CancelIoEx(_pipeHandle, nullptr);
        disconnect();

        logDEBUG("Waiting for IPC Thread...");
        _readerThread.join();
        logDEBUG("IPC Thread finished.");
    }
}

void IPCBase::disconnect() {
    if (_connected && _pipeHandle != INVALID_HANDLE_VALUE) {
        _connected = false;
        disconnected.emit();
        logNOTE("IPC disconnected.");
    }
}

auto IPCBase::send(const String& data) -> bool {
    if (!_connected || _pipeHandle == INVALID_HANDLE_VALUE) {
        logWARN("Not connected to send message.");
        return false;
    }

    OVERLAPPED overlap = {};
    overlap.hEvent = _writeEvent; // Use member variable
    ResetEvent(_writeEvent);      // Reset before use

    if (overlap.hEvent == nullptr) {
        logERROR("Failed to create event for WriteFile");
        return false;
    }

    DWORD bytesWritten = 0;
    BOOL ok =
        WriteFile(_pipeHandle, data.data(), static_cast<DWORD>(data.size()),
                  &bytesWritten, &overlap);

    if (ok == 0) {
        DWORD err = GetLastError();

        if (err == ERROR_IO_PENDING) {
            // Wait for write to complete with timeout
            DWORD waitResult = WaitForSingleObject(overlap.hEvent, _timeout);

            if (waitResult == WAIT_OBJECT_0) {
                // Get the actual number of bytes written
                if (!GetOverlappedResult(_pipeHandle, &overlap, &bytesWritten,
                                         FALSE)) {
                    err = GetLastError();
                    if (err == ERROR_NO_DATA || err == ERROR_BROKEN_PIPE) {
                        logERROR(
                            "WriteFile failed: pipe closed at the other end.");
                    } else {
                        logERROR("GetOverlappedResult for WriteFile failed, "
                                 "error code: {}",
                                 err);
                    }
                    return false;
                }
            } else if (waitResult == WAIT_TIMEOUT) {
                logERROR("WriteFile timed out after {}ms", _timeout);
                CancelIoEx(_pipeHandle, &overlap);
                return false;
            } else {
                logERROR("WaitForSingleObject failed for WriteFile");
                CancelIoEx(_pipeHandle, &overlap);
                return false;
            }
        } else if (err == ERROR_NO_DATA || err == ERROR_BROKEN_PIPE) {
            logERROR("WriteFile failed: pipe closed at the other end.");
            return false;
        } else {
            logERROR("WriteFile failed, error code: {}", err);
            return false;
        }
    }

    if (bytesWritten != data.size()) {
        logERROR("WriteFile incomplete: wrote {} of {} bytes", bytesWritten,
                 data.size());
        return false;
    }

    FlushFileBuffers(_pipeHandle);
    logDEBUG("Sent {} bytes via IPC.", bytesWritten);
    return true;
}

void IPCBase::run() {
    logDEBUG("Entering IPC thread.");

    char buffer[BUFFER_SIZE];

    while (_running) {
        if (!establish_connection()) {
            sleep_s(1);
            continue;
        }

        // Use member event instead of creating new one
        OVERLAPPED overlap = {};
        overlap.hEvent = _readEvent;

        while (_running && _connected && _pipeHandle != INVALID_HANDLE_VALUE) {
            DWORD bytesRead = 0;
            ResetEvent(_readEvent); // Reset before each read

            BOOL success = ReadFile(_pipeHandle, buffer, sizeof(buffer) - 1,
                                    &bytesRead, &overlap);

            if (success == 0) {
                DWORD err = GetLastError();

                if (err == ERROR_IO_PENDING) {
                    // Wait with timeout so we can check _running
                    while (_running) {
                        DWORD waitResult = WaitForSingleObject(_readEvent, 100);
                        if (waitResult == WAIT_OBJECT_0) {
                            // Get the result
                            if (!GetOverlappedResult(_pipeHandle, &overlap,
                                                     &bytesRead, FALSE)) {
                                err = GetLastError();
                                if (err == ERROR_BROKEN_PIPE) {
                                    logNOTE("Connection broken (broken pipe).");
                                } else {
                                    logERROR("GetOverlappedResult failed: {}",
                                             err);
                                }
                                disconnect();
                                break;
                            }
                            // Successfully read data
                            break;
                        }

                        if (waitResult == WAIT_TIMEOUT) {
                            continue; // Check _running again
                        }

                        logERROR("WaitForSingleObject failed");
                        disconnect();
                        break;
                    }

                    if (!_running) {
                        CancelIoEx(_pipeHandle, &overlap);
                        break;
                    }

                    if (!_connected) {
                        break;
                    }
                } else if (err == ERROR_BROKEN_PIPE) {
                    logNOTE("Connection broken (broken pipe).");
                    disconnect();
                    break;
                } else if (err == ERROR_OPERATION_ABORTED) {
                    logDEBUG("Read operation cancelled.");
                    disconnect();
                    break;
                } else {
                    logERROR("ReadFile failed: {}", err);
                    disconnect();
                    break;
                }
            }

            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                logDEBUG("IPC received {} bytes", bytesRead);
                dataReceived.emit(buffer);
            } else if (bytesRead == 0 && (success != 0)) {
                // Connection closed gracefully
                logNOTE("Connection closed (0 bytes read).");
                disconnect();
                break;
            }
        }

        // Don't close the event - it's reused
        cleanup_connection();

        if (_running) {
            sleep_ms(100);
        }
    }

    logDEBUG("IPC thread cleaning up...");
    if (_pipeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(_pipeHandle);
        _pipeHandle = INVALID_HANDLE_VALUE;
    }

    logDEBUG("Exiting IPC thread.");
}

// ============================================================================
// IPCServer - Server-Specific Implementation
// ============================================================================

IPCServer::IPCServer(const String& pipeName) : IPCBase(pipeName) {}

auto IPCServer::create_pipe() -> bool {
    auto fullPipeName = format_string(R"(\\.\pipe\%s)", _pipeName.c_str());

    _pipeHandle = CreateNamedPipe(
        fullPipeName.c_str(), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,           // Max instances
        BUFFER_SIZE, // Output buffer size
        BUFFER_SIZE, // Input buffer size
        _timeout, nullptr);

    if (_pipeHandle == INVALID_HANDLE_VALUE) {
        logERROR("Failed to create named pipe: {} (Error: {})", fullPipeName,
                 GetLastError());
        return false;
    }

    logDEBUG("Named pipe created: {}", fullPipeName);
    return true;
}

auto IPCServer::wait_for_connection() -> bool {
    logDEBUG("Waiting for IPC client connection...");

    OVERLAPPED overlap = {};
    overlap.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if (overlap.hEvent == nullptr) {
        logERROR("Failed to create event for ConnectNamedPipe");
        return false;
    }

    BOOL result = ConnectNamedPipe(_pipeHandle, &overlap);

    if (result == 0) {
        DWORD err = GetLastError();

        if (err == ERROR_IO_PENDING) {
            while (_running) {
                DWORD waitResult = WaitForSingleObject(overlap.hEvent, 100);
                if (waitResult == WAIT_OBJECT_0) {
                    break;
                }
            }
        } else if (err == ERROR_PIPE_CONNECTED) {
            SetEvent(overlap.hEvent);
        } else {
            logERROR("ConnectNamedPipe failed: {}", err);
            CloseHandle(overlap.hEvent);
            return false;
        }
    }

    CloseHandle(overlap.hEvent);

    if (_running) {
        _connected = true;
        connected.emit();
        logNOTE("IPC client connected!");
        return true;
    }

    return false;
}

auto IPCServer::establish_connection() -> bool {
    if (!create_pipe()) {
        logERROR("Cannot create pipe instance.");
        return false;
    }

    return wait_for_connection();
}

void IPCServer::cleanup_connection() {
    if (_pipeHandle != INVALID_HANDLE_VALUE) {
        DisconnectNamedPipe(_pipeHandle);
        CloseHandle(_pipeHandle);
        _pipeHandle = INVALID_HANDLE_VALUE;
    }
}

// ============================================================================
// IPCClient - Client-Specific Implementation
// ============================================================================

IPCClient::IPCClient(const String& pipeName) : IPCBase(pipeName) {}

auto IPCClient::connect_to_server() -> bool {
    auto fullPipeName = format_string(R"(\\.\pipe\%s)", _pipeName.c_str());

    logDEBUG("Attempting to connect to pipe: {}", fullPipeName);

    while (_running) {
        _pipeHandle =
            CreateFile(fullPipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
                       nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);

        if (_pipeHandle != INVALID_HANDLE_VALUE) {
            break;
        }

        DWORD err = GetLastError();

        if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PIPE_BUSY) {
            if (err == ERROR_PIPE_BUSY) {
                if (WaitNamedPipe(fullPipeName.c_str(), _timeout)) {
                    continue;
                }
            }

            logDEBUG("Pipe not available, retrying in {}s...",
                     _reconnectInterval);
            sleep_s(_reconnectInterval);
            continue;
        }

        logERROR("Failed to connect to pipe: {} (Error: {})", fullPipeName,
                 err);
        sleep_s(_reconnectInterval);
    }

    if (!_running) {
        return false;
    }

    // Set pipe to message mode
    DWORD mode = PIPE_READMODE_MESSAGE | PIPE_WAIT;
    BOOL success =
        SetNamedPipeHandleState(_pipeHandle, &mode, nullptr, nullptr);

    if (!success) {
        logERROR("SetNamedPipeHandleState failed: {}", GetLastError());
        CloseHandle(_pipeHandle);
        _pipeHandle = INVALID_HANDLE_VALUE;
        return false;
    }

    _connected = true;
    connected.emit();
    logNOTE("IPC connected to server!");
    return true;
}

auto IPCClient::establish_connection() -> bool { return connect_to_server(); }

void IPCClient::cleanup_connection() {
    if (_pipeHandle != INVALID_HANDLE_VALUE) {
        CloseHandle(_pipeHandle);
        _pipeHandle = INVALID_HANDLE_VALUE;
    }
}

} // namespace nv