#ifndef MIM_COMMON_H
#define MIM_COMMON_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

uint16_t read_port(char const *string);
struct sockaddr_in get_server_address(char const *host, uint16_t port);

// Following two functions come from Stevens' "UNIX Network Programming" book.
// Read n bytes from a descriptor. Use in place of read() when fd is a stream socket.
ssize_t	readn(int fd, void *vptr, size_t n);

// Write n bytes to a descriptor.
ssize_t	writen(int fd, const void *vptr, size_t n);

// Parses the s string to out int and returns true.
// If s is not an integer or is an integer that doesn't 
// satisfy minv <= s <= min then returns false.
bool parse_int(const char *s, int minv, int maxv, int &out);

#endif
