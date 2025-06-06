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
    // True if after HELLO and false otherwise.
    bool after_HELLO;
    // Number of correct PUTs by a player.
    int PUT_count;
    // True if a player received an answer to their last PUT.
    // False otherwise.
    bool received_PUT_answer;
} PlayerData;

enum class TimerAction { NONE, SEND_STATE, BAD_PUT };

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
void send_SCORING(const std::vector<int>& fds, std::vector<PlayerData*>& players);

// Send COEFF via descriptor fd, from the coefficient's file.
void send_COEFF(int fd, PlayerData& player, int K);

// Send PENALTY with point, value to a player via descriptor fd.
void send_PENALTY(int point, double value, int fd, PlayerData& player);

// Send BAD_PUT with point, value to a player via descriptor fd.
void send_BAD_PUT(int point, double value, int fd, PlayerData& player);

// Receives a message from fd into the k-th buffer, returns a line (without \r\n)
// if available, or empty string if not. If erase is set then server should erase 
// all the data concerning this player, because they disconnected.
std::string receive_msg(int fd, int k, bool& erase);

// Parses the message msg from the player represented by their descriptor fd.
// Sends back the necessary replies if necessary or sets the timer for specific
// type of timer that must be set by the server. Returns true on success and 
// false if there was an error.
bool handle_message(const std::string& msg, PlayerData& player, int fd, 
                    TimerAction& timer, const std::string& ip, int port,
                    int K, int& PUT_count, int N);


// Removes the buffer associated with the k-th player (used for cleaning up after a disconnect).
void erase_kth_player(int k);

// Removes all player buffers (used for cleaning up after a game ends).
void erase_players();

// Adds a new player and initializes their buffer; returns a default-initialized PlayerData.
PlayerData add_player();


#endif // SERVER_UTILS_H