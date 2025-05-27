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
    std::cout << "Sending HELLO.\n";
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

bool handle_message(const std::string& msg, std::vector<double>& coeffs, bool auto_mode,
                    std::vector<double>& state_vector, int fd,
                    std::vector<std::pair<int, double>>& pending_puts) {
        std::istringstream iss(msg);
        std::string command;
        if (!(iss >> command))
            return false;
        
        if (command == "COEFF") {
            // return handle_coeff_message();
        }
        else if (command == "STATE") {
            // return handle_state_message();
        }
        else if (command == "SCORING") {
            // return handle_scoring_message();
        }
        else if (command == "BAD_PUT") {
            // return handle_bad_put_message();
        }
        else if (command == "PENALTY") {
            // return handle_penalty_message();
        }
        else return false;
}

bool handle_penalty_message(std::istringstream& iss) {
    int point;
    std::string value_str;
    double value;
    if (!(iss >> point >> value_str) || !iss.eof()) return false;
    if (!is_valid_decimal(value_str)) return false;

    value = std::stod(value_str);
    std::cout << "Received penalty for point " << point << " with value " << value << ".\n";
    return true;
}

bool handle_bad_put_message(std::istringstream& iss) {
    int point;
    std::string value_str;
    double value;
    if (!(iss >> point >> value_str) || !iss.eof()) return false;
    if (!is_valid_decimal(value_str)) return false;
    
    value = std::stod(value_str);
    std::cout << "Received penalty for point " << point << " with value " << value << ".\n";
    return true;
}

bool handle_coeff_message(std::istringstream& iss, std::vector<double>& coeffs, bool auto_mode,
                    const std::vector<double>& state_vector, int fd,
                    std::vector<std::pair<int, double>>& pending_puts) {
    if (!(state_vector.empty() || coeffs.empty())) {
        return false;
    }
    std::string coeff_str;
    double coeff;
    bool ok = true;
    while (!iss.eof()) {
        if (!(iss >> coeff_str)) {
            ok = false;
            break;
        }
        if (!is_valid_decimal(coeff_str)) {
            ok = false;
            break;
        }
        coeff = std::stod(coeff_str);
        if (abs(coeff) > 100) {
            ok = false;
            break;
        }
        coeffs.push_back(coeff);
    }
    if (ok == false || coeffs.size() > 9 || coeffs.size() < 2) {
        coeffs.clear();
        return false;
    }
    std::cout << "Received coefficients";
    for (double n : coeffs) {
        std::cout << " " << n;
    }
    std::cout << ".\n";
    if (auto_mode) {
        // send_best_PUT();
    }
    else {
        for (auto put : pending_puts) {
            send_PUT(put.first, put.second, fd);
        }
    }
    pending_puts.clear();
    return true;
}

bool handle_scoring_message(std::istringstream& iss) {
    std::string message = "Game end, scoring:";
    std::string player;
    std::string score_str;
    while (!iss.eof()) {
        if (!(iss >> player >> score_str)) return false;
        if (!is_valid_decimal(score_str)) return false;
        message += " " + player + " " + score_str;
    }
    std::cout << message << ".\n";
    return true;
}

bool handle_state_message(std::istringstream& iss, const std::vector<double>& coeffs, bool auto_mode,
                    std::vector<double>& state_vector, int fd) {
    int k = state_vector.size();
    std::vector<double> tmp_state;
    bool known_size = k == 0 ? false : true;
    int counter = 0;
    std::string r_str;
    double r;
    std::string message = "Received state";
    while (!iss.eof()) {
        if (!(iss >> r_str)) return false;
        counter++;
        if (!is_valid_decimal(r_str)) return false;
        message += " " + r_str;
        r = std::stod(r_str);
        tmp_state.push_back(r);
    }
    if (known_size && tmp_state.size() != k) return false;
    for (int i = 0; i < k; i++) state_vector[i] = tmp_state[i];
    std::cout << message << ".\n";
    
    // TODO Compute the best put if auto_mode and send it.
    return true;
}