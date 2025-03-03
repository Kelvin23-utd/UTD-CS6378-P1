#pragma once
#include <vector>
#include <queue>
#include <string>

// Message structure with vector clock for causal ordering
struct Message {
    int sender_id;
    int seq_number;
    std::vector<int> vector_clock;
    std::string data;
};

class Process {
private:
    int id;                           // Process ID (0-3)
    std::vector<int> connections;     // Socket connections to other processes
    int server_socket = -1;           // Server socket for accepting connections
    std::vector<int> vector_clock;    // Vector clock for causal ordering
    std::queue<Message> buffer;       // Message buffer for out-of-order messages
    int msg_counter;                  // Counter for local messages
    std::vector<int> msg_delivered;   // Count of messages delivered from each process
    int messages_sent;                // Count of messages sent
    bool use_delay;                   // Flag for simulating network delay
    bool debug_mode;  // Add this new member for debug modes
    
    // Connection setup
    void setup_server_socket();
    void connect_to_process(int target_id);
    void accept_connection();
    
    // Message handling
    void send_message(int sock, const Message& msg);
    bool receive_message(int sock, Message& msg);
    void process_message(const Message& msg);
    bool can_deliver(const Message& msg);
    void deliver_message(const Message& msg);
    void check_buffer();
    
    // Utilities
    int random_int(int min, int max);
    void print_summary();

public:
    Process(int process_id, bool delay);
    void run();
    void connect_to_others();
    void broadcast_message();
    void handle_incoming();
    bool is_finished();
    Process(int process_id, bool delay, bool debug = false);
};