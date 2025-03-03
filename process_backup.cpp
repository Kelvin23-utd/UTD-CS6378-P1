#include "process.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <sstream>
#include <fcntl.h>
#include <stdexcept>
#include <errno.h>  // For errno access

// Define server ports for each process
const int BASE_PORT = 8000;
// Define hostnames for the four processes
// const std::string HOSTS[] = {"dc01", "dc02", "dc03", "dc04"};
const std::string HOSTS[] = {"localhost", "localhost", "localhost", "localhost"};

Process::Process(int process_id, bool delay, bool debug) : 
    id(process_id), 
    use_delay(delay),
    msg_counter(0),
    vector_clock(4, 0),
    msg_delivered(4, 0),
    messages_sent(0), 
    debug_mode(debug){
    
    // Initialize socket storage
    for (int i = 0; i < 4; i++) {
        if (i != id) {
            connections.push_back(-1);
        } else {
            connections.push_back(-1); // Placeholder for self
        }
    }
    
    std::cout << "Process " << id << " initialized. Delay mode: " << (use_delay ? "ON" : "OFF") << std::endl;
}

void Process::run() {
    try {
        // Step 1: Establish connections
        connect_to_others();
        
        std::cout << "Process " << id << ": All connections established" << std::endl;
        
        // Step 2: Main loop - broadcast and receive messages
        while (!is_finished()) {
            // Process incoming messages
            handle_incoming();
            
            // Broadcast a message if we haven't sent all 100 yet
            if (messages_sent < 100) {
                // Wait random time before sending
                int wait_time = random_int(1, 10);
                std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
                
                broadcast_message();
            }
        }
        
        // Print summary
        print_summary();
    }
    catch (const std::exception& e) {
        std::cerr << "Error in process " << id << ": " << e.what() << std::endl;
    }
    
    // Step 3: Clean up connections
    for (int sock : connections) {
        if (sock != -1) {
            close(sock);
        }
    }
    
    if (server_socket != -1) {
        close(server_socket);
    }
}

void Process::connect_to_others() {
    // Set up server socket to accept connections
    setup_server_socket();
    
    // Connect to processes with lower IDs
    for (int i = 0; i < id; i++) {
        connect_to_process(i);
    }
    
    // Accept connections from processes with higher IDs
    for (int i = id + 1; i < 4; i++) {
        accept_connection();
    }
}

void Process::setup_server_socket() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        throw std::runtime_error("Failed to create server socket: " + std::string(strerror(errno)));
    }
    
    // Allow socket reuse
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(server_sock);
        throw std::runtime_error("setsockopt failed: " + std::string(strerror(errno)));
    }
    
    // Bind to port
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(BASE_PORT + id);
    
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_sock);
        throw std::runtime_error("Bind failed: " + std::string(strerror(errno)));
    }
    
    // Listen for connections
    if (listen(server_sock, 5) < 0) {
        close(server_sock);
        throw std::runtime_error("Listen failed: " + std::string(strerror(errno)));
    }
    
    server_socket = server_sock;
}

void Process::connect_to_process(int target_id) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        throw std::runtime_error("Failed to create client socket: " + std::string(strerror(errno)));
    }
    
    // Resolve hostname
    struct hostent* host = gethostbyname(HOSTS[target_id].c_str());
    if (!host) {
        close(sock);
        throw std::runtime_error("Failed to resolve hostname: " + HOSTS[target_id]);
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(BASE_PORT + target_id);
    memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);
    
    // Try to connect with retries
    int max_retries = 5;
    for (int i = 0; i < max_retries; i++) {
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) >= 0) {
            // Send our ID to the server
            int msg = id;
            if (send(sock, &msg, sizeof(msg), 0) < 0) {
                close(sock);
                throw std::runtime_error("Failed to send ID: " + std::string(strerror(errno)));
            }
            
            // Connection successful
            connections[target_id] = sock;
            std::cout << "Connected to process " << target_id << std::endl;
            return;
        }
        
        std::cerr << "Connection attempt " << (i+1) << " to process " << target_id 
                 << " failed: " << strerror(errno) << ". Retrying..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    close(sock);
    throw std::runtime_error("Failed to connect to process " + std::to_string(target_id));
}

