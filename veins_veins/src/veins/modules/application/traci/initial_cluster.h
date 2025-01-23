#include <iostream>
#include <map>
#include <iomanip>

std::map<int, std::map<int, double>> transformMap(const std::map<int, double>& node_speed) {
    std::map<int, std::map<int, double>> result;

    for (const auto& [key, value] : node_speed) {
        int compartment;
        if (value >= 0 && value <= 35) compartment = 0;
        else if (value >= 35 && value <= 49) compartment = 1;
        else if (value > 55 && value <= 69) compartment = 2;
        else if (value > 75 && value <= 89) compartment = 3;
        else if (value > 95 && value <= 109) compartment = 4;
        else compartment = 5; // For values outside the defined ranges

        result[key] = {{compartment, value}};
    }

    return result;
}

void printMap(const std::map<int, std::map<int, double>>& map) {
    std::cout << "{\n";
    auto it = map.begin();
    auto end = map.end();
    while (it != end) {
        const auto& [key, inner_map] = *it;
        std::cout << "  " << key << ": {";
        for (const auto& [compartment, value] : inner_map) {
            std::cout << compartment << ": " << std::fixed << std::setprecision(1) << value;
        }
        std::cout << "}";
        if (++it != end) {
            std::cout << ",";
        }
        std::cout << "\n";
    }
    std::cout << "}\n";
}

/*int main() {
    std::map<int, double> node_speed = {
        {1, 10.2},
        {2, 40.0},
        {3, 55.5},
        {4, 75.0},
        {5, 90.0},
        {6, 120.0}
    };

    std::cout << "Original map:\n";
    for (const auto& [key, value] : node_speed) {
        std::cout << key << ": " << std::fixed << std::setprecision(1) << value << "\n";
    }

    auto transformed = transformMap(node_speed);

    std::cout << "\nTransformed map:\n";
    printMap(transformed);

    return 0;
}*/
