#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <string>

// Convert sockaddr to human-readable IP string; prefer IPv4 if v4-mapped
std::string sockaddr_to_ip(const struct sockaddr* sa);

// Setup dual-stack listener (IPv6+IPv4 fallback).
// Returns the descriptor of the listening socket.
int create_dual_stack(int port);

#endif