void Process::accept_connection() {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    int client_sock = accept(server_socket, (struct sockaddr*)&client_addr, &addr_len);
    if (client_sock < 0) {
        throw std::runtime_error("Accept failed: " + std::string(strerror(errno)));
    }
    
    // Receive client ID
    int client_id;
    if (recv(client_sock, &client_id, sizeof(client_id), 0) <= 0) {
        close(client_sock);
        throw std::runtime_error("Failed to receive client ID");
    }
    
    if (client_id < 0 || client_id >= 4 || client_id == id) {
        close(client_sock);
        throw std::runtime_error("Invalid client ID: " + std::to_string(client_id));
    }
    
    connections[client_id] = client_sock;
    std::cout << "Accepted connection from process " << client_id << std::endl;
}

void Process::broadcast_message() {
    // Create new message
    Message msg;
    msg.sender_id = id;
    msg.seq_number = msg_counter++;
    
    // Create vector clock for this message
    // Note: We set it up BEFORE incrementing our own clock
    msg.vector_clock.resize(4, 0);
    for (int i = 0; i < 4; i++) {
        // Copy the current vector clock values
        msg.vector_clock[i] = vector_clock[i];
    }
    
    // Update own vector clock AFTER creating the message copy
    vector_clock[id]++;
    
    // Also update the message's vector clock
    msg.vector_clock[id] = vector_clock[id];
    
    // Prepare message data
    std::stringstream ss;
    ss << "Message from P" << id << " #" << msg.seq_number;
    msg.data = ss.str();
    
    // Send to all other processes
    for (int i = 0; i < 4; i++) {
        if (i != id && connections[i] != -1) {
            try {
                send_message(connections[i], msg);
            } catch (const std::exception& e) {
                std::cerr << "Failed to send message to process " << i << ": " << e.what() << std::endl;
            }
        }
    }
    
    // Log sent message
    std::cout << "Sent message " << msg.seq_number << ", VC=[";
    for (size_t i = 0; i < msg.vector_clock.size(); i++) {
        std::cout << msg.vector_clock[i];
        if (i < msg.vector_clock.size() - 1) {
            std::cout << ",";
        }
    }
    std::cout << "]" << std::endl;
    
    messages_sent++;
}

void Process::handle_incoming() {
    fd_set readfds;
    struct timeval tv;
    int max_fd = -1;
    
    // Set up file descriptor set
    FD_ZERO(&readfds);
    for (int sock : connections) {
        if (sock != -1) {
            FD_SET(sock, &readfds);
            max_fd = std::max(max_fd, sock);
        }
    }
    
    if (max_fd == -1) {
        // No valid connections
        return;
    }
    
    // Set timeout to non-blocking
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    // Check for incoming data
    int activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);
    
    if (activity < 0) {
        std::cerr << "Select error: " << strerror(errno) << std::endl;
        return;
    }
    
    // Process incoming messages
    for (int i = 0; i < 4; i++) {
        if (i != id && connections[i] != -1 && FD_ISSET(connections[i], &readfds)) {
            Message msg;
            bool success = false;
            
            try {
                success = receive_message(connections[i], msg);
            } catch (const std::exception& e) {
                std::cerr << "Error receiving message from process " << i << ": " 
                         << e.what() << std::endl;
                continue;
            }
            
            if (success) {
                // Apply network delay if enabled
                if (use_delay) {
                    int delay = random_int(1, 5);
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay));
                }
                
                process_message(msg);
            } else {
                std::cerr << "Connection closed by process " << i << std::endl;
                close(connections[i]);
                connections[i] = -1;
            }
        }
    }
}

