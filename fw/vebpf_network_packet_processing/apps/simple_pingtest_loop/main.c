// #include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "board.h"
#include "pkt.h"

//#include "pkt.c" ss

#include "ipproto.h"
#include "arp.h"
#include "icmp.h"
//#include "icmp.c"  // writing this solved error of cannot find icmp ping function

#include "udpproto.h"
#include "protoconst.h"
#include "etcnet.h"
#include "ethproto.h"
#include "../../../../riscv_subsystem/sw/utils.h" 
#define 	memorder 0
#define	UDP_DEV_PORT	6783
#define	UDP_HOST_PORT	6784
// #include <linux/membarrier.h>
//#include "txfns.h"
// comment to recompile using MAKE :3
// need to change the board.h address values according to the edgetestbed addressing (have written these addresses down in my vsim3 folder)

//extern void	tx_busy(NET_PACKET *); 
//
// Acknowledge all interrupts, and shut all interrupt sources off
#define	CLEARPIC	0x7fff7fff  // 1111111111111110111111111111111 // 1 1111 1111 1111 1101 1111 1111 1111
// Turn off the master interrupt generation
#define	DISABLEINTS	0x80000000  // 1000 0000 0000 0000 0000 0000 0000 0000

#define	REPEATING_TIMER	0x80000000

unsigned	heartbeats = 0, lasthello;

NET_PACKET	*waiting_pkt = NULL;


void	tx_busy(NET_PACKET *txpkt) {
	if (waiting_pkt == NULL) {
		// printf("TX-BUSY\n");
		waiting_pkt = txpkt;
	} else if (txpkt != waiting_pkt) {
		//txstr("Busy collision--deleting waiting packet\n");
		free_pkt(waiting_pkt);
		waiting_pkt = txpkt;
	}
}



