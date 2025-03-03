////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	ipproto.c
//
// Project:	ZipVersa, Versa Brd implementation using ZipCPU infrastructure
//
// Purpose:	To encapsulate common functions associated with the IP protocol
//		portion of the network stack.
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
#include "board.h"
#include "pkt.h"
#include "protoconst.h"
#include "ethproto.h"
#include "ipproto.h"
#include "icmp.h"
#include "ipcksum.h"
#include "arp.h"
#include "etcnet.h"
// #include "../../../../riscv_subsystem/sw/utils.h" 
extern void printf(char *c, ...);  // including ../../../../riscv_subsystem/sw/utils.h in two files
								  // was giving compilation error. This extern method works.
#include "board.h"

// ETHERNET_MAC  dst_mac = DST_MAC;

uint32_t	my_ip_addr = DEFAULTIP,  // IPADDR(192,168,1,128) //#define	DEFAULTIP	IPADDR(192,168,15,22)
		my_ip_mask = LCLNETMASK;

uint32_t	dest_ip = DESTIP;

unsigned	ip_pktid = (unsigned)(BIG_PRIME * 241ll);

unsigned	ip_headersize(void) {
	return 5*4;
}

NET_PACKET *new_ippkt(unsigned ln) {
	NET_PACKET	*pkt;

	pkt = new_ethpkt(ln+20);  // total 28, 8 from icmp/total 32, 12 from udp fft		// 20 ? 18 bytes req for the dest src macs 2 byte eth type and 4 byte crc 
		// https://en.wikipedia.org/wiki/Internet_Protocol_version_4
		// 20 bytes (min) are req for ipv4 header


	pkt->p_length -= 20;	// plength is -20?  // nope.. pkt->p_length has already been allocated len by new_pkt()
	pkt->p_user += 20;		// increasing the size of p_user pointer	// pkt->p_user= pkt->p_raw; pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);
	return pkt;
}

NET_PACKET *new_ippkt_v2(unsigned ln) {
	NET_PACKET	*pkt;

	pkt = new_ethpkt_v2(ln+20);  // total 28, 8 from icmp/total 32, 12 from udp fft		// 20 ? 18 bytes req for the dest src macs 2 byte eth type and 4 byte crc 
		// https://en.wikipedia.org/wiki/Internet_Protocol_version_4
		// 20 bytes (min) are req for ipv4 header


	pkt->p_length -= 20;	// plength is -20?  // nope.. pkt->p_length has already been allocated len by new_pkt()
	pkt->p_user += 20;		// increasing the size of p_user pointer	// pkt->p_user= pkt->p_raw; pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);
	return pkt;
}

