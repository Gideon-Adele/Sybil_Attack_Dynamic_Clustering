// Copyright (C) 2011 David Eckhoff <eckhoff@cs.fau.de>
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

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"

#include <cmath>
#include <vector>
#include <random>
#include <iostream>
#include <limits>
#include <ctime>

std::mt19937 gen(std::time(0));
std::uniform_int_distribution<>dis(300, 320);

//#include "kCluster.h"

using namespace veins;

using namespace std;



void DemoBaseApplLayer::initialize(int stage)
{
    BaseApplLayer::initialize(stage);

    if (stage == 0) {

        // initialize pointers to other modules
        if (FindModule<TraCIMobility*>::findSubModule(getParentModule())) {
            mobility = TraCIMobilityAccess().get(getParentModule());
            traci = mobility->getCommandInterface();
            traciVehicle = mobility->getVehicleCommandInterface();
        }
        else {
            traci = nullptr;
            mobility = nullptr;
            traciVehicle = nullptr;
        }

        annotations = AnnotationManagerAccess().getIfExists();
        ASSERT(annotations);

        mac = FindModule<DemoBaseApplLayerToMac1609_4Interface*>::findSubModule(getParentModule());
        ASSERT(mac);

        // read parameters
        headerLength = par("headerLength");
        sendBeacons = par("sendBeacons").boolValue();
        beaconLengthBits = par("beaconLengthBits");
        beaconUserPriority = par("beaconUserPriority");
        beaconInterval = par("beaconInterval");

        dataLengthBits = par("dataLengthBits");
        dataOnSch = par("dataOnSch").boolValue();
        dataUserPriority = par("dataUserPriority");

        wsaInterval = par("wsaInterval").doubleValue();
        currentOfferedServiceId = -1;

        isParked = false;

        findHost()->subscribe(BaseMobility::mobilityStateChangedSignal, this);
        findHost()->subscribe(TraCIMobility::parkingStateChangedSignal, this);

        sendBeaconEvt = new cMessage("beacon evt", SEND_BEACON_EVT);
        sendWSAEvt = new cMessage("wsa evt", SEND_WSA_EVT);

        generatedBSMs = 0;
        generatedWSAs = 0;
        generatedWSMs = 0;
        receivedBSMs = 0;
        receivedWSAs = 0;
        receivedWSMs = 0;
    }
    else if (stage == 1) {

        // store MAC address for quick access
        myId = mac->getMACAddress();

        // simulate asynchronous channel access

        if (dataOnSch == true && !mac->isChannelSwitchingActive()) {
            dataOnSch = false;
            EV_ERROR << "App wants to send data on SCH but MAC doesn't use any SCH. Sending all data on CCH" << std::endl;
        }
        simtime_t firstBeacon = simTime();

        if (par("avoidBeaconSynchronization").boolValue() == true) {

            simtime_t randomOffset = dblrand() * beaconInterval;
            firstBeacon = simTime() + randomOffset;

            if (mac->isChannelSwitchingActive() == true) {
                if (beaconInterval.raw() % (mac->getSwitchingInterval().raw() * 2)) {
                    EV_ERROR << "The beacon interval (" << beaconInterval << ") is smaller than or not a multiple of  one synchronization interval (" << 2 * mac->getSwitchingInterval() << "). This means that beacons are generated during SCH intervals" << std::endl;
                }
                firstBeacon = computeAsynchronousSendingTime(beaconInterval, ChannelType::control);
            }

            if (sendBeacons) {
                scheduleAt(firstBeacon, sendBeaconEvt);
            }
        }
    }
}