void Process::send_message(int sock, const Message& msg) {
    // Send sender ID
    if (send(sock, &msg.sender_id, sizeof(msg.sender_id), 0) < 0) {
        throw std::runtime_error("Error sending sender ID: " + std::string(strerror(errno)));
    }
    
    // Send sequence number
    if (send(sock, &msg.seq_number, sizeof(msg.seq_number), 0) < 0) {
        throw std::runtime_error("Error sending sequence number: " + std::string(strerror(errno)));
    }
    
    // Send vector clock size
    int vc_size = msg.vector_clock.size();
    if (send(sock, &vc_size, sizeof(vc_size), 0) < 0) {
        throw std::runtime_error("Error sending vector clock size: " + std::string(strerror(errno)));
    }
    
    // Send vector clock
    for (int vc : msg.vector_clock) {
        if (send(sock, &vc, sizeof(vc), 0) < 0) {
            throw std::runtime_error("Error sending vector clock element: " + std::string(strerror(errno)));
        }
    }
    
    // Send data size
    int data_size = msg.data.size();
    if (send(sock, &data_size, sizeof(data_size), 0) < 0) {
        throw std::runtime_error("Error sending data size: " + std::string(strerror(errno)));
    }
    
    // Send data
    if (send(sock, msg.data.c_str(), data_size, 0) < 0) {
        throw std::runtime_error("Error sending data: " + std::string(strerror(errno)));
    }
}

bool Process::receive_message(int sock, Message& msg) {
    // Receive sender ID
    int result = recv(sock, &msg.sender_id, sizeof(msg.sender_id), 0);
    if (result < 0) {
        throw std::runtime_error("Error receiving sender ID: " + std::string(strerror(errno)));
    } else if (result == 0) {
        return false; // Connection closed
    }
    
    // Receive sequence number
    result = recv(sock, &msg.seq_number, sizeof(msg.seq_number), 0);
    if (result <= 0) {
        return false;
    }
    
    // Receive vector clock size
    int vc_size;
    result = recv(sock, &vc_size, sizeof(vc_size), 0);
    if (result <= 0) {
        return false;
    }
    
    // Receive vector clock
    msg.vector_clock.resize(vc_size);
    for (int i = 0; i < vc_size; i++) {
        result = recv(sock, &msg.vector_clock[i], sizeof(int), 0);
        if (result <= 0) {
            return false;
        }
    }
    
    // Receive data size
    int data_size;
    result = recv(sock, &data_size, sizeof(data_size), 0);
    if (result <= 0) {
        return false;
    }
    
    // Receive data
    char* buffer = new char[data_size + 1];
    result = recv(sock, buffer, data_size, 0);
    if (result <= 0) {
        delete[] buffer;
        return false;
    }
    
    buffer[data_size] = '\0';
    msg.data = std::string(buffer);
    delete[] buffer;
    
    return true;
}

void Process::process_message(const Message& msg) {
    // Check if message can be delivered
    if (can_deliver(msg)) {
        deliver_message(msg);
        // Check buffer for messages that can now be delivered
        check_buffer();
    } else {
        // Buffer the message
        buffer.push(msg);
    }
}

bool Process::can_deliver(const Message& msg) {
    // Special handling for the first message (seq_number = 0)
    if (msg.seq_number == 0) {
        // For the first message, we only check that the sender's clock is 1
        // and that other clocks are 0
        for (int i = 0; i < 4; i++) {
            if (i == msg.sender_id) {
                if (msg.vector_clock[i] != 1) {
                    return false;
                }
            } else {
                if (msg.vector_clock[i] != 0) {
                    return false;
                }
            }
        }
        return true;
    }
    
    // Special handling for the second message (seq_number = 1)
    if (msg.seq_number == 1) {
        // For the second message, we're more lenient
        // Just check that the sender's clock is at least 1
        return msg.vector_clock[msg.sender_id] >= 1;
    }
    
    // Normal causal ordering check for messages seq_number >= 2
    for (int i = 0; i < 4; i++) {
        if (i == msg.sender_id) {
            // For sender: msg.vc[sender] should be exactly one more than local vc[sender]
            if (msg.vector_clock[i] != vector_clock[i] + 1) {
                return false;
            }
        } else {
            // For others: msg.vc[j] should be less than or equal to local vc[j]
            if (msg.vector_clock[i] > vector_clock[i]) {
                return false;
            }
        }
    }
    return true;
}


