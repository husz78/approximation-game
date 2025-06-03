#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <vector>


typedef struct {
    std::string player_id;
    std::vector<double> coeffs;
    std::vector<double> state;
    double result;
} PlayerData;

// Opens the coefficient file and exits with error if it can't be opened.
void open_coeff_file(const std::string& coeff_file);

// Setup dual-stack listener (IPv6+IPv4 fallback).
// Returns the descriptor of the listening socket.
int create_dual_stack(int port);

#endif