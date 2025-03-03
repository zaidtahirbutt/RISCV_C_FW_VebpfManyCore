////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	icmp.c
//
// Project:	ZipVersa, Versa Brd implementation using ZipCPU infrastructure
//
// Purpose:	To exercise the network port by ...
//
//	1. Pinging another system, at 1PPS
//	2. Replying to ARP requests
//	3. Replying to external 'pings' requests
//
//	To configure this for your network, you will need to adjust the
//	following constants within this file:
//
//	my_ip_addr
//		This is the (fixed) IP address of your Arty board.  The first
//		octet of the IP address is kept in the high order word.
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
// #include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "board.h"
#include "pkt.h"
#include "etcnet.h"
#include "protoconst.h"
#include "ipcksum.h"
#include "arp.h"
#include "ipproto.h"
#include "icmp.h"
// #include "../../../../riscv_subsystem/sw/utils.h"
extern void printf(char *c, ...);  // including ../../../../riscv_subsystem/sw/utils.h in two files
								  // was giving compilation error. This extern method works. 

// #define	CLKFREQHZ	50000000


// My network ID.  The 192.168.15 part comes from the fact that this is a
// local network.  The .22 (last octet) is due to the fact that this is
// an unused ID on my network.

unsigned	icmppkt_id = 0;

void	icmp_reply(unsigned ipaddr, NET_PACKET *icmp_request) {
	ETHERNET_MAC	hwaddr;
	int		maxsz = 2048;

	maxsz = 1<<((_net1->n_rxcmd>>24)&0x0f);  // get rx pkt length
	if (maxsz > 2048)
		maxsz = 2048;

	int pktln = icmp_request->p_length;
	if (pktln < 8)
		pktln = 8;

	if (icmp_request->p_user[0] != ICMP_PING) {
		printf("ICMP: Message that\'s not a ping--not replying\n");
		return;
	} else if (pktln >= 1024) {
		printf("ICMP: Refusing to reply to a packet that\'s too large\n");
		return;
	}

	NET_PACKET	*pkt;
	unsigned	cksum;

	pkt = new_icmp(pktln);
	memcpy(pkt->p_user, icmp_request->p_user, pktln);
	pkt->p_user[0] = ICMP_ECHOREPLY;
	pkt->p_user[1] = 0;
	pkt->p_user[2] = 0;
	pkt->p_user[3] = 0;
		
	// Now, let's go fill in the IP and ICMP checksums
	cksum = ipcksum(pkt->p_length, pkt->p_user);
	pkt->p_user[2] = (cksum >> 8) & 0x0ff;
	pkt->p_user[3] = (cksum     ) & 0x0ff;

	tx_ippkt(pkt, IPPROTO_ICMP, my_ip_addr, ipaddr);
}

NET_PACKET *new_icmp(unsigned ln) {
	NET_PACKET	*pkt = new_ippkt(ln);
	return	pkt;
}

