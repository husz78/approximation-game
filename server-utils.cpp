#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <algorithm>

#include "server-utils.h"
#include "err.h"
#include "common.h"

#define BUF_SIZE 1024

// File stream for the coefficient file.
static std::ifstream coeffs;

typedef struct {
    char buf[BUF_SIZE];
    size_t start;
    size_t end;
} Buffer;

static std::vector<Buffer> buffers;


// Comparator function for comparing players (their ids).
static bool player_comp(const PlayerData* a, const PlayerData* b) {
    return a->player_id < b->player_id;
} 

// Calculates the result of the player based on their state and coefficients.
static void calculate_result(PlayerData& player) {
    for (size_t i = 0; i < player.state.size(); i++) {
        player.result += (player.state[i] - get_sum_in_x(i, player.coeffs)) * 
                    (player.state[i] - get_sum_in_x(i, player.coeffs));
    }
}

void open_coeff_file(const std::string& coeff_file) {
    coeffs.open(coeff_file);
    if (!coeffs.is_open()) {
        fatal("cannot open coefficient file: %s", coeff_file.c_str());
    }
}

void close_coeff_file() {
    if (coeffs.is_open()) {
        coeffs.close();
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
// Send BAD_PUT with point, value to a player via descriptor fd.
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
    player.received_PUT_answer = true;
    std::ostringstream formatted_value;
    formatted_value << std::fixed << std::setprecision(7) << value;
    std::cout << "Sending BAD_PUT " << point << " " << formatted_value.str() 
                << " to " << player.player_id << ".\n";
}

// Send PENALTY with point, value to a player via descriptor fd.
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
                << " to " << player.player_id << ".\n";
}

// Send COEFF via descriptor fd, from the coefficient's file.
void send_COEFF(int fd, PlayerData& player, int N) {
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
    if (player.coeffs.size() != (size_t)N + 1) {
        fatal("Coefficient file wrong format.");
    }
    line += "\r\n";
    if (writen(fd, line.c_str(), line.size()) != (ssize_t)line.size()) {
        syserr("write()");
    }
    std::cout << player.player_id << " gets coefficients";
    for (double value : player.coeffs) {
        std::cout << " " << value;
    }
    std::cout << ".\n";
}

// Send SCORING to players via descriptors fds.
void send_SCORING(const std::vector<int>& fds, std::vector<PlayerData*>& players) {
    std::ostringstream oss_msg;
    std::ostringstream oss_output;
    oss_msg << "SCORING";
    oss_output << "Game end, scoring:";
    std::sort(players.begin(), players.end(), player_comp);

    for (size_t i = 0; i < players.size(); i++) {
        calculate_result(*players[i]);
        oss_msg << " " << players[i]->player_id << " " << round7(players[i]->result);
        oss_output << " " << players[i]->player_id << " " << round7(players[i]->result);
    }
    oss_msg << "\r\n";
    oss_output << ".\n";

    for (int fd : fds) {
        if (writen(fd, oss_msg.str().c_str(), oss_msg.str().size()) < 0) {
            syserr("write()");
        }
    }
    std::cout << oss_output.str();
}

// Send STATE to player via descriptor fd 
// with a state of the approximation of the player.
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
    if (writen(fd, oss_msg.str().c_str(), oss_msg.str().size()) < 0) {
        syserr("write()");
    }
    player.received_PUT_answer = true;
    std::cout << oss_output.str();
}


std::string receive_msg(int fd, int k, bool& erase) {
    Buffer& buffer = buffers[k];
    while (true) {
        for (size_t i = buffer.start; i + 1 < buffer.end; ++i) {
            if (buffer.buf[i] == '\r' && buffer.buf[i+1] == '\n') {
                std::string line(buffer.buf + buffer.start, i - buffer.start);
                size_t new_start = i + 2;
                size_t rem = buffer.end - new_start;
                if (rem > 0) {
                    memmove(buffer.buf, buffer.buf + new_start, rem);
                }
                buffer.start = 0;
                buffer.end = rem;
                return line;
            }
        }
        if (buffer.end == BUF_SIZE) {
            syserr("read(): buffer overflow");
        }
        ssize_t n = read(fd, buffer.buf + buffer.end, BUF_SIZE - buffer.end);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return "";
            }
            syserr("read()");
        }
        if (n == 0) {
            erase = true;
            return "";
        }
        buffer.end += (size_t)n;
    }
}

bool handle_HELLO_message(std::istringstream& iss, PlayerData& player, int fd,
                            const std::string& ip, int port, int N) {
    if (player.after_HELLO) return false;
    if (!(iss >> player.player_id)) {
        return false;
    }
    if (!iss.eof()) return false;

    if (!is_valid_player_id(player.player_id)) return false;
    player.after_HELLO = true;

    std::cout << ip << ":" << port << " is now known as " << player.player_id << ".\n";
    send_COEFF(fd, player, N);
    return true;
}

bool handle_PUT_message(std::istringstream& iss, PlayerData& player, int fd,
                        TimerAction& timer, int K, int& PUT_count) {
    if (!player.after_HELLO) return false;
    int point;
    std::string value_str;
    double value;
    if (!(iss >> point >> value_str)) {
        return false;
    }
    if (is_valid_decimal(value_str)) value = std::stod(value_str);
    else return false;
    bool PENALTY_sent = false;
    if (!player.received_PUT_answer || player.coeffs.empty()) {
        send_PENALTY(point, value, fd, player);
        PENALTY_sent = true;
        player.received_PUT_answer = true;
    }
    if (player.coeffs.empty()) return true;
    if (point < 0 || point > K || value < -5 || value > 5) {
        timer = TimerAction::BAD_PUT;
    } else {
        if (!PENALTY_sent) {
            if (player.state.empty()) player.state.assign(K + 1, 0.0);
            player.PUT_count++;
            player.state[point] += value;
            PUT_count++;
            timer = TimerAction::SEND_STATE;
        }
    }
    if (!PENALTY_sent) player.received_PUT_answer = false;
    std::cout << player.player_id << " puts " << round7(value) << " in " << 
                point << ", current state";

    for (double val : player.state) {
        std::cout << " " << round7(val);
    }
    std::cout << ".\n";
    return true;
} 

bool handle_message(const std::string& msg, PlayerData& player, int fd, 
                    TimerAction& timer, const std::string& ip, int port,
                    int K, int& PUT_count, int N) {
    std::istringstream iss(msg);
    std::string command;
    
    if (!(iss >> command)) {
        return false;
    }

    if (command  == "HELLO") {
        return handle_HELLO_message(iss, player, fd, ip, port, N);
    }
    else if (command == "PUT") {
        return handle_PUT_message(iss, player, fd, timer, K, PUT_count);
    }
    else return false;

}

void erase_kth_player(int k) {
    if (k >= 0 && k < (int)buffers.size())
        buffers.erase(buffers.begin() + k);
}

void erase_players() {
    buffers.clear();
}

PlayerData add_player() {
    buffers.push_back({});
    memset(&buffers.back(), 0, sizeof(Buffer));
    PlayerData p;
    p.received_PUT_answer = true;
    p.after_HELLO = false;
    return p;
}