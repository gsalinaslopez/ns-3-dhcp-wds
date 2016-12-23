/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Network topology:
//      (client)      (server and gw)
//        MN ========== Router 
//

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"



using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("DhcpServiceTest");
 

  
  
/*void ConfigureUdpServer(NodeContainer MN, NodeContainer Router)
{
  Ptr<DhcpClient> dhcpClient = MN.Get(0)->GetApplication(0)->GetObject<DhcpClient>();
  Ipv4Address serveraddr = dhcpClient->GetOption_1();
  NS_LOG_INFO ("DHCP server is at=" << serveraddr);
  Ptr<UdpClient> udpClient = MN.Get(0)->GetApplication(1)->GetObject<UdpClient>();
  udpClient->SetRemote(serveraddr, 4000);
}
  */
int
main (int argc, char *argv[])
{
//
// Enable logging 
//
//  LogComponentEnable ("DhcpServiceTest", LOG_LEVEL_INFO);


 CommandLine cmd;
   cmd.Parse (argc, argv);
  LogComponentEnable ("DhcpClient", LOG_LEVEL_INFO);
  LogComponentEnable ("DhcpServer", LOG_LEVEL_INFO);
   LogComponentEnable ("DhcpServiceTest", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpServer", LOG_LEVEL_INFO);

  NodeContainer MN;
NodeContainer Router;
//
// Explicitly create the nodes required by the topology (depicted above).
//
  NS_LOG_INFO ("Create nodes.");

  MN.Create(1);
  Router.Create(1);
 
  NodeContainer net(MN, Router);
    
  NS_LOG_INFO ("Create channels.");
//
// Explicitly create the channels required by the topology (shown above).
//
  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", DataRateValue (DataRate (5000000)));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (50)));
  csma.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  NetDeviceContainer dev_net = csma.Install(net);

//
// We've got the "hardware" in place.  Now we need to add IP addresses.
//
  InternetStackHelper tcpip;
  tcpip.Install(MN);
  tcpip.Install(Router);


  //MN configuration: i/f create + setup
  Ptr<Ipv4> ipv4MN = net.Get(0)->GetObject<Ipv4> (); 
  uint32_t ifIndex1 = ipv4MN->AddInterface (dev_net.Get(0));
  ipv4MN->AddAddress (ifIndex1, Ipv4InterfaceAddress (Ipv4Address ("0.0.0.0"), Ipv4Mask ("/0")));
  ipv4MN->SetForwarding(ifIndex1, true);
  ipv4MN->SetUp (ifIndex1); 
  
  //Router configuration: i/f create + setup
  Ptr<Ipv4> ipv4Router = net.Get(1)->GetObject<Ipv4> (); 
  uint32_t ifIndex = ipv4Router->AddInterface (dev_net.Get(1));
  ipv4Router->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("172.30.0.1"), Ipv4Mask ("/0"))); //workaround (to support undirected broadcast in ns-3.12.1)!!!!!
  ipv4Router->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("172.30.0.1"), Ipv4Mask ("/24")));
  ipv4Router->SetForwarding(ifIndex, true);
  ipv4Router->SetUp (ifIndex); 
  uint16_t port = 67;
  DhcpServerHelper dhcp_server(Ipv4Address("172.30.0.0"), Ipv4Mask("/24"), Ipv4Address("172.30.0.1"), port);
  ApplicationContainer ap_dhcp_server = dhcp_server.Install(Router.Get(0));
  ap_dhcp_server.Start (Seconds (1.0)); 
  ap_dhcp_server.Stop (Seconds (10.0)); 

  DhcpClientHelper dhcp_client(port);
  ApplicationContainer ap_dhcp_client = dhcp_client.Install(MN.Get(0));
  ap_dhcp_client.Start (Seconds (1.0)); 
  ap_dhcp_client.Stop (Seconds (10.0)); 

 
  Simulator::Stop (Seconds (15.0)); 
  Ipv4Address address=ipv4MN->GetAddress(ifIndex, 0).GetLocal();
    NS_LOG_INFO (address);
//
// Now, do the actual simulation.
//
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Run ();

  Simulator::Destroy ();
 
  NS_LOG_INFO ("Done.");
}
