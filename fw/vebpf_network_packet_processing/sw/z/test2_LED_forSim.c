#include "mmio.h"


#define frameWidth 320
#define frameHeight 240
#define bytesPerPixel 2
#define buffSize frameHeight*frameWidth*bytesPerPixel


void main(void){
int x;

while(1){

x = 2; // this software tells to read from __atomic_load_n((int*)GPIO, memorder) // memorder = 0 , address
write_led(x);

}




return;
}
