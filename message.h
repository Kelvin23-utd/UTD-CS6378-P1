#pragma once
#include <vector>
#include <string>

struct Message {
    int sender_id;
    int seq_number;
    std::vector<int> vector_clock;
    std::string data;
};