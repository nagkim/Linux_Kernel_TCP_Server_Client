#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <linux/in.h>
#include <net/sock.h>
#include <linux/inet.h>


#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6244
#define BUF_LEN 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("nagkim");

struct socket *conn_socket = NULL;

int tcp_client_send(struct socket *sock, const char *buf, const size_t length)
{
    struct msghdr msg;
    struct kvec iov;
    int len, written = 0, left = length;

    iov.iov_base = (void *)buf;
    msg.msg_flags = 0;
    iov.iov_len = length;

    while (left > 0)
    {
        len = kernel_sendmsg(sock, &msg, &iov, 1, left);
        if (len == -EAGAIN || len == -ERESTARTSYS)
        {
            schedule_timeout_uninterruptible(2 * HZ);
        }
        else if (len < 0)
        {
            return len;
        }
        else
        {
            written += len;
            left -= len;
        }
    }

    return written;
}

int tcp_client_recv(struct socket *sock, char *buf, const size_t length)
{
    struct msghdr msg;
    struct kvec iov;
    int len, received = 0, left = length;

    iov.iov_base = (void *)buf;
    msg.msg_flags = 0;
    iov.iov_len = length;

    while (left > 0)
    {
        len = kernel_recvmsg(sock, &msg, &iov, 1, left, 0);
        if (len == -EAGAIN || len == -ERESTARTSYS)
        {
            schedule_timeout_uninterruptible(2 * HZ);
        }
        else if (len < 0)
        {
            return len;
        }
        else if (len == 0)
        {
            break;
        }
        else
        {
            received += len;
            left -= len;
        }
    }

    return received;
}

int tcp_client_connect(void)
{
    struct sockaddr_in saddr;
    int ret;

    ret = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &conn_socket);
    if (ret < 0)
    {
        printk(KERN_ERR "Failed to create socket: %d\n", ret);
        return ret;
    }
    
     printk(KERN_INFO "Before memset..\n");

    memset(&saddr, 0, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SERVER_PORT);
    ret = in4_pton(SERVER_IP, -1, (u8 *)&saddr.sin_addr.s_addr, -1, NULL);
    if (ret != 1)
    {
        printk(KERN_ERR "Invalid address\n");
        sock_release(conn_socket);
        return -EFAULT;
    }

    ret = kernel_connect(conn_socket, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in), 0);
    if (ret < 0)
    {
        printk(KERN_ERR "Failed to connect to server: %d\n", ret);
        sock_release(conn_socket);
        return ret;
    }

    return 0;
}

void tcp_client_disconnect(void)
{
    if (conn_socket)
    {
        kernel_sock_shutdown(conn_socket, SHUT_RDWR);
        sock_release(conn_socket);
        conn_socket = NULL;
    }
}

static int __init tcp_client_init(void)
{
    int ret;
    char buf[BUF_LEN] = "Hello from client";

    printk(KERN_INFO "TCP Client module loaded\n");

    ret = tcp_client_connect();

    if (ret < 0)
    {
        printk(KERN_ERR "Failed to connect to server\n");
        return ret;
    }

    ret = tcp_client_send(conn_socket, buf, strlen(buf));
    if (ret < 0)
    {
        printk(KERN_ERR "Failed to send message to server: %d\n", ret);
        tcp_client_disconnect();
        return ret;
    }

    memset(buf, 0, BUF_LEN);
    ret = tcp_client_recv(conn_socket, buf, BUF_LEN);
    if (ret < 0)
    {
        printk(KERN_ERR "Failed to receive message from server: %d\n", ret);
        tcp_client_disconnect();
        return ret;
    }

    printk(KERN_INFO "Received message from server: %s\n", buf);

    tcp_client_disconnect();

    return 0;
}

static void __exit tcp_client_exit(void)
{
    printk(KERN_INFO "TCP Client module unloaded\n");
}

module_init(tcp_client_init);
module_exit(tcp_client_exit);