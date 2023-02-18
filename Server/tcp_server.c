#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/kthread.h>

#include <linux/socket.h>
#include <linux/net.h>
#include <linux/in.h>
#include <net/tcp.h>
#include <linux/types.h>
#include <linux/inet.h>


#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/uio.h>



#define SERVER_PORT 6244
#define MAX_CLIENTS 10
#define MODULE_NAME "TCP_SERVER"
#define BUF_LEN 1024



MODULE_LICENSE("GPL");
MODULE_AUTHOR("nagkim");


static struct task_struct *server_thread;

struct client_info
{
    struct socket *sock;
    struct task_struct *thread;
};

static struct client_info client_list[MAX_CLIENTS];

static int client_thread_func(void *data)
{
    struct client_info *info = data;
    struct socket *sock = info->sock;
    char buf[BUF_LEN];
    int ret;
    struct kvec  iov;
    struct msghdr  msg;
    char response[] = "Hello from server";
    struct sockaddr_in caddr;

    while (!kthread_should_stop()) {
        memset(buf, 0, sizeof(buf));
        iov.iov_base = buf;
        iov.iov_len = sizeof(buf);
        msg.msg_name = &caddr;
        msg.msg_namelen = sizeof(caddr);
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags = 0;

        ret = kernel_recvmsg(sock, &msg, &iov, 1, sizeof(buf), MSG_WAITALL);

        if (ret < 0) {
            printk(KERN_ERR "Failed to receive message from client: %d\n", ret);
            break;
        }
        printk(KERN_INFO "Received message from client: %s\n", buf);

        // send response to client
        iov.iov_base = response;
        iov.iov_len = strlen(response);
        msg.msg_name = &caddr;
        msg.msg_namelen = sizeof(caddr);
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        msg.msg_flags = 0;

        ret = kernel_sendmsg(sock, &msg, &iov, 1, iov.iov_len);
	if (ret < 0) {
   	    printk(KERN_ERR "Error sending response: %d\n", ret);
            break;
	}
	
	
   
}
    kernel_sock_shutdown(sock, SHUT_RDWR);
    sock_release(sock);
    info->sock = NULL;
    kthread_stop(info->thread);

    return 0;

}	


static int server_thread_func(void *data)
{
    struct socket *sock, *newsock;
    struct sockaddr_in saddr, caddr;
    int ret, i;

    memset(&saddr, 0, sizeof(struct sockaddr_in));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(SERVER_PORT);

    ret = sock_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &sock);
    if (ret < 0)
    {
        printk(KERN_ERR "Failed to create socket: %d\n", ret);
        return ret;
    }

    ret = kernel_bind(sock, (struct sockaddr *)&saddr, sizeof(saddr));
    if (ret < 0)
    {
        printk(KERN_ERR "Failed to bind socket: %d\n", ret);
        sock_release(sock);
        return ret;
    }

    ret = kernel_listen(sock, MAX_CLIENTS);
    if (ret < 0)
    {
        printk(KERN_ERR "Failed to listen on socket: %d\n", ret);
        sock_release(sock);
        return ret;
    }

    while (!kthread_should_stop())
    {
        ret = kernel_accept(sock, &newsock, 0);
        if (ret < 0)
        {
            printk(KERN_ERR "Failed to accept client: %d\n", ret);
            continue;
        }

        for (i = 0; i < MAX_CLIENTS; i++)
        {
            if (client_list[i].sock == NULL)
            {
                client_list[i].sock = newsock;
                client_list[i].thread = kthread_run(client_thread_func, &client_list[i], "client_thread_%d", i);
                break;
            }
        }

        if (i == MAX_CLIENTS)
        {
            printk(KERN_WARNING "Max clients reached, closing new connection\n");
            kernel_sock_shutdown(newsock, SHUT_RDWR);
            sock_release(newsock);
        }
    }

    sock_release(sock);

    return 0;
}


static int __init tcp_server_init(void)
{
    int i;
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        client_list[i].sock = NULL;
        client_list[i].thread = NULL;
    }

    server_thread = kthread_run(server_thread_func, NULL, MODULE_NAME);
    printk(KERN_INFO "Server Started!\n");
    if (IS_ERR(server_thread))
    {
        printk(KERN_ERR "Failed to start server thread: %ld\n", PTR_ERR(server_thread));
        return PTR_ERR(server_thread);
    }

    return 0;
}

static void __exit tcp_server_exit(void)
{
    int i;

    for (i = 0; i < MAX_CLIENTS; i++)
    {
        if (client_list[i].sock != NULL)
        {
            kernel_sock_shutdown(client_list[i].sock, SHUT_RDWR);
            sock_release(client_list[i].sock);
            client_list[i].sock = NULL;
            kthread_stop(client_list[i].thread);
        }
        else
        {
        kthread_stop(server_thread);
        printk(KERN_INFO "Server Closed!\n");
        }
    }
}

module_init(tcp_server_init);
module_exit(tcp_server_exit);