simtime_t DemoBaseApplLayer::computeAsynchronousSendingTime(simtime_t interval, ChannelType chan)
{


     /*
      *avoid that periodic messages for one channel type are scheduled in the other channel interval
      * when alternate access is enabled in the MAC
      */


    simtime_t randomOffset = dblrand() * interval;
    simtime_t firstEvent;
    simtime_t switchingInterval = mac->getSwitchingInterval(); // usually 0.050s
    simtime_t nextCCH;


     /*
     * start event earliest in next CCH (or SCH) interval. For alignment, first find the next CCH interval
     * To find out next CCH, go back to start of current interval and add two or one intervals
     * depending on type of current interval
     * */


    if (mac->isCurrentChannelCCH()) {
        nextCCH = simTime() - SimTime().setRaw(simTime().raw() % switchingInterval.raw()) + switchingInterval * 2;
    }
    else {
        nextCCH = simTime() - SimTime().setRaw(simTime().raw() % switchingInterval.raw()) + switchingInterval;
    }

    firstEvent = nextCCH + randomOffset;

    // check if firstEvent lies within the correct interval and, if not, move to previous interval

    if (firstEvent.raw() % (2 * switchingInterval.raw()) > switchingInterval.raw()) {
        // firstEvent is within a sch interval
        if (chan == ChannelType::control) firstEvent -= switchingInterval;
    }
    else {
        // firstEvent is within a cch interval, so adjust for SCH messages
        if (chan == ChannelType::service) firstEvent += switchingInterval;
    }

    return firstEvent;
}

