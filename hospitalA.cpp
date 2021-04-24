#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <map>
#include <list>
#include <queue>
#include <set>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define LOCALHOST "127.0.0.1"
#define UDP_PORT "30591"
#define SCHEDULER_PORT "33591"

using namespace std;

// data stucture for min heap elements
struct Pair {
    int index;
    double distance;
    int parent;
    Pair(int i, double d, int p) {
        this->index = i;
        this->distance = d;
        this->parent = p;
    }
};

// min heap comparison
class MinHeapComp {
public:
    MinHeapComp() {}
    bool operator() (Pair &lhs, Pair &rhs) {
        return lhs.distance > rhs.distance;
    }
};

// construct map of graph
void readMap(map<int, map<int, double> > &graph, set<int> &locations) {
    string line;
    ifstream inf ("map.txt");
    if (inf.is_open()) {
        while (getline(inf, line)) {
            char char_line[line.size() + 1];
            strcpy(char_line, line.c_str());

            char* ptr = strtok(char_line, " ");
            int start = atoi(ptr);
            locations.insert(start);

            ptr = strtok(NULL, " ");
            int end = atoi(ptr);
            locations.insert(end);

            ptr = strtok(NULL, " ");
            double dist = atof(ptr);

            if (graph.count(start) == 0) {
                graph[start] = map<int, double>();
            }

            graph[start][end] = dist;

            if (graph.count(end) == 0) {
                graph[end] = map<int, double>();
            }

            graph[end][start] = dist;
        }
    }

    return;
}

// dijkstra's alhorithm
double findPath(map<int, map<int, double> > &graph, int start, int end) {
    // list<int> res;
    priority_queue<Pair, vector<Pair>, MinHeapComp> pq;
    pq.push(Pair(start, 0.0, -1));

    set<int> visited;
    // map<int, int> parent_map;
    double cur_sum = 0;
    double result = 0;

    while (!pq.empty()) {
        Pair p = pq.top();
        int cur_node = p.index;
        cur_sum = p.distance;
        if (visited.count(cur_node) > 0) {
            pq.pop();
            continue;
        }

        visited.insert(cur_node);
        // parent_map[cur_node] = p.parent;

        if (cur_node == end) {
            result = cur_sum;
            // cout << "shortest distance: " << setprecision(17) << cur_sum << endl;
            break;
        }

        map<int, double> neighbour = graph[cur_node];
        for (map<int, double>::iterator it = neighbour.begin(); it != neighbour.end(); ++it) {
            // cout << it->first << "," << it->second << endl;
            if (visited.count(it->first) == 0) {
                pq.push(Pair(it->first, cur_sum + it->second, cur_node));
            }
        }

        pq.pop();
    }

    // int backtrack = end;
    // while (backtrack != -1) {
    //     res.push_front(backtrack);
    //     backtrack = parent_map[backtrack];
    // }

    return result;
}

double calculateScore(double d, double a) {
    return 1 / (d * (1.1 - a));
}

// function for sending messages through UDP
// beej's tutorial
void sendMessage(char* msg, const char *port) {
    int status;
    struct addrinfo udp_hint;
    struct addrinfo *udp_servinfo;

    memset(&udp_hint, 0, sizeof udp_hint);
    udp_hint.ai_family = AF_UNSPEC;
    udp_hint.ai_socktype = SOCK_DGRAM;

    status = getaddrinfo(LOCALHOST, port, &udp_hint, &udp_servinfo);

    int socket_scheduler = socket(udp_servinfo->ai_family, udp_servinfo->ai_socktype, udp_servinfo->ai_protocol);
    int msg_len = strlen(msg);

    int send_bytes;
    if ((send_bytes = sendto(socket_scheduler, msg, msg_len, 0, udp_servinfo->ai_addr, udp_servinfo->ai_addrlen)) == -1) {
        cout << "send error" << endl;
        exit(1);
    }
    // cout << "send bytes: " << send_bytes << endl;

    freeaddrinfo(udp_servinfo);

    close(socket_scheduler);
}

