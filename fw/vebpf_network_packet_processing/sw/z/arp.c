////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	arp.c
//
// Project:	ZipVersa, Versa Brd implementation using ZipCPU infrastructure
//
// Purpose:	To encapsulate common functions associated with the ARP protocol
//		and hardware (ethernet MAC) address resolution.
//
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
#include "arp.h"
#include "board.h"
#include "etcnet.h"
#include "protoconst.h"
#include "ethproto.h"
//#include "../../../../riscv_subsystem/sw/utils.h" 
extern void printf(char *c, ...);  // including ../../../../riscv_subsystem/sw/utils.h in two files
								  // was giving compilation error. This extern method works.

#define	ARP_REQUEST	1
#define	ARP_REPLY	2

///////////
//
//
// Simplified ARP table and ARP requester
//
//
///////////
unsigned	arp_requests_sent = 0;
uint32_t	my_ip_router = DEFAULT_ROUTERIP;


// vip. assigning a value to router mac addr so our dev doesnt have to send an arp req
	// unassigning the router mac address for the arp req to be sent
ETHERNET_MAC	router_mac_addr;
// ETHERNET_MAC	router_mac_addr = BROADCAST_MAC;
// ETHERNET_MAC	router_mac_addr = DEFAULTMAC;

typedef	struct	{
	int		valid;
	unsigned	age, ipaddr;
	ETHERNET_MAC	mac;
} ARP_TABLE_ENTRY;

#define	NUM_ARP_ENTRIES	8
ARP_TABLE_ENTRY	arp_table[NUM_ARP_ENTRIES];

//
// Keep track of a log of all of our work for debugging purposes
typedef struct	{
	unsigned ipaddr;
	ETHERNET_MAC	mac;
} ARP_TABLE_LOG_ENTRY;

int	arp_logid = 0;
ARP_TABLE_LOG_ENTRY	arp_table_log[32];


static const char arp_packet[] = {
		// 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
		// 0x00, 0x00,
		0x00, 0x01, 0x08, 0x00,	// Ethernet, ARP EtherType
		0x06, 0x04, 0x00, 0x01,	// Addr length(s x2), Operation (Req)
		0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0, 0, 0, 0, 0, 
		0, 0, 0, 0 };
//https://en.wikipedia.org/wiki/Address_Resolution_Protocol
		//ARP packet is according to this

void	init_arp_table(void) {
	for(int k=0; k<NUM_ARP_ENTRIES; k++)
		arp_table[k].valid = 0;
}

int	get_next_arp_index(void) {
	int	eid, eldest = 0, unused_id = -1, oldage = 0, found=-1;
	for(eid=0; eid<NUM_ARP_ENTRIES; eid++) {
		if (!arp_table[eid].valid) {
			unused_id = eid;
			break;
		} else if (arp_table[eid].age > oldage) {
			oldage = arp_table[eid].age;
			eldest = eid;
		}
	}

	if (unused_id >= 0)
		return unused_id;
	return eldest;
}

NET_PACKET *new_arp(void) {
	return	new_ethpkt(28);
}

