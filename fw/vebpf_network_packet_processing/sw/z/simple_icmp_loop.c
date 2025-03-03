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

#include "protoconst.h"
#include "etcnet.h"
#include "ethproto.h"
// #include "../../../../riscv_subsystem/sw/utils.h" 
#define 	memorder 0
// #include <linux/membarrier.h>
//#include "txfns.h"
//s
// need to change the board.h address values according to the edgetestbed addressing (have written these addresses down in my vsim3 folder)

//extern void	tx_busy(NET_PACKET *); 



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

/*
typedef struct {
	int	p_usage_count, p_rawlen, p_length;
	char	*p_raw,	// Points to the beginning of raw packet memory
		*p_user;// Packet memory at the current protocol area  // The size of the character pointer is 8 bytes
} NET_PACKET;
*/

int	main(int argc, char **argv) {

// ************** I have hard coded the dest mac address so I dont have to use ARP ************************* ////////////////

	unsigned	host_ip = DEFAULT_ROUTERIP;  // IPADDR(192,168,15,1)  // in etcnet.h #define	IPADDR(A,B,C,D)	((((A)&0x0ff)<<24)|(((B)&0x0ff)<<16)|(((C)&0x0ff)<<8)|(D&0x0ff))

	// Clear the network reset
	unsigned tx_cmd_test;

	// _net1->n_txcmd = 0;	// here the txcmd bit is set to 0 here (the data here on this address _net1->n_txcmd)
	
	// call atomic store
	__atomic_store_n((unsigned*)&(_net1->n_txcmd), 0, memorder);



	// tx_cmd_test = _net1->n_txcmd;
	tx_cmd_test = __atomic_load_n((unsigned*)&(_net1->n_txcmd), memorder);

	
	// static volatile ENETPACKET *const _net1 = ((ENETPACKET *)0x00500000);  // Ethnet data struct pointer address
		// DISL version, static volatile ENETPACKET *const _net1 = ((ENETPACKET *)0x20500000) = 32'b0010 0000 0101 0000 0000 0000 0000 0000;
	/* 
	typedef	struct ENETPACKET_S {
		unsigned	n_rxcmd, n_txcmd;
		uint64_t	n_mac;	// 8 bytes unsigned or 64 bit
		unsigned	n_rxmiss, n_rxerr, n_rxcrc, n_txcol;
	} ENETPACKET;  // *_net1 

	*/

	{ // Set the MAC address (device)
		char *macp = (char *)&_net1->n_mac;  // VIP step here that I missed that &_net1->n_mac is a struct pointer and is being casted as a char pointer so that it can be increased in terms of bytes rather than size of stucts
		// hence macp[0] and macp[1] have a difference of 1 byte since it was cast to char* ptr
		// we are giving *macp pointer the address of _net1->n_mac and setting it byte by byte (by casting it as char*) 

		ETHERNET_MAC upper = DEFAULTMAC >> 32;  // typedef	uint64_t ETHERNET_MAC; #define	DEFAULTMAC	0xa25345b6fb5eul
		unsigned	upper32 = (unsigned) upper;

		//maacp[2] and macp[3] ignored in rtl
		// macp[1] = (upper32 >>  8) & 0x0ff;  // char* will point to a byte of data
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

	icmp_send_ping(host_ip);  // first go to arp.c and give the "ETHERNET_MAC	router_mac_addr;" variable a non zero value
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
		if (waiting_pkt == NULL) {  // make sure we don't have a packet waiting in line to be sent due to the hw tx pipeline being busy (FPGA ethernet module)

			icmp_send_ping(host_ip);
		}//}


		//if (pic & BUSPIC_NETTX) {  // in finished code we will have some sort of interrupt telling us that the packet transmission in FPGA hw has been completed, only then we will send this waiting packet
		if (waiting_pkt != NULL) {

			NET_PACKET *pkt = waiting_pkt;
			waiting_pkt = NULL; 
			tx_pkt(pkt);  //pkt.h

		}//}
		



	}


// This function is being used in pkt.c. The txpkt() looks at the FPGA ethernet module hw tx ctrl register and if it busy
	// sending tx packet, then txpkt() uses this function to hold the txpkt till 
// void	tx_busy(NET_PACKET *txpkt) {
// 	if (waiting_pkt == NULL) {
// 		// printf("TX-BUSY\n");
// 		waiting_pkt = txpkt;
// 	} else if (txpkt != waiting_pkt) {
// 		//txstr("Busy collision--deleting waiting packet\n");
// 		free_pkt(waiting_pkt);
// 		waiting_pkt = txpkt;
// 	}
// }


}
