#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string>

#include "server-utils.h"
#include "err.h"


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
            std::memcpy(&ipv4, a + 12, sizeof ipv4);
            inet_ntop(AF_INET, &ipv4, buf, sizeof buf);
            return buf;
        }
        inet_ntop(AF_INET6, &sin6->sin6_addr, buf, sizeof buf);
        return buf;
    }
    return {};
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