#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <netdb.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <cmath>

#include "err.h"
#include "common.h"


ssize_t writen(int fd, const void *vptr, size_t n) {
    ssize_t nleft, nwritten;
    const char *ptr;

    ptr = (const char *)vptr; // Can't do pointer arithmetic on void*.
    nleft = n;
    while (nleft > 0) {
        if ((nwritten = write(fd, ptr, nleft)) <= 0)
            return nwritten;  // error

        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}


bool parse_int(const char *s, int minv, int maxv, int &out) {
    errno = 0;
    char *end;
    long val = strtol(s, &end, 10);
    if (errno != 0 || *end != '\0') return false;
    if (val < minv || val > maxv) return false;
    out = (int)val;
    return true;
}

std::string sockaddr_to_ip(const struct sockaddr* sa) {
    char buf[INET6_ADDRSTRLEN];
    if (sa->sa_family == AF_INET) {
        auto* sin = (const struct sockaddr_in*)sa;
        inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof buf);
        return buf;
    } else if (sa->sa_family == AF_INET6) {
        auto* sin6 = (const struct sockaddr_in6*)sa;
        const uint8_t* a = sin6->sin6_addr.s6_addr;
        bool v4 = true;
        for (int i = 0; i < 10; ++i)
            if (a[i] != 0) { v4 = false; break; }
        if (v4 && a[10] == 0xff && a[11] == 0xff) {
            struct in_addr ipv4;
            memcpy(&ipv4, a + 12, sizeof ipv4);
            inet_ntop(AF_INET, &ipv4, buf, sizeof buf);
            return buf;
        }
        inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof buf);
        return buf;
    }
    return {};
}


bool is_valid_decimal(const std::string& str) {
    if (str.empty()) return false;
    
    size_t pos = 0;
    
    if (str[pos] == '-') {
        pos++;
        if (pos >= str.length()) return false; 
    }
    
    bool has_digits_before = false;
    while (pos < str.length() && std::isdigit(str[pos])) {
        has_digits_before = true;
        pos++;
    }
    
    if (!has_digits_before) return false; 
    
    if (pos < str.length() && str[pos] == '.') {
        pos++;
        
        size_t decimal_digits = 0;
        while (pos < str.length() && std::isdigit(str[pos])) {
            decimal_digits++;
            pos++;
        }
        
        if (decimal_digits > 7) return false;
    }
    
    return pos == str.length();
}

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

double round7(double x) {
    return std::round(x * 1e7) / 1e7;
}

double get_sum_in_x(int x, const std::vector<double>& coeffs) {
    double sum = coeffs[0];
    int exp = 1;
    for (size_t i = 1; i < coeffs.size(); i++) {
        exp *= x;
        sum += coeffs[i] * exp;
    }
    return sum;
}