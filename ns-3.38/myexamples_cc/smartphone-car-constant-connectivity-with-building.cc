#include <ns3/applications-module.h>
#include <ns3/core-module.h>
#include <ns3/internet-module.h>
#include <ns3/mobility-module.h>
#include <ns3/netsimulyzer-module.h>
#include <ns3/network-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/buildings-module.h>
#include <iostream>

using namespace ns3;

void WriteThroughput(Ptr<netsimulyzer::ThroughputSink> sink, Ptr<const Packet> packet)
{
    sink->AddPacket(packet);
}

double currentErrorRate = 0.0;

void IncreasePacketLoss(Ptr<RateErrorModel> errorModel, double stepLoss, double maxLoss, double stopTime)
{
    double currentTime = Simulator::Now().GetSeconds();

    if (currentTime >= stopTime)
    {
        std::cout << "Maximum packet loss reached at " << currentTime << "s. Packet loss: " << maxLoss * 100 << "%\n";
        return;
    }

    currentErrorRate = std::min(maxLoss, currentErrorRate + stepLoss);
    errorModel->SetAttribute("ErrorRate", DoubleValue(currentErrorRate));

    std::cout << "[DEBUG] Time: " << currentTime << "s | Packet Loss: " << currentErrorRate * 100 << "%\n";

    Simulator::Schedule(Seconds(0.5), &IncreasePacketLoss, errorModel, stepLoss, maxLoss, stopTime);
}

void RestoreConnectivity(Ptr<RateErrorModel> errorModel)
{
    errorModel->SetAttribute("ErrorRate", DoubleValue(0.0));
    currentErrorRate = 0.0;
    std::cout << "[DEBUG] Connectivity restored at: " << Simulator::Now().GetSeconds() << "s.\n";
}

int main(int argc, char* argv[])
{
    double durationUser = 20.0;
    std::string outputFileName = "smartphone-car-constant-connectivity-with-building.json";

    CommandLine cmd(__FILE__);
    cmd.AddValue("outputFileName", "The name of the file to write the NetSimulyzer trace info", outputFileName);
    cmd.AddValue("duration", "Duration (in seconds) of the simulation", durationUser);
    cmd.Parse(argc, argv);

    NS_ABORT_MSG_IF(durationUser < 3.0, "Scenario must be at least three seconds long");
    const auto duration = Seconds(durationUser);

    NS_ABORT_MSG_IF(outputFileName.empty(), "`outputFileName` must not be empty");

    NodeContainer nodes;
    nodes.Create(3);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector3D(25.0, 5.0, 0.0));
    positionAlloc->Add(Vector3D(5.0, 5.0, 0.0));
    positionAlloc->Add(Vector3D(15.0, -10.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(nodes);

    nodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -2.5, 0.0));
    nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, 0.0, 0.0));
    nodes.Get(2)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, 0.0, 0.0));

    Ptr<netsimulyzer::Orchestrator> orchestrator = CreateObject<netsimulyzer::Orchestrator>(outputFileName);
    orchestrator->SetTimeStep(MilliSeconds(100), Time::MS);
    orchestrator->SetAttribute("PollMobility", BooleanValue(true));
 Ptr<Building> building = CreateObject<Building>();
    building->SetBoundaries(Box(60.0, 70.0, 0.0, 10.0, 0.0, 20.0));
    building->SetBuildingType(Building::Residential);
    building->SetExtWallsType(Building::ConcreteWithWindows);
    building->SetNFloors(3);
    building->SetNRoomsX(2);
    building->SetNRoomsY(2);

    BuildingContainer buildingContainer;
    buildingContainer.Add(building);

    netsimulyzer::BuildingConfigurationHelper buildingHelper(orchestrator);
    buildingHelper.Install(buildingContainer);


    BuildingsHelper::Install(nodes);

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("100ms"));

    NetDeviceContainer netDevices = pointToPoint.Install(nodes.Get(0), nodes.Get(1));

    InternetStackHelper stack;
    stack.Install(nodes.Get(0));
    stack.Install(nodes.Get(1));

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer interfaces = address.Assign(netDevices);

    const uint16_t echoPort = 9;
    UdpEchoServerHelper echoServer(echoPort);
    ApplicationContainer serverApp = echoServer.Install(nodes.Get(1));
    serverApp.Start(Seconds(1.0));
    serverApp.Stop(duration - Seconds(1.0));

    UdpEchoClientHelper echoClient(interfaces.GetAddress(1), echoPort);
    echoClient.SetAttribute("MaxPackets", UintegerValue(10000));
    echoClient.SetAttribute("Interval", TimeValue(Seconds(0.1)));
    echoClient.SetAttribute("PacketSize", UintegerValue(1024));

    ApplicationContainer clientApp = echoClient.Install(nodes.Get(0));
clientApp.Start(Seconds(2.0));
    clientApp.Stop(Seconds(5.0));  

    Ptr<RateErrorModel> errorModelClient = CreateObject<RateErrorModel>();
    Ptr<RateErrorModel> errorModelServer = CreateObject<RateErrorModel>();

    errorModelClient->SetAttribute("ErrorRate", DoubleValue(0.0));
    errorModelServer->SetAttribute("ErrorRate", DoubleValue(0.0));

    netDevices.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(errorModelClient));
    netDevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(errorModelServer));

    double stepLoss = 0.05;
    double maxLoss = 1.0;
    double degradationDuration = 5.0;

    Simulator::Schedule(Seconds(5.0), &IncreasePacketLoss, errorModelClient, stepLoss, maxLoss, 5.0 + degradationDuration);
    Simulator::Schedule(Seconds(5.0), &IncreasePacketLoss, errorModelServer, stepLoss, maxLoss, 5.0 + degradationDuration);

    netsimulyzer::NodeConfigurationHelper nodeHelper(orchestrator);

    nodeHelper.Set("Model", netsimulyzer::models::CAR_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::BLUE));
    nodeHelper.Install(nodes.Get(0));

    nodeHelper.Set("Model", netsimulyzer::models::SMARTPHONE_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::RED));
    nodeHelper.Install(nodes.Get(1));

    nodeHelper.Set("Model", netsimulyzer::models::CELL_TOWER_POLE_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::GREEN));
    nodeHelper.Install(nodes.Get(2));

    Ptr<netsimulyzer::ThroughputSink> clientThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Client Throughput (TX)");
    clientThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::BLUE_VALUE);
    clientApp.Get(0)->TraceConnectWithoutContext("Tx", MakeBoundCallback(&WriteThroughput, clientThroughput));

    Ptr<netsimulyzer::ThroughputSink> serverThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Server Throughput (RX)");
    serverThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::RED_VALUE);
    serverApp.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&WriteThroughput, serverThroughput));

    Simulator::Stop(duration);
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}



