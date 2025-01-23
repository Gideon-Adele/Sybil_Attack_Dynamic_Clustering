#include <vector>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <set>
#include <iostream>

class DynamicCluster {
public:
    static std::vector<double> selectClusterHeads(const std::vector<double>& speedValues, double T) {
        if (T < 0) {
            throw std::invalid_argument("Threshold must be non-negative");
        }
        if (speedValues.empty()) {
            return std::vector<double>();
        }

        // Sort values and remove duplicates using set
        std::set<double> uniqueSpeeds(speedValues.begin(), speedValues.end());
        std::vector<double> sorted(uniqueSpeeds.begin(), uniqueSpeeds.end());

        std::vector<double> clusterHeads;
        double currentSum = sorted[0];
        int currentCount = 1;
        double clusterStart = sorted[0];

        for (size_t i = 1; i < sorted.size(); i++) {
            if (sorted[i] - clusterStart < T) {
                currentSum += sorted[i];
                currentCount++;
            } else {
                double avg = std::round(currentSum / currentCount);
                if (clusterHeads.empty() || clusterHeads.back() != avg) {
                    clusterHeads.push_back(avg);
                }
                clusterStart = sorted[i];
                currentSum = sorted[i];
                currentCount = 1;
            }
        }

        // Add the last cluster if it's different from the previous one
        double lastAvg = std::round(currentSum / currentCount);
        if (clusterHeads.empty() || clusterHeads.back() != lastAvg) {
            clusterHeads.push_back(lastAvg);
        }

        // Remove any duplicate cluster heads
        clusterHeads.erase(std::unique(clusterHeads.begin(), clusterHeads.end()), clusterHeads.end());

        return clusterHeads;
    }
};
