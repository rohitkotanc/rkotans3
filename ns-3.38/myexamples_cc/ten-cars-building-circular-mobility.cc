#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/netsimulyzer-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/buildings-module.h>
#include <cmath>
#include <string>

using namespace ns3;

int main(int argc, char* argv[])
{
    double durationUser = 20.0;
    std::string outputFileName = "ten-cars-building-circular-mobility.json";

    CommandLine cmd(__FILE__);
    cmd.AddValue("outputFileName", "The name of the file to write the NetSimulyzer trace info", outputFileName);
    cmd.AddValue("duration", "Duration (in Seconds) of the simulation", durationUser);
    cmd.Parse(argc, argv);

    NS_ABORT_MSG_IF(durationUser < 3.0, "Scenario must be at least three seconds long");
    const auto duration = Seconds(durationUser);

    NodeContainer nodes;
    nodes.Create(11); 

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::WaypointMobilityModel");
    mobility.Install(nodes);

    nodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0.0, 0.0, 0.0));

    double radii[] = {5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0};
    double speeds[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    int numWaypoints = 72;

    int closestNodeIndex = 1;
    int farthestNodeIndex = 1;

    for (int j = 1; j <= 10; ++j) {
        Ptr<WaypointMobilityModel> mobilityModel = nodes.Get(j)->GetObject<WaypointMobilityModel>();
        for (int i = 0; i < numWaypoints; ++i) {
            double angle = 2 * M_PI * i / numWaypoints;
            double x = radii[j - 1] * cos(angle);
            double y = -radii[j - 1] * sin(angle);
            double time = i * (2 * M_PI * radii[j - 1] / speeds[j - 1]) / numWaypoints;
            mobilityModel->AddWaypoint(Waypoint(Seconds(time), Vector(x, y, 0.0)));
        }

        if (radii[j - 1] < radii[closestNodeIndex - 1]) closestNodeIndex = j;
        if (radii[j - 1] > radii[farthestNodeIndex - 1]) farthestNodeIndex = j;
    }

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("0ms"));

    NetDeviceContainer device = pointToPoint.Install(nodes.Get(closestNodeIndex), nodes.Get(farthestNodeIndex));

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(device);

    const uint16_t echoPort = 9;
    UdpEchoServerHelper echoServer(echoPort);
    ApplicationContainer serverApp = echoServer.Install(nodes.Get(farthestNodeIndex));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(duration - Seconds(1.0));

    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), echoPort);
    echoClient.SetAttribute("MaxPackets", UintegerValue(100000));
    echoClient.SetAttribute("Interval", TimeValue(MilliSeconds(100)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp = echoClient.Install(nodes.Get(closestNodeIndex));
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(duration - Seconds(1.0));

    Ptr<netsimulyzer::Orchestrator> orchestrator = CreateObject<netsimulyzer::Orchestrator>(outputFileName);
    orchestrator->SetTimeStep(MilliSeconds(100), Time::MS);
    orchestrator->SetAttribute("PollMobility", BooleanValue(true));

    netsimulyzer::NodeConfigurationHelper nodeHelper(orchestrator);

    for (int i = 1; i <= 10; ++i) {
        nodeHelper.Set("Name", StringValue("Node"));  
        nodeHelper.Set("Model", StringValue(netsimulyzer::models::CAR_VALUE));
        if (i == closestNodeIndex || i == farthestNodeIndex) {
            nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::Color3(255, 0, 0)));
        }
        nodeHelper.Install(nodes.Get(i));
    }

    Ptr<Building> building = CreateObject<Building>();
    building->SetBoundaries(Box(0.0, 10.0, 0.0, 10.0, 0.0, 20.0));
    building->SetBuildingType(Building::Commercial);
    building->SetExtWallsType(Building::ConcreteWithWindows);
    building->SetNFloors(5);
    building->SetNRoomsX(3);
    building->SetNRoomsY(3);

    BuildingContainer buildingContainer;
    buildingContainer.Add(building);

    netsimulyzer::BuildingConfigurationHelper buildingHelper(orchestrator);
    buildingHelper.Install(buildingContainer);

    BuildingsHelper::Install(nodes);

  
    Ptr<netsimulyzer::ThroughputSink> clientThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Client Throughput (TX)");
    clientThroughput->SetAttribute("Interval", TimeValue(Seconds(0.1)));
    clientThroughput->SetAttribute("Unit", EnumValue(netsimulyzer::ThroughputSink::Unit::Byte));
    clientThroughput->SetAttribute("TimeUnit", EnumValue(Time::Unit::S));
    clientThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::BLUE_VALUE);

    Ptr<netsimulyzer::ThroughputSink> serverThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Server Throughput (RX)");
    serverThroughput->SetAttribute("Interval", TimeValue(Seconds(0.1)));
    serverThroughput->SetAttribute("Unit", EnumValue(netsimulyzer::ThroughputSink::Unit::Byte));
    serverThroughput->SetAttribute("TimeUnit", EnumValue(Time::Unit::S));
    serverThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::RED_VALUE);

    clientApp.Get(0)->TraceConnectWithoutContext("Tx", MakeCallback(&netsimulyzer::ThroughputSink::AddPacket, clientThroughput));
    serverApp.Get(0)->TraceConnectWithoutContext("Rx", MakeCallback(&netsimulyzer::ThroughputSink::AddPacket, serverThroughput));

    Ptr<netsimulyzer::SeriesCollection> collection = CreateObject<netsimulyzer::SeriesCollection>(orchestrator);
    collection->SetAttribute("Name", StringValue("Client and Server Throughput (TX and RX)"));
    collection->SetAttribute("HideAddedSeries", BooleanValue(false));
    collection->Add(clientThroughput->GetSeries());
    collection->Add(serverThroughput->GetSeries());

    NS_LOG_UNCOND("Starting simulation...");
    Simulator::Stop(duration);
    Simulator::Run();
    Simulator::Destroy();
    NS_LOG_UNCOND("Simulation complete.");
}



