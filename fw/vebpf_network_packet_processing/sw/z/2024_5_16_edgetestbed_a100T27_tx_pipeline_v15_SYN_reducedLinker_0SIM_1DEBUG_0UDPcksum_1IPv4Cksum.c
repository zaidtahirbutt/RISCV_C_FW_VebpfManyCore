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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ***************** remember to adjust these constants in board.h as per network subsystem configs *****************************

// // Make this constant equal to 1 if you are generating hex file for simulation and 0 for synthesis
// #define SIMULATION_TESTING 1 // 0 //1 //0 //1 //0
// #define DEBUG 0 // 1 //0 //1 

// // this constant should be equal to TX_PKT_DESC_TABLE_DEPTH parameter from network subsystem
// #define DESC_TABLE_TX_DEPTH = 4
// #define BITS_REQ_FOR_DESC_TABLE_TX_DEPTH 2  // logBASE2(DESC_TABLE_TX_DEPTH)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// #define 	memorder 0
	// defined in board.h

// #define MAX_RX_PKT_MEM	65536  	   // 2^16
// #define MAX_RX_PKT_MEM	32768  // 2^15
// #define MAX_RX_PKT_MEM	16384  // 2^14

#define MAX_RX_PKT_MEM	4096  // 2^12

// #define MAX_RX_PKT_MEM	1024  // 2^10
	// Due to the following condition, there wasn't enough space for a 2nd rx pkt, the rxpkt to mem writing FSM did not move:
		// (total_mem_words_used_rx_pkts_reg + 512) < rx_pkt_alloc_mem_words_size)

// keep at least memory for 4 MAX_ETH_PKT_LEN tx_pkts
// #define MAX_TX_PKT_MEM	32768  // 2^15

// reducing tx memory for testing.. 1518 x 4 
#define MAX_TX_PKT_MEM	6072  
	

// this is the least amount of bytes we want available for a eth pkt
// #define MAX_ETH_PKT_LEN 1518
	// declared in board.h
	
#define ETH_HDR_LEN		14
// #define ETH_RX_PKT1_LEN_HARDCORDED 298 

// Make this constant equal to 1 if you are generating hex file for simulation 
// #define SIMULATION_TESTING 0
	// defining this in pkt.c as well

#define LOADING_RX_PKT_DATA 1

#define NOT_LOADING_RX_PKT_DATA 0

// #define EXPECECTED_CORRECT_EBPF_RESULT 2

#define EBPF_RESULT_DONT_CARE 1

uint32_t	destination_ip = DESTIP;
uint32_t	dest_udp_port = DEST_UDP_PORT;
uint32_t	src_udp_port = SRC_UDP_PORT;


//extern void	tx_busy(NET_PACKET *); 

// Acknowledge all interrupts, and shut all interrupt sources off
// #define	CLEARPIC	0x7fff7fff  // 1111111111111110111111111111111 // 1 1111 1111 1111 1101 1111 1111 1111
// // Turn off the master interrupt generation
// #define	DISABLEINTS	0x80000000  // 1000 0000 0000 0000 0000 0000 0000 0000

// #define	REPEATING_TIMER	0x80000000

// unsigned	heartbeats = 0, lasthello;

//////////////// notes for this c file: //////////////////////  

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

