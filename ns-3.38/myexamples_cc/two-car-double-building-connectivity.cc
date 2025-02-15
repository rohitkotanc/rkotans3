#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/mobility-module.h>
#include <ns3/internet-module.h>
#include <ns3/applications-module.h>
#include <ns3/point-to-point-module.h>
#include <ns3/buildings-module.h>
#include <ns3/netsimulyzer-module.h>

using namespace ns3;

void WriteThroughput(Ptr<netsimulyzer::ThroughputSink> sink, Ptr<const Packet> packet)
{
    sink->AddPacket(packet);
}

void DropConnectivity(Ptr<RateErrorModel> clientErrorModel, Ptr<RateErrorModel> serverErrorModel)
{
    clientErrorModel->SetAttribute("ErrorRate", DoubleValue(1.0));
    serverErrorModel->SetAttribute("ErrorRate", DoubleValue(1.0));
    std::cout << "Connectivity dropped at: " << Simulator::Now().GetSeconds() << " seconds." << std::endl;
}

void RestoreConnectivity(Ptr<RateErrorModel> clientErrorModel, Ptr<RateErrorModel> serverErrorModel)
{
    clientErrorModel->SetAttribute("ErrorRate", DoubleValue(0.0));
    serverErrorModel->SetAttribute("ErrorRate", DoubleValue(0.0));
    std::cout << "Connectivity restored at: " << Simulator::Now().GetSeconds() << " seconds." << std::endl;
}

int main(int argc, char* argv[])
{
    double durationUser = 20.0;
    std::string outputFileName = "two-car-double-building-connectivity.json";

    CommandLine cmd;
    cmd.AddValue("outputFileName", "The name of the file to write the NetSimulyzer trace info", outputFileName);
    cmd.AddValue("duration", "Duration (in seconds) of the simulation", durationUser);
    cmd.Parse(argc, argv);

    NS_ABORT_MSG_IF(durationUser < 3.0, "Scenario must be at least three seconds long");
    const auto duration = Seconds(durationUser);

    NS_ABORT_MSG_IF(outputFileName.empty(), "`outputFileName` must not be empty");

    NodeContainer nodes;
    nodes.Create(2);

    MobilityHelper mobility;
    Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator>();
    positionAlloc->Add(Vector3D(55.0, 5.0, 0.0));
    positionAlloc->Add(Vector3D(-25.0, 5.0, 0.0));
    mobility.SetPositionAllocator(positionAlloc);
    mobility.SetMobilityModel("ns3::ConstantVelocityMobilityModel");
    mobility.Install(nodes);

    nodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -5.0, 0.0));
    nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -5.0, 0.0));

    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    pointToPoint.SetChannelAttribute("Delay", StringValue("0ms"));

    NetDeviceContainer netDevices = pointToPoint.Install(nodes.Get(0), nodes.Get(1));

    InternetStackHelper stack;
    stack.Install(nodes);

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
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(duration - Seconds(1.0));

    Ptr<RateErrorModel> clientErrorModel = CreateObject<RateErrorModel>();
    Ptr<RateErrorModel> serverErrorModel = CreateObject<RateErrorModel>();

    netDevices.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue(clientErrorModel));
    netDevices.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(serverErrorModel));

    Simulator::Schedule(Seconds(5.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(6.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(17.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(18.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);

    Ptr<netsimulyzer::Orchestrator> orchestrator = CreateObject<netsimulyzer::Orchestrator>(outputFileName);
    orchestrator->SetTimeStep(MilliSeconds(100), Time::MS);
    orchestrator->SetAttribute("PollMobility", BooleanValue(true));

    netsimulyzer::NodeConfigurationHelper nodeHelper(orchestrator);
    nodeHelper.Set("Model", netsimulyzer::models::CAR_VALUE);
    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::BLUE));
    nodeHelper.Install(nodes.Get(0));

    nodeHelper.Set("HighlightColor", netsimulyzer::OptionalValue<netsimulyzer::Color3>(netsimulyzer::RED));
    nodeHelper.Install(nodes.Get(1));

    Ptr<Building> building1 = CreateObject<Building>();
    building1->SetBoundaries(Box(15.0, 20.0, -25.0, -20.0, 0.0, 45.0));
    building1->SetBuildingType(Building::Residential);
    building1->SetExtWallsType(Building::ConcreteWithWindows);
    building1->SetNFloors(3);
    building1->SetNRoomsX(2);
    building1->SetNRoomsY(2);

    Ptr<Building> building2 = CreateObject<Building>();
    building2->SetBoundaries(Box(15.0, 20.0, -85.0, -80.0, 0.0, 15.0));
    building2->SetBuildingType(Building::Commercial);
    building2->SetExtWallsType(Building::ConcreteWithWindows);
    building2->SetNFloors(8);
    building2->SetNRoomsX(1);
    building2->SetNRoomsY(1);

    BuildingContainer buildingContainer;
    buildingContainer.Add(building1);
    buildingContainer.Add(building2);

    netsimulyzer::BuildingConfigurationHelper buildingHelper(orchestrator);
    buildingHelper.Install(buildingContainer);

    Ptr<netsimulyzer::ThroughputSink> clientThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Client Throughput (RX)");
    clientThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::BLUE_VALUE);
    clientApp.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&WriteThroughput, clientThroughput));

    Ptr<netsimulyzer::ThroughputSink> serverThroughput = CreateObject<netsimulyzer::ThroughputSink>(orchestrator, "UDP Echo Server Throughput (RX)");
    serverThroughput->GetSeries()->SetAttribute("Color", netsimulyzer::RED_VALUE);
    serverApp.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&WriteThroughput, serverThroughput));

    Simulator::Stop(duration);
    Simulator::Run();
    Simulator::Destroy();

    return 0;
}