int	main(int argc, char **argv) {

// ************** I have hard coded the dest mac address so I dont have to use ARP ************************* ////////////////
	// I have hard coded the dest mac address so I dont have to use ARP
	NET_PACKET	*rcvd;
	unsigned	now = 0, lastping = 0;
	unsigned	host_ip = DEFAULT_ROUTERIP; // IPADDR(192,168,0,1)  // My laptop's IP  
	// IPADDR(192,168,15,1)  // in etcnet.h #define	IPADDR(A,B,C,D)	((((A)&0x0ff)<<24)|(((B)&0x0ff)<<16)|(((C)&0x0ff)<<8)|(D&0x0ff))
	unsigned	pic;

	heartbeats = 0;
	lasthello  = 0;

	// *_buspic = CLEARPIC; // will come to this later #define	CLEARPIC	0x7fff7fff  // 1111111111111110111111111111111 // 1 1111 1111 1111 1101 1111 1111 1111
	__atomic_store_n((unsigned*)(_buspic), CLEARPIC, memorder);
	
	// *_buspic = DISABLEINTS;
	__atomic_store_n((unsigned*)(_buspic), DISABLEINTS, memorder);
	// Turn off the master interrupt generation
	// #define	DISABLEINTS	0x80000000  // 1000 0000 0000 0000 0000 0000 0000 0000

	// Clear the network reset
	// _net1->n_txcmd = 0;	// here the txcmd bit is set to 0 here (the data here on this address _net1->n_txcmd)  
	__atomic_store_n((unsigned*)&(_net1->n_txcmd), 0, memorder);



	{ // Set the MAC address (device)
		char *macp = (char *)&_net1->n_mac;  // VIP step here that I missed that &_net1->n_mac is a struct pointer and is being casted as a char pointer so that it can be increased in terms of bytes rather than size of stucts
		// hence macp[0] and macp[1] have a difference of 1 byte since it was cast to char* ptr
		// we are giving *macp pointer the address of _net1->n_mac and setting it byte by byte (by casting it as char*) 

		ETHERNET_MAC upper = DEFAULTMAC >> 32;  // typedef	uint64_t ETHERNET_MAC; #define	DEFAULTMAC	0xa25345b6fb5eul
		unsigned	upper32 = (unsigned) upper;

		//maacp[2] and macp[3] ignored in rtl
		// macp[1] = (upper32 >>  8) & 0x0ff;  // char* will point to a byte of data
		// the sequence of these bytes at least don't matter since we are writing at the address we have mentioned
			// but we have a problem when a load happens before a store e.g., a load happens before the store of the reset bit
			// to the tx ctrl register
		__atomic_store_n((char*)(macp + 1), ((upper32 >>  8) & 0x0ff), memorder);

		// macp[0] = (upper32      ) & 0x0ff;
		__atomic_store_n((char*)(macp), ((upper32      ) & 0x0ff), memorder);  

		// macp[7] = (DEFAULTMAC >> 24) & 0x0ff;
		__atomic_store_n((char*)(macp + 7), ((DEFAULTMAC >> 24) & 0x0ff), memorder); 
		
		// macp[6] = (DEFAULTMAC >> 16) & 0x0ff;
		__atomic_store_n((char*)(macp + 6), ((DEFAULTMAC >> 16) & 0x0ff), memorder); 
		
		// macp[5] = (DEFAULTMAC >>  8) & 0x0ff;
		__atomic_store_n((char*)(macp + 5), ((DEFAULTMAC >>  8) & 0x0ff), memorder); 
		
		// macp[4] = (DEFAULTMAC      ) & 0x0ff;
		__atomic_store_n((char*)(macp + 4), ((DEFAULTMAC      ) & 0x0ff), memorder); 
	}

	waiting_pkt = NULL;

	printf("\n\n\n"
	"+-----------------------------------------+\n"
	"+----       Starting Ping test        ----+\n"
	// "+----123456789               987654321----+\n"
	"+-----------------------------------------+\n"
	"\n\n\n");

	//printf("Sending ICMP ping to IP address at %3d.%3d.%3d.%3d\n", ((host_ip>>24)&0x0ff), ((host_ip>>16)&0x0ff), ((host_ip>> 8)&0x0ff), ((host_ip)&0x0ff));
	printf("Sending ICMP ping to IP address at %d %d %d %d \n", ((host_ip>>24)&0x0ff), ((host_ip>>16)&0x0ff), ((host_ip>> 8)&0x0ff), ((host_ip)&0x0ff));

	icmp_send_ping(host_ip); 

	
	//icmp_send_ping(host_ip);  // first go to arp.c and give the "ETHERNET_MAC	router_mac_addr;" variable a non zero value
	// in icmp_send_ping we have tx_ippkt(), we have arp_lookup() in tx_ippkt()	
	// go to arp_lookup() def in arp.c. Give the "ETHERNET_MAC	router_mac_addr;" variable a non zero value,
	// which means that our device has the has the router mac address and does not need to send an ARP request
	// and then wait for the routers reply packet and update its ARP-table according to the mac and IP addresses
	// in that recieved ARP reply packet from the router, and then be able to send an ICMP ping packet with those received addresses. 
	// So in order to avoid all these extra steps, for this code, I'll give the variable "router_mac_addr" a non-zero value
	// manually, for testing purposes. In this way only our tx pipeline of this code is being utilized and testing would
	// be simpler this way.

	while(1){

		// if (pic & BUSPIC_TIMER) { // in the final version we will have some interrupt here (I think its a bus timer interrupt from the FPGA hw)
		heartbeats++;

		// add code to retry for icmp_ping here based on timer or a simple if condition check heartbeats, and if
		// it reaches a certain value, reset it and send and icmp ping. But using a sys timer would be better
		// or do this at the end of this code:
		/*
		if (waiting_pkt == NULL) {  // make sure we don't have a packet waiting in line to be sent due to the hw tx pipeline being busy (FPGA ethernet module)

			icmp_send_ping(host_ip);
		}//}		
		*/
		if (heartbeats >= 32949672) {//4294967295
			if (waiting_pkt == NULL) {  // make sure we don't have a packet waiting in line to be sent due to the hw tx pipeline being busy (FPGA ethernet module)

				printf("Resending ICMP ping to IP address at %d %d %d %d \n", ((host_ip>>24)&0x0ff), ((host_ip>>16)&0x0ff), ((host_ip>> 8)&0x0ff), ((host_ip)&0x0ff));
				icmp_send_ping(host_ip);
			}
			heartbeats = 0;
		}


		
		// Check for any interrupts  // pic is from interrupt controller I think 
		//pic = *_buspic;  // _buspic pointer address points to status of the interrupts on the bus in the zipversa rtl hw,there is a bus interrupt controller in the zipversa fpga hw rtl 
		pic = __atomic_load_n((unsigned*)(_buspic), memorder);  // _buspic pointer address points to status of the interrupts on the bus in the zipversa rtl hw,there is a bus interrupt controller in the zipversa fpga hw rtl 		
		
		// I dont think an interrupt service routine based on interrupts to the processor are used in this c code but
		// the bus interrupt controller module in the FPGA and its interrupt outputs are being used in this c code.

		// timer stuff insert later

		// BUSPIC_NETRX = = b 1000 0000 = d 128 = 0x80 //#define	BUSPIC_NETRX	BUSPIC(7)
		
		//if (pic & BUSPIC_NETRX) { // interrupt telling us that we have recieved a packet  // fpga interrupt controller
			// still enters this even if (__atomic_load_n((unsigned*)&(_net1->n_rxcmd), memorder))  & ENET_RXAVAIL = 0
		if ((__atomic_load_n((unsigned*)&(_net1->n_rxcmd), memorder))  & ENET_RXAVAIL){
			unsigned	ipsrc, ipdst;

			// We've received a packet
			rcvd = rx_pkt();
			
			printf("Entered main while1 loop if (pic & BUSPIC_NETRX) we have recieved a packet \n");

			// *_buspic = BUSPIC_NETRX;  // settings of fpga interrupt controller? // BUSPIC_NETRX = = b 1000 0000 = d 128 = 0x80 
			__atomic_store_n((unsigned*)(_buspic), BUSPIC_NETRX, memorder);

			if (NULL != rcvd) {
				
			// Don't let the subsystem free this packet (yet)
			rcvd->p_usage_count++;

			switch(ethpkt_ethtype(rcvd)) {
			case ETHERTYPE_ARP:        //#define	ETHERTYPE_ARP		0x0806
				printf("RXPKT - ARP\n");
				rx_ethpkt(rcvd);
				rx_arp(rcvd); // Frees the packet
				// from arp_lookup func in arp.c
					// so first time this above isnt entered and arp req is sent and then the processor waits for the arp reply
					// after getting arp reply the processor updates the arp table in which router mac addr is given to it
					//		○ https://www.youtube.com/watch?v=tXzKjtMHgWI&ab_channel=CertBros
					//			ARP to router example at the end
				break;
			case ETHERTYPE_IP: {
				unsigned	subproto;

				printf("RXPKT - IP\n");
				rx_ethpkt(rcvd);

				ipsrc = ippkt_src(rcvd);
				ipdst = ippkt_dst(rcvd);
				subproto = ippkt_subproto(rcvd);  // sub protocol?
				rx_ippkt(rcvd);

				if (ipdst == my_ip_addr) {
				switch(subproto) {
					case IPPROTO_ICMP:
						if (rcvd->p_user[0] == ICMP_PING)	icmp_reply(ipsrc, rcvd);
						else
							printf("RX PING <<------ SUCCESS!!!\n");  // our main job of icmp ping-ing is done
						// Free the packet
						free_pkt(rcvd);
						break;
					default:
						printf("UNKNOWN-IP -----\n");  //not directed at our device
						pkt_reset(rcvd);
						dump_ethpkt(rcvd);
						printf("\n");
						// Free the packet
						free_pkt(rcvd);  // removing the data
						break;
				}}}
				break;
			default:
				printf("Received unknown ether-type %d (0x%04x)\n",  // some other ether type
					ethpkt_ethtype(rcvd),
					ethpkt_ethtype(rcvd));
				pkt_reset(rcvd);
				dump_ethpkt(rcvd);
				printf("\n");
				// Free the packet
				free_pkt(rcvd);
				break;
			}

			// Now we can free the packet ourselves
			free_pkt(rcvd);
		}} else if ((__atomic_load_n((unsigned*)&(_net1->n_rxcmd), memorder)) & ENET_RXCLRERR) {
		// }} else if (_net1->n_rxcmd & ENET_RXCLRERR) {
			// printf("Network has detected an error, %08x\n", _net1->n_rxcmd);
			// printf("Network has detected an error, %08x\n", (__atomic_load_n((unsigned*)&(_net1->n_rxcmd), memorder)));
			printf("Network has detected an error, %d \n", (__atomic_load_n((unsigned*)&(_net1->n_rxcmd), memorder)));
			// _net1->n_rxcmd = ENET_RXCLRERR | ENET_RXCLR;
			__atomic_store_n((unsigned*)&(_net1->n_rxcmd), (ENET_RXCLRERR | ENET_RXCLR), memorder);

			// ENET_RXCLRERR = b'0111 1000 0000 0000 0000
			// ENET_RXCLRERR | ENET_RXCLR = 0x7C000 = d'507904 = b'0111 1100 0000 0000 0000
		}

		// #define	BUSPIC_NETTX	BUSPIC(6)  //#define BUSPIC(X) (1<<X) = 1000000 = 64  //#define BUSPIC(X) (1<<X)	
		if (pic & BUSPIC_NETTX) {  // will come back to this later 
			// so yes I was right, we have assign	o_tx_int = !tx_busy; assign	o_rx_int = (rx_valid)&&(!rx_clear);
			// pins in the zipversa code that send interrupts to the rv core when tx is not busy (i think for when its not busy) (for tx int) 
				//#define	BUSPIC_NETTX	BUSPIC(6)  //#define BUSPIC(X) (1<<X) = 1000000 = 64
				// from zipversa assign	bus_int_vector = { 1'b0,1'b0,1'b0,1'b0,	1'b0,1'b0,flashdbg_int,	net1rx_int,	net1tx_int,	enetscope_int, uartrxf_int, uarttxf_int, spio_int, systimer_int, wbfft_int};
					// the tx int net1tx_int is the 7th bit i.e., bus_int_vector[6]  #define BUSPIC(X) (1<<X)	
					// theres also net1tx_int in a rv int vector assign	picorv_int_vec = { (31)1'b0, net1rx_int,  net1tx_int, uartrxf_int, uarttxf_int, systimer_int, gpio_int};
					
			// so pic I think reads the bus interrupt rtl hw module and there is a controller for that there in the rtl hw 
			// what do the interrupts to the rv core do?
				// https://www.opensourceforu.com/2011/01/handling-interrupts/
				// https://www.geeksforgeeks.org/interrupts/
			// for initial testing of code we can go without the interrupt controller etc and just tx packets and see the ethernet pipeline simulation
			// So I dont think zipversa is using  interrupt request (irq) of riscv and instead is using its own interrupt controller

			// We've finished transmitting our last packet.
			// See if another's waiting, and then transmit that.a
			
			//
			if (waiting_pkt != NULL) {  // VIPPPP // in finished code we will have some sort of interrupt telling us that the packet transmission in FPGA hw has been completed, only then we will send this waiting packet
				NET_PACKET	*pkt = waiting_pkt;
				printf("Re-transmitting the busy packet\n");
				waiting_pkt = NULL;  // tx_busy puts txpkt in waiting_pkt 
				tx_pkt(pkt);
				// *_buspic = BUSPIC_NETTX;  // tx bit in interrupt controller set to 0 here
				__atomic_store_n((unsigned*)(_buspic), BUSPIC_NETTX, memorder);
			}
		}
	}
}
