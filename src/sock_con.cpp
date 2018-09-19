#include "../inc/sock_con.h"

#include <iostream>
#include <fstream>
#include <regex>
// #include <map>
#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/event.h>

int Sock::set_nonblock(int fd) {
	int flags;
#if defined(O_NONBLOCK)
	if(-1 == (flags = fcntl(fd, F_GETFL, 0)))
		flags = 0;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
#else
	flags = 1;
	return ioctl(fd, FIOBIO, &flags);
#endif
}

std::string Request::get_response(std::string req_header, std::string dir) {
	std::regex rgx_get(R"((^GET){1}(?:\s){1}(?:\/){1}(\w+\.\w+){1})");
	std::smatch m;
	regex_search(req_header, m, rgx_get);

	std::string rm = m[1].str();
	std::string path = m[2].str();

	std::string answ;
	if(rm != "GET" && path == "")
		answ = Response::nf();
	else {
		std::ifstream f(dir + path);
					
		if(!f)
			answ = Response::nf();
		else {
			f.seekg (0, f.end);
			int length = f.tellg();
			f.seekg (0, f.beg);

			char *page_buf = new char[length];
			f.read(page_buf, length);
			std::string page(page_buf);
			
			answ = Response::ok(page);

			std::cout << length << std::endl;
			std::cout << rm << " /" << path << std::endl;
		}
					
		f.close();
	}

	return answ;
}

std::string Response::ok(std::string msg) {
	std::stringstream answ;
	answ << "HTTP/1.1 200 OK\r\n"
			 << "Content-Type: text/html\r\n"
			 << "Content-Length: " << msg.size() << "\r\n"
			 << "\r\n"
			 << msg;

	return answ.str();
}

std::string Response::nf() {
	std::stringstream answ;
	answ << "HTTP/1.1 404 NOT FOUND\r\n"
			 << "Content-Type: text/html\r\n"
			 << "Content-Length: 0\r\n"
			 << "\r\n\r\n";

	return answ.str();
}

int Sock::assept(struct kevent &KEvent, std::pair<int,  sockaddr> &slave, int &KQueue) {
	struct sockaddr slave_sa;
	socklen_t size_slave_sa = sizeof(slave_sa);
	int slave_sock = accept(master_socket, &slave_sa, &size_slave_sa);
	set_nonblock(slave_sock);

	bzero(&KEvent, sizeof(KEvent));
	EV_SET(&KEvent, slave_sock, EVFILT_READ, EV_ADD, 0, 0, 0);
	kevent(KQueue, &KEvent, 1, nullptr, 0, nullptr);

	slave.first = slave_sock;
	slave.second = slave_sa;
				
	return 0;
}

struct ThrArgs {
	int KQueue;
	std::string dir;
	std::pair<int, sockaddr> slave;
};

void *Sock::slave_thread(void *args) {
	int KQueue = ((ThrArgs *)args)->KQueue;
	std::string dir = ((ThrArgs *)args)->dir;
	std::pair<int, sockaddr> slave = ((ThrArgs *)args)->slave;

	struct kevent KEvent;
	for(;;) {
		bzero(&KEvent, sizeof(KEvent));
		kevent(KQueue, nullptr, 0, &KEvent, 1, nullptr);

		if(KEvent.filter & EVFILT_READ) {
			if(KEvent.ident == slave.first) {
				std::string req_header;

				static char buf[1024];
				bzero(buf, sizeof(buf));
	
				int resv_size = recv(slave.first, buf, sizeof(buf), MSG_NOSIGNAL);
				if(resv_size <= 0) {
					close(KEvent.ident);
					std::cout << "disconnection" << std::endl;
					return args;
				}
				
				std::stringstream ss;
				ss << buf;
				std::getline(ss, req_header);

				Request request;
				std::string answ = request.get_response(req_header, dir);
				
				send(slave.first, answ.c_str(), answ.size(), MSG_NOSIGNAL);
			}
		}
	}

	return args;
}

int Sock::handler(int KQueue, std::string dir) {
	std::pair<int, sockaddr> slave;
	struct kevent KEvent;
	for(;;) {
		bzero(&KEvent, sizeof(KEvent));
		kevent(KQueue, nullptr, 0, &KEvent, 1, nullptr);
		
		if(KEvent.filter & EVFILT_READ) {
			if(KEvent.ident == master_socket) {
				assept(KEvent, slave, KQueue);

				ThrArgs thr_args = {KQueue, dir, slave};
				pthread_t th_id;
				pthread_create(&th_id, NULL, &Sock::slave_thread, &thr_args);
				pthread_detach(th_id);
			}
		}
	}
}
