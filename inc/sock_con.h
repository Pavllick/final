#ifndef SOCK_CON_H
#define SOCK_CON_H

#include <sstream>
#include <sys/socket.h>

#ifndef MSG_NOSIGNAL
  #define MSG_NOSIGNAL 0
#ifdef SO_NOSIGPIPE
  #define CEPH_USE_SO_NOSIGPIPE
#else
  #error "Cannot block SIGPIPE!"
#endif
#endif

struct Request {
	std::string get_response(std::string req_header, std::string dir);
	// Request(std::string req_header, std::string dir);

	std::string answ;
};

struct Response {
	static std::string ok(std::string msg);
	static std::string nf();
};

class Sock {
public:
	static int set_nonblock(int fd);
	
	int assept(struct kevent &KEvent, std::pair<int,  sockaddr> &slave, int &KQueue);
	static void *slave_thread(void *args);
	int handler(int KQueue, std::string dir);

	int master_socket;
};

#endif // SOCK_CON_H