void DemoBaseApplLayer::populateWSM(BaseFrame1609_4* wsm, LAddress::L2Type rcvId, int serial)
{
    wsm->setRecipientAddress(rcvId);
    wsm->setBitLength(headerLength);

    if (DemoSafetyMessage* bsm = dynamic_cast<DemoSafetyMessage*>(wsm)) {
        std::string vehicleId = mobility->getExternalId(); //Added line by me
        veins::TraCICommandInterface::Vehicle veh= traci->vehicle(vehicleId);

        std::mt19937 gen(std::time(0));
        std::uniform_int_distribution<>dis(300, 320);

        int vehicleIdtoInt  = std::stoi(vehicleId);
        vehicleIDY.push_back(vehicleIdtoInt);

        random_device rd;
        mt19937 g(rd());

        //shuffle(vehicleIDY.begin(), vehicleIDY.end(), g);
        size_t vehicleIDY_totalsize = vehicleIDY.size();

        size_t first =std::ceil(vehicleIDY_totalsize * 0.10);
        size_t second =std::ceil(vehicleIDY_totalsize * 0.10);
        size_t third =std::ceil(vehicleIDY_totalsize * 0.10);
        size_t fourth =std::ceil(vehicleIDY_totalsize * 0.10);
        size_t five =std::ceil(vehicleIDY_totalsize * 0.10);
        size_t six =std::ceil(vehicleIDY_totalsize * 0.10);
        size_t seven =std::ceil(vehicleIDY_totalsize * 0.15);
        size_t eight =std::ceil(vehicleIDY_totalsize * 0.15);
        size_t nine =std::ceil(vehicleIDY_totalsize * 0.10);


        struct clusterWithId{
            std::vector<int>data;
            int id;
        };

        clusterWithId cluster1;
        cluster1.data ={vehicleIDY.begin(), vehicleIDY.begin()+ first};
        cluster1.id =1;

        clusterWithId cluster2;
        cluster2.data ={vehicleIDY.begin()+ first, vehicleIDY.begin()+ first+second};
        cluster2.id =2;

        clusterWithId cluster3;
        cluster3.data ={vehicleIDY.begin()+ first+second, vehicleIDY.begin()+ first+second+third};
        cluster3.id =3;

        clusterWithId cluster4;
        cluster4.data ={vehicleIDY.begin()+ first+second+third, vehicleIDY.begin()+ first+second+third+fourth};
        cluster4.id =4;

        clusterWithId cluster5;
        cluster5.data ={vehicleIDY.begin()+ first+second+third+fourth, vehicleIDY.begin()+ first+second+third+fourth+five};
        cluster5.id =5;

        clusterWithId cluster6;
        cluster6.data ={vehicleIDY.begin()+ first+second+third+fourth+five, vehicleIDY.begin()+ first+second+third+fourth+five+six};
        cluster6.id =6;

        clusterWithId cluster7;
        cluster7.data ={vehicleIDY.begin()+ first+second+third+fourth+five+six, vehicleIDY.begin()+ first+second+third+fourth+five+six+seven};
        cluster7.id =7;

        clusterWithId cluster8;
        cluster8.data ={vehicleIDY.begin()+ first+second+third+fourth+five+six+seven, vehicleIDY.begin()+ first+second+third+fourth+five+six+seven+eight};
        cluster8.id =8;

        clusterWithId cluster9;
        cluster9.data ={vehicleIDY.begin()+ first+second+third+fourth+five+six+seven+eight, vehicleIDY.begin()+ first+second+third+fourth+five+six+seven+eight+nine};
        cluster9.id =9;


        shuffle(cluster1.data.begin(), cluster1.data.end(), g);
        shuffle(cluster2.data.begin(), cluster2.data.end(), g);
        shuffle(cluster3.data.begin(), cluster3.data.end(), g);
        shuffle(cluster4.data.begin(), cluster4.data.end(), g);
        shuffle(cluster5.data.begin(), cluster5.data.end(), g);
        shuffle(cluster6.data.begin(), cluster6.data.end(), g);
        shuffle(cluster7.data.begin(), cluster7.data.end(), g);
        shuffle(cluster8.data.begin(), cluster8.data.end(), g);
        shuffle(cluster9.data.begin(), cluster9.data.end(), g);

        int centerClusterIndex1 = static_cast<int>(cluster1.data.size()/2);
        auto centerClusterIndex2 = cluster2.data.size()/2;
        auto centerClusterIndex3 = cluster3.data.size()/2;
        auto centerClusterIndex4 = cluster4.data.size()/2;
        auto centerClusterIndex5 = cluster5.data.size()/2;
        auto centerClusterIndex6 = cluster6.data.size()/2;
        auto centerClusterIndex7 = cluster7.data.size()/2;
        auto centerClusterIndex8 = cluster8.data.size()/2;
        auto centerClusterIndex9 = cluster9.data.size()/2;


        size_t currentIndex1 =0;
        for (const int& node: cluster1.data){
            if (currentIndex1 == centerClusterIndex1){
                for (int i=0; i<15; ++i){
                    if (i==0){
                        auto vehicleSpeed = rand()% 8 + 5;
                        speedValues.push_back(vehicleSpeed);
                        nodeIds.push_back(node);
                        bsm->setVehicleSpeed(vehicleSpeed);
                        node_speed3[10][node]=vehicleSpeed;
                        std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";

                    }
                    else{
                auto vehicleSpeed = rand()% 8 + 5;
                speedValues.push_back(vehicleSpeed);
                nodeIds.push_back(node);
                bsm->setVehicleSpeed(vehicleSpeed);
                node_speed3[10][node+i+350]=vehicleSpeed;
                std::cout<< "Node " << node+i+350 << ": BSM sent " << vehicleSpeed << "\n";
                    }
                }

            }

            else {
            auto vehicleSpeed = rand()% 8 + 20;
            speedValues.push_back(vehicleSpeed);
            nodeIds.push_back(node);
            bsm->setVehicleSpeed(vehicleSpeed);
            node_speed3[cluster1.id][node]=vehicleSpeed;
            std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
            }
            ++currentIndex1;
        }
        cout<<std::endl;

        size_t currentIndex2 =0;
        for (const int& node: cluster2.data){
            if (currentIndex2 == centerClusterIndex2){
                for (int i=0; i<15; ++i){
                    if (i==0){
                      auto vehicleSpeed = rand()% 8 + 5;
                      speedValues.push_back(vehicleSpeed);
                      nodeIds.push_back(node);
                      bsm->setVehicleSpeed(vehicleSpeed);
                      node_speed3[10][node]=vehicleSpeed;
                      std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                }else{
                    auto vehicleSpeed = rand()% 8 + 5;
                    speedValues.push_back(vehicleSpeed);
                    nodeIds.push_back(node);
                    bsm->setVehicleSpeed(vehicleSpeed);
                    node_speed3[10][node+i+400]=vehicleSpeed;
                    //node_speed3[1][node+i+400]=vehicleSpeed;
                    std::cout<< "Node " << node+i+400 << ": BSM sent " << vehicleSpeed << "\n";
                    }
                }
            }
           else {
                auto vehicleSpeed = rand() % 8 + 30;
                speedValues.push_back(vehicleSpeed);
                nodeIds.push_back(node);
                bsm->setVehicleSpeed(vehicleSpeed);
                node_speed3[cluster2.id][node]=vehicleSpeed;
                std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
            }
            ++currentIndex2;
        }
        cout<<std::endl;

        size_t currentIndex3 =0;
        for (const int& node: cluster3.data){
            if (currentIndex3 == centerClusterIndex3){
                for (int i=0; i<15; ++i){
                  if (i==0){
                        auto vehicleSpeed = rand()% 8 + 5;
                        speedValues.push_back(vehicleSpeed);
                        nodeIds.push_back(node);
                        bsm->setVehicleSpeed(vehicleSpeed);
                        node_speed3[10][node]=vehicleSpeed;
                        //node_speed3[1][node]=vehicleSpeed;
                        std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                    } else{
                auto vehicleSpeed = rand()% 8 + 5;
                speedValues.push_back(vehicleSpeed);
                nodeIds.push_back(node);
                bsm->setVehicleSpeed(vehicleSpeed);
                node_speed3[10][node+i+450]=vehicleSpeed;
                //node_speed3[1+ i + 500][node]=vehicleSpeed;
                std::cout<< "Node " << node + i + 450 << ": BSM sent " << vehicleSpeed << "\n";
                    }
                }
            }else {
                auto vehicleSpeed = rand() % 8 + 40;
                speedValues.push_back(vehicleSpeed);
                nodeIds.push_back(node);
                bsm->setVehicleSpeed(vehicleSpeed);
                node_speed3[cluster3.id][node]=vehicleSpeed;
                std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
            }
           ++currentIndex3;
        }
        cout<<std::endl;


        size_t currentIndex4 =0;
               for (const int& node: cluster4.data){
                   if (currentIndex4 == centerClusterIndex4){
                       for (int i=0; i<15; ++i){
                         if (i==0){
                               auto vehicleSpeed = rand()% 8 + 5;
                               speedValues.push_back(vehicleSpeed);
                               nodeIds.push_back(node);
                               bsm->setVehicleSpeed(vehicleSpeed);
                               node_speed3[10][node]=vehicleSpeed;
                               //node_speed3[1][node]=vehicleSpeed;
                              std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                           }
                         else{
                       auto vehicleSpeed = rand()% 8 + 5;
                       speedValues.push_back(vehicleSpeed);
                       nodeIds.push_back(node);
                       bsm->setVehicleSpeed(vehicleSpeed);
                       node_speed3[10][node + i + 500]=vehicleSpeed;
                       //node_speed3[1][node + i + 600]=vehicleSpeed;
                      std::cout<< "Node " << node + i + 500 << ": BSM sent " << vehicleSpeed << "\n";
                           }
                       }
                   }else {
                       auto vehicleSpeed = rand() % 8 + 50;
                       speedValues.push_back(vehicleSpeed);
                       nodeIds.push_back(node);
                       bsm->setVehicleSpeed(vehicleSpeed);
                       node_speed3[cluster4.id][node]=vehicleSpeed;
                       std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                   }
                  ++currentIndex4;
               }
               cout<<std::endl;


               size_t currentIndex5 =0;
                              for (const int& node: cluster5.data){
                                  if (currentIndex5 == centerClusterIndex5){
                                      for (int i=0; i<15; ++i){
                                        if (i==0){
                                              auto vehicleSpeed = rand()% 8 + 5;
                                              speedValues.push_back(vehicleSpeed);
                                              nodeIds.push_back(node);
                                              bsm->setVehicleSpeed(vehicleSpeed);
                                              node_speed3[10][node]=vehicleSpeed;
                                              //node_speed3[1][node]=vehicleSpeed;
                                             std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                                          }
                                        else{
                                      auto vehicleSpeed = rand()% 8 + 5;
                                      speedValues.push_back(vehicleSpeed);
                                      nodeIds.push_back(node);
                                      bsm->setVehicleSpeed(vehicleSpeed);
                                      node_speed3[10][node + i + 550]=vehicleSpeed;
                                      //node_speed3[1][node + i + 600]=vehicleSpeed;
                                     std::cout<< "Node " << node + i + 550 << ": BSM sent " << vehicleSpeed << "\n";
                                          }
                                      }
                                  }else {
                                      auto vehicleSpeed = rand() % 8 + 60;
                                      speedValues.push_back(vehicleSpeed);
                                      nodeIds.push_back(node);
                                      bsm->setVehicleSpeed(vehicleSpeed);
                                      node_speed3[cluster5.id][node]=vehicleSpeed;
                                      std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                                  }
                                 ++currentIndex5;
                              }
                              cout<<std::endl;


                              size_t currentIndex6 =0;
                                                            for (const int& node: cluster6.data){
                                                                if (currentIndex6 == centerClusterIndex6){
                                                                    for (int i=0; i<15; ++i){
                                                                      if (i==0){
                                                                            auto vehicleSpeed = rand()% 8 + 5;
                                                                            speedValues.push_back(vehicleSpeed);
                                                                            nodeIds.push_back(node);
                                                                            bsm->setVehicleSpeed(vehicleSpeed);
                                                                            node_speed3[10][node]=vehicleSpeed;
                                                                            //node_speed3[1][node]=vehicleSpeed;
                                                                           std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                                                                        }
                                                                      else{
                                                                    auto vehicleSpeed = rand()% 8 + 5;
                                                                    speedValues.push_back(vehicleSpeed);
                                                                    nodeIds.push_back(node);
                                                                    bsm->setVehicleSpeed(vehicleSpeed);
                                                                    node_speed3[10][node + i + 600]=vehicleSpeed;
                                                                    //node_speed3[1][node + i + 600]=vehicleSpeed;
                                                                   std::cout<< "Node " << node + i + 600 << ": BSM sent " << vehicleSpeed << "\n";
                                                                        }
                                                                    }
                                                                }else {
                                                                    auto vehicleSpeed = rand() % 8 + 70;
                                                                    speedValues.push_back(vehicleSpeed);
                                                                    nodeIds.push_back(node);
                                                                    bsm->setVehicleSpeed(vehicleSpeed);
                                                                    node_speed3[cluster6.id][node]=vehicleSpeed;
                                                                    std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                                                                }
                                                               ++currentIndex6;
                                                            }
                                                            cout<<std::endl;


                                                            size_t currentIndex7 =0;
                                                                                          for (const int& node: cluster7.data){
                                                                                              if (currentIndex7 == centerClusterIndex7){
                                                                                                  for (int i=0; i<15; ++i){
                                                                                                    if (i==0){
                                                                                                          auto vehicleSpeed = rand()% 8 + 5;
                                                                                                          speedValues.push_back(vehicleSpeed);
                                                                                                          nodeIds.push_back(node);
                                                                                                          bsm->setVehicleSpeed(vehicleSpeed);
                                                                                                          node_speed3[10][node]=vehicleSpeed;
                                                                                                          //node_speed3[1][node]=vehicleSpeed;
                                                                                                         std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                                                                                                      }
                                                                                                    else{
                                                                                                  auto vehicleSpeed = rand()% 8 + 5;
                                                                                                  speedValues.push_back(vehicleSpeed);
                                                                                                  nodeIds.push_back(node);
                                                                                                  bsm->setVehicleSpeed(vehicleSpeed);
                                                                                                  node_speed3[10][node + i + 650]=vehicleSpeed;
                                                                                                  //node_speed3[1][node + i + 600]=vehicleSpeed;
                                                                                                 std::cout<< "Node " << node + i + 650 << ": BSM sent " << vehicleSpeed << "\n";
                                                                                                      }
                                                                                                  }
                                                                                              }else {
                                                                                                  auto vehicleSpeed = rand() % 8 + 80;
                                                                                                  speedValues.push_back(vehicleSpeed);
                                                                                                  nodeIds.push_back(node);
                                                                                                  bsm->setVehicleSpeed(vehicleSpeed);
                                                                                                  node_speed3[cluster7.id][node]=vehicleSpeed;
                                                                                                  std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                                                                                              }
                                                                                             ++currentIndex7;
                                                                                          }
                                                                                          cout<<std::endl;


                                                                                          size_t currentIndex8 =0;
                                                                                                                                                                                    for (const int& node: cluster8.data){
                                                                                                                                                                                        if (currentIndex8 == centerClusterIndex8){
                                                                                                                                                                                            for (int i=0; i<15; ++i){
                                                                                                                                                                                              if (i==0){
                                                                                                                                                                                                    auto vehicleSpeed = rand()% 8 + 5;
                                                                                                                                                                                                    speedValues.push_back(vehicleSpeed);
                                                                                                                                                                                                    nodeIds.push_back(node);
                                                                                                                                                                                                    bsm->setVehicleSpeed(vehicleSpeed);
                                                                                                                                                                                                    node_speed3[10][node]=vehicleSpeed;
                                                                                                                                                                                                    //node_speed3[1][node]=vehicleSpeed;
                                                                                                                                                                                                   std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                                                                                                                                                                                                }
                                                                                                                                                                                              else{
                                                                                                                                                                                            auto vehicleSpeed = rand()% 8 + 5;
                                                                                                                                                                                            speedValues.push_back(vehicleSpeed);
                                                                                                                                                                                            nodeIds.push_back(node);
                                                                                                                                                                                            bsm->setVehicleSpeed(vehicleSpeed);
                                                                                                                                                                                            node_speed3[10][node + i + 700]=vehicleSpeed;
                                                                                                                                                                                            //node_speed3[1][node + i + 600]=vehicleSpeed;
                                                                                                                                                                                           std::cout<< "Node " << node + i + 700 << ": BSM sent " << vehicleSpeed << "\n";
                                                                                                                                                                                                }
                                                                                                                                                                                            }
                                                                                                                                                                                        }else {
                                                                                                                                                                                            auto vehicleSpeed = rand() % 8 + 90;
                                                                                                                                                                                            speedValues.push_back(vehicleSpeed);
                                                                                                                                                                                            nodeIds.push_back(node);
                                                                                                                                                                                            bsm->setVehicleSpeed(vehicleSpeed);
                                                                                                                                                                                            node_speed3[cluster8.id][node]=vehicleSpeed;
                                                                                                                                                                                            std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                                                                                                                                                                                        }
                                                                                                                                                                                       ++currentIndex8;
                                                                                                                                                                                    }
                                                                                                                                                                                    cout<<std::endl;

                                                                                                                                                                                                                                                                                                                                                              size_t currentIndex9 =0;
                                                                                                                                                                                                                                                                                                                                                                        for (const int& node: cluster9.data){
                                                                                                                                                                                                                                                                                                                                                                            if (currentIndex9 == centerClusterIndex9){
                                                                                                                                                                                                                                                                                                                                                                                for (int i=0; i<15; ++i){
                                                                                                                                                                                                                                                                                                                                                                                  if (i==0){
                                                                                                                                                                                                                                                                                                                                                                                        auto vehicleSpeed = rand()% 8 + 5;
                                                                                                                                                                                                                                                                                                                                                                                        speedValues.push_back(vehicleSpeed);
                                                                                                                                                                                                                                                                                                                                                                                        nodeIds.push_back(node);
                                                                                                                                                                                                                                                                                                                                                                                        bsm->setVehicleSpeed(vehicleSpeed);
                                                                                                                                                                                                                                                                                                                                                                                        node_speed3[10][node]=vehicleSpeed;
                                                                                                                                                                                                                                                                                                                                                                                        //node_speed3[1][node]=vehicleSpeed;
                                                                                                                                                                                                                                                                                                                                                                                       std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                                                                                                                                                                                                                                                                                                                                                                                    }
                                                                                                                                                                                                                                                                                                                                                                                  else{
                                                                                                                                                                                                                                                                                                                                                                                auto vehicleSpeed = rand()% 8 + 5;
                                                                                                                                                                                                                                                                                                                                                                                speedValues.push_back(vehicleSpeed);
                                                                                                                                                                                                                                                                                                                                                                                nodeIds.push_back(node);
                                                                                                                                                                                                                                                                                                                                                                                bsm->setVehicleSpeed(vehicleSpeed);
                                                                                                                                                                                                                                                                                                                                                                                node_speed3[10][node + i + 750]=vehicleSpeed;
                                                                                                                                                                                                                                                                                                                                                                                //node_speed3[1][node + i + 600]=vehicleSpeed;
                                                                                                                                                                                                                                                                                                                                                                               std::cout<< "Node " << node + i + 750 << ": BSM sent " << vehicleSpeed << "\n";                                                            //comment
                                                                                                                                                                                                                                                                                                                                                                                    }
                                                                                                                                                                                                                                                                                                                                                                                }
                                                                                                                                                                                                                                                                                                                                                                            }else {
                                                                                                                                                                                                                                                                                                                                                                                auto vehicleSpeed = rand() % 8 + 100;
                                                                                                                                                                                                                                                                                                                                                                                speedValues.push_back(vehicleSpeed);
                                                                                                                                                                                                                                                                                                                                                                                nodeIds.push_back(node);
                                                                                                                                                                                                                                                                                                                                                                                bsm->setVehicleSpeed(vehicleSpeed);
                                                                                                                                                                                                                                                                                                                                                                                node_speed3[cluster9.id][node]=vehicleSpeed;
                                                                                                                                                                                                                                                                                                                                                                                std::cout<< "Node " << node << ": BSM sent " << vehicleSpeed << "\n";
                                                                                                                                                                                                                                                                                                                                                                            }
                                                                                                                                                                                                                                                                                                                                                                           ++currentIndex9;
                                                                                                                                                                                                                                                                                                                                                                        }
                                                                                                                                                                                                                                                                                                                                                                        cout<<std::endl;




        bsm->setSenderPos(curPosition);
        bsm->setSenderSpeed(curSpeed);
        bsm->setPsid(-1);
        bsm->setChannelNumber(static_cast<int>(Channel::cch));
        bsm->addBitLength(beaconLengthBits);
        wsm->setUserPriority(beaconUserPriority);
    }
    else if (DemoServiceAdvertisment* wsa = dynamic_cast<DemoServiceAdvertisment*>(wsm)) {
        wsa->setChannelNumber(static_cast<int>(Channel::cch));
        wsa->setTargetChannel(static_cast<int>(currentServiceChannel));
        wsa->setPsid(currentOfferedServiceId);
        wsa->setServiceDescription(currentServiceDescription.c_str());
    }
    else {
        if (dataOnSch)
            wsm->setChannelNumber(static_cast<int>(Channel::sch1)); // will be rewritten at Mac1609_4 to actual Service Channel. This is just so no controlInfo is needed
        else
            wsm->setChannelNumber(static_cast<int>(Channel::cch));
        wsm->addBitLength(dataLengthBits);
        wsm->setUserPriority(dataUserPriority);
    }
}

