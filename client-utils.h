#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <string>

// Validates player_id - checks if it contains only digits and English letters.
bool is_valid_player_id(const std::string& player_id);

#endif