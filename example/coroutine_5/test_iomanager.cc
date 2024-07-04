#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>

#include "iomanager.hpp"

using namespace qc;

int sockfd;
void watch_io_read();

void do_io_write() {
    std::cout << "write callback" << std::endl;
    int so_err;
    socklen_t len = size_t(so_err);
    getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &so_err, &len);
    if (so_err) {
        std::cout << "connect fail" << std::endl;
    } 
    std::cout << "connect succ" << std::endl;

    char buf[128] = "hello i am client!\n";
    int writelen = write(sockfd, buf, sizeof(buf));
    std::cout << "client send succ : " << buf << std::endl;
}

void do_io_read() {
    std::cout << "read callback" << std::endl;
    char buf[1024] = {0};
    int readlen = 0;
    readlen = read(sockfd, buf, sizeof(buf));
    if (readlen > 0) {
        buf[readlen] = '\0';
        std::cout << "read " << readlen << " bytes, read: " << buf << std::endl;
    } else if (readlen == 0) {
        std::cout << "peer closed";
        close(sockfd);
        return;
    } else {
        std::cout << "err, errno = " << errno << ", errstr" << strerror(errno) << std::endl;
    }

    // 这里事件执行完之后才会清除事件,事件中不能再添加相同事件,
    // read 之后重新添加读事件回调,这里不能直接调用addEvent,因为在当前位置fd的读上下文中依然有效如果直接调用addEvent相当于重复添加相同事件
    IOManager::GetThis()->add_task(watch_io_read);
}

void watch_io_read() {
    std::cout << "watch_io_read" << std::endl;
    IOManager::GetThis()->addEvent(sockfd, READ, do_io_read);
}

void test_io() {
    std::cout << "test_io" << std::endl;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    qc_assert(sockfd > 0);

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8080);

    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr.s_addr);

    int rt = connect(sockfd, (const sockaddr *)&servaddr, sizeof(servaddr));
    if (rt != 0) {
        if (errno == EINPROGRESS) {
            std::cout << "EINPROGRESS" << std::endl;
            // 注册写事件回调,只用于判断connect是否成功
            // 非阻塞的TCP套接字connect一般无法立即建立连接,要通过套接字可写来判断是否链接成功
            IOManager::GetThis()->addEvent(sockfd, WRITE, do_io_write);

            // 注册都时间回调,一次性的
            IOManager::GetThis()->addEvent(sockfd, READ, do_io_read);
        }
    }
}

void test_iomanager() {
    IOManager iom(1,false);
    iom.add_task(test_io);
    while (1);
}

int main(int argc, char *argv[]) {

    test_iomanager();

    return 0;
}