int main(int argc, char *argv[]) {
    // initialize
    // command line input
    int location = atoi(argv[1]);
    int capacity = atoi(argv[2]);
    int occupancy = atoi(argv[3]);
    double availability = (double)(capacity - occupancy) / capacity;

    // set up UDP socket (receiver)
    // code from beej's tutorial
    int status_recv;
    struct addrinfo udp_hint_recv;
    struct addrinfo *udp_servinfo_recv;

    memset(&udp_hint_recv, 0, sizeof udp_hint_recv);
    udp_hint_recv.ai_family = AF_UNSPEC;
    udp_hint_recv.ai_socktype = SOCK_DGRAM;

    status_recv = getaddrinfo(LOCALHOST, UDP_PORT, &udp_hint_recv, &udp_servinfo_recv);

    int socket_udp_recv = socket(udp_servinfo_recv->ai_family, udp_servinfo_recv->ai_socktype, udp_servinfo_recv->ai_protocol);
    bind(socket_udp_recv, udp_servinfo_recv->ai_addr, udp_servinfo_recv->ai_addrlen);
    freeaddrinfo(udp_servinfo_recv);
    cout << "Hospital A is up and running using UDP on port " << UDP_PORT << ".\n";
    cout << "Hospital A has total capacity " << capacity << " and initial occupancy " << occupancy << ".\n";

    // send initilization information to scheduler through UDP
    // create initial message
    char *msg = new char[256];
    strcpy(msg, "A;");
    strcat(msg, argv[2]);
    strcat(msg, ";");
    strcat(msg, argv[3]);
    sendMessage(msg, SCHEDULER_PORT);

    // read map
    map<int, map<int, double> > graph;
    set<int> locations;
    readMap(graph, locations);

    // wait for query from scheduler
    struct sockaddr_storage sender_addr;
    socklen_t addr_len = sizeof sender_addr;
    int numbytes;

    while(true) {
        char *buf = new char[256];
        numbytes = recvfrom(socket_udp_recv, buf, 255, 0, (struct sockaddr *)&sender_addr, &addr_len);
        if (numbytes == -1) {
            perror("recvfrom");
            exit(1);
        }

        int phase = atoi(strtok(buf, ";"));
        // cout << "phase: " << phase << endl;
        switch (phase) {
            case 2: {
                // receives client location through UDP
                char *client_location = strtok(NULL, "");
                int cl = atoi(client_location);
                cout << "Hospital A has received input from client at location " << cl << ".\n";

                char *response = new char[256];

                if (locations.count(cl) == 0) {
                    // if location not found
                    // return error message phase 3e
                    cout << "Hospital A does not have the location " << cl << " in map.\n";
                    strcpy(response, "3e;A;None;None;");
                    sendMessage(response, SCHEDULER_PORT);
                    cout << "Hospital A has sent \"location not found\" to the Scheduler.\n";
                } else {
                    // find distance of shortest path and calculate score
                    double distance = findPath(graph, cl, location);
                    char *d_char = new char[20];
                    char *s_char = new char[20];

                    if (distance == 0) {
                        strcpy(d_char, "None");
                        strcpy(s_char, "None");
                    } else {
                        strcpy(d_char, to_string(distance).c_str());
                        double score = calculateScore(distance, availability);
                        strcpy(s_char, to_string(score).c_str());
                    }

                    cout << "Hospital A has found the shortest path to client, distance = " << d_char << ".\n";
                    cout << "Hospital A has the score = " << s_char << ".\n";

                    strcpy(response, "3;A;");
                    strcat(response, d_char);
                    strcat(response, ";");
                    strcat(response, s_char);
                    strcat(response, ";");
                    sendMessage(response, SCHEDULER_PORT);
                    cout << "Hospital A has sent score = " << s_char << " and distance = " << d_char << " to the Scheduler.\n";
                }
                break;
            }
            case 4:
                // receives assignment from scheduler
                occupancy += 1;
                availability = (double)(capacity - occupancy) / capacity;
                cout << "Hospital A has been assigned to a client, occupation is updated to " << occupancy 
                << ", availability is updated to " << availability << ".\n";
                break;
        }
    }

    close(socket_udp_recv);

    return 0;
}