#ifndef ARGS_PARSING_H
#define ARGS_PARSING_H

#include <iostream>
#include <unistd.h>
#include <stdlib.h>

struct Args {
	int get_args(int argc, char **argv) {
		int arg_pres;
		while((arg_pres = getopt(argc, argv, "h:p:d:")) != -1) {
			switch(arg_pres) {
			case 'h':
				host = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'd':
				dir  = optarg;
				break;
			}
		}
	
		if(host == NULL || port == 0)
			return -1;

		if(dir == "")
			return -1;
			//dir = "./";
		if(dir[dir.size() - 1] != '/')
			dir.append("/");

		return 0;
	}
	
	char *host = NULL;
	int port = 0;
	std::string dir = "";
};

#endif // ARGS_PARSING_H