void	ip_set(NET_PACKET *pkt, unsigned subproto, unsigned src,
		unsigned dest) {
	unsigned	cksum;
	// subproto = #define	IPPROTO_ICMP	1 in protoconst.h
	// pkt->p_length = 28 = 0x1c
	// herer we set the 20 packets for IP packet

	pkt->p_user[0] = 0x45;
	pkt->p_user[1] = 0x00;
	pkt->p_user[2] = (pkt->p_length >> 8)&0x0ff;  // = 00  //pkt->p_length = 28 = 0x1c //ICMP is implemented as part of the IP layer and
	// ICMP comes after the IP header that is why it is added after the IP data using p_user https://en.wikipedia.org/wiki/Internet_Control_Message_Protocol#header_type
	pkt->p_user[3] = (pkt->p_length     )&0x0ff;  // = 0x1c
	//
	ip_pktid += BIG_PRIME; // unsigned	ip_pktid = (unsigned)(BIG_PRIME * 241ll); A BIG prime number #define	BIG_PRIME	0x0134513b
	pkt->p_user[4] = (ip_pktid >> 8) & 0x0ff;  // missed this ip_pktid = (unsigned)(BIG_PRIME * 241ll);  for simulation checking
	pkt->p_user[5] = (ip_pktid & 0x0ff);
	pkt->p_user[6] = 0x00;	// Flags and
	pkt->p_user[7] = 0x00;	// Fragmentation offset
	//
	// Time-to-live, sub-protocol
	pkt->p_user[8] = 0x80;
	pkt->p_user[9] = subproto;
	pkt->p_user[10] = 0x00;	// Header checksum, come back for these, confirmed in sim
	pkt->p_user[11] = 0x00;
	//
	// My IP address
	pkt->p_user[12] = (src >> 24)&0x0ff;	// my_ip_addr = DEFAULTIP = IPADDR(192,168,15,22) = 0xc0a80f16, confirmed in simulation
	pkt->p_user[13] = (src >> 16)&0x0ff;
	pkt->p_user[14] = (src >>  8)&0x0ff;
	pkt->p_user[15] = (src      )&0x0ff;
	//
	// Destination IP address
	pkt->p_user[16] = (dest >> 24)&0x0ff;	// host_ip = DEFAULT_ROUTERIP;  // IPADDR(192,168,15,1) = c0a80f01
	pkt->p_user[17] = (dest >> 16)&0x0ff;
	pkt->p_user[18] = (dest >>  8)&0x0ff;
	pkt->p_user[19] = (dest      )&0x0ff;
	//

	// Calculate the checksum (IPV4 cksum is mandatory I think unlike UDP cksum)
	cksum = ipcksum(ip_headersize(), pkt->p_user);
	pkt->p_user[10] = ((cksum>>8) & 0x0ff);
	pkt->p_user[11] = ( cksum     & 0x0ff);
}
/*tx_ippkt(pkt, IPPROTO_ICMP, my_ip_addr, ping_ip_addr);	//#define	IPPROTO_ICMP	1 in protoconst.h
	// my_ip_addr and ping_ip_addr are empty uint32_t variables here from etcnet.h*/
		// my ip is #define	DEFAULTIP	IPADDR(192,168,15,22), ping_ip_addr is //	unsigned	host_ip = DEFAULT_ROUTERIP;  // IPADDR(192,168,15,1)  // in etcnet.h #define
