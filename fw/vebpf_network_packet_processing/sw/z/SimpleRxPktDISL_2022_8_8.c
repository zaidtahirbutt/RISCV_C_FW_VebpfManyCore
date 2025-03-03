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
// a
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

	
	NET_PACKET	*rcvd;
	int led2 = 2;

	while(1){


		if ((__atomic_load_n((unsigned*)&(_net1->n_rxcmd), memorder))  & ENET_RXAVAIL){ // ENET_RXAVAIL is 0100 0000 0000 0000 i.e., the 15th index idx[14] is 1 which is rx_bram_av?
			
			// We've received a packet
			rcvd = rx_pkt();			

			if (NULL != rcvd) {
					
				// Don't let the subsystem free this packet (yet)
				rcvd->p_usage_count++;

				// Turn on LED2 HIGH
				write_led(led2);

				// Now we can free the packet ourselves
				free_pkt(rcvd);

				break;  // for while loop not for switch statment as in prev version of the code

			}
		}
	}	

// return 0;

}
