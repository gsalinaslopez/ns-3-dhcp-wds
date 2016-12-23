/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

#include "ns3/test.h"

using namespace ns3;


class DhcpTestCase1 : public TestCase
{
public:
  DhcpTestCase1 ();
  virtual ~DhcpTestCase1 ();

private:
  virtual void DoRun (void);
};


DhcpTestCase1::DhcpTestCase1 ()
  : TestCase ("Dhcp test case ")
{
}

DhcpTestCase1::~DhcpTestCase1 ()
{
}


void
DhcpTestCase1::DoRun (void)
{
  /*Set up devices*/
  NodeContainer MN;
  NodeContainer Router;
  MN.Create(1);
  Router.Create(1);
  NodeContainer net(MN, Router);

  /*Set up channel*/
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
  csma.SetChannelAttribute ("Delay", StringValue ("2ms"));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  NetDeviceContainer dev_net = csma.Install(net);

  /*Install TCP Stack*/
  InternetStackHelper tcpip;
  tcpip.Install(MN);
  tcpip.Install(Router);

  /*Setup a null address for DHCP Client*/ 
  Ptr<Ipv4> ipv4MN = net.Get(0)->GetObject<Ipv4> (); 
  uint32_t ifIndex1 = ipv4MN->AddInterface (dev_net.Get(0));
  ipv4MN->AddAddress (ifIndex1, Ipv4InterfaceAddress (Ipv4Address ("0.0.0.0"), Ipv4Mask ("/0")));
  ipv4MN->SetForwarding(ifIndex1, true);
  ipv4MN->SetUp (ifIndex1); 
  
  /*Setup the Server*/ 
  Ptr<Ipv4> ipv4Router = net.Get(1)->GetObject<Ipv4> (); 
  uint32_t ifIndex = ipv4Router->AddInterface (dev_net.Get(1));
  ipv4Router->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("172.30.0.1"), Ipv4Mask ("/0"))); 
  ipv4Router->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("172.30.0.1"), Ipv4Mask ("/24")));
  ipv4Router->SetForwarding(ifIndex, true);
  ipv4Router->SetUp (ifIndex);

  /*Install DHCP Server Protocol*/ 
  uint16_t port = 67;
  DhcpServerHelper dhcp_server(Ipv4Address("172.30.0.0"), Ipv4Mask("/24"), Ipv4Address("172.30.0.1"), port);
  ApplicationContainer ap_dhcp_server = dhcp_server.Install(Router.Get(0));
  ap_dhcp_server.Start (Seconds (1.0)); 
  ap_dhcp_server.Stop (Seconds (10.0)); 

  /*Install DHCP Client Protocol*/
  DhcpClientHelper dhcp_client(port);
  ApplicationContainer ap_dhcp_client = dhcp_client.Install(MN.Get(0));
  ap_dhcp_client.Start (Seconds (1.0)); 
  ap_dhcp_client.Stop (Seconds (10.0)); 

  /*Run Simulation*/
  Simulator::Stop (Seconds (15.0)); 
  Simulator::Run ();

  /*Assert the network of client and the pool*/
  Ipv4Address address=ipv4MN->GetAddress(ifIndex, 0).GetLocal();
  NS_TEST_ASSERT_MSG_EQ (Ipv4Address("172.30.0.0").Get(),(address.Get() & Ipv4Mask ("/24").Get()),address);
  
  Simulator::Destroy ();
}
class DhcpTestCase2 : public TestCase
{
public:
  DhcpTestCase2 ();
  virtual ~DhcpTestCase2 ();

private:
  virtual void DoRun (void);
};


DhcpTestCase2::DhcpTestCase2 ()
  : TestCase ("Dhcp test case ")
{
}

DhcpTestCase2::~DhcpTestCase2 ()
{
}