void	tx_ippkt(NET_PACKET *pkt, unsigned subproto, unsigned src,
		unsigned dest) {
	if (pkt->p_user - pkt->p_raw < 20+8) {	// should be 20 + 14 by including src mac
			// pkt_user increased by +28 cx of the parent functions that generated the pkt, 
			// the ethernetpkt, the ippkt etc pkt->p_user= pkt->p_raw; pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);
		printf("ERR: IPPKT doesn't have enough room for its headers\n");
		printf("ERR: p_user = &p_raw[%d]\n",
			pkt->p_user - pkt->p_raw); // was part of the printf
		dump_ippkt(pkt);
		free_pkt(pkt);

		//*_gpio = 0x0100010;		// static volatile unsigned *const _gpio = ((unsigned *)6291480);
		// writing something to the gpio module in rtl, most prob some error code :3
		
		// deref pointer and writing value to it
		return;
	}
	


	// so first ICMP info was inserted into p_user array in 8 bytes, now we move p_user back 20 to insert 20 bytes of IP header
	// since p_user is a char *, hence this pointer points to a char which is of the size 1 byte thats why we are counting bytes 
	// for each element and hence we doing pkt->p_user   -= ip_headersize();	// 20

	// since p_user is the starting point for data insertion at p_user[0], hence we need to pull it back for 20 bytes

	pkt->p_user   -= ip_headersize();	// 20
		// writing ip header
			// So after the exp below. We are going back ip_headersize address spaces back, taking the pointer pkt->p_user
			// ip_headersize() address space back (where we did leave spaces )

	// This explanation coming from the function icmp_send_ping that is using this tx_ippkt function
	// pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET); // pkt->p_user= pkt->p_raw; 
	// *p_user is actually a pointer so if I use pkt->p_user, it referes to that pointer!!!!!!!!!!!!!!! Address I think!
	// this idea is helpful in the func below tx_ippkt

	// since p_length was reset to 8 after ICMP packets insertion, now we add 20 more to the length since
	// our ipheader size would be 20 bytes
		// it actually is correct since p_user value is the address value since it was declared a pointer and p_user[0] is the dereferenced
		// value of p_user i.e., p_user[0] means *p_user. hence how we are subtracting 20 from p_user (the address) and manipulating it this
		// way is correct!

	pkt->p_length += ip_headersize();  
		// writing ip header
		// now the packet len is 8 (from ICMP) + 20 (from this IP data) bytes = 28 bytes
		// pkt->p_length = 28 = 0x1c
		// here we set the 20 IP packets
	
	ip_set(pkt, subproto, src, dest);  
		// ipv4 header is set in this function
			// the fields defined above // already set ip packet here
			// my ip is #define	DEFAULTIP	IPADDR(192,168,15,22),dest ping_ip_addr is //	unsigned	host_ip = DEFAULT_ROUTERIP;  // IPADDR(192,168,15,1)  // in etcnet.h #define
			// subproto = #define	IPPROTO_ICMP	1 in protoconst.h
			// 20 bytes for ip packet.
			// pkt->p_length = 28 = 0x1c

	ETHERNET_MAC	mac;	// typedef	uint64_t ETHERNET_MAC;
	
	// we are sending an ARP req packet even before the ICMP packet (if ARP packet needs to be sent) in 
	// arp_lookup below
	if (arp_lookup(dest, &mac) == 0) {	// this is entered after we get arp reply back from router and that updates our arptable 	
	// from arp_lookup func in arp.c
		// so first time this above isnt entered and arp req is sent and then the processor waits for the arp reply
		// after getting arp reply the processor updates the arp table in which router mac addr is given to it
		//		○ https://www.youtube.com/watch?v=tXzKjtMHgWI&ab_channel=CertBros
		//			ARP to router example at the end

	//ipaddr == my_ip_router ipaddr == my_ip_router return 0;
	// dest ping_ip_addr is //	unsigned	host_ip = DEFAULT_ROUTERIP;  // IPADDR(192,168,15,1)  // in etcnet.h #define

		
		// printf("ARP-LOOKUP SUCCESS: %3d.%3d.%3d.%3d is at %02x:%02x:%02x:%02x:%02x:%02x\n",
		// 	((dest>>24)&0x0ff), ((dest>>16)&0x0ff),
		// 	((dest>> 8)&0x0ff), ((dest    )&0x0ff),
		// 	//
		// 	((int)((mac>>40)&0x0ff)), ((int)((mac>>32)&0x0ff)),
		// 	((int)((mac>>28)&0x0ff)), ((int)((mac>>16)&0x0ff)),
		// 	((int)((mac>> 8)&0x0ff)), ((int)((mac    )&0x0ff)));

		printf("ARP-LOOKUP SUCCESS: %d.%d.%d.%d is at %d:%d:%d:%d:%d:%d\n",
			((dest>>24)&0x0ff), ((dest>>16)&0x0ff),
			((dest>> 8)&0x0ff), ((dest    )&0x0ff),
			//
			((int)((mac>>40)&0x0ff)), ((int)((mac>>32)&0x0ff)),
			((int)((mac>>24)&0x0ff)), ((int)((mac>>16)&0x0ff)),
			((int)((mac>> 8)&0x0ff)), ((int)((mac    )&0x0ff)));
		
		

		// *mac = router_mac_addr;
		return tx_ethpkt(pkt, ETHERTYPE_IP, mac);  //mac is of the router after our arp req and rep from router
	} else {
		// printf("ARP-LOOKUP failed: Could not find IP address for %3d.%3d.%3d.%3d\n",
		// 	((dest>>24)&0x0ff), ((dest>>16)&0x0ff),
		// 	((dest>> 8)&0x0ff), ((dest    )&0x0ff));

		printf("ARP-LOOKUP failed: Could not find IP address for %d.%d.%d.%d\n",
			((dest>>24)&0x0ff), ((dest>>16)&0x0ff),
			((dest>> 8)&0x0ff), ((dest    )&0x0ff));

		

		free_pkt(pkt);
	}
	// return 1;
}

