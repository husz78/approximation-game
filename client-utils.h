#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <string>
#include <vector>


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

// Parses the message and then based on the type handles the message 
// (displays necessary diagnostic output, sends a response etc.).
// Returns true if success and false if wrong message.
bool handle_message(const std::string& msg, std::vector<double>& coeffs,
                    bool auto_mode, std::vector<double>& state_vector, int fd,
                    std::vector<std::pair<int, double>>& pending_puts, bool& exit);

#endif