void
DhcpTestCase2::DoRun (void)
{

  /*Set up devices*/
  NodeContainer MN;
  NodeContainer Router;
  MN.Create(2);
  Router.Create(1);
  NodeContainer net(MN, Router);

  /*Set up channel*/
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("5Mbps"));
  csma.SetChannelAttribute ("Delay", StringValue ("2ms"));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  NetDeviceContainer dev_net = csma.Install(net);

  /*Install TCP Stack*/
  InternetStackHelper tcpip;
  tcpip.Install(MN);
  tcpip.Install(Router);

  /*Setup a null address for DHCP Client*/ 
  Ptr<Ipv4> ipv4MN = net.Get(0)->GetObject<Ipv4> (); 
  uint32_t ifIndex1 = ipv4MN->AddInterface (dev_net.Get(0));
  ipv4MN->AddAddress (ifIndex1, Ipv4InterfaceAddress (Ipv4Address ("0.0.0.0"), Ipv4Mask ("/0")));
  ipv4MN->SetForwarding(ifIndex1, true);
  ipv4MN->SetUp (ifIndex1); 

  Ptr<Ipv4> ipv4MN1 = net.Get(1)->GetObject<Ipv4> (); 
  uint32_t ifIndex2 = ipv4MN1->AddInterface (dev_net.Get(1));
  ipv4MN1->AddAddress (ifIndex2, Ipv4InterfaceAddress (Ipv4Address ("0.0.0.0"), Ipv4Mask ("/0")));
  ipv4MN1->SetForwarding(ifIndex2, true);
  ipv4MN1->SetUp (ifIndex2); 
  
  /*Setup the Server*/ 
  Ptr<Ipv4> ipv4Router = net.Get(2)->GetObject<Ipv4> (); 
  uint32_t ifIndex = ipv4Router->AddInterface (dev_net.Get(2));
  ipv4Router->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("10.0.0.1"), Ipv4Mask ("/0"))); 
  ipv4Router->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("10.0.0.1"), Ipv4Mask ("/24")));
  ipv4Router->SetForwarding(ifIndex, true);
  ipv4Router->SetUp (ifIndex);

  /*Install DHCP Server Protocol*/ 
  uint16_t port = 67;
  DhcpServerHelper dhcp_server(Ipv4Address("10.0.0.0"), Ipv4Mask("/24"), Ipv4Address("10.0.0.1"), port);
  ApplicationContainer ap_dhcp_server = dhcp_server.Install(Router.Get(0));
  ap_dhcp_server.Start (Seconds (1.0)); 
  ap_dhcp_server.Stop (Seconds (10.0)); 

  /*Install DHCP Client Protocol*/
  DhcpClientHelper dhcp_client(port);
  ApplicationContainer ap_dhcp_client = dhcp_client.Install(MN.Get(0));
  ap_dhcp_client.Start (Seconds (1.0)); 
  ap_dhcp_client.Stop (Seconds (10.0)); 

  DhcpClientHelper dhcp_client1(port);
  ApplicationContainer ap_dhcp_client1 = dhcp_client1.Install(MN.Get(1));
  ap_dhcp_client1.Start (Seconds (1.0)); 
  ap_dhcp_client1.Stop (Seconds (10.0)); 


  /*Run Simulation*/
  Simulator::Stop (Seconds (15.0)); 
  Simulator::Run ();

  /*Assert the network of client and the pool*/
  Ipv4Address address=ipv4MN->GetAddress(ifIndex1, 0).GetLocal();
  NS_TEST_ASSERT_MSG_EQ (Ipv4Address("10.0.0.0").Get(),(address.Get() & Ipv4Mask ("/24").Get()),address);

  Ipv4Address address1=ipv4MN->GetAddress(ifIndex2, 0).GetLocal();
  NS_TEST_ASSERT_MSG_EQ (Ipv4Address("10.0.0.0").Get(),(address1.Get() & Ipv4Mask ("/24").Get()),address);
  
  Simulator::Destroy ();
}

class DhcpTestSuite : public TestSuite
{
public:
  DhcpTestSuite ();
};

DhcpTestSuite::DhcpTestSuite ()
  : TestSuite ("dhcp", UNIT)
{
  AddTestCase (new DhcpTestCase1, TestCase::QUICK);
  AddTestCase (new DhcpTestCase2, TestCase::QUICK);
}

static DhcpTestSuite dhcpTestSuite;

