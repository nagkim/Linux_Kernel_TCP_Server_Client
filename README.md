# Linux_Kernel_TCP_Server_Client

This module implements TCP server and client that can send a message to a server and receive a response. The IP address and port of the server are defined in the code.

To use this module, you can follow these steps:

1. Clone this repo on your linux machine and extract.
2. Open the terminal where you extract the file and make root (with sudo su).
3. make
4. type 'sudo insmod tcp_server.ko'
5. type 'sudo insmod tcp_client.ko'
6. After the modules are loaded, client will attempt to connect to the server and send messages.
7. If the connection is successful, the module will print the message received from the server to the kernel log. To see messages type 'dmesg' to the terminal.
8. To unload the modules, run the command 'rmmod tcp_client' and 'rmmod tcp_server'

Before loading the client module, make sure that the server is running and listening on the correct port. You can test the module by running it and checking the kernel log for the message received from the server.

