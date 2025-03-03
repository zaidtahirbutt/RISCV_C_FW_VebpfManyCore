import serial
import time
import sys

print("This is the name of the program:", sys.argv[0])

print("Argument List:", str(sys.argv))

# default baud_rate as per prev projected from 8 Oct 2024
    # 921600 baudrate for other projects
baud_rate = 1000000
# baud_rate = 1204820  # worked when sys clk was 100 MHz! and CLKs per Bit = 83 !
# baud_rate = 921600

if (baud_rate != 1000000):
    print(f"Warning, the default value of baud_rate has been changed to {baud_rate} from value of 1000000")
else:
    print(f"Default value of baud_rate = {baud_rate} is being used")

if (len(sys.argv) == 1):
    ser = serial.Serial('/dev/ttyUSB1', baud_rate)  # 0 is being used as JTAG to upload bit file and only 1 is avail to be used as UART
    # ser = serial.Serial('/dev/ttyUSB0', 1000000)
    # ser = serial.Serial('/dev/ttyUSB3', 1000000)
else:
    print(f"type(sys.argv[1]) = {type(sys.argv[1])}")
    print(f"type(sys.argv[2]) = {type(sys.argv[2])}")
    print(f"type(baud_rate) = {type(baud_rate)}")
    print(f"type(int(sys.argv[2])) = {type(int(sys.argv[2]))}")
    device = "/dev/" + sys.argv[1]		# I can provide an input USB number that I want
    baud_rate = int(sys.argv[2])
    ser = serial.Serial(device, baud_rate)


program = []
address = []
counter = 0;

# Insert hex file name for uploading RISCV inst_mem to FPGA UART
with open('firmware.hex') as file:
    for line in file:
        #print(line) # Prints each while line separately
        if "@" in line: 
            counter = int(line.split('@')[1],16)    # there is one element on left of @ (an empty set) in the list of items returned by spilliting at all @ signs then 
            # print(line.split('@'))  #['', '00000000\n']
            # print(line.split('@')[1])  #00000000 (since first element is [0] and it is empty set, whereas [1] element is 000000)
            # print(int(line.split('@')[1], 16))  #0
            # print(int("FF", base=16))  #255 # int() converts the input string whose base we mention, into decimal integer

            #print(counter)  # Prints 0 
        else:
            nl_rm_line =  line.split('\n')[0];  
            nl_rm_line =  nl_rm_line.split(' ');
            #print(nl_rm_line)  # ['37', '01', '00', '10', 'EF', '00', '00', '13', 'EF', '00', '00', '00', '13', '15', '85', '00']
            #sys.exit()   
            if len(nl_rm_line) == 0: continue
            words = int(len(nl_rm_line)/4)  # words and their multiples in each line of hex file then :3
            #print(words)    # 4
            #print(list(range(words)))   # [0, 1, 2, 3] => from range(0, 4)
            #sys.exit()
            for i in range(words):  # we can use the same for loop method with lists
                program.append(nl_rm_line[4*i+3] + nl_rm_line[4*i+2] + nl_rm_line[4*i+1] + nl_rm_line[4*i]) # This means
                
                # first four bytes of hex are -> 37 01 00 10
                #print(program)  #'10000137']  # this is true wrt to program appeend
                #sys.exit()

                # Little Endian Style for each word (i.e., LSB in each word is stored at the lowest data location, the bottom of the barrel)
                address.append(counter)  # for first words our address is 0x00 since it is appended first here before addition
                counter = counter + 4;  # So the counter officially starts at 0x00
                # Each address loc represents one byte, we do plus 4 to point to the next 4 four bytes
                # and memory is accessed in terms of words, so PC is incremented by 4 each time. Byte offset of 2 is used to access 
                # the bytes within the word.                
            #print(counter)  # 16
            #sys.exit()
            #print(program)  # ['10000137', '130000EF', '000000EF', '00851513']
            #sys.exit()
    #print(program)
    #sys.exit()

#print(len(program)) # 92   # each program element is 4 bytes so total bytes is 92 x 4 = 368
#sys.exit()
#print(counter)  # 368
#sys.exit()
#y = 0
            
