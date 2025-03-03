#include "process.h"
#include <iostream>
#include <string>
#include <stdexcept>

int main(int argc, char* argv[]) {
    try {
        // Validate command line arguments
        if (argc < 2 || argc > 4) {
            std::cerr << "Usage: " << argv[0] << " <process_id> [delay] [debug]" << std::endl;
            return 1;
        }
        
        // Parse process ID
        int process_id = std::stoi(argv[1]);
        if (process_id < 0 || process_id > 3) {
            std::cerr << "Error: Process ID must be between 0 and 3" << std::endl;
            return 1;
        }
        
        // Check for delay flag
        bool use_delay = false;
        if (argc >= 3 && std::string(argv[2]) == "delay") {
            use_delay = true;
        }
        
        // Check for debug flag
        bool debug_mode = false;
        if ((argc >= 3 && std::string(argv[2]) == "debug") || 
            (argc >= 4 && std::string(argv[3]) == "debug")) {
            debug_mode = true;
        }
        
        // Create and run the process
        Process process(process_id, use_delay, debug_mode);
        process.run();
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}