void Process::deliver_message(const Message& msg) {
    // Update vector clock - take component-wise maximum
    for (int i = 0; i < 4; i++) {
        vector_clock[i] = std::max(vector_clock[i], msg.vector_clock[i]);
    }
    
    // Log delivery
    std::cout << "Delivered message " << msg.seq_number << " from P" << msg.sender_id 
              << ", VC=[";
    for (size_t i = 0; i < msg.vector_clock.size(); i++) {
        std::cout << msg.vector_clock[i];
        if (i < msg.vector_clock.size() - 1) {
            std::cout << ",";
        }
    }
    std::cout << "]" << std::endl;
    
    // Update delivery statistics
    msg_delivered[msg.sender_id]++;
    
    if (debug_mode && msg_delivered[msg.sender_id] == 99) {
        std::cout << "DEBUG: Process " << id << " has received message 99 from P" 
                  << msg.sender_id << std::endl;
    }
}

void Process::check_buffer() {
    bool delivered = true;
    while (delivered && !buffer.empty()) {
        delivered = false;
        
        // Create a temporary buffer
        std::queue<Message> temp_buffer;
        
        // Process all messages in buffer
        while (!buffer.empty()) {
            Message msg = buffer.front();
            buffer.pop();
            
            if (can_deliver(msg)) {
                deliver_message(msg);
                delivered = true;
            } else {
                temp_buffer.push(msg);
            }
        }
        
        // Update buffer with remaining messages
        buffer = temp_buffer;
    }
}

bool Process::is_finished() {
    if (debug_mode) {
        std::cout << "Process " << id << " checking termination: sent=" << messages_sent;
        for (int i = 0; i < 4; i++) {
            if (i != id) {
                std::cout << ", delivered from P" << i << "=" << msg_delivered[i];
            }
        }
        std::cout << std::endl;
        
        // Print buffer contents if there are items in it
        if (!buffer.empty()) {
            std::cout << "DEBUG: Process " << id << " has " << buffer.size() 
                      << " messages in buffer" << std::endl;
            
            // Create a copy of the buffer to inspect
            std::queue<Message> temp_buffer = buffer;
            int count = 0;
            
            // Display up to 5 messages from the buffer
            while (!temp_buffer.empty() && count < 5) {
                Message msg = temp_buffer.front();
                temp_buffer.pop();
                
                std::cout << "  Buffer[" << count << "]: From P" << msg.sender_id 
                          << ", seq=" << msg.seq_number << ", VC=[";
                for (size_t i = 0; i < msg.vector_clock.size(); i++) {
                    std::cout << msg.vector_clock[i];
                    if (i < msg.vector_clock.size() - 1) {
                        std::cout << ",";
                    }
                }
                std::cout << "]" << std::endl;
                
                count++;
            }
            
            if (temp_buffer.size() > 0) {
                std::cout << "  ... and " << temp_buffer.size() << " more messages" << std::endl;
            }
        }
    }
    
    // Check if we've sent all messages
    if (messages_sent < 100) {
        return false;
    }
    
    // Check if we've received all messages from each process
    for (int i = 0; i < 4; i++) {
        if (i != id && msg_delivered[i] < 100) {
            return false;
        }
    }
    
    return true;
}

void Process::print_summary() {
    std::cout << "======= SUMMARY =======" << std::endl;
    std::cout << "Process " << id << " final state:" << std::endl;
    std::cout << "Messages sent: " << messages_sent << std::endl;
    for (int i = 0; i < 4; i++) {
        if (i != id) {
            std::cout << "Messages delivered from P" << i << ": " << msg_delivered[i] << std::endl;
        }
    }
    std::cout << "======================" << std::endl;
}

int Process::random_int(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}