#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define LOCALHOST "127.0.0.1"
#define SCH_TCP_PORT "34591"

using namespace std;

int main(int argc, char *argv[]) {
	cout << "The client is up and running." << endl;
	char *cl = argv[1];

	// tcp socket for scheduler
	int status_tcp;
    struct addrinfo tcp_hint;
    struct addrinfo *tcp_servinfo;

    memset(&tcp_hint, 0, sizeof tcp_hint);
    tcp_hint.ai_family = AF_UNSPEC;
    tcp_hint.ai_socktype = SOCK_STREAM;

    status_tcp = getaddrinfo(LOCALHOST, SCH_TCP_PORT, &tcp_hint, &tcp_servinfo);
    if (status_tcp == -1) {
    	perror("getaddrinfo");
    	exit(1);
    }

    int socket_tcp = socket(tcp_servinfo->ai_family, tcp_servinfo->ai_socktype, tcp_servinfo->ai_protocol);
    if (socket_tcp == -1) {
    	perror("socket");
    	exit(1);
    }

    if (connect(socket_tcp, tcp_servinfo->ai_addr, tcp_servinfo->ai_addrlen) == -1) {
    	perror("connect");
    	exit(1);
    }

    char *msg = new char[256];
    strcat(msg, cl);
    int len = strlen(msg);

    int bytes = send(socket_tcp, msg, len, 0);
    if (bytes == -1) {
    	perror("send");
    	exit(1);
    }

	cout << "The client has sent query to Scheduler using TCP: client location " << cl << ".\n";

	char *buf = new char[256];
	int recv_bytes = recv(socket_tcp, buf, 255, 0);
	char *assign = strtok(buf, ";");
	cout << "The client has received results from the Scheduler: assigned to Hospital " << assign << ".\n";

	if (strcmp(assign, "None") == 0) {
		char *err_msg = strtok(NULL, ";");
		if (strcmp(err_msg, "NF") == 0) {
			cout << "Location " << cl << " not found.\n";
		} else if (strcmp(err_msg, "SCR") == 0) {
			cout << "Score = None, No assignment.\n";
		}
	}

	freeaddrinfo(tcp_servinfo);

	close(socket_tcp);

	return 0;
}