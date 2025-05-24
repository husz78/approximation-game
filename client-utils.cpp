#include "client-utils.h"

bool is_valid_player_id(const std::string& player_id) {
    if (player_id.empty()) return false;
    
    for (char c : player_id) {
        if (!((c >= '0' && c <= '9') || 
              (c >= 'a' && c <= 'z') ||  
              (c >= 'A' && c <= 'Z'))) { 
            return false;
        }
    }
    return true;
}