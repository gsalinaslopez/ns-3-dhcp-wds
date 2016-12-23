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
*/

#include "ns3/assert.h"
#include "ns3/log.h"
#include "ns3/header.h"
#include "ns3/simulator.h"
#include "dhcp-header.h"
#include "ns3/address-utils.h"


namespace ns3 {
  
  NS_LOG_COMPONENT_DEFINE ("DhcpHeader");
  NS_OBJECT_ENSURE_REGISTERED (DhcpHeader);
  
  
  DhcpHeader::DhcpHeader ()
  //  : m_type (0)
  //    m_ts (Simulator::Now ().GetTimeStep ())
  {
  }
  
  void DhcpHeader::SetType (uint8_t type)
  {
    m_op = type;
  }
  uint8_t DhcpHeader::GetType (void) const
  {
    return m_op;
  }
  
  void DhcpHeader::SetChaddr (Mac48Address addr)
  {
    m_chaddr = addr;
  }
  Mac48Address DhcpHeader::GetChaddr (void)
  {
    return m_chaddr;
  }
  
  void DhcpHeader::SetYiaddr (Ipv4Address addr)
  {
    m_yiaddr = addr;
  }
  Ipv4Address DhcpHeader::GetYiaddr (void) const
  {
    return m_yiaddr;
  }
  
  void DhcpHeader::SetOption_1 (Ipv4Address addr)
  {
    m_option_1 = addr;
  }
  Ipv4Address DhcpHeader::GetOption_1 (void) const
  {
    return m_option_1;
  }
  
  TypeId DhcpHeader::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::DhcpHeader")
    .SetParent<Header> ()
    .AddConstructor<DhcpHeader> ()
    ;
    return tid;
  }
  TypeId DhcpHeader::GetInstanceTypeId (void) const
  {
    return GetTypeId ();
  }
  void DhcpHeader::Print (std::ostream &os) const
  {
    os << "(type=" << m_op << ")";
  }
  uint32_t DhcpHeader::GetSerializedSize (void) const
  {
    return DHCP_HEADER_LENGTH;
  }
  
  void
  DhcpHeader::Serialize (Buffer::Iterator start) const
  {
    Buffer::Iterator i = start;
    i.WriteU8 (m_op);
    WriteTo (i, m_chaddr);
    WriteTo (i, m_yiaddr);
    WriteTo (i, m_option_1);
  }
  uint32_t DhcpHeader::Deserialize (Buffer::Iterator start)
  {
    Buffer::Iterator i = start;
    m_op = i.ReadU8 ();
    ReadFrom (i, m_chaddr);
    ReadFrom(i,m_yiaddr);
    ReadFrom(i,m_option_1);
    return GetSerializedSize ();
  }
  
} // namespace ns3
