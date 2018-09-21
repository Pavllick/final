#include <iostream>
#include <fstream>
// #include <regex>
#include <map>
#include <sstream>

#include <sys/epoll.h>
// #include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
// #include <sys/event.h>

#include <signal.h>

#include "./inc/args_parsing.h"
#include "./inc/sock_con.h"
#include "./inc/daemonize.h"

#ifndef MSG_NOSIGNAL
  #define MSG_NOSIGNAL 0
  #ifdef SO_NOSIGPIPE
    #define CEPH_USE_SO_NOSIGPIPE
  #else
    #error "Cannot block SIGPIPE!"
  #endif
#endif

int open_con(Args args) {
	Sock con;
	con.master_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	int optval = 1;
	setsockopt(con.master_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	
	struct sockaddr_in sa;
	sa.sin_family = AF_INET;
	sa.sin_port = htons(args.port);
	sa.sin_addr.s_addr = inet_addr(args.host);
	// inet_aton(args.host, &(sa.sin_addr));
	// sa.sin_addr.s_addr = htonl(INADDR_ANY);
	
	bind(con.master_socket, (struct sockaddr *)(&sa), sizeof(sa));
	Sock::set_nonblock(con.master_socket);
	listen(con.master_socket, SOMAXCONN);

	unsigned int EPoll = epoll_create1(0);
	struct epoll_event Event;
	Event.data.fd = con.master_socket;
	Event.events = EPOLLIN;
	epoll_ctl(EPoll, EPOLL_CTL_ADD, con.master_socket, &Event);

	con.handler(EPoll, args.dir);

	// int KQueue = kqueue();

	// struct kevent KEvent;
	// bzero(&KEvent, sizeof(KEvent));
	// EV_SET(&KEvent, con.master_socket, EVFILT_READ, EV_ADD, 0, 0, 0);
	// kevent(KQueue, &KEvent, 1, nullptr, 0, nullptr);

	// con.handler(KQueue, args.dir);
	
	return 0;
}

int main(int argc, char **argv) {
	Args args;
	if(args.get_args(argc, argv) == -1) {
		std::cout << "Arg err!" << std::endl;
		return 1;
	}

	daemonize();

	std::cout << open_con(args) << std::endl;

	return 0;
}
