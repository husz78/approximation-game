#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <string>

// Validates player_id - checks if it contains only digits and English letters.
bool is_valid_player_id(const std::string& player_id);

// Sends HELLO message to the tcp connection represented by descriptor fd.
// Returns 0 on success and -1 on error.
int send_HELLO(const std::string& player_id, int fd);

// Sends PUT message to the tcp connection represented by descriptor fd.
// Prints the error and exits on error.
void send_PUT(int point, double value, int fd);

// Gets the point and value from STDIN. Prints error when wrong line format.
bool get_input_from_stdin(int& point, double& value);

// Reads a message from the socket represented by fd descriptor.
// Returns either the whole message or empty string if didn't read
// whole message. It may be called multiple times until it returns
// non-empty string because it buffers the message.
std::string receive_msg(int fd);

#endif