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
    double durationUser = 95.0;
    std::string outputFileName = "two-car-fifteen-building-connectivity.json";

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

    nodes.Get(0)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -1.0, 0.0));
    nodes.Get(1)->GetObject<ConstantVelocityMobilityModel>()->SetVelocity(Vector(0.0, -1.0, 0.0));
  


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

    Simulator::Schedule(Seconds(6.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(7.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(12.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(13.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(18.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(19.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
   Simulator::Schedule(Seconds(24.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(25.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(30.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(31.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(36.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(37.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(42.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(43.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(48.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(49.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(54.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(55.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(60.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(61.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(66.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(67.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(72.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(73.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(78.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(79.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(84.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(85.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);
Simulator::Schedule(Seconds(90.0), &DropConnectivity, clientErrorModel, serverErrorModel);
    Simulator::Schedule(Seconds(91.0), &RestoreConnectivity, clientErrorModel, serverErrorModel);


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
    building1->SetBoundaries(Box(15.0, 20.0, -1.0, -3.0, 0.0, 45.0));
    building1->SetBuildingType(Building::Residential);
    building1->SetExtWallsType(Building::ConcreteWithWindows);
    building1->SetNFloors(3);
    building1->SetNRoomsX(2);
    building1->SetNRoomsY(2);

        Ptr<Building> building2 = CreateObject<Building>();
    building2->SetBoundaries(Box(15.0, 20.0, -7.0, -9.0, 0.0, 45.0));
    building2->SetBuildingType(Building::Residential);
    building2->SetExtWallsType(Building::ConcreteWithWindows);
    building2->SetNFloors(3);
    building2->SetNRoomsX(2);
    building2->SetNRoomsY(2);

    Ptr<Building> building3 = CreateObject<Building>();
    building3->SetBoundaries(Box(15.0, 20.0, -13.0, -15.0, 0.0, 45.0));
    building3->SetBuildingType(Building::Residential);
    building3->SetExtWallsType(Building::ConcreteWithWindows);
    building3->SetNFloors(3);
    building3->SetNRoomsX(2);
    building3->SetNRoomsY(2);

    Ptr<Building> building4 = CreateObject<Building>();
    building4->SetBoundaries(Box(15.0, 20.0, -19.0, -21.0, 0.0, 45.0));
    building4->SetBuildingType(Building::Residential);
    building4->SetExtWallsType(Building::ConcreteWithWindows);
    building4->SetNFloors(3);
    building4->SetNRoomsX(2);
    building4->SetNRoomsY(2);


    Ptr<Building> building5 = CreateObject<Building>();
    building5->SetBoundaries(Box(15.0, 20.0, -25.0, -27.0, 0.0, 45.0));
    building5->SetBuildingType(Building::Residential);
    building5->SetExtWallsType(Building::ConcreteWithWindows);
    building5->SetNFloors(3);
    building5->SetNRoomsX(2);
    building5->SetNRoomsY(2);


    Ptr<Building> building6 = CreateObject<Building>();
    building6->SetBoundaries(Box(15.0, 20.0, -31.0, -33.0, 0.0, 45.0));
    building6->SetBuildingType(Building::Residential);
    building6->SetExtWallsType(Building::ConcreteWithWindows);
    building6->SetNFloors(3);
    building6->SetNRoomsX(2);
    building6->SetNRoomsY(2);


    Ptr<Building> building7 = CreateObject<Building>();
    building7->SetBoundaries(Box(15.0, 20.0, -37.0, -39.0, 0.0, 45.0));
    building7->SetBuildingType(Building::Residential);
    building7->SetExtWallsType(Building::ConcreteWithWindows);
    building7->SetNFloors(3);
    building7->SetNRoomsX(2);
    building7->SetNRoomsY(2);


    Ptr<Building> building8 = CreateObject<Building>();
    building8->SetBoundaries(Box(15.0, 20.0, -43.0, -45.0, 0.0, 45.0));
    building8->SetBuildingType(Building::Residential);
    building8->SetExtWallsType(Building::ConcreteWithWindows);
    building8->SetNFloors(3);
    building8->SetNRoomsX(2);
    building8->SetNRoomsY(2);


    Ptr<Building> building9 = CreateObject<Building>();
    building9->SetBoundaries(Box(15.0, 20.0, -49.0, -51.0, 0.0, 45.0));
    building9->SetBuildingType(Building::Residential);
    building9->SetExtWallsType(Building::ConcreteWithWindows);
    building9->SetNFloors(3);
    building9->SetNRoomsX(2);
    building9->SetNRoomsY(2);


    Ptr<Building> building10 = CreateObject<Building>();
    building10->SetBoundaries(Box(15.0, 20.0, -55.0, -57.0, 0.0, 45.0));
    building10->SetBuildingType(Building::Residential);
    building10->SetExtWallsType(Building::ConcreteWithWindows);
    building10->SetNFloors(3);
    building10->SetNRoomsX(2);
    building10->SetNRoomsY(2);


    Ptr<Building> building11 = CreateObject<Building>();
    building11->SetBoundaries(Box(15.0, 20.0, -61.0, -63.0, 0.0, 45.0));
    building11->SetBuildingType(Building::Residential);
    building11->SetExtWallsType(Building::ConcreteWithWindows);
    building11->SetNFloors(3);
    building11->SetNRoomsX(2);
    building11->SetNRoomsY(2);


    Ptr<Building> building12 = CreateObject<Building>();
    building12->SetBoundaries(Box(15.0, 20.0, -67.0, -69.0, 0.0, 45.0));
    building12->SetBuildingType(Building::Residential);
    building12->SetExtWallsType(Building::ConcreteWithWindows);
    building12->SetNFloors(3);
    building12->SetNRoomsX(2);
    building12->SetNRoomsY(2);


    Ptr<Building> building13 = CreateObject<Building>();
    building13->SetBoundaries(Box(15.0, 20.0, -73.0, -75.0, 0.0, 45.0));
    building13->SetBuildingType(Building::Residential);
    building13->SetExtWallsType(Building::ConcreteWithWindows);
    building13->SetNFloors(3);
    building13->SetNRoomsX(2);
    building13->SetNRoomsY(2);


    Ptr<Building> building14 = CreateObject<Building>();
    building14->SetBoundaries(Box(15.0, 20.0, -79.0, -81.0, 0.0, 45.0));
    building14->SetBuildingType(Building::Residential);
    building14->SetExtWallsType(Building::ConcreteWithWindows);
    building14->SetNFloors(3);
    building14->SetNRoomsX(2);
    building14->SetNRoomsY(2);


    Ptr<Building> building15 = CreateObject<Building>();
    building15->SetBoundaries(Box(15.0, 20.0, -85.0, -87.0, 0.0, 45.0));
    building15->SetBuildingType(Building::Residential);
    building15->SetExtWallsType(Building::ConcreteWithWindows);
    building15->SetNFloors(3);
    building15->SetNRoomsX(2);
    building15->SetNRoomsY(2);


    BuildingContainer buildingContainer;
    buildingContainer.Add(building1);
    buildingContainer.Add(building2);
buildingContainer.Add(building3);
buildingContainer.Add(building4);
buildingContainer.Add(building5);
buildingContainer.Add(building6);
buildingContainer.Add(building7);
buildingContainer.Add(building8);
buildingContainer.Add(building9);
buildingContainer.Add(building10);
buildingContainer.Add(building11);
buildingContainer.Add(building12);
buildingContainer.Add(building13);
buildingContainer.Add(building14);
buildingContainer.Add(building15);

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