void	tx_ippkt_v2(NET_PACKET *pkt, unsigned subproto, unsigned src, unsigned dest, void *tx_pkt_current_start_mem_address) {

	if (pkt->p_user - pkt->p_raw < 20+14) {	// should be 20 + 14 by including src mac
			// pkt_user increased by +28 cx of the parent functions that generated the pkt, 
			// the ethernetpkt, the ippkt etc pkt->p_user= pkt->p_raw; pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET);
				// pointer arithematic happening here
		
		// commenting our print statements for now
		// printf("ERR: IPPKT doesn't have enough room for its headers\n");
		// printf("ERR: p_user = &p_raw[%d]\n",
		// 	pkt->p_user - pkt->p_raw); // was part of the printf
		dump_ippkt(pkt);
		free_pkt(pkt);

		// print error msg and turn on warning led
		if ((DEBUG) || (SIMULATION_TESTING)) {


			if (SIMULATION_TESTING && DEBUG) {

				// printing error message
				printf("There is an error in tx_ippkt_v2");

				// led = 5 means there is an error
				write_led(5); 

			} else if (SIMULATION_TESTING) {

				// led = 5 means there is an error
				write_led(5); 

			} else if (DEBUG) {

				// printing error message
				printf("\nThere is an error in tx_ippkt_v2\n");

				// led = 5 means there is an error
				write_led(5); 

			}

		} 

		//*_gpio = 0x0100010;		// static volatile unsigned *const _gpio = ((unsigned *)6291480);
		// writing something to the gpio module in rtl, most prob some error code :3
		
		// deref pointer and writing value to it
		return;
	}
	


	// so first ICMP info was inserted into p_user array in 8 bytes, now we move p_user back 20 to insert 20 bytes of IP header
	// since p_user is a char *, hence this pointer points to a char which is of the size 1 byte thats why we are counting bytes 
	// for each element and hence we doing pkt->p_user   -= ip_headersize();	// 20

	// since p_user is the starting point for data insertion at p_user[0], hence we need to pull it back for 20 bytes

	pkt->p_user   -= ip_headersize();	// 20
		// writing ip header
			// So after the exp below. We are going back ip_headersize address spaces back, taking the pointer pkt->p_user
			// ip_headersize() address space back (where we did leave spaces )

	// This explanation coming from the function icmp_send_ping that is using this tx_ippkt function
	// pkt->p_raw  = ((char *)pkt) + sizeof(NET_PACKET); // pkt->p_user= pkt->p_raw; 
	// *p_user is actually a pointer so if I use pkt->p_user, it referes to that pointer!!!!!!!!!!!!!!! Address I think!
	// this idea is helpful in the func below tx_ippkt

	// since p_length was reset to 8 after ICMP packets insertion, now we add 20 more to the length since
	// our ipheader size would be 20 bytes
		// it actually is correct since p_user value is the address value since it was declared a pointer and p_user[0] is the dereferenced
		// value of p_user i.e., p_user[0] means *p_user. hence how we are subtracting 20 from p_user (the address) and manipulating it this
		// way is correct!

	pkt->p_length += ip_headersize();  
		// writing ip header
		// now the packet len is 8 (from ICMP) + 20 (from this IP data) bytes = 28 bytes
		// pkt->p_length = 28 = 0x1c
		// here we set the 20 IP packets

	// check later if it is mandatory  to calculate crc for ip header 
	ip_set(pkt, subproto, src, dest);  
		// ipv4 header is set in this function
			// the fields defined above // already set ip packet here
			// my ip is #define	DEFAULTIP	IPADDR(192,168,15,22),dest ping_ip_addr is //	unsigned	host_ip = DEFAULT_ROUTERIP;  // IPADDR(192,168,15,1)  // in etcnet.h #define
			// subproto = #define	IPPROTO_ICMP	1 in protoconst.h
			// 20 bytes for ip packet.
			// pkt->p_length = 28 = 0x1c

	ETHERNET_MAC  src_mac = SRC_MAC;
	ETHERNET_MAC  dst_mac = DST_MAC;

	if (DEBUG)
		printf("\n\n\n****** Inside tx_ippkt_v2() and entering tx_ethpkt_v2() ******\n\n\n");

	tx_ethpkt_v2(pkt, ETHERTYPE_IP, src_mac, dst_mac, tx_pkt_current_start_mem_address);

	if (DEBUG)
		printf("\n\n\n****** Inside tx_ippkt_v2() and exited tx_ethpkt_v2() ******\n\n\n");

	// block commenting the code below since we don't need ARP functionality, we are hardcoding the dst MAC according to the dst IP here

	// BLOCK COMMENT STARTS below

	/*ETHERNET_MAC	mac;	// typedef	uint64_t ETHERNET_MAC;
	
	// we are sending an ARP req packet even before the ICMP packet (if ARP packet needs to be sent) in 
	// arp_lookup below
	if (arp_lookup(dest, &mac) == 0) {	// this is entered after we get arp reply back from router and that updates our arptable 	
	// from arp_lookup func in arp.c
		// so first time this above isnt entered and arp req is sent and then the processor waits for the arp reply
		// after getting arp reply the processor updates the arp table in which router mac addr is given to it
		//		○ https://www.youtube.com/watch?v=tXzKjtMHgWI&ab_channel=CertBros
		//			ARP to router example at the end

	//ipaddr == my_ip_router ipaddr == my_ip_router return 0;
	// dest ping_ip_addr is //	unsigned	host_ip = DEFAULT_ROUTERIP;  // IPADDR(192,168,15,1)  // in etcnet.h #define

		
		// printf("ARP-LOOKUP SUCCESS: %3d.%3d.%3d.%3d is at %02x:%02x:%02x:%02x:%02x:%02x\n",
		// 	((dest>>24)&0x0ff), ((dest>>16)&0x0ff),
		// 	((dest>> 8)&0x0ff), ((dest    )&0x0ff),
		// 	//
		// 	((int)((mac>>40)&0x0ff)), ((int)((mac>>32)&0x0ff)),
		// 	((int)((mac>>28)&0x0ff)), ((int)((mac>>16)&0x0ff)),
		// 	((int)((mac>> 8)&0x0ff)), ((int)((mac    )&0x0ff)));

		printf("ARP-LOOKUP SUCCESS: %d.%d.%d.%d is at %d:%d:%d:%d:%d:%d\n",
			((dest>>24)&0x0ff), ((dest>>16)&0x0ff),
			((dest>> 8)&0x0ff), ((dest    )&0x0ff),
			//
			((int)((mac>>40)&0x0ff)), ((int)((mac>>32)&0x0ff)),
			((int)((mac>>24)&0x0ff)), ((int)((mac>>16)&0x0ff)),
			((int)((mac>> 8)&0x0ff)), ((int)((mac    )&0x0ff)));
		
		

		// *mac = router_mac_addr;
		return tx_ethpkt(pkt, ETHERTYPE_IP, mac);  //mac is of the router after our arp req and rep from router
	} else {
		// printf("ARP-LOOKUP failed: Could not find IP address for %3d.%3d.%3d.%3d\n",
		// 	((dest>>24)&0x0ff), ((dest>>16)&0x0ff),
		// 	((dest>> 8)&0x0ff), ((dest    )&0x0ff));

		printf("ARP-LOOKUP failed: Could not find IP address for %d.%d.%d.%d\n",
			((dest>>24)&0x0ff), ((dest>>16)&0x0ff),
			((dest>> 8)&0x0ff), ((dest    )&0x0ff));

		

		free_pkt(pkt);
	}*/
	// return 1;
}

