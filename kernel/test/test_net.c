#include <common.h>
#include <net/net.h>
#include <net/socket.h>
#include <net/socket_type.h>

#include <klib.h>

#define HOST_IP "10.0.2.2"  // 主机在虚拟网络中的地址
#define HOST_PORT 5555      // 转发端口
#define BUFFER_SIZE 1024

#define	INADDR_ANY		((unsigned long int) 0x00000000)

int     call_sys_socket(int domain, int type, int protocol);
int     call_sys_connect(int fd, const struct sockaddr * address, socklen_t address_len);
int     call_sys_bind(int fd, const struct sockaddr * address, socklen_t address_len);
ssize_t call_sys_send(int fd, const void * buffer, size_t length, int flags);
ssize_t call_sys_recv(int fd, void * buffer, size_t length, int flags);
int     call_sys_close(int fd);

void test_net() {
    // 创建UDP套接字
    int sockfd = call_sys_socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        error("Socket creation failed");
        return;
    }
    PASS("Socket creation");

    // 设置虚拟机本地地址
    struct sockaddr_in local_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(0),  // 让系统自动分配端口
        .sin_addr.s_addr = INADDR_ANY
    };
    
    // 绑定套接字到本地地址
    if (call_sys_bind(sockfd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        error("Bind failed");
        call_sys_close(sockfd);
        return;
    }
    PASS("Bind");
    
    uint32 hip;
    inet_aton(HOST_IP, &hip);
    // 设置主机目标地址
    struct sockaddr_in host_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(HOST_PORT),
        .sin_addr.s_addr = hip
    };
    
    // 使用connect建立"连接"（针对UDP，这实际上设置默认目标）
    if (call_sys_connect(sockfd, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0) {
        error("Connect to host failed");
        call_sys_close(sockfd);
        return;
    }
    
    PASS("Connect");
    PASS("UDP client ready. Sending messages to %s:%d", HOST_IP, HOST_PORT);

    char buffer[BUFFER_SIZE];
    int sequence = 0;
    
    while (1) {
        // 准备要发送的消息
        int len = snprintf(buffer, BUFFER_SIZE, "UDP Message %d from VM", ++sequence);
        
        // 发送消息到主机
        int sent_bytes = call_sys_send(sockfd, buffer, len, 0);
        if (sent_bytes < 0) {
            error("Send failed");
            break;
        }
        
        PASS("Send");
        log("Sent %d bytes: %s", sent_bytes, buffer);
        
        // 尝试接收响应（阻塞模式）
        memset(buffer, 0, BUFFER_SIZE);
        int recv_bytes = call_sys_recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        
        if (recv_bytes < 0) {
            error("Receive failed");
            break;
        } else if (recv_bytes == 0) {
            log("Connection closed by host");
            break;
        } else {
            buffer[recv_bytes] = '\0';
            log("Received %d bytes: %s", recv_bytes, buffer);
        }

        PASS("Send");
        
        // sleep(1);  // 等待1秒
    }

    call_sys_close(sockfd);
    PASS("Net test pass");
}
