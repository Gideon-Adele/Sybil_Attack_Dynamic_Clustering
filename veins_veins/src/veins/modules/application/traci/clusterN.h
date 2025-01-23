#include <iostream>
#include <map>
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>
#include <numeric>

struct Centroid {
    double value;
    int cluster;
};

double calculateDistance(double a, double b) {
    return std::abs(a - b);
}

std::map<int, std::map<int, double>> kMeansClustering(const std::map<int, std::map<int, double>>& node_speed, int max_iterations = 100) {
    std::vector<Centroid> centroids = {
        {20.0, 1}, {47.5, 2}, {65.0, 3}, {80.0, 4}, {97.0, 5}
    };

    std::vector<std::pair<int, std::pair<int, double>>> data_points;
    for (const auto& outer_pair : node_speed) {
        for (const auto& inner_pair : outer_pair.second) {
            data_points.push_back({outer_pair.first, inner_pair});
        }
    }

    for (int iteration = 0; iteration < max_iterations; ++iteration) {
        // Assign points to clusters
        std::vector<std::vector<double>> cluster_points(centroids.size());
        for (const auto& point : data_points) {
            double min_distance = std::numeric_limits<double>::max();
            int closest_centroid = 0;
            for (size_t i = 0; i < centroids.size(); ++i) {
                double distance = calculateDistance(point.second.second, centroids[i].value);
                if (distance < min_distance) {
                    min_distance = distance;
                    closest_centroid = i;
                }
            }
            cluster_points[closest_centroid].push_back(point.second.second);
        }

        // Update centroids
        bool centroids_changed = false;
        for (size_t i = 0; i < centroids.size(); ++i) {
            if (!cluster_points[i].empty()) {
                double new_centroid = std::accumulate(cluster_points[i].begin(), cluster_points[i].end(), 0.0) / cluster_points[i].size();
                if (std::abs(new_centroid - centroids[i].value) > 1e-6) {
                    centroids[i].value = new_centroid;
                    centroids_changed = true;
                }
            }
        }

        if (!centroids_changed) {
            break;
        }
    }

    // Assign final clusters
    std::map<int, std::map<int, double>> clusters;
    for (const auto& point : data_points) {
        double min_distance = std::numeric_limits<double>::max();
        int closest_centroid = 0;
        for (size_t i = 0; i < centroids.size(); ++i) {
            double distance = calculateDistance(point.second.second, centroids[i].value);
            if (distance < min_distance) {
                min_distance = distance;
                closest_centroid = i;
            }
        }
        int cluster = centroids[closest_centroid].cluster;
        clusters[cluster * 1000000 + point.first][point.second.first] = point.second.second;
    }

    return clusters;
}

// Test function
void printClusters(const std::map<int, std::map<int, double>>& clusters) {
    for (const auto& cluster : clusters) {
        std::cout << "Cluster " << cluster.first / 1000000 << ":" << std::endl;
        for (const auto& node : cluster.second) {
            std::cout << "  Node " << cluster.first % 1000000 << ", " << node.first << ": " << node.second << std::endl;
        }
        std::cout << std::endl;
    }
}

/*
int main() {
    std::map<int, std::map<int, double>> node_speed = {
        {1, {{1, 30.5}, {2, 45.0}, {3, 65.2}}},
        {2, {{1, 80.0}, {2, 95.5}, {3, 35.8}}},
        {3, {{1, 50.2}, {2, 72.1}, {3, 100.0}}}
    };

    // Call kMeansClustering with only the required argument
    std::map<int, std::map<int, double>> clustered_result = kMeansClustering(node_speed);
    printClusters(clustered_result);

    return 0;
}
*/


