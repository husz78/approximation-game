#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H


// Setup dual-stack listener (IPv6+IPv4 fallback).
// Returns the descriptor of the listening socket.
int create_dual_stack(int port);

#endif