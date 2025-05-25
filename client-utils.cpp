#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstring>
#include <unistd.h>

#include "client-utils.h"
#include "common.h"
#include "err.h"

static constexpr size_t BUF_SIZE = 65536;
static char buf[BUF_SIZE];
static size_t buf_start = 0, buf_end = 0;


bool is_valid_player_id(const std::string& player_id) {
    if (player_id.empty()) return false;
    
    for (char c : player_id) {
        if (!((c >= '0' && c <= '9') || 
              (c >= 'a' && c <= 'z') ||  
              (c >= 'A' && c <= 'Z'))) { 
            return false;
        }
    }
    return true;
}

int send_HELLO(const std::string& player_id, int fd) {
    std::string message = "HELLO " + player_id + "\r\n";
    if (writen(fd, message.c_str(), message.size()) <= 0) {
        syserr("write()");
        return -1;
    }
    return 0;
}

void send_PUT(int point, double value, int fd) {
    std::ostringstream oss;
    oss << "PUT " << point << " " << std::fixed << std::setprecision(7) << value << "\r\n";
    std::string message = oss.str();
    
    if (writen(fd, message.c_str(), message.size()) != (ssize_t)message.size()) {
        syserr("write()");
    }
    std::cout << "Putting " << value << " in " << point << ".\n";
}

bool get_input_from_stdin(int& point, double& value) {
    std::string line;
    
    std::istringstream iss(line);
    if (!(iss >> point >> value)) {
        error("invalid input line %s", line.c_str());
        return false;
    }
    
    std::string remaining;
    if (iss >> remaining) {
        error("invalid input line %s", line.c_str());
        return false;
    }
    return true;
}

std::string receive_msg(int fd) {
    while (true) {
        for (size_t i = buf_start; i + 1 < buf_end; ++i) {
            if (buf[i] == '\r' && buf[i+1] == '\n') {
                std::string line(buf + buf_start, i - buf_start);
                size_t new_start = i + 2;
                size_t rem = buf_end - new_start;
                if (rem > 0) {
                    memmove(buf, buf + new_start, rem);
                }
                buf_start = 0;
                buf_end = rem;
                return line;
            }
        }

        if (buf_end == BUF_SIZE) {
            syserr("receive_msg: buffer overflow");
        }
        ssize_t n = read(fd, buf + buf_end, BUF_SIZE - buf_end);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return "";
            }
            syserr("read()");
        }
        if (n == 0) {
            return "";
        }
        buf_end += (size_t)n;
    }
}