void DemoBaseApplLayer::receiveSignal(cComponent* source, simsignal_t signalID, cObject* obj, cObject* details)
{
    Enter_Method_Silent();
    if (signalID == BaseMobility::mobilityStateChangedSignal) {
        handlePositionUpdate(obj);
    }
    else if (signalID == TraCIMobility::parkingStateChangedSignal) {
        handleParkingUpdate(obj);
    }
}

void DemoBaseApplLayer::handlePositionUpdate(cObject* obj)
{
    ChannelMobilityPtrType const mobility = check_and_cast<ChannelMobilityPtrType>(obj);
    curPosition = mobility->getPositionAt(simTime());
    curSpeed = mobility->getCurrentSpeed();
}

void DemoBaseApplLayer::handleParkingUpdate(cObject* obj)
{
    isParked = mobility->getParkingState();
}

void DemoBaseApplLayer::handleLowerMsg(cMessage* msg)
{

    BaseFrame1609_4* wsm = dynamic_cast<BaseFrame1609_4*>(msg);
    ASSERT(wsm);

    if (DemoSafetyMessage* bsm = dynamic_cast<DemoSafetyMessage*>(wsm)) {
        receivedBSMs++;
        onBSM(bsm);
    }
    else if (DemoServiceAdvertisment* wsa = dynamic_cast<DemoServiceAdvertisment*>(wsm)) {
        receivedWSAs++;
        onWSA(wsa);
    }
    else {
        receivedWSMs++;
        onWSM(wsm);
    }

    delete (msg);
}

