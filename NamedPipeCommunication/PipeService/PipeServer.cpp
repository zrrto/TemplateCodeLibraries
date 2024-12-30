#include "PipeServer.h"

PipeServer::PipeServer(const std::string &pipe_name, int buffer_size) {
    if (!pipe_name.empty()) {
        std::wstring wide_str(pipe_name.begin(), pipe_name.end());
        lpsz_pipe_name_ = wide_str.c_str();
    }

    if (buffer_size > 1024) {
        buffer_size_ = buffer_size;
    }

    std::function<void(LPVOID)> func = std::bind(&PipeServer::ConnectionHandler, this, std::placeholders::_1);
    thread_pool_ = std::make_shared<ThreadPool>(func);
    h_event_ = CreateEvent(NULL, TRUE, FALSE, NULL);
}

PipeServer::~PipeServer() {
    if (server_thread_->joinable()) {
        server_thread_->join();
    }
    CloseHandle(h_event_);
}

bool PipeServer::Start() {
    stop_server_ = false;
    thread_pool_->Start();
    auto thread_func = std::bind(&PipeServer::ServerMain, this);
    server_thread_ = std::make_shared<std::thread>(thread_func);
    return true;
}

bool PipeServer::Stop() {
    stop_server_ = true;
    thread_pool_->Stop();
    bool result = SetEvent(h_event_);
    if (result == FALSE) {
        printf("SetEvent to close server failed, GLE=%d.\n", GetLastError());
    }
    
    return result;
}

void PipeServer::ConnectionHandler(LPVOID lp_pipe) {
    if (lp_pipe == NULL) {
        printf("ConnectionHandler got an unexpected NULL value in lp_pipe.\n");
        return;
    }
    HANDLE h_heap = GetProcessHeap();
    TCHAR* pch_request = (TCHAR*)HeapAlloc(h_heap, 0, buffer_size_ * sizeof(TCHAR));
    TCHAR* pch_reply = (TCHAR*)HeapAlloc(h_heap, 0, buffer_size_ * sizeof(TCHAR));
    DWORD bytes_read = 0, bytes_reply = 0, bytes_written = 0;

    if (pch_request == NULL || pch_reply == NULL) {
        printf("ConnectionHandler got an unexpected NULL heap allocation. pch_request == NULL: %d\n", pch_request == NULL);
        if (pch_reply != NULL) {
            HeapFree(h_heap, 0, pch_reply);
        }
        if (pch_request != NULL) {
            HeapFree(h_heap, 0, pch_request);
        }
        return;
    }

    printf("ConnectionHandler receiving and processing messages.\n");
    HANDLE h_pipe = (HANDLE)lp_pipe;
    while (1) {
        bool success = ReadFile(h_pipe, pch_request, buffer_size_ * sizeof(TCHAR), &bytes_read, NULL);
        if (!success || bytes_read == 0) {
            if (GetLastError() == ERROR_BROKEN_PIPE) {
                printf("ConnectionHandler: client disconnected.\n");
            }
            else {
                printf("ConnectionHandler ReadFile failed, GLE=%d.\n", GetLastError());
            }
            break;
        }

        GetAnswerToRequest(pch_request, pch_reply, &bytes_reply);
        success = WriteFile(h_pipe, pch_reply, bytes_reply, &bytes_written, NULL);
        if (!success || bytes_reply != bytes_written) {
            printf("InstanceThread WriteFile failed, GLE=%d.\n", GetLastError());
            break;
        }
    }

    FlushFileBuffers(h_pipe);
    DisconnectNamedPipe(h_pipe);
    CloseHandle(h_pipe);

    HeapFree(h_heap, 0, pch_request);
    HeapFree(h_heap, 0, pch_reply);
    printf("InstanceThread exiting.\n");
    return;
}

void PipeServer::GetAnswerToRequest(LPTSTR pch_request, LPTSTR pch_reply, LPDWORD pch_bytes) {
    _tprintf(TEXT("Client Request String:\"%s\"\n"), pch_request); // printf 不能直接处理Unicode字符,无法完整打印消息
    if (FAILED(StringCchCopy(pch_reply, buffer_size_, TEXT("default answer from server")))) {
        *pch_bytes = 0;
        pch_reply[0] = 0;
        printf("StringCchCopy failed, no outgoing message.\n");
        return;
    }
    *pch_bytes = (lstrlen(pch_reply) + 1) * sizeof(TCHAR);
}

void PipeServer::ServerMain() {
    HANDLE h_pipe = INVALID_HANDLE_VALUE;
    OVERLAPPED ov;
    ov.hEvent = h_event_;

    for (;;) {
        printf("\nPipe Server: Main thread awaiting client connection on %s\n", lpsz_pipe_name_);
        h_pipe = CreateNamedPipe(
            lpsz_pipe_name_, 
            PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, 
            PIPE_UNLIMITED_INSTANCES,    // max. instances  
            buffer_size_,                // output buffer size 
            buffer_size_,                // input buffer size 
            0,                           // client time-out 
            NULL);                       // default security attribute 

        if (h_pipe == INVALID_HANDLE_VALUE) {
            printf("CreateNamedPipe failed, GLE=%d.\n", GetLastError());
            return;
        }

        printf("After CreateNamedPipe, h_pipe: %d\n", h_pipe);
        ConnectNamedPipe(h_pipe, &ov);
        if (GetLastError() == ERROR_IO_PENDING) {
            WaitForSingleObject(ov.hEvent, INFINITE);
            if (stop_server_) {
                printf("Pipeserver was closed by user!\n");
                CloseHandle(h_pipe);
                return;
            }
        }

        DWORD dwBytes;
        if (GetOverlappedResult(h_pipe, &ov, &dwBytes, FALSE)) {
            thread_pool_->AddTask(h_pipe);
        }
        else {
            // 错误处理
            CloseHandle(h_pipe);
        }
    }
}
