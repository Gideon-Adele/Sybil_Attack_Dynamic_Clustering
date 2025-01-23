//
// Copyright (C) 2016 David Eckhoff <david.eckhoff@fau.de>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#include "veins/modules/application/traci/MyVeinsApp.h"

using namespace veins;


#include <typeinfo>
#include <random>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <map>
#include "kCluster.h"
#include "node_speed.h"
#include "transform_node_speed.h"
#include "k_dynamic_cluster.h"

#include "Accuracy4.h"

using namespace std;


Define_Module(veins::MyVeinsApp);

std::map <int, double> node_speed;
//auto node_speed2 = transformMap(node_speed);

void MyVeinsApp::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        // initialize pointers to other modules
        EV<<"initializing"<< par("appName").stringValue()<<std::endl;

    }
    else if (stage == 1) {
        // Initializing members that require initialized other modules goes here
    }

}

void MyVeinsApp::finish()
{
    DemoBaseApplLayer::finish();
    // statistics recording goes here
    // Print results
    /*std::vector<double> initial_centroids = {10.0, 40.5, 65.0, 80.0, 97.0};

        std::map<int, std::map<int, double>> cluster_node = kMeansClustering(node_speed_map, initial_centroids);
        print_map(cluster_node);

        double threshold = 1.0;
        calculatePerformanceMetrics(cluster_node, node_speed_map, threshold);
        //End of Scenario 1*/

          std::vector<double> dynamic_cluster_head = DynamicCluster::selectClusterHeads(speedValues, 5);

            std::vector<double> static_cluster_head = {5.0, 20.0, 40.0, 80.0, 100.0};

            std::map<int, std::map<int, double>> s_cluster_node = kMeansClustering(new_node_speed_map, static_cluster_head);
            std::cout<<"K Clusters \n";
            print_map(s_cluster_node);

            std::cout<<"Performance Ev. for K Clusters \n";


            double threshold = 1.0;

            calculatePerformanceMetrics(s_cluster_node, new_node_speed_map, threshold);


            std::map<int, std::map<int, double>> d_cluster_node = kMeansClustering(new_node_speed_map, dynamic_cluster_head);
            std::cout<<"\n Dynamic Clusters \n";
            print_map(d_cluster_node);

            std::cout<<"\nPerformance Ev. for Dynamic Clusters (Threshold = 5) \n";
            calculatePerformanceMetrics(d_cluster_node, new_node_speed_map, threshold);

            //Threshold = 10
             std::vector<double> dynamic_cluster_head2 = DynamicCluster::selectClusterHeads(speedValues, 10);
             std::map<int, std::map<int, double>> d_cluster_node2 = kMeansClustering(new_node_speed_map, dynamic_cluster_head2);
             std::cout<<"\nPerformance Ev. for Dynamic Clusters (Threshold = 10) \n";
             calculatePerformanceMetrics(d_cluster_node2, new_node_speed_map, threshold);

            //Threshold = 20
            std::vector<double> dynamic_cluster_head3 = DynamicCluster::selectClusterHeads(speedValues, 20);
            std::map<int, std::map<int, double>> d_cluster_node3 = kMeansClustering(node_speed_map, dynamic_cluster_head3);
            std::cout<<"\nPerformance Ev. for Dynamic Clusters (Threshold = 20) \n";
            calculatePerformanceMetrics(d_cluster_node3, new_node_speed_map, threshold);




}

void MyVeinsApp::onBSM(DemoSafetyMessage* bsm)
{
    // Your application has received a beacon message from another car or RSU
    // code for handling the message goes here
    //node_speed.insert(std::make_pair(vehicleIDY, vehicleIDY));
    node_speed_map =process_node_speed(node_speed3);
    new_node_speed_map = transformNodeSpeedMap(node_speed_map);

   // auto node_speed2 = transformMap(node_speed);

}

void MyVeinsApp::onWSM(BaseFrame1609_4* wsm)
{
    // Your application has received a data message from another car or RSU
    // code for handling the message goes here, see TraciDemo11p.cc for examples
}

void MyVeinsApp::onWSA(DemoServiceAdvertisment* wsa)
{
    // Your application has received a service advertisement from another car or RSU
    // code for handling the message goes here, see TraciDemo11p.cc for examples
}

void MyVeinsApp::handleSelfMsg(cMessage* msg)
{
    DemoBaseApplLayer::handleSelfMsg(msg);
    // this method is for self messages (mostly timers)
    // it is important to call the DemoBaseApplLayer function for BSM and WSM transmission

}

void MyVeinsApp::handlePositionUpdate(cObject* obj)
{
    DemoBaseApplLayer::handlePositionUpdate(obj);
    // the vehicle has moved. Code that reacts to new positions goes here.
    // member variables such as currentPosition and currentSpeed are updated in the parent class
}
