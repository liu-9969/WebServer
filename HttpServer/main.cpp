#include "HttpServer.h"

int main(int argc, char** argv)
{
	// TODO 读配置文件
	int port = 80;
	if(argc >= 2) {
		port = atoi(argv[1]);
	}
	int numThread = 1;
	if(argc >= 3) {
		numThread = atoi(argv[2]);
	}

	swings::HttpServer server(port, numThread);
	server.run();

	return 0;
}