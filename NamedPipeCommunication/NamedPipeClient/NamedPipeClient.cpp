#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#define BUFSIZE 512

int main(int argc, TCHAR* argv[]) {
    HANDLE h_pipe;
    LPCTSTR lpv_message = TEXT("Default message from client.");
    TCHAR  ch_buf[BUFSIZE];
    BOOL   success = FALSE;
    DWORD  cb_read, cb_to_write, cb_written, dw_mode;
    LPCTSTR lpsz_pipename = TEXT("\\\\.\\pipe\\my_test_named_pipe");

    if (argc > 1) {
        lpv_message = argv[1];
    }

    while (1) {
        h_pipe = CreateFile(lpsz_pipename, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (h_pipe != INVALID_HANDLE_VALUE) {
            break;
        }

        if (GetLastError() != ERROR_PIPE_BUSY) {
            _tprintf(TEXT("Could not open pipe. GLE=%d\n"), GetLastError());
            return -1;
        }

        // All pipe instances are busy, so wait for 20 seconds. 
        if (!WaitNamedPipe(lpsz_pipename, 20000)) {
            printf("Could not open pipe: 20 second wait timed out.");
            return -1;
        }
    }

    // The pipe connected; change to message-read mode. 
    dw_mode = PIPE_READMODE_MESSAGE;
    success = SetNamedPipeHandleState(h_pipe, &dw_mode, NULL, NULL); 
    if (!success) {
        _tprintf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
        return -1;
    }

    cb_to_write = (lstrlen(lpv_message) + 1) * sizeof(TCHAR);
    _tprintf(TEXT("Sending %d byte message: \"%s\"\n"), cb_to_write, lpv_message);

    success = WriteFile(h_pipe, lpv_message, cb_to_write, &cb_written, NULL); 
    if (!success) {
        _tprintf(TEXT("WriteFile to pipe failed. GLE=%d\n"), GetLastError());
        return -1;
    }

    printf("\nMessage sent to server, receiving reply as follows:\n");
    do {
        success = ReadFile(h_pipe, ch_buf, BUFSIZE * sizeof(TCHAR),&cb_read, NULL);
        if (!success && GetLastError() != ERROR_MORE_DATA) {
            break;
        }
        _tprintf(TEXT("\"%s\"\n"), ch_buf);
    } while (!success);  // repeat loop if ERROR_MORE_DATA 

    if (!success) {
        _tprintf(TEXT("ReadFile from pipe failed. GLE=%d\n"), GetLastError());
        return -1;
    }

    printf("\n<End of message, press ENTER to terminate connection and exit>");
    _getch();
    CloseHandle(h_pipe);
    return 0;
}
