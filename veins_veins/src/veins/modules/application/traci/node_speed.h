#include <iostream>
#include <map>
#include <set>

std::map<int, std::map<int, double>> process_node_speed(const std::map<int, std::map<int, double>>& node_speed) {
    std::map<int, std::map<int, double>> result;
    std::set<int> used_inner_keys;

    for (const auto& outer_pair : node_speed) {
        for (const auto& inner_pair : outer_pair.second) {
            int inner_key = inner_pair.first;
            double value = inner_pair.second;

            // Check if the inner key is positive, less than 1000, and not used before
            if (inner_key > 0 && inner_key < 2000 && used_inner_keys.find(inner_key) == used_inner_keys.end()) {
                result[outer_pair.first][inner_key] = value;
                used_inner_keys.insert(inner_key);
            }
        }
    }

    return result;
}

void print_map(const std::map<int, std::map<int, double>>& map) {
    for (const auto& outer_pair : map) {
        for (const auto& inner_pair : outer_pair.second) {
            std::cout << "node_speed[" << outer_pair.first << "][" << inner_pair.first << "] = "
                      << inner_pair.second << std::endl;
        }
    }
}

/*
int main() {
    std::map<int, std::map<int, double>> node_speed;
    node_speed[1][10] = 5.0;
    node_speed[2][10] = 6.0;
    node_speed[3][5] = 3.0;
    node_speed[4][15] = 4.0;
    node_speed[5][15] = 4.0;

    std::cout << "Original map:" << std::endl;
    print_map(node_speed);

    std::map<int, std::map<int, double>> processed_map = process_node_speed(node_speed);

    std::cout << "\nProcessed map:" << std::endl;
    print_map(processed_map);

    return 0;
}*/