void	icmp_send_ping(unsigned ping_ip_addr) {  // unsigned means unsigned int from what I remember and its size is 4 bytes
	NET_PACKET *pkt;
	unsigned	cksum;

	// Form a packet to transmit
	pkt = new_icmp(8);// i think 8 is length of packet	.. // 8 bytes https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol#header_rest	
	//The ICMP header starts after the IPv4 header and is identified by IP protocol number '1'.

	// here full minimum size is give for ICMP, IP, ETH
	// new_icmp -> new_ippkt(ln)-> new_ethpkt(ln=8+20) -> new_pkt(28+8)
	// so the packet is made bottom up. Layer 3 - 2
		// or rather top down Layer 3 - 2  NOOOOOOOOOO
			// no its top down since the p_user is filled with ICMP then p_user is decreased to put ARP/IP data then decreased to put ethernet data 

	// pkt len is 36 and pkt_p_usr = pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET); I>E

	// at the last func new_pkt() we have this command // pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET); // pkt->p_user= pkt->p_raw; 

	// here 

	//
	// Ping payload: type = 0x08 (PING, the response will be zero)
	//	CODE = 0
	//	Checksum will be filled in later
	
	//#define	ICMP_ECHO	8
	//#define	ICMP_PING	ICMP_ECHO protoconst.h

	// now p_user is at pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET); value!

	

	// here we start here with inserting ICMP info
	// pkt->p_user= pkt->p_raw;  // giving the address of where p_user would begin // giving the pointer its address value!!!!!
	//*p_user is a char* pointer

	pkt->p_user[0] = ICMP_PING;	// 8 // pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET); // pkt->p_user= pkt->p_raw; 
	// so now the p_user pointer which has been casted as a char* ptr by p_raw, is being dereferened here!
	// so successive elements of p_user[0] and p_user[1] have address difference of 1 byte :3

	// That is the reason by pkt was allocated memory of sizeof(ENETPACKET) + msglen
	// because msglen are the added bytes to which we now are adding through p_user

	// *p_user is actually a pointer so if I use pkt->p_user, it referes to that pointer!!!!!!!!!!!!!!! Address I think!
	// this idea is helpful in the func below tx_ippkt


	pkt->p_user[1] = 0x00;  //Control messages 0 – Echo Reply[7]: 14 	0		Echo reply (used to ping)
	pkt->p_user[2] = 0x00;
	pkt->p_user[3] = 0x00;
	//
	icmppkt_id += BIG_PRIME + (BIG_PRIME << 2);		// #define	BIG_PRIME	0x0134513b
	pkt->p_user[4] = (icmppkt_id >> 24)&0x0ff;
	pkt->p_user[5] = (icmppkt_id >> 16)&0x0ff;
	pkt->p_user[6] = (icmppkt_id >>  8)&0x0ff;
	pkt->p_user[7] = (icmppkt_id >>  0)&0x0ff;

	// Calculate the PING payload checksum
	// pkt->p_user[2] = 0;
	// pkt->p_user[3] = 0;
	cksum = ipcksum(8, pkt->p_user);
	pkt->p_user[2] = (cksum >> 8) & 0x0ff;  // seen in sim
	pkt->p_user[3] = (cksum     ) & 0x0ff;

	// Finally, send the packet -- 9*4 = our total number of octets
	pkt->p_length = 8;		// reset the p_length variable here to 8 since we inserted 8 bytes of ICMP message
	tx_ippkt(pkt, IPPROTO_ICMP, my_ip_addr, ping_ip_addr);	//#define	IPPROTO_ICMP	1 in protoconst.h
	// my_ip_addr and ping_ip_addr are empty uint32_t variables here from etcnet.h
		// uint32_t	my_ip_addr = DEFAULTIP, defined in ipproto.c
		// ping_ip_addr is coming from main file

	// so the ICMP is sent !!

	// in the function tx_pkt function in here n_txcmd is being set! After getting data from eth module in FPGA 
}

void	dump_icmp(NET_PACKET *pkt) {
	unsigned 	cksum, pktsum;

	printf("ICMP TYPE : 0x%02x", pkt->p_user[0] & 0x0ff);
	if (pkt->p_user[0] == ICMP_PING)
		printf(" Ping request\n");
	else if (pkt->p_user[0] == ICMP_ECHOREPLY)
		printf(" Ping reply\n");
	else
		printf(" (Unknown ICMP)\n");

	printf("ICMP CODE : 0x%02x", pkt->p_user[1] & 0x0ff);
	if ((pkt->p_user[1] != 0)
		&&((pkt->p_user[0] == ICMP_PING)
			||((pkt->p_user[0] == ICMP_ECHOREPLY)))) {
		printf(" -- Unexpected code\n");
	} else
		printf("\n");


	pktsum = ((pkt->p_user[2] & 0x0ff) << 8)
			| (pkt->p_user[3] & 0x0ff);
	pkt->p_user[2] = 0;
	pkt->p_user[3] = 0;
	cksum = ipcksum(pkt->p_length, pkt->p_user);
	pkt->p_user[2] = (pktsum >> 8) & 0x0ff;
	pkt->p_user[3] = (pktsum     ) & 0x0ff;
	printf("ICMP CKSUM: 0x%02x", pktsum);
	if (cksum != pktsum)
		printf(" -- NO-MATCH against %04x\n", cksum);
	else
		printf("\n");

	printf("ICMP DATA :\n");
	for(unsigned k=4; k<pkt->p_length; k++) {
		printf("%02x ", pkt->p_user[k]);
		if ((k & 0x03)==3)
			printf("\n");
	} if (((pkt->p_length -1)&0x03)!=3)
		printf("\n");
}