void	rx_ippkt(NET_PACKET *pkt) {
	unsigned	sz = (pkt->p_user[0] & 0x0f)*4; // x4 for in bytes size // Internet Header Length (IHL) Explained below
	// Internet Header Length (IHL) 
	/* The IPv4 header is variable in size due to the optional 14th field (options). 
	The IHL field contains the size of the IPv4 header, it has 4 bits that specify the number of 32-bit words in the header. 
	The minimum value for this field is 5,[35] which indicates a length of 5 × 32 bits = 160 bits = 20 bytes. As a 4-bit field, 
	the maximum value is 15, this means that the maximum size of the IPv4 header is 15 × 32 bits = 480 bits = 60 bytes.https://en.wikipedia.org/wiki/IPv4#Address_representations
	The IPv4 packet header consists of 14 fields, of which 13 are required. The 14th field is optional and aptly named: options. The fields in the header are packed with the most significant byte first (big endian), and for the diagram and discussion, the most significant bits are considered to come first (MSB 0 bit numbering). 
	The most significant bit is numbered 0, so the version field is actually found in the four most significant bits of the first byte, for example.*/

	pkt->p_user   += sz;  // move to higher OSI layer (e.g udp) :3
	pkt->p_length -= sz;
}

unsigned	ippkt_src(NET_PACKET *pkt) {
	unsigned	ipsrc;

	ipsrc = (pkt->p_user[12] & 0x0ff);
	ipsrc = (pkt->p_user[13] & 0x0ff) | (ipsrc << 8);
	ipsrc = (pkt->p_user[14] & 0x0ff) | (ipsrc << 8);
	ipsrc = (pkt->p_user[15] & 0x0ff) | (ipsrc << 8);
	return ipsrc;
}

