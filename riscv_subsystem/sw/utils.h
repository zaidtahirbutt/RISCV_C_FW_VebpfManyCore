#include "../config/address_map.h"
#include <stdarg.h>
#include <_syslist.h>

#define 	memorder 0

const char digits[16] = {'0','1','2','3','4','5','6','7','8','9', 'A', 'B', 'C', 'D', 'E', 'F'};


void write_led(int val){
	__atomic_store_n((int*)GPIO, val, memorder);

}				  

int read_sw(){
	return (__atomic_load_n((int*)GPIO, memorder));

}	


void write_uart(int val){
	__atomic_store_n((int*)UART, val, memorder);

}				  

int read_uart(){
	return (__atomic_load_n((int*)UART, memorder));

}	


int read_timer(){
	return (__atomic_load_n((int*)TIMER, memorder));

}	


void write_config(int mem_id, int mem_loc, int val){
	int effective_address = (mem_id << 24) | (mem_loc << 2);
	__atomic_store_n((int*)(CONFIG+effective_address), val, memorder);

}	


void sleep(int microseconds){
	int start = read_timer();
	while ((read_timer()-start) < microseconds);
	return;
}

void prints (char* str){
	int i = 0;
	while ((int)str[i] != 0){
		write_uart(str[i]);
		i = i+1;
	}
	return;
}


void printi(int val){
	if (val == 0){
		write_uart('0');
		return;
	}

	if (val < 0){
		write_uart('-');
		val = val*-1;
	}
	int num [10];
	for (int i=0;i<10;i=i+1){
		num[i] = val - 10*(val/10); val = val/10;
	}
	int start = 0;

	for (int i=10;i>0;i=i-1){
		if (start)
			write_uart(digits[num[i-1]]);
		else if (num[i-1] > 0){
			write_uart(digits[num[i-1]]);
			start = 1;
		}
		else 
			write_uart(0);
		
	}
	return;
}

void done(){
	prints("!q!\n\r");
	return;
}


//https://stackoverflow.com/questions/46631410/how-to-write-custom-printf
void printf(char *c, ...)
{
    char *s;
    va_list lst;
    va_start(lst, c);
    while(*c != '\0')
    {
        if(*c != '%')
        {
            write_uart(*c);
            c++;
            continue;
        }

        c++;

        if(*c == '\0')
        {
            break;
        }

        switch(*c)
        {
            case 's': prints(va_arg(lst, char *)); break;
            case 'd': printi(va_arg(lst, int)); break;
        }
        c++;
    }
}

void putchar(int c){
	write_uart(c);
}


void puts(char* c){
	prints(c);
}

void *
_sbrk (incr)
     int incr;
{
   extern char   end; /* Set by linker.  */
   static char * heap_end;
   char *        prev_heap_end;

   if (heap_end == 0)
     heap_end = & end;

   prev_heap_end = heap_end;
   heap_end += incr;

   return (void *) prev_heap_end;
}