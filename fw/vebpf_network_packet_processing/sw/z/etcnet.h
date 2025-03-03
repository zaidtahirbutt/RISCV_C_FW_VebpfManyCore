////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	etcnet.h
//
// Project:	ZipVersa, Versa Brd implementation using ZipCPU infrastructure
//
// Purpose:	Most Linux systems maintain a variety of configuration files
//		in their /etc directory telling them how the network is
//	configured.  Since we don't have access to a filesystem (yet), we'll
//	maintain our network configuration here in this C header file.  Please
//	adjust this according to your own network configuration.  Other network
//	enabled programs in this directory should pick up any changes to this
//	file and adjust themselves appropriately.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2019, Gisselquist Technology, LLC
//
// This program is free software (firmware): you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  (It's in the $(ROOT)/doc directory.  Run make with no
// target there if the PDF file isn't present.)  If not, see
// <http://www.gnu.org/licenses/> for a copy.
//
// License:	GPL, v3, as defined and found on www.gnu.org,
//		http://www.gnu.org/licenses/gpl.html
//
//
////////////////////////////////////////////////////////////////////////////////
//
//
#ifndef	ETCNET_H
#define	ETCNET_H

#include "ethproto.h"

#define	IPADDR(A,B,C,D)	((((A)&0x0ff)<<24)|(((B)&0x0ff)<<16)	\
			|(((C)&0x0ff)<<8)|(D&0x0ff))

// Adjust these three lines as necessary to change from one network to another
//
//
// First, the default MAC --- this is the MAC of the Versa board.  It was
// generated using /dev/rand, and should probably be changed to something
// else on your configuration.
// #define	DEFAULTMAC	  0xa25345b6fb5eul
#define	DEFAULTMAC	  0x112233445566ul 
	// same as src mac for now

// #define	DST_MAC	  0xaabbccddeefful  // great to recognize in simulation
				 //08:00:27:58:9D:6A

#define	DST_MAC	  0x080027589D6Aul
	// making destination mac equal to dest mac of my laptops NIC = 08:00:27:58:9D:6A
// #define	DST_MAC	  0xfffffffffffful
	// using broadcast mac as DESTMAC

// #define	SRC_MAC	  0x112233445566ul
// #define	SRC_MAC	  0x112233445566ul // having 1 at lsb of first byte is maybe a multicast address and maybe used for hacking cx wireshark was giving error
#define	SRC_MAC	  0x020000000000ul
	// I'll add this src mac address in arp address table with src IP

#define BROADCAST_MAC 0xfffffffffffful

// Now, for the IP setup defaults
//
// These include the default IP of the Arty, 192.168.15.22.  This comes from
// the fact that this is the network number of my local network (a network with
// no other purpose than to test my Arty), that my local network is not run by
// DHCP (or if it were, that this address is reserved to be a static IP).
//#define	DEFAULTIP	IPADDR(192,168,15,22)
// #define	DEFAULTIP	IPADDR(192,168,0,99)
#define	DEFAULTIP	IPADDR(192,168,1,128)
// #define SRC_UDP_PORT 4049
// #define SRC_UDP_PORT 1234
#define SRC_UDP_PORT 1024
#define	DESTIP	IPADDR(192,168,1,100)
// #define DEST_UDP_PORT 2049
#define DEST_UDP_PORT 1234
// #define DEST_UDP_PORT 1235
// #define DEST_UDP_PORT 1024

//
// The next issue is how to deal with packets that are not on the local network.
// The first step is recognizing them, and that's the purpose of the netmask.
// You might also see this netmask as represented by IPADDR(255,255,255,0),
// or even 255.255.255.0.  I've just converted it to unsignd here.
#define	LCLNETMASK	0xffffff00
// So, if an IP address doesn't match as coming from my local network, then it
// needs to be sent to the router first.  While the router IP address isn't
// used for that purpose, it is used in the ARP query to find the MAC address
// of the router.  As a result, we need it and define our default here.

// #define	DEFAULT_ROUTERIP	IPADDR(192,168,15,1)
// #define	DEFAULT_ROUTERIP	IPADDR(192,168,0,1)  //  ip of my house router xD
#define	DEFAULT_ROUTERIP	IPADDR(192,168,1,1) // if I set the laptop's Gateway, this is the value

// type1 firewall rules
#define	EBPF_RULE1_SRC_IP		IPADDR(255,255,255,255)
#define	EBPF_RULE2_SRC_IP		IPADDR(127,0,0,0)
#define	EBPF_RULE3_SRC_IP		IPADDR(240,0,0,0)
#define	EBPF_RULE4_SRC_IP		IPADDR(0,0,0,0)

// the above are used for simulation, these below are used for synthesis
#define	EBPF_RULE1_SRC_IP_SYN		IPADDR(245,255,255,255)
#define	EBPF_RULE2_SRC_IP_SYN		IPADDR(128,0,0,0)
#define	EBPF_RULE3_SRC_IP_SYN		IPADDR(240,0,0,0)
#define	EBPF_RULE4_SRC_IP_SYN		IPADDR(1,0,0,0)

// type2 firewall rules
#define	EBPF_RULE5_DEST_UDP_PORT		111
#define	EBPF_RULE6_DEST_UDP_PORT		2000
#define	EBPF_RULE7_DEST_UDP_PORT		37
#define	EBPF_RULE8_DEST_UDP_PORT		135
#define	EBPF_RULE9_DEST_UDP_PORT		137
#define	EBPF_RULE10_DEST_UDP_PORT		138
#define	EBPF_RULE11_DEST_UDP_PORT		161
#define	EBPF_RULE12_DEST_UDP_PORT		162
#define	EBPF_RULE13_DEST_UDP_PORT		514

// type3 firewall rules
#define	EBPF_RULE14_DEST_UDP_PORT		69
#define	EBPF_RULE15_DEST_UDP_PORT		2049
#define	EBPF_RULE16_DEST_UDP_PORT		389
#define	EBPF_RULE17_DEST_UDP_PORT		4045

#define	DEFAULT_DISL_ARTY_IP	IPADDR(192,168,1,128)
#define	DEFAULT_DISL_HOST_IP	IPADDR(192,168,1,100)

// #define	DEFAULT_ROUTERIP	IPADDR(192,168,0,161)  // ip of my laptop

// #define	DEFAULT_ROUTERIP	IPADDR(192,168,0,1)  // ip of my house router xD


// All of these constants will need to be copied into a series of global
// variables, whose names are given below.  They will then be represented by
// these (following) names within the code.  Note that this also includes the
// MAC address of the router, which will need to be filled in by the ARP
// resolution routine(s).
extern	ETHERNET_MAC	my_mac_addr, router_mac_addr;  // typedef	uint64_t ETHERNET_MAC;
extern	uint32_t	my_ip_addr, my_ip_router;
extern	uint32_t	dest_ip;
extern	uint32_t	my_ip_mask;

#endif
