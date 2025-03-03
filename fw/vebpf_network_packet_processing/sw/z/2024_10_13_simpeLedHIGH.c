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
#include "../../../../riscv_subsystem/sw/utils.h" // include just once!
#define 	memorder 0

// #define MAX_RX_PKT_MEM	210000000  // almost = 210 MB .. keeping in mind arty100t has total DDR3 mem = 256 MBs
// #define MAX_RX_PKT_MEM	32768  // 2^15  = 32 kB
// #define MAX_RX_PKT_MEM	16384  // 2^14

#define MAX_RX_PKT_MEM	8192  // 2^13
// #define MAX_RX_PKT_MEM	4096  // 2^12

// #define MAX_RX_PKT_MEM	1024  // 2^10
	// Due to the following condition, there wasn't enough space for a 2nd rx pkt, the rxpkt to mem writing FSM did not move:
		// (total_mem_words_used_rx_pkts_reg + 512) < rx_pkt_alloc_mem_words_size)

#define ETH_HDR_LEN		14
// #define ETH_RX_PKT1_LEN_HARDCORDED 298 

// Make this constant equal to 1 if you are generating hex file for simulation 
// #define SIMULATION_TESTING 0
	// defining this in pkt.c as well

#define LOADING_RX_PKT_DATA 1

#define NOT_LOADING_RX_PKT_DATA 0

// #define EXPECECTED_CORRECT_EBPF_RESULT 2

#define EBPF_RESULT_DONT_CARE 1
#define TOTAL_RXPKTS_IN_EXPERIMENT_OF_FIREWALL_TYPE1 8000
#define TOTAL_RXPKTS_IN_EXPERIMENT_OF_FIREWALL_TYPE2 18000
#define TOTAL_RXPKTS_IN_EXPERIMENT_OF_FIREWALL_TYPE3 8000
#define TOTAL_RXPKTS_IN_EXPERIMENT_OF_FIREWALL_TYPE4 34000

#define IGNORE_INITIAL_FIREWALL_MALICIOUS_RXPKTS 20 //4
#define TOTAL_MALICIOUS_RXPKTS_FOR_LATENCY_CAL 8000  // for TYPE1 firewall Experiments // 1100 // 5


NET_PACKET	*waiting_pkt = NULL;

TX_PKTS_DESC_TABLE_TX_STATUS tx_pkts_status_desc_table_tx_obj;

// in tx_busy() we are just alotting waiting_pkt with the tx_pkt ... in our main() we need code that keeps checking if
// the waiting_pkt is not NULL, then try to send that waiting_pkt out.. and since this is udp, we will drop any previous
// waiting_pkt when tx_busy() is called from tx_pkt()... I'll use the same methodolgy in my code..
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



int main (int argc, char **argv) {

	unsigned sw_value = 0;

	// wait for sw-4 to go to 1 to start this program below
    if (!SIMULATION_TESTING) {
	
	    // waiting for sw_value to become 8
	    while(1) {
	    
		    write_led(1);
	    	sw_value = read_sw();

    		printf("\nTurn sw4 HIGH to start reading received rxpkts in the arty100T FPGA\n");

	    	// wait 2 seconds
	    	sleep(2000000);
	    		// puting sleep here so that we can connect the serial monitor and read the printed data
	    		// otherwise it is not possible to connect the serial monitor if the host pc UART port is 
	    		// busy reading the print statements without delay

		    if (sw_value == 8) {

		    	write_led(0);
		    	break;

		    }
		
		}

	} 



	while (1) {


		write_led(3);
		printf("\nHELLO WORLD\n");
		sleep(500000);
		write_led(0);
		printf("\nHELLO WORLD\n");
		sleep(500000);


	}




}