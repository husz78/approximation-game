#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <fstream>
#include <vector>
#include <poll.h>
#include <chrono>
#include <fcntl.h>
#include <sstream>


#include "err.h" 
#include "common.h"
#include "server-utils.h"


using Clock     = std::chrono::steady_clock;
using TimePoint = Clock::time_point;


struct Client {
    int            fd;
    PlayerData     data{};
    TimePoint      hello_deadline;
    TimePoint      next_action;
    TimerAction    action = TimerAction::NONE;
    int            last_bad_point = 0; // for BAD_PUT timer
    double         last_bad_value = 0.0;
    std::string    ip;
    int            port = 0;
};

// Prints the usage of the program.
void usage(const char* prog) {
    std::cerr << "Usage: " << prog
              << " [-p port] [-k K] [-n N] [-m M] -f coeff_file\n"
              << "  -p port    server port (0–65535), default 0\n"
              << "  -k K       max point K (1–10000), default 100\n"
              << "  -n N       poly degree N (1–8), default 4\n"
              << "  -m M       max PUTs M (1–12341234), default 131\n"
              << "  -f file    coeffs file (required)\n";
}


// Parses the arguments and checks if they're valid. If they're then
// corresponding variables are set. If they're not then print an error
// and exit with code 1.
void parse_args(int& port, int& K, int& N, int& M, std::string& coeff_file,
                int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, "p:k:n:m:f:")) != -1) {
        switch (opt) {
        case 'p':
            if (!parse_int(optarg, 0, 65535, port)) {
                fatal("invalid port: %s", optarg);
            }
            break;
        case 'k':
            if (!parse_int(optarg, 1, 10000, K)) {
                fatal("invalid K: %s", optarg);
            }
            break;
        case 'n':
            if (!parse_int(optarg, 1, 8, N)) {
                fatal("invalid N: %s", optarg);
            }
            break;
        case 'm':
            if (!parse_int(optarg, 1, 12341234, M)) {
                fatal("invalid M: %s", optarg);
            }
            break;
        case 'f':
            coeff_file = optarg;
            break;
        default:
            usage(argv[0]);
            fatal("invalid argument");
        }
    }
    if (coeff_file.empty()) {
        usage(argv[0]);
        fatal("missing -f parameter");
    }
    
    open_coeff_file(coeff_file);
}

