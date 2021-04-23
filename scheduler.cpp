#include <iostream>
#include <list>
#include <set>
#include <map>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define PORT_A "30591"
#define PORT_B "31591"
#define PORT_C "32591"
#define PORT_UDP "33591"
#define PORT_TCP "34591"
#define LOCALHOST "127.0.0.1"
#define BACKLOG 10
#define HOSPITAL_NUM 3

using namespace std;

struct Pair {
	char name;
	double distance;
	double score;
	Pair(char n, double d, double s) {
		this->name = n;
		this->distance = d;
		this->score = s;
	}
};

void sendMessage(char *msg, char name, int phase) {
	char *port = new char[100];
	switch (name) {
		case 'A':
			strcpy(port, PORT_A);
			break;
		case 'B':
			strcpy(port, PORT_B);
			break;
		case 'C':
			strcpy(port, PORT_C);
			break;
	}

	int status_send;
    struct addrinfo udp_hint_send;
    struct addrinfo *udp_servinfo_send;

    memset(&udp_hint_send, 0, sizeof udp_hint_send);
    udp_hint_send.ai_family = AF_UNSPEC;
    udp_hint_send.ai_socktype = SOCK_DGRAM;

    status_send = getaddrinfo(LOCALHOST, port, &udp_hint_send, &udp_servinfo_send);

    int socket_hospital_send = socket(udp_servinfo_send->ai_family, udp_servinfo_send->ai_socktype, 
    	udp_servinfo_send->ai_protocol);
    int msg_len = strlen(msg);

    int send_init;
    if ((send_init = sendto(socket_hospital_send, msg, msg_len, 0, udp_servinfo_send->ai_addr, 
    	udp_servinfo_send->ai_addrlen)) == -1) {
        cout << "send error" << endl;
        exit(1);
    }

    switch (phase) {
    	case 2:
    		cout << "The Scheduler has sent client location to Hospital " << name << " using UDP over port " << port << endl;
    		break;
    	case 4:
    		cout << "The Scheduler has sent the result to Hospital " << name << " using UDP over port " << port << endl;
    		break;
    }

    freeaddrinfo(udp_servinfo_send);

    close(socket_hospital_send);
}

int countAvailableHos(map<char, int> m, char *buf) {
	int count = 0;
	char *port;

	for (map<char, int>::iterator it = m.begin(); it != m.end(); ++it) {
		if (it->second > 0) {
			sendMessage(buf, it->first, 2);
			count++;
		}
	}

	return count;
}

char* decide(list<Pair> l) {
	char *res = new char[1];
	double max_score = -1;
	double d = -1;
	for (list<Pair>::iterator it = l.begin(); it != l.end(); ++it) {
		if (it->score > max_score) {
			max_score = it->score;
			d = it->distance;
			res[0] = it->name;
		} else if (it->score == max_score && it->distance < d) {
			d = it->distance;
			res[0] = it->name;
		}
	}

	return res;
}