int	main(int argc, char **argv) {

	// ***vip!!!*** set this constant to 1 for simulation => #define SIMULATION_TESTING 0 //1 //0
	
	
	NET_PACKET	*rcvd;
	NET_PACKET	*txpkt;
	unsigned 		led2 = 2;
	
	// unsigned 		led3 = 4;	// means 0b100
	unsigned 		led3 = 4;	// should have been int since atomic load for led write function requires 
		
	unsigned 		led4 = 8;	// means 0b1000
	unsigned			sw3 = 4;
	unsigned 		sw3_value = 0;
	unsigned 	sw_value = 0;
	unsigned 	rx_pkt_eth_type;
	unsigned	rx_pkt_ip_sub_protocol;
	unsigned	rx_pkt_ip_src; 
	unsigned	rx_pkt_ip_dst; 
	unsigned	rx_pkt_udp_port_dst; 
	unsigned	rx_pkt_udp_port_src;

	// csr1 register: // csr1 gives rx pkt len and rx status words
	unsigned	net_csr1;

	// // csr for csr_tx1
	// unsigned	net_csr5_tx1;
	// unsigned	net_csr5_tx1_FULL, net_csr5_tx1_EMPTY;

	/*  Commenting out the whole rxpkt memory allocation 


	NET_PACKET	*rx_pkt_1;
	unsigned rxpkt_vebpf_dest, rxpkt_vebpf_valid, rxpkt_vebpf_error, rxpkt_vebpf_empty, rxpkt_vebpf_avail;
	
	// Allocating starting memory address and total memory to network subsystem for rxpkts below:

	// allocate a buffer with room to add 0-15 bytes to ensure 16-alignment 
    void* ptr = (void*)malloc(MAX_RX_PKT_MEM + 15); // do free this pointer when mem no longer required (have a condition in place to do that)

    // round up to multiple of 16: add 15 and then round down by masking 
    void *rx_pkt_start_mem_addr = (void*)(((long)ptr+15) & ~0x0F);
	    // had made an error here while using the "->" notation I assumed it would be giving the struct pointer member's address
	    // but the "->" operator dereferences it. See https://www.geeksforgeeks.org/arrow-operator-in-c-c-with-examples/
	    // So I need to add an "&"
	    	// I think its due to the fact that I didn't declare csr1-4 as ponters in the struct

    // write the 4 byte memory ptr address of this allocated memory (starting heap address cx its dynamic allocation) to network subsystem
    __atomic_store_n((unsigned*)&(_net_csrs->csr3), rx_pkt_start_mem_addr, memorder);  // rx_pkt_start_mem_addr ptr value is 32 bit for rv32

    // write the allocated memory size for rx pkts to the network subsystem alloc size register
    __atomic_store_n((unsigned*)&(_net_csrs->csr4), MAX_RX_PKT_MEM, memorder);

    */

    // Allocating starting memory address and total memory to network subsystem for TX_PKTs below:

	/* allocate a buffer with room to add 0-15 bytes to ensure 16-alignment */
    void* tx_pkt_mem_ptr = (void*)malloc(MAX_TX_PKT_MEM + 15); // do free this pointer when mem no longer required (have a condition in place to do that)

    /* round up to multiple of 16: add 15 and then round down by masking */
    void *tx_pkt_start_mem_addr = (void*)(((long)tx_pkt_mem_ptr+15) & ~0x0F);

    // calculate the upper limit of the mem address allocated for tx pkts 
    void *tx_pkt_upper_limit_mem_addr = (void*)(((long)tx_pkt_start_mem_addr + (MAX_TX_PKT_MEM - 1))); 
    	// converting the ptr to long for doing arithematic on it


    // assign initial total memory allocated for tx_pkts in memory to the variable tracking the avialble tx_pkt memory
    unsigned tx_pkt_current_available_memory = MAX_TX_PKT_MEM;

    // if we are dubugging on hardware or doing simulation, then we update the debug csr register
    if (DEBUG || SIMULATION_TESTING) {

    	// update the csr6_tx1 csr in network subsystem indicating the tx_pkt_current_available_memory
    	__atomic_store_n((unsigned*)&(_net_csrs->csr6_tx1), tx_pkt_current_available_memory, memorder);
    } 

    // memory in use by tx_pkts 
    unsigned tx_pkt_current_used_memory = 0;

    // current and previous wr_ptr and rd_ptr from network subsystem
    unsigned tx_pkt_prev_wr_ptr = 0;
    unsigned tx_pkt_curr_wr_ptr = 0;
    unsigned tx_pkt_prev_rd_ptr = 0;
    unsigned tx_pkt_curr_rd_ptr = 0;

    // initialize the tx_pkts_status_desc_table_tx_obj members to 0;
    for(unsigned i=0; i < DESC_TABLE_TX_DEPTH; i++) {

    	tx_pkts_status_desc_table_tx_obj.tx_pkt_active[i] = 0;
    	tx_pkts_status_desc_table_tx_obj.tx_pkt_len_bytes[i] = 0;
    	tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[i] = NULL;


    }

    // initializing the tx_pkt_current_start_mem_address of first bin of tx_pkts_status_desc_table_tx_obj to "tx_pkt_start_mem_addr"
    tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[0] = tx_pkt_start_mem_addr;


  	// if flag is 1 then it means there is enough space in memory and in desc_table_tx in network subsystem for sending out the tx_pkt
    int send_tx_pkt_flag = 0;
    	// return 1 for enough space to send a txpkt
    	// return 0 for NOT enough space to send a txpkt
    	// return -1 for an error condition


    // wait for sw-4 to go to 1 to start this program below
    if (!SIMULATION_TESTING) {
	
	    // waiting for sw_value to become 8
	    while(1) {
	    
		    write_led(1);
	    	sw_value = read_sw();

	    	if (!(SIMULATION_TESTING)) 
	    		printf("\nTurn sw4 HIGH to start transmitting txpkts from arty100T FPGA\n");

	    	// wait 2 seconds
	    	sleep(2000000);
	    		// puting sleep here so that we can connect the serial monitor and read the printed data
	    		// otherwise it is not possible to connect the serial monitor if the host pc UART port is 
	    		// busy reading the print statements without delay

		    if (sw_value == 8) {

		    	write_led(0);
		    	printf("\nTransmitting now!\n");
		    	break;

		    }
		
		}

	}
   
	int counter = 0;

	// keep on sending the tx_pkt being built below
	while(1) {

		if (SIMULATION_TESTING) {
			write_led(13);
			write_led(0);
			write_led(13);
			write_led(0);
		}
	    // Build the tx_pkt

	    // build a tx_pkt that has 4 bytes of UDP data pay_load available
	    	// total len = 1442 bytes (42 bytes of headers data)
	    // txpkt = new_udppkt_v2(4);   // using v2 function cx we are sending out src mac in the packet unlike the prev ver of this function
	    // txpkt = new_udppkt_v2(1000);   // using v2 function cx we are sending out src mac in the packet unlike the prev ver of this function
	    // txpkt = new_udppkt_v2(4);   
	    // txpkt = new_udppkt_v2(69);   
	    txpkt = new_udppkt_v2(71);   

	    // 4 bytes of tx_pkt udp data payload
	    txpkt->p_user[0] = 255;
	    txpkt->p_user[1] = 0; 
	    txpkt->p_user[2] = 255;
	    txpkt->p_user[3] = 0;

	    if (DEBUG) {

		printf("\nIn main func before sending to tx_pkt_memory_availability_calculation() tx_pkt_curr_wr_ptr = %d\n", tx_pkt_curr_wr_ptr);
		printf("\nIn main func before sending to tx_pkt_memory_availability_calculation() tx_pkt_curr_rd_ptr = %d\n", tx_pkt_curr_rd_ptr);
		printf("\nIn main func before sending to tx_pkt_memory_availability_calculation() &tx_pkt_curr_wr_ptr = %d\n", &tx_pkt_curr_wr_ptr);
		printf("\nIn main func before sending to tx_pkt_memory_availability_calculation() &tx_pkt_curr_rd_ptr = %d\n", &tx_pkt_curr_rd_ptr);
		
		}


	    // calculate if there is enough memory and space in network subsystem desc_table_tx fifo available to send a txpkt
	    send_tx_pkt_flag = tx_pkt_memory_availability_calculation(txpkt, &tx_pkt_curr_wr_ptr, &tx_pkt_prev_wr_ptr, 
	    															&tx_pkt_curr_rd_ptr, &tx_pkt_prev_rd_ptr, &tx_pkt_current_available_memory,
	    															&tx_pkts_status_desc_table_tx_obj, tx_pkt_upper_limit_mem_addr, tx_pkt_start_mem_addr);
	    if (SIMULATION_TESTING) {
		    write_led(9);
		    write_led(0);
		    write_led(9);
		    write_led(send_tx_pkt_flag);
		    write_led(11);
		    write_led(0);
		    write_led(11);
		    write_led(0);
		}


	    if (send_tx_pkt_flag == 1) {

	    	if (DEBUG)
	    		printf("\n\n\n\n **** Entered if (send_tx_pkt_flag == 1) and now sending txpkt **** \n\n\n\n");

		    // send the tx_pkt if there is enough memory and space in network subsystem desc_table_tx
		    tx_udp_v2(txpkt, DESTIP, SRC_UDP_PORT, DEST_UDP_PORT, 
		    			tx_pkts_status_desc_table_tx_obj.tx_pkt_current_start_mem_address[tx_pkt_curr_wr_ptr]);

		    if (DEBUG)
				printf("\n\n\n****** Exited tx_udp_v2() ******\n\n\n");	

		    if (SIMULATION_TESTING) 
			    write_led(15);
		
		}
		else if (send_tx_pkt_flag == -1) {

			// means there is en error, exit the program with error msg
			if (DEBUG) {
				printf("\nError msg -1 was returned from  tx_pkt_memory_availability_calculation(), exiting program with led = 14\n");
			}

			write_led(14);
			// exit main
			return -1;

		} 
		// else if there is no space then we send the tx_pkt to waiting tx_pkt
		// else if there is no space then we clear the tx_pkt using free_pkt()	
		else {

			if (DEBUG)
	    		printf("\n\n\n\n **** Entered else if there is no space then we clear the tx_pkt using free_pkt() **** \n\n\n\n");

			// alot txpkt to waiting_pkt 
			// tx_busy(txpkt);
	
			// if you don't wana do tx_busy then free the memory
			free_pkt(txpkt);

			if (SIMULATION_TESTING)
				write_led(13);

		}

		if (SIMULATION_TESTING) 
			write_led(0);

		// setting the flag back to its default value of 0 again
		send_tx_pkt_flag = 0;

		if (SIMULATION_TESTING) {
			write_led(14);
			write_led(0);
			write_led(14);
			write_led(0);
		}

		if (!SIMULATION_TESTING) {

			write_led(15);
			printf("\nWaiting 0.02 seconds before sending next tx_pkt!\n");
			sleep(20000);
			write_led(0);


		}

		if (!SIMULATION_TESTING) {

			counter = counter + 1; 


			// once 10 txpkts are sent break while loop 
			if (counter >= 10) {

				// keep led13 ON indicating 10 tpkts have been sent
				write_led(13);

				// break;
				while (1) {


					printf("\nSent 10 txpkts.. Waiting for sw to become > 0 with 2 sec delay in prints of this msg\n");
					sleep(2000000);

					counter = 0;

					sw_value = read_sw();

					if (sw_value > 0) {

						printf("\nsw must be > 0 and led must be 0 for 2 sec before next 10 txpkts are sent\n");
						write_led(0);
						sleep(2000000);
						break;
					}

				}

			}

		}

	}

	// keep led13 ON indicating 10 tpkts have been sent
	// write_led(13);

}

