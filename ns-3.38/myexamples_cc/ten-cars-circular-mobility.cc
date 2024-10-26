#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/netsimulyzer-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>
#include <cmath>
#include <string>

using namespace ns3;

void WriteServerThroughput(Ptr<const Packet> packet)
{
    NS_LOG_UNCOND("Packet received by server, adding to throughput sink.");
}

int main(int argc, char* argv[])
{
    double durationUser = 20.0;
    std::string outputFileName = "ten-cars-circular-mobility.json";

    CommandLine cmd(__FILE__);
    cmd.AddValue("outputFileName", "The name of the file to write the NetSimulyzer trace info", outputFileName);
    cmd.AddValue("duration", "Duration (in Seconds) of the simulation", durationUser);
    cmd.Parse(argc, argv);

    NS_ABORT_MSG_IF(durationUser < 3.0, "Scenario must be at least three seconds long");
    const auto duration = Seconds(durationUser);

    NS_ABORT_MSG_IF(outputFileName.empty(), "`outputFileName` must not be empty");

    NodeContainer nodes;
    nodes.Create(11);  // 1 central pole (node 0) and 10 cars

    MobilityHelper mobility;
    mobility.SetMobilityModel("ns3::WaypointMobilityModel");
    mobility.Install(nodes);

    nodes.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0.0, 0.0, 0.0));

    double radii[] = {5.0, 10.0, 15.0, 20.0, 25.0, 30.0, 35.0, 40.0, 45.0, 50.0};
    double speeds[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0};
    int numWaypoints = 72;

    for (int j = 1; j <= 10; ++j) {
        Ptr<WaypointMobilityModel> carMobility = nodes.Get(j)->GetObject<WaypointMobilityModel>();
        for (int i = 0; i < numWaypoints; ++i) {
            double angle = 2 * M_PI * i / numWaypoints;
            double x = radii[j - 1] * cos(angle);
            double y = - radii[j - 1] * sin(angle);
            double time = i * (2 * M_PI * radii[j - 1] / speeds[j - 1]) / numWaypoints;
            carMobility->AddWaypoint(Waypoint(Seconds(time), Vector(x, y, 0.0)));
        }
    }

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("0ms"));

    std::vector<NetDeviceContainer> netDevices;
    for (int i = 0; i < 10; ++i) {
        netDevices.push_back(pointToPoint.Install(nodes.Get(i + 1), nodes.Get(0))); // Cars connecting to central pole
    }

    InternetStackHelper stack;
    stack.Install(nodes);

    Ipv4AddressHelper address;
    for (size_t i = 0; i < netDevices.size(); ++i) {
        std::string base = "10.1." + std::to_string(i + 1) + ".0";
        address.SetBase(base.c_str(), "255.255.255.0");
        address.Assign(netDevices[i]);
    }

    const auto echoPort = 9u;
    UdpEchoServerHelper echoServer{echoPort};

    auto serverApp = echoServer.Install(nodes.Get(1u));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(duration - Seconds(1.0));

    UdpEchoClientHelper echoClient(address.NewAddress(), echoPort);
    echoClient.SetAttribute("MaxPackets", UintegerValue(10'000u));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(2.0)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024u));

    auto clientApp = echoClient.Install(nodes.Get(0u)); // Central node acting as client
    clientApp.Start(Seconds(2.0));
    clientApp.Stop(duration - Seconds(1.0));

    // Orchestrator setup for visualization
    auto orchestrator = CreateObject<netsimulyzer::Orchestrator>(outputFileName);
    orchestrator->SetTimeStep(MilliSeconds(100), Time::MS);
    orchestrator->SetAttribute("PollMobility", BooleanValue(true));

    netsimulyzer::NodeConfigurationHelper nodeHelper{orchestrator};

    std::vector<std::pair<ns3::netsimulyzer::Color3, ns3::StringValue>> nodeDetails = {
        std::make_pair(ns3::netsimulyzer::Color3(128, 128, 128), ns3::StringValue(netsimulyzer::models::CELL_TOWER_POLE_VALUE)),
        std::make_pair(ns3::netsimulyzer::Color3(255, 0, 0), ns3::StringValue(netsimulyzer::models::CAR_VALUE)),
        std::make_pair(ns3::netsimulyzer::Color3(0, 0, 255), ns3::StringValue(netsimulyzer::models::CAR_VALUE)),
        std::make_pair(ns3::netsimulyzer::Color3(0, 255, 0), ns3::StringValue(netsimulyzer::models::CAR_VALUE)),
        std::make_pair(ns3::netsimulyzer::Color3(255, 255, 0), ns3::StringValue(netsimulyzer::models::CAR_VALUE)),
        std::make_pair(ns3::netsimulyzer::Color3(255, 0, 0), ns3::StringValue(netsimulyzer::models::CAR_VALUE)),
        std::make_pair(ns3::netsimulyzer::Color3(0, 0, 255), ns3::StringValue(netsimulyzer::models::CAR_VALUE)),
        std::make_pair(ns3::netsimulyzer::Color3(255, 165, 0), ns3::StringValue(netsimulyzer::models::CAR_VALUE)),
        std::make_pair(ns3::netsimulyzer::Color3(255, 255, 255), ns3::StringValue(netsimulyzer::models::CAR_VALUE)),
        std::make_pair(ns3::netsimulyzer::Color3(0, 0, 0), ns3::StringValue(netsimulyzer::models::CAR_VALUE)),
        std::make_pair(ns3::netsimulyzer::Color3(128, 0, 128), ns3::StringValue(netsimulyzer::models::CAR_VALUE))
    };

    for (size_t i = 0; i < nodeDetails.size(); ++i) {
        nodeHelper.Set("Model", nodeDetails[i].second);
        nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(nodeDetails[i].first));
        nodeHelper.Install(nodes.Get(i));
    }

    auto clientThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Client Throughput (TX)");
    clientThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::BLUE_VALUE);
    clientThroughput->SetAttribute("Interval", TimeValue(Seconds(1.0)));
    clientThroughput->SetAttribute("Unit", EnumValue(netsimulyzer::ThroughputSink::Unit::Byte));
    clientThroughput->SetAttribute("TimeUnit", EnumValue(Time::Unit::S));

    clientApp.Get(0u)->TraceConnectWithoutContext("Tx", MakeCallback(&netsimulyzer::ThroughputSink::AddPacket, clientThroughput));

    auto serverThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Server Throughput (RX)");
    serverThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::RED_VALUE);
    serverThroughput->SetAttribute("Unit", EnumValue(netsimulyzer::ThroughputSink::Unit::Byte));
    serverThroughput->SetAttribute("TimeUnit", EnumValue(Time::Unit::S));

    serverApp.Get(0u)->TraceConnectWithoutContext("Rx", MakeCallback(&WriteServerThroughput));

    auto collection = CreateObject<netsimulyzer::SeriesCollection>(orchestrator);
    collection->SetAttribute("Name", StringValue("Client and Server Throughput (TX and RX)"));
    collection->SetAttribute("HideAddedSeries", BooleanValue(false));

    collection->Add(clientThroughput->GetSeries());
    collection->Add(serverThroughput->GetSeries());

    NS_LOG_UNCOND("Starting the simulation...");
    
    Simulator::Stop(duration);
    Simulator::Run();
    Simulator::Destroy();
    
    NS_LOG_UNCOND("Simulation completed.");
}





