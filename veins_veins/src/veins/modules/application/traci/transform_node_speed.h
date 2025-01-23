#include <iostream>
#include <map>
#include <vector>
#include <algorithm>

std::map<int, std::map<int, double>> transformNodeSpeedMap(
    const std::map<int, std::map<int, double>>& node_speed_map) {

    std::map<int, std::map<int, double>> new_node_speed_map;

    // If the map is empty, return empty map
    if (node_speed_map.empty()) {
        return new_node_speed_map;
    }

    // Find the highest first key in the map
    int max_first_key = node_speed_map.rbegin()->first;

    // Create a mapping of old keys to new keys
    std::map<int, int> key_mapping;

    // Assign new keys: highest becomes 1, others get incremented
    for (const auto& outer_pair : node_speed_map) {
        int old_key = outer_pair.first;
        if (old_key == max_first_key) {
            key_mapping[old_key] = 1;
        } else {
            key_mapping[old_key] = old_key + 1;
        }
    }

    // Create the new map with transformed keys
    for (const auto& outer_pair : node_speed_map) {
        int old_key = outer_pair.first;
        int new_key = key_mapping[old_key];

        // Copy inner map with the same key-value pairs
        for (const auto& inner_pair : outer_pair.second) {
            new_node_speed_map[new_key][inner_pair.first] = inner_pair.second;
        }
    }

    return new_node_speed_map;
}

// Helper function to print the map (for testing)
void printMap(const std::map<int, std::map<int, double>>& map) {
    for (const auto& outer_pair : map) {
        for (const auto& inner_pair : outer_pair.second) {
            std::cout << "map[" << outer_pair.first << "]["
                     << inner_pair.first << "] = "
                     << inner_pair.second << std::endl;
        }
    }
}

/*
int main() {
    // Create test map
    std::map<int, std::map<int, double>> node_speed_map;
    node_speed_map[1][23] = 40;
    node_speed_map[2][24] = 41;
    node_speed_map[3][20] = 51;
    node_speed_map[5][30] = 39;

    std::cout << "Original map:\n";
    printMap(node_speed_map);

    auto new_node_speed_map = transformNodeSpeedMap(node_speed_map);

    std::cout << "\nTransformed map:\n";
    printMap(new_node_speed_map);

    return 0;
}*/