void	send_arp_request(int ipaddr) {  // ipaddr is dest ipaddr
	NET_PACKET *pkt;
	pkt = new_arp(); // msglen is 36 for it just like the icmp IP packet
	// ARP is not included in IP header, it is its own protocol

	memcpy(pkt->p_user, arp_packet, sizeof(arp_packet));  //arp packet len is 28 bytes

	// first 7 bytes of p->p_user are written with arp_packet data when we do this memcpy above

	pkt->p_user[ 7] = ARP_REQUEST;  //#define	ARP_REQUEST	1 ,,, arp reply is 2
	//pkt->p_user[ 7] is pointing to the 8th byte in the arp_packet which is 0x01
	// Operation (OPER) accd to wikipeida and it Specifies the operation that the sender is performing: 1 for request, 2 for reply.
		// I was mixing address pointing with bitwise access of arrays FP.

	// Initial ETHERTYPE_ARP is in the arp_packet array
	// pkt->p_buf[6] = (ETHERTYPE_ARP >> 8)&0x0ff;
	// pkt->p_buf[7] = (ETHERTYPE_ARP & 0x0ff);

	// I guess the dest mac address has been stripped off for the network pkt 
	// My mac address
	// p_user is a char* ptr anyway so this byte dereferencing using array notation is possible		// form wikipedia
	pkt->p_user[ 8] = (my_mac_addr >> 40) & 0x0ff;  // Sender hardware address (SHA) (first 2 bytes as per wiki
	pkt->p_user[ 9] = (my_mac_addr >> 32) & 0x0ff;	// Sender hardware address (SHA) (first 2 bytes
	pkt->p_user[10] = (my_mac_addr >> 24) & 0x0ff;	// (next 2 bytes)
	pkt->p_user[11] = (my_mac_addr >> 16) & 0x0ff;	// (next 2 bytes)
	pkt->p_user[12] = (my_mac_addr >>  8) & 0x0ff;	// (last 2 bytes)
	pkt->p_user[13] = (my_mac_addr      ) & 0x0ff;	// (last 2 bytes)

	// My IP address								// form wikipedia
	pkt->p_user[14] = (my_ip_addr >> 24) & 0x0ff;	// Sender protocol address (SPA) (first 2 bytes)
	pkt->p_user[15] = (my_ip_addr >> 16) & 0x0ff;	// Sender protocol address (SPA) (first 2 bytes)
	pkt->p_user[16] = (my_ip_addr >>  8) & 0x0ff;	// (last 2 bytes)
	pkt->p_user[17] = (my_ip_addr      ) & 0x0ff;	// (last 2 bytes)

	// Target addresses are initially all set to zero: OK :3
	pkt->p_user[18] = 0;
	pkt->p_user[19] = 0;
	pkt->p_user[20] = 0;
	pkt->p_user[21] = 0;
	pkt->p_user[22] = 0;
	pkt->p_user[23] = 0;
	pkt->p_user[24] = (ipaddr >> 24)&0x0ff;	// Target protocol address (TPA) (first 2 bytes)
	pkt->p_user[25] = (ipaddr >> 16)&0x0ff; // Target protocol address (TPA) (first 2 bytes)
	pkt->p_user[26] = (ipaddr >>  8)&0x0ff; // (last 2 bytes)
	pkt->p_user[27] = (ipaddr      )&0x0ff; // (last 2 bytes)

	arp_requests_sent++;
	ETHERNET_MAC broadcast = 0x0fffffffffffful;
	
	// printf("tx_ethpkt: Requesting ARP of %3d.%3d.%3d.%3d\n",
	// 	pkt->p_user[24]&0x0ff,
	// 	pkt->p_user[25]&0x0ff,
	// 	pkt->p_user[26]&0x0ff,
	// 	pkt->p_user[27]&0x0ff);

	printf("tx_ethpkt: Requesting ARP of %d.%d.%d.%d\n",
		pkt->p_user[24]&0x0ff,
		pkt->p_user[25]&0x0ff,
		pkt->p_user[26]&0x0ff,
		pkt->p_user[27]&0x0ff);

	
	
	tx_ethpkt(pkt, ETHERTYPE_ARP, broadcast);  // #define	ETHERTYPE_ARP		0x0806
}
/*// The next issue is how to deal with packets that are not on the local network.
// The first step is recognizing them, and that's the purpose of the netmask.
// You might also see this netmask as represented by IPADDR(255,255,255,0),
// or even 255.255.255.0.  I've just converted it to unsignd here.
#define	LCLNETMASK	0xffffff00*/

