#pragma once

#include <string>
#include <windows.h> 
#include <stdio.h> 
#include <tchar.h>
#include <strsafe.h>
#include <memory>

#include "ThreadPool.h"

class PipeServer {
public:
    explicit PipeServer(const std::string &pipeName = "", int buffer_size = 1024);
    ~PipeServer();
    bool Start();
    bool Stop();

private:
    void ConnectionHandler(LPVOID lp_pipe);
    void GetAnswerToRequest(LPTSTR pch_request, LPTSTR pch_reply, LPDWORD pch_bytes);
    void ServerMain();

private:
    LPCTSTR lpsz_pipe_name_ = TEXT("\\\\.\\pipe\\my_test_named_pipe");
    int buffer_size_ = 1024;
    std::shared_ptr<ThreadPool> thread_pool_;

    bool stop_server_ = false;
    HANDLE h_event_ = nullptr;

    std::shared_ptr<std::thread> server_thread_;
};

