#include "../inc/sock_con.h"

#include <iostream>
#include <fstream>
// #include <regex>
#include <sstream>
#include <string.h>
#include <set>

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#define MAX_EVENTS 32

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

std::pair<std::string, std::string> parse_find(std::string str_buf) {
	std::pair<std::string, std::string> params("", "");
	
	std::size_t find_pos = 0;
	find_pos = str_buf.find("GET");
	if(find_pos == std::string::npos)
		return params;
	else
		params.first = "GET";

	find_pos = 0;
	find_pos = str_buf.find("/");
	if(find_pos == std::string::npos) {
		return params;
	}
		
	if(str_buf[find_pos + 1] == ' ') {
		return params;
	}
	str_buf = str_buf.substr(find_pos + 1);
		
	find_pos = str_buf.find("?");
	if(find_pos != std::string::npos)
		params.second = str_buf.substr(0, find_pos);
	else {
		find_pos = str_buf.find(" ");
		if(find_pos != std::string::npos)
			params.second = str_buf.substr(0, find_pos);
	}
	 
	return params;
}

std::string Request::get_response(std::string req_header, std::string dir) {
	// std::regex rgx_get(R"((^GET){1}(?:\s){1}(?:\/){1}(\w+\.\w+){1})");
	// std::smatch m;
	// regex_search(req_header, m, rgx_get);

	std::pair<std::string, std::string> parse = parse_find(req_header);

	// std::string rm = m[1].str();
	// std::string path = m[2].str();
	std::string rm = parse.first;
	std::string path = parse.second;
	
	std::string answ;
	if(rm != "GET" || path == "")
		answ = Response::nf();
	else {
		std::string dir_path = dir + path;
		std::ifstream f(dir_path.c_str());
					
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

int Sock::assept(std::pair<int,  sockaddr> &slave, unsigned int &EPoll) {
	struct sockaddr slave_sa;
	socklen_t size_slave_sa = sizeof(slave_sa);
	int slave_sock = accept(master_socket, &slave_sa, &size_slave_sa);
	set_nonblock(slave_sock);

	struct epoll_event Event;
	Event.data.fd = slave_sock;
	Event.events = EPOLLIN;
	epoll_ctl(EPoll, EPOLL_CTL_ADD, slave_sock, &Event);

	slave.first = slave_sock;
	slave.second = slave_sa;
				
	return 0;
}

struct ThrArgs {
	unsigned int EPoll;
	std::string *dir;
	std::pair<int, sockaddr> slave;
	std::set<int> *fds;
	pthread_mutex_t *mtx;
};

void *Sock::slave_thread(void *args) {
	int EPoll = ((ThrArgs *)args)->EPoll;
	std::string dir = *((ThrArgs *)args)->dir;
	std::pair<int, sockaddr> slave = ((ThrArgs *)args)->slave;
	std::set<int> *fds = ((ThrArgs *)args)->fds;

	static char buf[1024];
	bzero(buf, sizeof(buf));

	int resv_size = recv(slave.first, buf, sizeof(buf), MSG_NOSIGNAL);
	if(resv_size <= 0) {
		shutdown(slave.first, SHUT_RDWR);
		close(slave.first);
		std::cout << "disconnection" << std::endl;

		pthread_mutex_lock(((ThrArgs *)args)->mtx);
		fds->erase(fds->find(slave.first));
		pthread_mutex_unlock(((ThrArgs *)args)->mtx);
	
		return args;
	}
				
	std::stringstream ss;
	ss << buf;
	std::string req_header;
	std::getline(ss, req_header);

	Request request;
	std::string answ = request.get_response(req_header, dir);
				
	send(slave.first, answ.c_str(), answ.size(), MSG_NOSIGNAL);

	shutdown(slave.first, SHUT_RDWR);
	close(slave.first);

	pthread_mutex_lock(((ThrArgs *)args)->mtx);
	fds->erase(fds->find(slave.first));
	pthread_mutex_unlock(((ThrArgs *)args)->mtx);
	
	return args;
}

int Sock::handler(unsigned int EPoll, std::string dir) {
	std::pair<int, sockaddr> slave;
	std::set<int> fds;

	std::cout << dir << std::endl;

	pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
	for(;;) {
		struct epoll_event Events[MAX_EVENTS];
		int n_events = epoll_wait(EPoll, Events, MAX_EVENTS, -1);
		// std::cout << n_events << std::endl;
		
		for(int i = 0; i < n_events; i++) {
			if(Events[i].data.fd == master_socket)
				assept(slave, EPoll);
			else {
				if(fds.find(Events[i].data.fd) == fds.end()) {
			
					pthread_mutex_lock(&mtx);
					fds.insert(Events[i].data.fd);
					pthread_mutex_unlock(&mtx);
					
					slave.first = Events[i].data.fd;
					ThrArgs thr_args = {EPoll, &dir, slave, &fds, &mtx};
					pthread_t th_id;
					pthread_create(&th_id, NULL, &Sock::slave_thread, &thr_args);
					pthread_detach(th_id);
				}
			}
		}
	}
}