int main(int argc, char* argv[]) {
    int port = 0;
    int K    = 100;
    int N    = 4;
    int M    = 131;
    std::string coeff_file;
    int PUT_count = 0;

    parse_args(port, K, N, M, coeff_file, argc, argv);

    std::cout << "Starting server on port " << port
              << " with K=" << K
              << " N=" << N
              << " M=" << M
              << " file='" << coeff_file << "'\n";

    int listen_fd = create_dual_stack(port);
    // Set listen_fd to non-blocking.
    fcntl(listen_fd, F_SETFL, fcntl(listen_fd, F_GETFL, 0) | O_NONBLOCK);

    std::vector<Client> clients;
    std::vector<pollfd> pollfds;
    pollfds.push_back({listen_fd, POLLIN, 0});

    while (true) {
        auto now = Clock::now();
        auto nearest = now + std::chrono::hours(24);
        for (auto &c : clients) {
            if (!c.data.after_HELLO)
                nearest = std::min(nearest, c.hello_deadline);
            if (c.action != TimerAction::NONE)
                nearest = std::min(nearest, c.next_action);
        }
        int timeout = std::chrono::duration_cast<std::chrono::milliseconds>(nearest - now).count();
        if (timeout < 0) timeout = 0;

        int ready = poll(pollfds.data(), pollfds.size(), timeout);
        now = Clock::now();

        // Handle expired timers.
        for (size_t i = 1; i < pollfds.size(); ++i) {
            Client &c = clients[i-1];
            if (!c.data.after_HELLO && now >= c.hello_deadline) {
                close(c.fd);
                pollfds.erase(pollfds.begin() + i);
                clients.erase(clients.begin() + (i-1));
                --i;
                continue;
            }
            if (c.action == TimerAction::SEND_STATE && now >= c.next_action) {
                send_STATE(c.fd, c.data);
                c.action = TimerAction::NONE;
            }
            if (c.action == TimerAction::BAD_PUT && now >= c.next_action) {
                send_BAD_PUT(c.last_bad_point, c.last_bad_value, c.fd, c.data);
                c.action = TimerAction::NONE;
            }
        }

        if (ready > 0) {
            // Handle new client.
            if (pollfds[0].revents & POLLIN) {
                struct sockaddr_storage addr;
                socklen_t addrlen = sizeof(addr);
                int new_fd = accept(listen_fd, (struct sockaddr*)&addr, &addrlen);
                if (new_fd >= 0) {
                    // Set new client socket to non-blocking.
                    fcntl(new_fd, F_SETFL, fcntl(new_fd, F_GETFL, 0) | O_NONBLOCK);
                    Client nc;
                    nc.fd = new_fd;
                    nc.hello_deadline = Clock::now() + std::chrono::seconds(3);
                    nc.ip = sockaddr_to_ip((struct sockaddr*)&addr);
                    if (addr.ss_family == AF_INET) {
                        nc.port = ntohs(((struct sockaddr_in*)&addr)->sin_port);
                    } else if (addr.ss_family == AF_INET6) {
                        nc.port = ntohs(((struct sockaddr_in6*)&addr)->sin6_port);
                    }
                    nc.data = add_player();
                    clients.push_back(nc);
                    pollfds.push_back({new_fd, POLLIN, 0});
                    std::cout << "New client [" << nc.ip << "]:" << nc.port << ".\n";
                }
            }
            // Handle existing clients.
            for (size_t i = 1; i < pollfds.size(); ++i) {
                // Check for game end and if yes then end game
                // and start a new one.
                if (PUT_count == M) {
                    std::vector<int> fds;
                    std::vector<PlayerData*> players;
                    for (size_t j = 1; j < pollfds.size(); j++) {
                        fds.push_back(pollfds[j].fd);
                        players.push_back(&clients[j - 1].data);
                        
                    }
                    send_SCORING(fds, players);
                    erase_players();
                    clients.clear();
                    for (size_t i = 1; i < pollfds.size(); i++) close(pollfds[i].fd);
                    pollfds.clear();
                    pollfds.push_back({listen_fd, POLLIN, 0});
                    PUT_count = 0;
                    sleep(1);
                    break;
                }
                if (pollfds[i].revents & POLLIN) {
                    bool erase = false;
                    std::string msg = receive_msg(pollfds[i].fd, i-1, erase);
                    if (erase) {
                        close(pollfds[i].fd);
                        PUT_count -= clients[i - 1].data.PUT_count;
                        pollfds.erase(pollfds.begin() + i);
                        clients.erase(clients.begin() + (i-1));
                        erase_kth_player(i - 1);
                        --i;
                        continue;
                    }
                    Client &c = clients[i-1];
                    TimerAction timer = TimerAction::NONE;
                    if (handle_message(msg, c.data, c.fd, timer, c.ip, c.port, K, PUT_count)) {
                        if (timer == TimerAction::SEND_STATE) {
                            int low = 0;
                            for (char ch : c.data.player_id) {
                                if (ch >= 'a' && ch <= 'z') ++low;
                            }
                            c.action = TimerAction::SEND_STATE;
                            c.next_action = Clock::now() + std::chrono::seconds(low);
                        } else if (timer == TimerAction::BAD_PUT) {
                            std::istringstream iss(msg);
                            std::string cmd; int pt; double val;
                            iss >> cmd >> pt >> val;
                            c.last_bad_point = pt;
                            c.last_bad_value = val;
                            c.action = TimerAction::BAD_PUT;
                            c.next_action = Clock::now() + std::chrono::seconds(1);
                        } else {
                            c.action = TimerAction::NONE;
                        }
                    } else {
                        error("bad message from [%s]:%d, %s: %s", c.ip, c.port, c.data.player_id, msg);
                    }
                }
            }
        }
    }
    close(listen_fd);
    return 0;
}