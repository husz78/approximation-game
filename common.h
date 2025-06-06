#ifndef MIM_COMMON_H
#define MIM_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <string>
#include <vector>


// Write n bytes to a descriptor.
ssize_t	writen(int fd, const void *vptr, size_t n);

// Parses the s string to out int and returns true.
// If s is not an integer or is an integer that doesn't 
// satisfy minv <= s <= min then returns false.
bool parse_int(const char *s, int minv, int maxv, int &out);

// Convert sockaddr to human-readable IP string; prefer IPv4 if v4-mapped.
std::string sockaddr_to_ip(const struct sockaddr* sa);

// Checks if the given string represents a valid decimal number.
bool is_valid_decimal(const std::string& str);

// Validates player_id - checks if it contains only digits and English letters.
bool is_valid_player_id(const std::string& player_id);

// Returns the double rounded up to 7 decimal places.
double round7(double x);

// Calculate the polynomial with coeffs coefficients in point x.
double get_sum_in_x(int x, const std::vector<double>& coeffs);

#endif
