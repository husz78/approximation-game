#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <cerrno>
#include <netdb.h>
#include <signal.h>
#include <poll.h>

#include "err.h"
#include "common.h"
#include "client-utils.h"

// Prints usage of the client.
static void usage(const char* prog) {
    std::cerr << "Usage: " << prog
              << " -u player_id -s server -p port "
              << "[-4] [-6] [-a]\n";
}


// Parses the arguments and checks if they're valid. If they're then
// corresponding variables are set. If they're not then print an error
// and exit with code 1.
static void parse_args(int argc, char** argv, std::string& player_id,
                       std::string& server, std::string& port, bool& force4,
                       bool& force6, bool& auto_mode) {
    int opt;
    while ((opt = getopt(argc, argv, "u:s:p:46a")) != -1) {
        switch (opt) {
        case 'u':
            player_id = optarg;
            break;
        case 's':
            server = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        case '4':
            force4 = true;
            break;
        case '6':
            force6 = true;
            break;
        case 'a':
            auto_mode = true;
            break;
        default:
            usage(argv[0]);
            fatal("invalid argument");
        }
    }
    if (server.empty()) {
        usage(argv[0]);
        fatal("missing -s parameter");
    }
    if (port.empty()) {
        usage(argv[0]);
        fatal("missing -p parameter");
    }
    if (!is_valid_player_id(player_id)) {
        usage(argv[0]);
        fatal("invalid player_id, it should contain only digits and letters");
    }
}

int main(int argc, char* argv[]) {
    std::string player_id, server, port;
    bool force4     = false;
    bool force6     = false;
    bool auto_mode  = false;

    parse_args(argc, argv, player_id, server, port, force4, force6, auto_mode);

    std::cout << "player_id=" << player_id
              << " server="    << server
              << " port="      << port
              << " force4="    << force4
              << " force6="    << force6
              << " auto="      << auto_mode << "\n";
    
    struct addrinfo hints;
    struct addrinfo *result;
    hints.ai_socktype = SOCK_STREAM;
    if (force4 && !force6) {
        hints.ai_family = AF_INET;
    }
    else if (!force4 && force6) {
        hints.ai_family = AF_INET6;
    }
    else {
        hints.ai_family = AF_UNSPEC;
    }
    int gai = getaddrinfo(server.c_str(), port.c_str(), &hints, &result);
    if (gai != 0) {
        fatal("getaddrinfo(%s): %s", server.c_str(), gai_strerror(gai));
    }
    struct addrinfo* ai = result;

    int sock_fd = socket(ai->ai_family, ai->ai_socktype, 0);
    if (sock_fd < 0) {
        syserr("socket()");
    }
    if (connect(sock_fd, ai->ai_addr, (socklen_t)ai->ai_addrlen) == -1) {
        syserr("connect()");
    }
    std::cout << "Connected to [" << sockaddr_to_ip(ai->ai_addr) << "]:" << 
                ((struct sockaddr_in*)ai->ai_addr)->sin_port << ".\n";
    signal(SIGPIPE, SIG_IGN);

    close(sock_fd);
    
    return 0;
}

void auto_play(int fd) {
    return;
}

void input_play(int fd) {
    struct pollfd poll_fds[2];

    poll_fds[0].fd = STDIN_FILENO;
    poll_fds[0].events = POLLIN;
    
    poll_fds[1].fd = fd;
    poll_fds[1].events = POLLIN;

    
    while (true) {
        for (int i = 0; i < 2; i++) poll_fds[i].revents = 0;

        int result = poll(poll_fds, 2, -1);
        if (result < 0) {
            syserr("poll()");
        }
        else if (result > 0) {
            if (poll_fds[0].revents & POLLIN) {
                int point;
                double value;
                get_input_from_stdin(point, value);
            }
            if (poll_fds[1].revents & POLLIN) {
                std::string msg = receive_msg(fd);
                if (msg.empty()) continue;
                
            }
        }
    }
}