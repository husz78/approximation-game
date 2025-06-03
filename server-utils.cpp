#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>

#include "server-utils.h"
#include "err.h"
#include "common.h"

static std::ifstream coeffs;

void open_coeff_file(const std::string& coeff_file) {
    coeffs.open(coeff_file);
    if (!coeffs.is_open()) {
        fatal("cannot open coefficient file: %s", coeff_file.c_str());
    }
}

int create_dual_stack(int port) {
    std::string port_s = std::to_string(port);
    struct addrinfo hints{}, *res;
    hints.ai_family   = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;

    int err = getaddrinfo(nullptr, port_s.c_str(), &hints, &res);
    int listen_fd = -1;
    if (err == 0) {
        listen_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (listen_fd >= 0) {
            int yes = 1;
            setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
            int off = 0;
            setsockopt(listen_fd, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof off);
            if (bind(listen_fd, res->ai_addr, res->ai_addrlen) < 0 ||
                listen(listen_fd, SOMAXCONN) < 0) {
                close(listen_fd);
                listen_fd = -1;
            }
        }
        freeaddrinfo(res);
    }
    if (listen_fd < 0) {
        struct addrinfo hints4{}, *res4;
        hints4.ai_family   = AF_INET;
        hints4.ai_socktype = SOCK_STREAM;
        hints4.ai_flags    = AI_PASSIVE;
        err = getaddrinfo(nullptr, port_s.c_str(), &hints4, &res4);
        if (err != 0) fatal("getaddrinfo IPv4: %s", gai_strerror(err));
        listen_fd = socket(res4->ai_family, res4->ai_socktype, res4->ai_protocol);
        if (listen_fd < 0) fatal("socket IPv4");
        int yes = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
        if (bind(listen_fd, res4->ai_addr, res4->ai_addrlen) < 0)
            syserr("bind IPv4");
        if (listen(listen_fd, SOMAXCONN) < 0)
            syserr("listen IPv4");
        freeaddrinfo(res4);
    }

    return listen_fd;
}

void send_BAD_PUT(int point, double value, int fd, PlayerData& player) {
    player.result += 10;
    std::ostringstream oss;
    oss << "BAD_PUT " << point << " " << std::fixed << std::setprecision(7) <<
                     value << "\r\n";
    std::string message = oss.str();
    if (writen(fd, message.c_str(), message.size()) != (ssize_t)message.size())
    {
        syserr("write()");
    }
    std::ostringstream formatted_value;
    formatted_value << std::fixed << std::setprecision(7) << value;
    std::cout << "Sending BAD_PUT " << point << " " << formatted_value.str() 
                << " to " << player.player_id;
}

void send_PENALTY(int point, double value, int fd, PlayerData& player) {
    player.result += 20;
    std::ostringstream oss;
    oss << "PENALTY " << point << " " << std::fixed << std::setprecision(7) <<
                     value << "\r\n";
    std::string message = oss.str();
    if (writen(fd, message.c_str(), message.size()) != (ssize_t)message.size())
    {
        syserr("write()");
    }
    std::ostringstream formatted_value;
    formatted_value << std::fixed << std::setprecision(7) << value;
    std::cout << "Sending PENALTY " << point << " " << formatted_value.str() 
                << " to " << player.player_id;
}

void send_COEFF(int fd, PlayerData& player) {
    std::string line;
    if (!std::getline(coeffs, line)) {
        fatal("Coefficient file exhausted.");
    }
    std::istringstream iss(line);
    std::string command;
    iss >> command;
    double coeff;
    while (iss >> coeff) {
        player.coeffs.push_back(coeff);
    }
    if (writen(fd, line.c_str(), line.size()) != (ssize_t)line.size()) {
        syserr("write()");
    }
    std::cout << player.player_id << " gets coefficients";
    for (double value : player.coeffs) {
        std::cout << " " << value;
    }
    std::cout << ".\n";
}

void send_SCORING(const std::vector<int>& fds, std::vector<PlayerData>& players) {
    std::ostringstream oss_msg;
    std::ostringstream oss_output;
    oss_msg << "SCORING";
    oss_output << "Game end, scoring:";
    for (int i = 0; i < players.size(); i++) {
        calculate_result(players[i]);
        oss_msg << " " << players[i].player_id << " " << round7(players[i].result);
        oss_output << " " << players[i].player_id << " " << round7(players[i].result);
    }
    oss_msg << "\r\n";
    oss_output << ".\n";

    for (int fd : fds) {
        if (writen(fd, oss_msg.str().c_str(), sizeof(oss_msg.str().c_str())) < 0) {
            syserr("write()");
        }
    }
    std::cout << oss_output.str();
}

void send_STATE(int fd, PlayerData& player) {
    std::ostringstream oss_msg, oss_output;
    oss_msg << "STATE";
    oss_output << "Sending state";

    for (double point : player.state) {
        oss_msg << " " << point;
        oss_output << " " << point;
    }
    oss_msg << "\r\n";
    oss_output << ".\n";
    if (writen(fd, oss_msg.str().c_str(), sizeof(oss_msg.str().c_str())) < 0) {
        syserr("write()");
    }
    std::cout << oss_output.str();
}

void calculate_result(PlayerData& player) {
    for (int i = 0; i < player.state.size(); i++) {
        player.result += (player.state[i] - get_sum_in_x(i, player.coeffs)) * 
                    (player.state[i] - get_sum_in_x(i, player.coeffs));
    }
}