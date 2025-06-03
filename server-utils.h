#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <vector>

// This is the structure representing a player's data
// that the server stores.
typedef struct {
    // Player's id, alphanumeric.
    std::string player_id; 
    // Coefficients of the polynomial of a given player.
    std::vector<double> coeffs; 
    // State of the current approximation of a player.
    std::vector<double> state;
    // The result of the player.
    double result; 
} PlayerData;

// Opens the coefficient file and exits with error if it can't be opened.
void open_coeff_file(const std::string& coeff_file);

// Closes the coefficient file.
void close_coeff_file();

// Setup dual-stack listener (IPv6+IPv4 fallback).
// Returns the descriptor of the listening socket.
int create_dual_stack(int port);

// Send STATE to player via descriptor fd 
// with a state of the approximation of the player.
void send_STATE(int fd, PlayerData& player);

// Send SCORING to players via descriptors fds.
void send_SCORING(const std::vector<int>& fds, std::vector<PlayerData>& players);

// Send COEFF via descriptor fd, from the coefficient's file.
void send_COEFF(int fd, PlayerData& player);

// Send PENALTY with point, value to a player via descriptor fd.
void send_PENALTY(int point, double value, int fd, PlayerData& player);

// Send BAD_PUT with point, value to a player via descriptor fd.
void send_BAD_PUT(int point, double value, int fd, PlayerData& player);


#endif