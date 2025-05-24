#include <sstream>
#include <iomanip>
#include <iostream>

#include "client-utils.h"
#include "common.h"
#include "err.h"


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