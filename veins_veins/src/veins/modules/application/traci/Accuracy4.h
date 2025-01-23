#include <iostream>
#include <map>
#include <cmath>
#include <iomanip>

// Function to calculate performance metrics
void calculatePerformanceMetrics(
    const std::map<int, std::map<int, double>>& clusters,
    const std::map<int, std::map<int, double>>& node_speed2,
    double threshold
) {
    int truePositives = 0;
    int falsePositives = 0;
    int falseNegatives = 0;
    int trueNegatives = 0;

    for (const auto& [outerKey, innerMap1] : node_speed2) {
        for (const auto& [innerKey, groundTruthValue] : innerMap1) {
            bool groundTruthPositive = std::abs(groundTruthValue) > threshold;
            bool predictedPositive = false;

            // Check if the key exists in clusters
            auto outerIt = clusters.find(outerKey);
            if (outerIt != clusters.end()) {
                auto innerIt = outerIt->second.find(innerKey);
                if (innerIt != outerIt->second.end()) {
                    predictedPositive = std::abs(innerIt->second) > threshold;
                }
            }

            if (groundTruthPositive && predictedPositive) {
                truePositives++;
            } else if (!groundTruthPositive && predictedPositive) {
                falsePositives++;
            } else if (groundTruthPositive && !predictedPositive) {
                falseNegatives++;
            } else {
                trueNegatives++;
            }
        }
    }

    // Calculate metrics
    double accuracy = static_cast<double>(truePositives + trueNegatives) /
                      (truePositives + trueNegatives + falsePositives + falseNegatives) * 100.0;

    double precision = truePositives > 0 ?
                       static_cast<double>(truePositives) / (truePositives + falsePositives) * 100.0 : 0.0;

    double recall = truePositives > 0 ?
                    static_cast<double>(truePositives) / (truePositives + falseNegatives) * 100.0 : 0.0;

    // Output results
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Accuracy: " << accuracy << "%" << std::endl;
    std::cout << "Precision: " << precision << "%" << std::endl;
    std::cout << "Recall: " << recall << "%" << std::endl;
}

