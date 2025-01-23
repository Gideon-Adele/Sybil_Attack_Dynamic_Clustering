#include <iostream>
#include <vector>
#include <map>
#include <cmath>
#include <limits>
#include <algorithm>

std::map<int, std::map<int, double>> kMeansClustering(const std::map<int, std::map<int, double>>& node_speed,
                                                      std::vector<double> centroids,
                                                      int max_iterations = 100) {
    std::map<int, std::map<int, double>> cluster_node;
    std::vector<std::pair<int, double>> all_speeds;

    // Collect all speeds with their corresponding subnode
    for (const auto& outer : node_speed) {
        for (const auto& inner : outer.second) {
            all_speeds.emplace_back(inner.first, inner.second);
        }
    }

    int num_clusters = centroids.size();
    std::vector<int> assignments(all_speeds.size());

    for (int iteration = 0; iteration < max_iterations; ++iteration) {
        bool changed = false;

        // Assign each point to the nearest centroid
        for (size_t i = 0; i < all_speeds.size(); ++i) {
            double min_distance = std::numeric_limits<double>::max();
            int closest_centroid = 0;

            for (int j = 0; j < num_clusters; ++j) {
                double distance = std::abs(all_speeds[i].second - centroids[j]);
                if (distance < min_distance) {
                    min_distance = distance;
                    closest_centroid = j;
                }
            }

            if (assignments[i] != closest_centroid) {
                assignments[i] = closest_centroid;
                changed = true;
            }
        }

        if (!changed) {
            break;
        }

        // Recalculate centroids
        std::vector<double> new_centroids(num_clusters, 0.0);
        std::vector<int> counts(num_clusters, 0);

        for (size_t i = 0; i < all_speeds.size(); ++i) {
            new_centroids[assignments[i]] += all_speeds[i].second;
            counts[assignments[i]]++;
        }

        for (int j = 0; j < num_clusters; ++j) {
            if (counts[j] > 0) {
                centroids[j] = new_centroids[j] / counts[j];
            }
        }
    }

    // Create the output map
    for (size_t i = 0; i < all_speeds.size(); ++i) {
        int cluster = assignments[i] + 1; // Adding 1 to make clusters 1-5 instead of 0-4
        cluster_node[cluster][all_speeds[i].first] = all_speeds[i].second;
    }

    return cluster_node;
}

/*
int main() {
    std::map<int, std::map<int, double>> node_speed = {
        {3, {{10, 41.3}, {11, 15.2}}},
        {4, {{20, 63.7}, {21, 92.1}}},
        {5, {{30, 78.4}, {31, 37.8}}}
    };

    std::vector<double> initial_centroids = {10.0, 40.5, 65.0, 80.0, 97.0};

    std::map<int, std::map<int, double>> cluster_node = kMeansClustering(node_speed, initial_centroids);

    // Print results
    for (const auto& cluster : cluster_node) {
        for (const auto& node : cluster.second) {
            std::cout << "Cluster " << cluster.first << ", Node " << node.first
                      << ": " << node.second << std::endl;
        }
    }

    return 0;
}*/