for i in range(len(program)):
    x = address[i]  # is 0x00 for the first word of instruction 
    x1 = (x&255).to_bytes(1, byteorder='big')   # x&(255=0b11111111) setting other bits other than the 8 LSbs to zero
    
    #print(x1)  # b'\x00' for first loop
    #sys.exit()

    # so the prog counter will be 
    
    #y = y + 1
    
    # if (y == 2):
    #     print(x1)  # b'\x04' 
    #     sys.exit()

    x2 = ((x>>8)&255).to_bytes(1, byteorder='big')

    #print(x2)   # b'\x00' for first loop
    #sys.exit()

    # if (y == 2):
    #     print(x1)  # b'\x04'
    #     sys.exit()

    x3 = ((x>>16)&255).to_bytes(1, byteorder='big')

    # if (y == 2):
    #     print(x1)  # b'\x04'
    #     sys.exit()

    x4 = ((x>>24)&255).to_bytes(1, byteorder='big')
    # each program element is 4 bytes so total bytes is 92 x 4 = 368
    # Each program element is stored as little endian for the bytes

    # with each program element first 4 bytes for its PC address 
    # are sent in little endian (wrt byte) style i.e., first LSB byte of the
    # PC (instruction address) is sent first on the UART till the 4th byte of the PC instruction address

    ser.write(x1)   # The first 4 bytes sent are address bytes
    # PC isnt included in the firmware.hex file, just its start @00000000 is included, so we count 
    # the bytes after it and sent the PC values along with the instructions for the even and odd indices respectively
    time.sleep(0.001)
    ser.write(x2)
    time.sleep(0.001)
    ser.write(x3)
    time.sleep(0.001)
    ser.write(x4)
    time.sleep(0.001)

    #program[i] has 4 bytes of instruction data which is pointed to by the PC (each byte is pointed to by it and PC is incremented by 4 to point to the next 4 bytes)

    x = int(program[i],16)    # print(int("FF", base=16))  #255 # int() converts the input string whose base we mention, into decimal integer
    
    #print(nl_rm_line)  # ['37', '01', '00', '10', 'EF', '00', '00', '13', 'EF', '00', '00', '00', '13', '15', '85', '00']
    # THIS ABOVE IS NOT SENT, the little endian style below is what is sent

    #print(program)  #'10000137']  # this is true wrt to program appeend

    # print(x)  # 268,435,767 (integer -> decimal) which is 1000 0137 in hex which is little endian of hex file first 4 bytes
    # sys.exit()

    x1 = (x&255).to_bytes(1, byteorder='big')
    # SOLVED this issue. I was getting b'7' and in ASCII 7 means 110111 in binary which is 0x37 in HEX
    # but why was it showing it in ASCII yamero

    # print(x&255)    #55 -> 0x37
    # sys.exit()

    #y = (55).to_bytes(1, byteorder='big')

    # print(y)    #b'7' 
    # print(y.hex())  #37
    # sys.exit()

    #x1 = (x&255).to_bytes(2, byteorder='big')   #b'\x007'

    # print(x1)   # b'7'  #b'\x007'
    # sys.exit()

    #print(bin(x1))  # TypeError: 'bytes' object cannot be interpreted as an integer
    #print(int(x1))  # 7
    #sys.exit()

    x2 = ((x>>8)&255).to_bytes(1, byteorder='big')
    x3 = ((x>>16)&255).to_bytes(1, byteorder='big')
    x4 = ((x>>24)&255).to_bytes(1, byteorder='big')

    # print(x1)   # b'7'
    # SOLVED this issue. I was getting b'7' and in ASCII 7 means 110111 in binary which is 0x37 in HEX
    # but why was it showing it in ASCII yamero

    # sys.exit()

    ser.write(x1)
    time.sleep(0.001)

    # print(x2)   # b'\x01'
    # sys.exit()

    ser.write(x2)
    time.sleep(0.001)
    
    # print(x3)   # b'\x00'
    # sys.exit()

    ser.write(x3)
    time.sleep(0.001)

    # print(x4)   # b'\x10'
    # sys.exit()

    # first 4 bytes in hex are 37 01 00 10 
    # So first 4 bytes are send in Big Endian byte style for the 4 bytes set for each PC

    ser.write(x4)
    time.sleep(0.001)

print ("done")