// this function stays the same since there was a difference only for the ethernet header as my implementation has dst MAC 6 extra bytes
unsigned	ippkt_dst(NET_PACKET *pkt) {  
	unsigned	ipdst;

	ipdst = (pkt->p_user[16] & 0x0ff);
	ipdst = (pkt->p_user[17] & 0x0ff) | (ipdst << 8);
	ipdst = (pkt->p_user[18] & 0x0ff) | (ipdst << 8);
	ipdst = (pkt->p_user[19] & 0x0ff) | (ipdst << 8);
	return ipdst;
}


unsigned	ippkt_subproto(NET_PACKET *pkt) {
	unsigned	subproto;

	subproto = (pkt->p_user[9] & 0x0ff);
	return subproto;
}

void	dump_ippkt(NET_PACKET *pkt) {
	unsigned	ihl = (pkt->p_user[0] & 0x0f)*4, proto, cksum, calcsum;

	printf("IP VERSION: %d\n", (pkt->p_user[0] >> 4) & 0x0f);
	printf("IP HDRLEN : %d bytes\n", ihl);
	if (pkt->p_user[0] != 0x45) {
		printf("IP -- SOMETHING\'S WRONG HERE ******\n");
		for(unsigned k=pkt->p_user - pkt->p_raw; k<pkt->p_rawlen; k++) {
			printf("%02x ", pkt->p_raw[k]&0x0ff);
			if ((k & 7)==7)
				printf("\n    ");
		}
		printf("\n\nDumping the raw packet now:\n    ");
		for(unsigned k=0; k<pkt->p_rawlen; k++) {
			printf("%02x ", pkt->p_raw[k]&0x0ff);
			if ((k & 7)==7)
				printf("\n    ");
		} printf("\n");
		for(;;) ;
	}

	unsigned pktln = ((pkt->p_user[2]&0x0ff)<< 8)
		| (pkt->p_user[3] & 0x0ff);
	printf("IP PKTLEN : %d", pktln);
	if (pktln > pkt->p_length) {
		printf(" --- LONGER THAN BUFFER!\n");
		return;
	} else if (pktln < pkt->p_length)
		printf(" --- Smaller than buffer\n");
	else
		printf("\n");

	printf("IP PKTTTL : %d\n", pkt->p_user[8] & 0x0ff);
	proto = pkt->p_user[9] & 0x0ff;
	printf("IP PROTO  : %d", pkt->p_user[9] & 0x0ff);
	if (proto == IPPROTO_UDP)
		printf(" (UDP)\n");
	else if (proto == IPPROTO_TCP)
		printf(" (TCP)\n");
	else if (proto == IPPROTO_ICMP)
		printf(" (ICMP)\n");
	else
		printf(" (unknown)\n");
	cksum = ((pkt->p_user[10] & 0x0ff) << 8)
			| (pkt->p_user[11] & 0x0ff);
	pkt->p_user[10] = 0;
	pkt->p_user[11] = 0;
	calcsum = ipcksum(ihl, pkt->p_user);
	printf("IP CKSUM  : 0x%02x", cksum);
	pkt->p_user[10] = (cksum >> 8)&0x0ff;
	pkt->p_user[11] = (cksum     )&0x0ff;
	if (cksum != calcsum)
		printf(" -- NO-MATCH against %04x\n", calcsum);
	else
		printf("\n");
	printf("IP SOURCE : %3d.%3d.%3d.%3d\n",
		pkt->p_user[12], pkt->p_user[13],
		pkt->p_user[14], pkt->p_user[15]);
	printf("IP DESTN  : %3d.%3d.%3d.%3d\n",
		pkt->p_user[16], pkt->p_user[17],
		pkt->p_user[18], pkt->p_user[19]);
	if ((pkt->p_user[0] & 0x0f) > 5) {
		printf("IP OPTIONS  : (Dump not yet supported)\n");
	}

	pkt->p_user   += ihl;
	pkt->p_length -= ihl;
	if (proto == IPPROTO_UDP) {
		// dump_udpproto(pkt);
	} else if (proto == IPPROTO_ICMP) {
		dump_icmp(pkt);
	} else if (proto == IPPROTO_TCP) {
		// dump_tcpproto(pkt);
	} else {
		printf("IP PAYLOAD: (dump not (yet) supported)\n");
	}
	pkt->p_user   -= ihl;
	pkt->p_length += ihl;
}
