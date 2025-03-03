#include "mmio.h"
//#include<string.h>




void main(void){

//char uart_output[6] = "Hello";
//char uart_output[6] = "Hello";
// char uart_output[5] = {'H','e','l','l','o'};
int x;

//int numz[5] = {1, 2, 3, 4, 5};
int numz[5] = {9, 8, 7, 6, 5};
//int numz[5] = {91, 92, 93, 94, 95};
// int size_numz = sizeof(numz)/sizeof(numz[0]); // finding total elements in array numz
char holder;

while(1){

	// write_uart(7);
	// write_uart('z');
	x = read_sw(); // this software tells to read from __atomic_load_n((int*)GPIO, memorder) // memorder = 0 , address
	write_led(x);  //write the switch states to leds
	if(x>0)	{
// 		write_uart(112);

// 		//for(int i = 0; i < strlen(uart_output); i++){
// 		//for(int i = 0; i < 6; i++){
// 		// for(int i = 0; i < 5; i++){
// 		// 	// strlen gives exact num of chars in string as it 
// 		// 	// omits the null char \0 at the end whereas
// 		// 	//sizeof does not omit that

// 		// 	write_uart(uart_output[i]);

// 		// }

// 		 // for(int i = 0; i < size_numz; i++){
		 for(int i = 0; i < 5; i++){
        	
        	//write_uart(numz[i]);
        	// write_uart('z'); // works
        	holder = numz[i] + '0';
        	write_uart(holder);
    	}
		

	}
		



 }




return;
}