//// ipaddr dest IP addr is ping_ip_addr is //	unsigned	host_ip = DEFAULT_ROUTERIP;  // IPADDR(192,168,15,1)  // in etcnet.h #define
int	arp_lookup(unsigned ipaddr, ETHERNET_MAC *mac) {
	int	eid, eldest = 0, unused_id = -1, oldage = 0, found=-1;
/*A bitwise XOR "^" is a binary operation that takes two bit patterns of equal length and performs 
the logical exclusive OR operation on each pair of corresponding bits. The result in each position is 1 if 
only one of the bits is 1, but will be 0 if both are 0 or both are 1.*/
	//// my_ip_addr is #define	DEFAULTIP	IPADDR(192,168,15,22)
	// (x) i think converts x into its logical true or false (false if x == 0)
		//(IPADDR(192,168,15,1) ^ IPADDR(192,168,15,22)) = 
	if (((((ipaddr ^ my_ip_addr) & my_ip_mask) != 0)  // is ((ipaddr ^ my_ip_addr) & my_ip_mask)  =0 for the first iteration (explained this is a c file in saved files as arp_XORing.c)
		|| (ipaddr == my_ip_router))  // ipaddr == my_ip_router is True my_ip_router = DEFAULT_ROUTERIP;
			&&(router_mac_addr)) { //router_mac_addr is not assigned for the first loop so its zero i think
			// this if condition checks if we have the router mac AND if the requested ipaddr is either router ip or !dev_src_ip

		*mac = router_mac_addr;
		return 0;
	}  
	// so first time this above isnt entered and arp req is sent and then the processor waits for the arp reply
	// after getting arp reply the processor updates the arp table in which router mac addr is given to it

	for(eid=0; eid<NUM_ARP_ENTRIES; eid++) {
		if (arp_table[eid].valid) {
			if (arp_table[eid].ipaddr == ipaddr) {
				arp_table[eid].age = 0;
				*mac = arp_table[eid].mac;
				return 0;
			} else if (arp_table[eid].age > oldage) {
				oldage = arp_table[eid].age++;
				eldest = eid;
				if (oldage >= 0x010000)
					arp_table[eid].valid = 0;
			} else
				arp_table[eid].age++;
		}
	}

	// printf("ARP lookup for %3d.%3d.%3d.%3d failed, sending ARP request\n",
	// 	(ipaddr >> 24)&0x0ff,
	// 	(ipaddr >> 16)&0x0ff,
	// 	(ipaddr >>  8)&0x0ff,
	// 	(ipaddr      )&0x0ff);

	printf("ARP lookup for %d.%d.%d.%d failed, sending ARP request\n",
		(ipaddr >> 24)&0x0ff,
		(ipaddr >> 16)&0x0ff,
		(ipaddr >>  8)&0x0ff,
		(ipaddr      )&0x0ff);

	

	//		○ https://www.youtube.com/watch?v=tXzKjtMHgWI&ab_channel=CertBros
	//			ARP to router example at the end
	send_arp_request(ipaddr); // ipaddr is dest ip addr // didnt see this
	return 1;// comes here after first iteration i belv
}

void	arp_table_add(unsigned ipaddr, ETHERNET_MAC mac) {
	ETHERNET_MAC	lclmac;
	int		eid;

	arp_table_log[arp_logid].ipaddr = ipaddr;
	arp_table_log[arp_logid].mac = mac;
	arp_logid++;
	arp_logid&= 31;  // only keeping 5 bits. 32 IP addresses only then I guess
	

	if (ipaddr == my_ip_addr)
		return;
	// Missing the 'if'??
	else if (ipaddr == my_ip_router) {
		router_mac_addr = mac;  // now router has added our device to its memory
	} else if (arp_lookup(ipaddr, &lclmac)==0) {
		if (mac != lclmac) {
			for(eid=0; eid<NUM_ARP_ENTRIES; eid++) {
				if ((arp_table[eid].valid)&&
					(arp_table[eid].ipaddr == ipaddr)) {
					volatile int *ev = &arp_table[eid].valid;
					// Prevent anyone from using an invalid
					// entry while we are updating it
					*ev = 0;
					arp_table[eid].age = 0;
					arp_table[eid].mac = mac;
					*ev = 1;
					break;
				}
			}
		}
	} else {
		volatile int *ev = &arp_table[eid].valid;
		eid = get_next_arp_index();

		// Prevent anyone from using an invalid entry while we are
		// updating it
		*ev = 0;
		arp_table[eid].age = 0;
		arp_table[eid].ipaddr = ipaddr;
		arp_table[eid].mac = mac;
		*ev = 1;
	}
}

