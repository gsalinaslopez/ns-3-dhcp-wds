/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 UPB
 *
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
 *
 * Author: Radu Lupu <rlupu@elcom.pub.ro>
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <ns3/ipv4.h>
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include <ns3/ipv4-static-routing-helper.h>
#include "dhcp-client.h"
#include "dhcp-header.h"

namespace ns3 {
  
  NS_LOG_COMPONENT_DEFINE ("DhcpClient");
  NS_OBJECT_ENSURE_REGISTERED (DhcpClient);
  
  TypeId
  DhcpClient::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::DhcpClient")
      .SetParent<Application> ()
      .AddConstructor<DhcpClient> ()
      .AddAttribute (
		     "RemoteAddress",
		     "The destination Ipv4Address of the outbound packets",
		     Ipv4AddressValue (),
		     MakeIpv4AddressAccessor (&DhcpClient::m_peerAddress),
		     MakeIpv4AddressChecker ())
      .AddAttribute ("RemotePort", "The destination port of the outbound packets",
		     UintegerValue (67),
		     MakeUintegerAccessor (&DhcpClient::m_peerPort),
		     MakeUintegerChecker<uint16_t> ());
    return tid;
  }
  
  DhcpClient::DhcpClient() : m_option1(Ipv4Address::GetAny())
  {
    NS_LOG_FUNCTION_NOARGS ();
    m_socket = 0;
    m_ipc_socket = 0;
    m_refreshEvent = EventId ();
    m_rtrsEvent = EventId();
    m_state = IDLEE;
  }
  
  DhcpClient::~DhcpClient()
  {
    NS_LOG_FUNCTION_NOARGS ();
  }
  
  void
  DhcpClient::SetRemote (Ipv4Address ip, uint16_t port)
  {
    m_peerAddress = ip;
    m_peerPort = port;
  }
  
  Ipv4Address DhcpClient::GetOption_1(void){
    return m_option1;
  }
  
  
  void
  DhcpClient::DoDispose (void)
  {
    NS_LOG_FUNCTION_NOARGS ();
    Application::DoDispose ();
  }
  
  void
  DhcpClient::StartApplication (void)
  {
    NS_LOG_FUNCTION_NOARGS ();
    
    if (m_socket == 0)
      {
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	m_socket = Socket::CreateSocket (GetNode (), tid);
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 68);
	m_socket->SetAllowBroadcast (true);
	m_socket->Bind (local);
      }
    m_socket->SetRecvCallback(MakeCallback(&DhcpClient::NetHandler, this));
    
    if (m_ipc_socket == 0)
      {
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	m_ipc_socket = Socket::CreateSocket (GetNode (), tid);
	InetSocketAddress local = InetSocketAddress (Ipv4Address::GetLoopback(), DHCP_IPC_PORT);
	m_ipc_socket->Bind (local);
      }
    m_ipc_socket->SetRecvCallback(MakeCallback(&DhcpClient::IPCHandler, this));
    GetNode()->GetDevice(0)->AddLinkChangeCallback(MakeCallback(&DhcpClient::LinkStateHandler, this));
    
    RunEfsm(); //assumes the link is up at this moment!
  }
  
  void
  DhcpClient::StopApplication ()
  {
    NS_LOG_FUNCTION_NOARGS ();
    Simulator::Cancel (m_refreshEvent);
  }
  
  void DhcpClient::IPCHandler (Ptr<Socket> socket){
    NS_LOG_FUNCTION (this << socket);
    Address from;
    uint8_t data[8]; //at length of the option name i.e. "Option_1" !!!
    
    socket->RecvFrom(data, 8, 0, from);
    if(m_state == IDLEE) m_option1 = Ipv4Address("0.0.0.0");
    socket->SendTo((uint8_t*) &m_option1, 4, 0, from);
  }
  
  void DhcpClient::NetHandler (Ptr<Socket> socket){
    NS_LOG_FUNCTION (this << socket);
    DhcpClient::RunEfsm();
  }
  
  void DhcpClient::LinkStateHandler(void){
    
    if(GetNode()->GetDevice(0)->IsLinkUp()){
      NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  " << "LINK UP!!!! at " << Simulator::Now().GetSeconds());
      m_socket->SetRecvCallback(MakeCallback(&DhcpClient::NetHandler, this));
      RunEfsm();
    }
    else{
      NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "LINK DOWN!!!! at " << Simulator::Now().GetSeconds()); //reinitialization
      Simulator::Remove(m_refreshEvent); //stop refresh timer!!!!
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());  //stop receiving DHCP messages !!!
      m_state = IDLEE;
      
      Ptr<Ipv4> ipv4MN = GetNode()->GetObject<Ipv4> ();
      int32_t ifIndex = ipv4MN->GetInterfaceForDevice(GetNode()->GetDevice(0)); //it is assumed this node has a single net device installed !
      ipv4MN->RemoveAddress(ifIndex, 0); //it is assumed this node has a single IP address settled !
      Ipv4StaticRoutingHelper ipv4RoutingHelper;
      Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4MN);
      staticRouting->RemoveRoute(0); //it is assumed this node has a single route settled at this moment!
      
      ipv4MN->AddAddress (ifIndex, Ipv4InterfaceAddress (Ipv4Address ("0.0.0.0"), Ipv4Mask ("/0")));
    }
    
  }
  
  void DhcpClient::RtrsHandler(void){
    NS_LOG_INFO("DHCP_DISCOVER retransmission event at " << Simulator::Now().GetSeconds());
    
    m_state = IDLEE;
    RunEfsm();
  }
  
  
  
  
  void DhcpClient::RunEfsm(void){
    NS_LOG_FUNCTION_NOARGS ();
    DhcpHeader header;
    Ptr<Packet> packet;
    Address from;
    
    switch(m_state){
    case IDLEE:
      packet = Create<Packet> (DHCP_HEADER_LENGTH);
      header.SetType (DHCPDISCOVER);
      header.SetChaddr(Mac48Address::ConvertFrom(GetNode()->GetDevice(0)->GetAddress()));
      packet->AddHeader (header);
      
      if ((m_socket->SendTo(packet, 0, InetSocketAddress(Ipv4Address("255.255.255.255"), m_peerPort))) >= 0){
        NS_LOG_INFO ("[node " << GetNode()->GetId() << "] Trace TX: DHCP DISCOVER" );
      }else{
        NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Error while sending DHCP DISCOVER to " << m_peerAddress);
      }
      m_state = WAIT_OFFER;
      m_socket->SetRecvCallback(MakeCallback(&DhcpClient::NetHandler, this)); //Start net handler when packet is received
      m_rtrsEvent = Simulator::Schedule (Seconds (RTRS_TIMEOUT), &DhcpClient::RtrsHandler, this); //Set up retransmission event on failure
      break;
      
    case WAIT_OFFER:
      packet = Create<Packet> (DHCP_HEADER_LENGTH);
      packet = m_socket->RecvFrom(from);
      
      if (packet->GetSize () > 0){
        packet->RemoveHeader (header);
        if (header.GetType() == DHCPOFFER){
          
          if(header.GetChaddr() != Mac48Address::ConvertFrom(GetNode()->GetDevice(0)->GetAddress())){
            break; //stop if offer not for me
          }
          Simulator::Remove (m_rtrsEvent);
          m_myAddress = header.GetYiaddr();
          m_option1 = header.GetOption_1();
          
          NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace RX: DHCP OFFER leased addr = " << m_myAddress << " Option_1 value= " << m_option1);
          packet = Create<Packet> (DHCP_HEADER_LENGTH);
          header.SetType (DHCPREQ); //Send DHCP Request
          header.SetOption_1(m_myAddress);
          header.SetChaddr(Mac48Address::ConvertFrom(GetNode()->GetDevice(0)->GetAddress()));
          packet->AddHeader (header);
          m_socket->SendTo(packet, 0, InetSocketAddress(Ipv4Address("255.255.255.255"), m_peerPort));
        }}
      m_state = WAIT_ACK;
      m_rtrsEvent = Simulator::Schedule (Seconds (100), &DhcpClient::RtrsHandler, this);
      break;
    case WAIT_ACK:
        
        
      packet = Create<Packet> (DHCP_HEADER_LENGTH);
      packet = m_socket->RecvFrom(from);
        
      if (packet->GetSize () > 0){
	packet->RemoveHeader (header);
	while(header.GetChaddr() != Mac48Address::ConvertFrom(GetNode()->GetDevice(0)->GetAddress())) //Receive packets till ack received
          { packet = m_socket->RecvFrom(from);packet->RemoveHeader (header);}
	if (header.GetType() == DHCPACK){
            
	  if(header.GetChaddr() != Mac48Address::ConvertFrom(GetNode()->GetDevice(0)->GetAddress())){
	    NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace TX: DHCP ACK NOT MINE");
	    break;
	  }
	  NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace TX: DHCP ACK RECEIVED");
	  Simulator::Remove (m_rtrsEvent);
	  Ptr<Ipv4> ipv4 = GetNode()->GetObject<Ipv4> ();
	  int32_t ifIndex = ipv4->GetInterfaceForDevice(GetNode()->GetDevice(0));
	  ipv4->RemoveAddress(ifIndex, 0);
            
	  ipv4->AddAddress (ifIndex, Ipv4InterfaceAddress ((Ipv4Address)m_myAddress, Ipv4Mask ("/24"))); //BEAWARE: the node IP mask is hardcoded here, In general mask does not affect host !!!!!!!!!!!!!
	  ipv4->SetUp (ifIndex);
            
	  InetSocketAddress remote = InetSocketAddress (InetSocketAddress::ConvertFrom (from).GetIpv4 (), m_peerPort);
	  m_socket->Connect(remote);
            
	  Ipv4StaticRoutingHelper ipv4RoutingHelper;
	  Ptr<Ipv4StaticRouting> staticRouting = ipv4RoutingHelper.GetStaticRouting (ipv4);
	  staticRouting->SetDefaultRoute(InetSocketAddress::ConvertFrom (from).GetIpv4 (), ifIndex, 0);
            
	  m_peerAddress= InetSocketAddress::ConvertFrom (from).GetIpv4 ();
	  NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Current DHCP Server is =" << m_peerAddress);
            
	  ipv4->SetUp (0);
            
	  m_state = REFRESH_LEASE;
	  m_refreshEvent = Simulator::Schedule (Seconds (REFRESH_TIMEOUT), &DhcpClient::RunEfsm, this);
	}
	else if(header.GetType() == DHCPNACK)
          {
            if(header.GetChaddr() != Mac48Address::ConvertFrom(GetNode()->GetDevice(0)->GetAddress())){
              break;
            }
            RtrsHandler();
          }
          
      }
      break;
    case REFRESH_LEASE:
      if(Simulator::IsExpired(m_refreshEvent)){
	uint32_t addr = m_myAddress.Get();
	packet = Create<Packet>((uint8_t*)&addr, sizeof(header));
	header.SetType (DHCPREQ);
	header.SetOption_1(m_myAddress);
	header.SetChaddr(Mac48Address::ConvertFrom(GetNode()->GetDevice(0)->GetAddress()));
	packet->AddHeader (header);
          
	if ((m_socket->SendTo(packet, 0, InetSocketAddress(m_peerAddress, m_peerPort))) >= 0){
	  NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Trace TX: DHCP REQUEST");
	}else{
	  NS_LOG_INFO ("[node " << GetNode()->GetId() << "]  "<< "Error while sending DHCP REQ to " << m_peerAddress);
	}
	m_state= WAIT_ACK;
          
      }
      break;
    }
  }
    
    
} // Namespace ns3
