#define 	GPIO 		0x10000000	// these addresses seem 4 bytes apart. is there a reason for tha?
// Does there have to be memory alignment by word here as well?

#define 	UART		0x10000018 
#define 	I2C		0x1000001C  
#define 	SPI		0x1000041C  
#define 	CL		0x1000081C  

#define memorder 0

int _c2i(int a, int b, int c, int d){
	int _a;
	_a = a << 8;
	_a = _a | b;
	_a = _a << 8;
	_a = _a | c;
	_a = _a << 8;
	_a = _a | d;
	return _a;
}


int _i2c(int a, int index){
	int _a, _b, _c, _d;
	_a = a;
	if (index == 0)
		return _a;
	_b = _a >> 8;
	if (index == 1)
		return _b;
	_c = _b >> 8;
	if (index == 2)
		return _c;
	_d = _c >> 8;
	return _d;
}



int cl(int a, int b){
	__atomic_store_n((int*)(CL + 0x10), a, memorder);
	__atomic_store_n((int*)(CL + 0x18), b, memorder);
	__atomic_store_n((int*)CL , 1, memorder);
	int done = (__atomic_load_n((int*)CL, memorder) >> 1) & 1;  
	while (!done)
		done = (__atomic_load_n((int*)CL, memorder) >> 1) & 1;  // the circle 
	// barckets return the boolean true or false, if any num other than 0 is returned
	// we get a true  i.e., a 1 here.


	return __atomic_load_n((int*)(CL + 0x20), memorder);
}				  



void write_led(int val){
	__atomic_store_n((int*)GPIO, val, memorder);
	//*((int*)GPIO) = val; // here we have atomic storee instead of load and we are storing a data value at this pointer address above, while avoiding the deletions due to compiler optimizations as stated below
}				  

int read_sw(){
	return (__atomic_load_n((int*)GPIO, memorder));
	//return (*((int*) GPIO));  // as per this, the value of variable pointed to by the address pointer int* GPIO, normally these kinds of commands would have been cut out/optimized out by the compiler, but using atomic load, these are not optimized out. So read store word command sends GPIO address to be read from, so the riscv puts this address on the axi cross bar which transferout to axi_araddr which then goes to gpio module, then the switch states is readout in axi_rdata and transferred out to axi cross bar from there it is transferred out to riscv axi rdata and read by the riscv processor
}	


void write_uart(int val){
	__atomic_store_n((int*)UART, val, memorder);
	//*((int*)UART) = val;
}				  

int read_uart(){
	return (__atomic_load_n((int*)UART, memorder));
	//return (*((int*) UART));
}	


void write_i2c(int index, int val){
	__atomic_store_n((int*)I2C + index, val, memorder);
	//*((int*)I2C + index) = val;
}				  

int read_i2c(int index){
	return (__atomic_load_n((int*)I2C + index, memorder));
	//return (*((int*) I2C + index));
}	


void write_spi(int index, int val){
	__atomic_store_n((int*)SPI + index, val, memorder);
	//*((int*)SPI + index) = val;
}				  

int read_spi(int index){
	return (__atomic_load_n((int*)SPI + index, memorder));
	//return (*((int*) SPI + index));
}	