void DemoBaseApplLayer::handleSelfMsg(cMessage* msg)
{
    switch (msg->getKind()) {
    case SEND_BEACON_EVT: {
        DemoSafetyMessage* bsm = new DemoSafetyMessage();
        populateWSM(bsm);
        sendDown(bsm);
        scheduleAt(simTime() + beaconInterval, sendBeaconEvt);
        break;
    }
    case SEND_WSA_EVT: {
        DemoServiceAdvertisment* wsa = new DemoServiceAdvertisment();
        populateWSM(wsa);
        sendDown(wsa);
        scheduleAt(simTime() + wsaInterval, sendWSAEvt);
        break;
    }
    default: {
        if (msg) EV_WARN << "APP: Error: Got Self Message of unknown kind! Name: " << msg->getName() << endl;
        break;
    }
    }
}

void DemoBaseApplLayer::finish()
{
    recordScalar("generatedWSMs", generatedWSMs);
    recordScalar("receivedWSMs", receivedWSMs);

    recordScalar("generatedBSMs", generatedBSMs);
    recordScalar("receivedBSMs", receivedBSMs);

    recordScalar("generatedWSAs", generatedWSAs);
    recordScalar("receivedWSAs", receivedWSAs);
}

DemoBaseApplLayer::~DemoBaseApplLayer()
{
    cancelAndDelete(sendBeaconEvt);
    cancelAndDelete(sendWSAEvt);
    findHost()->unsubscribe(BaseMobility::mobilityStateChangedSignal, this);
}

