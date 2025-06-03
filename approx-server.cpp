#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <fstream>

#include "err.h" 
#include "common.h"
#include "server-utils.h"


// Prints the usage of the program.
void usage(const char* prog) {
    std::cerr << "Usage: " << prog
              << " [-p port] [-k K] [-n N] [-m M] -f coeff_file\n"
              << "  -p port    server port (0–65535), default 0\n"
              << "  -k K       max point K (1–10000), default 100\n"
              << "  -n N       poly degree N (1–8), default 4\n"
              << "  -m M       max PUTs M (1–12341234), default 131\n"
              << "  -f file    coeffs file (required)\n";
}


// Parses the arguments and checks if they're valid. If they're then
// corresponding variables are set. If they're not then print an error
// and exit with code 1.
void parse_args(int& port, int& K, int& N, int& M, std::string& coeff_file,
                int argc, char** argv) {
    int opt;
    while ((opt = getopt(argc, argv, "p:k:n:m:f:")) != -1) {
        switch (opt) {
        case 'p':
            if (!parse_int(optarg, 0, 65535, port)) {
                fatal("invalid port: %s", optarg);
            }
            break;
        case 'k':
            if (!parse_int(optarg, 1, 10000, K)) {
                fatal("invalid K: %s", optarg);
            }
            break;
        case 'n':
            if (!parse_int(optarg, 1, 8, N)) {
                fatal("invalid N: %s", optarg);
            }
            break;
        case 'm':
            if (!parse_int(optarg, 1, 12341234, M)) {
                fatal("invalid M: %s", optarg);
            }
            break;
        case 'f':
            coeff_file = optarg;
            break;
        default:
            usage(argv[0]);
            fatal("invalid argument");
        }
    }
    if (coeff_file.empty()) {
        usage(argv[0]);
        fatal("missing -f parameter");
    }
    
    open_coeff_file(coeff_file);
}

int main(int argc, char* argv[]) {
    int port = 0;
    int K    = 100;
    int N    = 4;
    int M    = 131;
    std::string coeff_file;


    parse_args(port, K, N, M, coeff_file, argc, argv);

    std::cout << "Starting server on port " << port
              << " with K=" << K
              << " N=" << N
              << " M=" << M
              << " file='" << coeff_file << "'\n";

    int listen_fd = create_dual_stack(port);
    

    close(listen_fd);
    return 0;
}