void	send_arp_reply(ETHERNET_MAC dest_mac_addr, unsigned dest_ip_addr) {
	NET_PACKET *pkt;
	pkt = new_arp();

	memcpy(pkt->p_user, arp_packet, sizeof(arp_packet));

	// first 7 bytes of p->p_user are written with arp_packet data when we do this memcpy above

	pkt->p_user[ 7] = ARP_REPLY;

	// My mac address
	pkt->p_user[ 8] = (my_mac_addr >> 40) & 0x0ff;
	pkt->p_user[ 9] = (my_mac_addr >> 32) & 0x0ff;
	pkt->p_user[10] = (my_mac_addr >> 24) & 0x0ff;
	pkt->p_user[11] = (my_mac_addr >> 16) & 0x0ff;
	pkt->p_user[12] = (my_mac_addr >>  8) & 0x0ff;
	pkt->p_user[13] = (my_mac_addr      ) & 0x0ff;

	// My IP address
	pkt->p_user[14] = (my_ip_addr >> 24) & 0x0ff;
	pkt->p_user[15] = (my_ip_addr >> 16) & 0x0ff;
	pkt->p_user[16] = (my_ip_addr >>  8) & 0x0ff;
	pkt->p_user[17] = (my_ip_addr      ) & 0x0ff;

	// Target addresses are initially all set to zero
	pkt->p_user[18] = (dest_mac_addr >> 40) & 0x0ff;
	pkt->p_user[19] = (dest_mac_addr >> 32) & 0x0ff;
	pkt->p_user[20] = (dest_mac_addr >> 24) & 0x0ff;
	pkt->p_user[21] = (dest_mac_addr >> 16) & 0x0ff;
	pkt->p_user[22] = (dest_mac_addr >>  8) & 0x0ff;
	pkt->p_user[23] = (dest_mac_addr      ) & 0x0ff;
	pkt->p_user[24] = (dest_ip_addr >> 24)&0x0ff;
	pkt->p_user[25] = (dest_ip_addr >> 16)&0x0ff;
	pkt->p_user[26] = (dest_ip_addr >>  8)&0x0ff;
	pkt->p_user[27] = (dest_ip_addr      )&0x0ff;

	tx_ethpkt(pkt, ETHERTYPE_ARP, dest_mac_addr);
}