void DemoBaseApplLayer::startService(Channel channel, int serviceId, std::string serviceDescription)
{
    if (sendWSAEvt->isScheduled()) {
        throw cRuntimeError("Starting service although another service was already started");
    }

    mac->changeServiceChannel(channel);
    currentOfferedServiceId = serviceId;
    currentServiceChannel = channel;
    currentServiceDescription = serviceDescription;

    simtime_t wsaTime = computeAsynchronousSendingTime(wsaInterval, ChannelType::control);
    scheduleAt(wsaTime, sendWSAEvt);
}

void DemoBaseApplLayer::stopService()
{
    cancelEvent(sendWSAEvt);
    currentOfferedServiceId = -1;
}

void DemoBaseApplLayer::sendDown(cMessage* msg)
{
    checkAndTrackPacket(msg);
    BaseApplLayer::sendDown(msg);
}

void DemoBaseApplLayer::sendDelayedDown(cMessage* msg, simtime_t delay)
{
    checkAndTrackPacket(msg);
    BaseApplLayer::sendDelayedDown(msg, delay);
}

void DemoBaseApplLayer::checkAndTrackPacket(cMessage* msg)
{
    if (dynamic_cast<DemoSafetyMessage*>(msg)) {
        EV_TRACE << "sending down a BSM" << std::endl;
        generatedBSMs++;
    }
    else if (dynamic_cast<DemoServiceAdvertisment*>(msg)) {
        EV_TRACE << "sending down a WSA" << std::endl;
        generatedWSAs++;
    }
    else if (dynamic_cast<BaseFrame1609_4*>(msg)) {
        EV_TRACE << "sending down a wsm" << std::endl;
        generatedWSMs++;
    }
}
