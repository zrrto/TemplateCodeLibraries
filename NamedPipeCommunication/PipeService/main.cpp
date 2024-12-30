#include "PipeServer.h"

int main() {
    PipeServer server;
    server.Start();
    Sleep(60 * 1000);
    // 模拟服务器运行60s后用户关闭程序
    server.Stop();
    return 0;
}