void	rx_arp(NET_PACKET *pkt) {
	for(unsigned k=0; k<6; k++) {
		// Ignore any malformed packets
		if (pkt->p_user[k] != arp_packet[k]) {
			printf("Mismatch, (p_user[%d] = %02x) != (arp_packet[%d] = %02x)\n",
				k, pkt->p_user[k]&0x0ff, k,arp_packet[k]&0x0ff);
			printf("p_user = p_raw + %d\n",
				pkt->p_user - pkt->p_raw);
			printf("Ignoring ARP packet -- dump follows\n");
			dump_raw(pkt);
			pkt_reset(pkt);
			dump_ethpkt(pkt);
			free_pkt(pkt);
			/*

			for(;;) ;
			*/
			return;
		}
	}

	uint64_t	src_mac;
	unsigned	src_ip;

	src_mac = (pkt->p_user[ 8] & 0x0ff);
	src_mac = (pkt->p_user[ 9] & 0x0ff) | (src_mac << 8);
	src_mac = (pkt->p_user[10] & 0x0ff) | (src_mac << 8);
	src_mac = (pkt->p_user[11] & 0x0ff) | (src_mac << 8);
	src_mac = (pkt->p_user[12] & 0x0ff) | (src_mac << 8);
	src_mac = (pkt->p_user[13] & 0x0ff) | (src_mac << 8);

	// we have src mac from the 8-13th bytes and src up from 14th-17th bytes :3 (bytes start at 0 :3)

	src_ip = (pkt->p_user[14] & 0x0ff);
	src_ip = (pkt->p_user[15] & 0x0ff) | (src_ip << 8);  // shift to left to make space for the new 8 bits xD constructing the ip here using <<
	src_ip = (pkt->p_user[16] & 0x0ff) | (src_ip << 8);
	src_ip = (pkt->p_user[17] & 0x0ff) | (src_ip << 8);


	if (pkt->p_user[7] == 1) {
		// This is an ARP request that we need to reply to
		send_arp_reply(src_mac, src_ip);
	} else if (pkt->p_user[7] == 2) {
		// This is a reply to (hopefully) one of our prior requests
		//arp_table_add(src_ip, src_mac);  // saved in arp table now
		// printf("Added to arp table \n");
		printf("rx_arp() Added to arp table: %d.%d.%d.%d is at %d:%d:%d:%d:%d:%d\n",
			((src_ip>>24)&0x0ff), ((src_ip>>16)&0x0ff),
			((src_ip>> 8)&0x0ff), ((src_ip    )&0x0ff),
			//
			((int)((src_mac>>40)&0x0ff)), ((int)((src_mac>>32)&0x0ff)),
			((int)((src_mac>>24)&0x0ff)), ((int)((src_mac>>16)&0x0ff)),
			((int)((src_mac>> 8)&0x0ff)), ((int)((src_mac    )&0x0ff)));
	}

	free_pkt(pkt);
}

void	dump_arppkt(NET_PACKET *pkt) {
	printf("DUMP: ARP");
	if (pkt->p_user[7] == ARP_REQUEST)
		printf("-REQUEST\n");
	else if (pkt->p_user[7] == ARP_REPLY)
		printf("-REPLY\n");
	else
	

		printf(" ... unknown ARP packet type\n");
		printf("  ARP-HDR: %02x:%02x:%02x:%02x:%02x:%02x : %02x%02x\n",
			pkt->p_user[0]&0x0ff, pkt->p_user[1]&0x0ff,
			pkt->p_user[2]&0x0ff, pkt->p_user[3]&0x0ff,
			pkt->p_user[4]&0x0ff, pkt->p_user[5]&0x0ff,
			pkt->p_user[6]&0x0ff, pkt->p_user[7]&0x0ff);
			
		printf("  SRC MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
			pkt->p_user[8]&0x0ff,
			pkt->p_user[9]&0x0ff,
			pkt->p_user[10]&0x0ff,
			pkt->p_user[11]&0x0ff,
			pkt->p_user[12]&0x0ff,
			pkt->p_user[13]&0x0ff);
		printf("  SRC IP: %3d.%3d.%3d.%3d\n",
			pkt->p_user[14]&0x0ff,
			pkt->p_user[15]&0x0ff,
			pkt->p_user[16]&0x0ff,
			pkt->p_user[17]&0x0ff);

		printf("  DST MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
			pkt->p_user[18]&0x0ff,
			pkt->p_user[19]&0x0ff,
			pkt->p_user[20]&0x0ff,
			pkt->p_user[21]&0x0ff,
			pkt->p_user[22]&0x0ff,
			pkt->p_user[23]&0x0ff);
		printf("  DST IP: %3d.%3d.%3d.%3d\n",
			pkt->p_user[24]&0x0ff,
			pkt->p_user[25]&0x0ff,
			pkt->p_user[26]&0x0ff,
			pkt->p_user[27]&0x0ff);
}