int main() {
	cout << "The Scheduler is up and running." << endl;

	int udp_status_recv;
	struct addrinfo udp_hint_recv;
	struct addrinfo *udp_servinfo_recv;
	struct sockaddr_storage sender_addr;

	memset(&udp_hint_recv, 0, sizeof udp_hint_recv);
	udp_hint_recv.ai_family = AF_UNSPEC;
	udp_hint_recv.ai_socktype = SOCK_DGRAM;

	if ((udp_status_recv = getaddrinfo(LOCALHOST, PORT_UDP, &udp_hint_recv, &udp_servinfo_recv)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(udp_status_recv));
		exit(1);
	}

	int socket_udp_recv = socket(udp_servinfo_recv->ai_family, udp_servinfo_recv->ai_socktype, udp_servinfo_recv->ai_protocol);
	bind(socket_udp_recv, udp_servinfo_recv->ai_addr, udp_servinfo_recv->ai_addrlen);
	freeaddrinfo(udp_servinfo_recv);

	socklen_t addr_len = sizeof sender_addr;
	int numbytes;

	map<char, int> availability_map;

	// UDP socket listening to initial messages from hospital
	char *initial_msg = new char[256];
	strcpy(initial_msg, "1;");
	sendMessage(initial_msg, 'A', 1);
	sendMessage(initial_msg, 'B', 1);
	sendMessage(initial_msg, 'C', 1);

	while (availability_map.size() < HOSPITAL_NUM) {
		char *buf = new char[256];
		numbytes = recvfrom(socket_udp_recv, buf, 255, 0, (struct sockaddr *)&sender_addr, &addr_len);
		if (numbytes == -1) {
			perror("recvfrom");
			exit(1);
		}

		char *ptr = strtok(buf, ";");
		char name = ptr[0];

		ptr = strtok(NULL, ";");
		int capacity = atoi(ptr);

		ptr = strtok(NULL, "");
		int initial_occupancy = atoi(ptr);

		// store availability locally
		availability_map[name] = capacity - initial_occupancy;

		cout << "The Scheduler has received information from Hospital " << name << 
		": total capacity is " << capacity << " and initial occupancy is " << initial_occupancy << ".\n";
	}

	// cout << "initialization finished" << availability_map.size() << endl;

	// set up TCP with client
	int status_tcp;
    struct addrinfo tcp_hint;
    struct addrinfo *tcp_servinfo;

    memset(&tcp_hint, 0, sizeof tcp_hint);
    tcp_hint.ai_family = AF_UNSPEC;
    tcp_hint.ai_socktype = SOCK_STREAM;

    getaddrinfo(LOCALHOST, PORT_TCP, &tcp_hint, &tcp_servinfo);
    int socket_tcp = socket(tcp_servinfo->ai_family, tcp_servinfo->ai_socktype, tcp_servinfo->ai_protocol);
    bind(socket_tcp, tcp_servinfo->ai_addr, tcp_servinfo->ai_addrlen);

    freeaddrinfo(tcp_servinfo);

    listen(socket_tcp, BACKLOG);
    socklen_t sin_size;
    bool illegal;
    bool not_found;

    while(true) {
    	// wait for query from clients
    	char *buf = new char[256];
    	sin_size = sizeof sender_addr;
    	int new_fd = accept(socket_tcp, (struct sockaddr *)&sender_addr, &sin_size);
    	numbytes = recv(new_fd, buf, 255, 0);
    	cout << "The Scheduler has received client at location " << buf << " from the client using TCP over port "
    	<< PORT_TCP << ".\n";

    	list<Pair> query_list;
    	char *query = new char[256];
    	strcpy(query, "2;");
    	strcat(query, buf);
    	int available_count = countAvailableHos(availability_map, query);
    	set<char> query_set;

    	illegal = false;
    	not_found = false;

    	while (query_set.size() < available_count) {
    		char *response = new char[256];
			numbytes = recvfrom(socket_udp_recv, response, 255, 0, (struct sockaddr *)&sender_addr, &addr_len);
			char *phase = strtok(response, ";");
			// cout << phase << endl;
			char n = strtok(NULL, ";")[0];
			char *d = strtok(NULL, ";");
			char *s = strtok(NULL, ";");
			cout << "The Scheduler has received map information from Hospital " << n << " , the score = " << s 
			<< " and the distance = " << d << ".\n";

			if (strcmp(phase, "3e") == 0) {
				illegal = true;
				not_found = true;
			} else if (strcmp(d, "None") == 0) {
				illegal = true;
			} else if (!illegal) {
				query_list.push_back(Pair(n, atof(d), atof(s)));
			}

			query_set.insert(n);
    	}

    	char *reply = new char[256];
    	// if !illegal, compare and pick
		if (illegal) {
			strcat(reply, "None;");
			if (not_found) {
				strcat(reply, "NF;");
			} else {
				strcat(reply, "SCR;");
			}
		} else {
			char *choice = decide(query_list);
			cout << "The Scheduler has assigned Hospital " << choice << " to the client.\n";
			strcat(reply, choice);
		}

		// cout << "reply: " << reply << endl;
    	send(new_fd, reply, strlen(reply), 0);
    	cout << "The Scheduler has sent the result to client using TCP over port " << PORT_TCP << ".\n";

    	char *assigned_hospital = strtok(reply, ";");
    	if (strcmp(assigned_hospital, "None") != 0) {
    		char *msg = new char[256];
    		strcpy(msg, "4;");
    		sendMessage(msg, assigned_hospital[0], 4);
    	}
    }

	close(socket_udp_recv); 
	close(socket_tcp);

	return 0;
}