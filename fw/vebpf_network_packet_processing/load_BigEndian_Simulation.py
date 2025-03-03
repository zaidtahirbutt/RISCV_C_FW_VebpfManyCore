# this script converts RISCV pgm_mem hex file to a sim hex file for the COCOTB simulator to simulate the C code for softcore RISCV for FPGA in the opensource simulator (icarus verilog + cocotb)

import serial
import time
import sys
import binascii

# if (len(sys.argv) == 1):  # meaning no ttyUSB arg was given and its just python3 load.py
# 	ser = serial.Serial('/dev/ttyUSB1', 1000000)
# else:
# 	device = "/dev/" + sys.argv[1]		# I can provide an input USB number that I want
# 	ser = serial.Serial(device, 1000000)


program = []
address = []
counter = 0

# Insert name of file to convert to sim file here:
with open('firmware.hex') as file:
    # **** VIP!!! REMEMBER TO CHANGE BOTH NAME OF INPUT HEX FILE AND OUTPUT HEX SIM FILE ****
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
            nl_rm_line =  line.split('\n')[0];  # https://www.w3schools.com/python/ref_string_split.asp
            # This upper line means split the input string on basis of next line character
            # so the data is taken in as string
            #print("hello \n",nl_rm_line) 
            #hello 
            #37 01 00 10 EF 00 00 13 EF 00 00 00 13 15 85 00
            nl_rm_line =  nl_rm_line.split(' ');
            # This means for each line, separate out the strings that have a space in between them
            #print(nl_rm_line)  # ['37', '01', '00', '10', 'EF', '00', '00', '13', 'EF', '00', '00', '00', '13', '15', '85', '00']
            #sys.exit()   
            if len(nl_rm_line) == 0: continue
            words = int(len(nl_rm_line)/4)  # words and their multiples in each line of hex file then :3
            #print(words)    # 4
            #print(list(range(words)))   # [0, 1, 2, 3] => from range(0, 4)
            #sys.exit()
            for i in range(words):  # we can use the same for loop method with lists
                program.append(nl_rm_line[4*i+3] + nl_rm_line[4*i+2] + nl_rm_line[4*i+1] + nl_rm_line[4*i]) # This means
                #byte 0 is the first byte in the hex file, it is little endian there :3
                # first four bytes of hex are -> 37 01 00 10
                #print(program)  #['10000137']  # this is true wrt to program appeend
                #print(program[0])  #10000137
                # print(program[i])
                    # 10000137
                    # 130000EF
                    # 000000EF
                    # 00851513

                # if(i == 3): sys.exit()

                # so right now the words written in program are in form of string :3
                # with open('firmware_for_sim.hex', 'wb') as f:
                #     #f.write(program[i]) # error TypeError: a bytes-like object is required, not 'str'
                #     #f.write(bytes(program[i])) # error TypeError: a bytes-like object is required, not 'str'
                #     #f.write(hex(program[i])) # error TypeError: a bytes-like object is required, not 'str'
                #     #f.write(binascii.unhexlify(program[i])) # no error
                #     #f.write("text to write\n")
                #     f.write("\n")
                #     f.close()

                # with open('readme12.txt', 'w') as f:
                #     # f.write('readme') #"works"
                #     f.write("readme") # same as above
                # sys.exit("Bye :3")       

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
    # print(program)  # full program is printed with words as elements of the list
    # sys.exit()

 # name of output sim file
with open('firmware_sim.hex', 'w') as f:  
    for word in program:
        f.write(word)
        f.write("\n")
        # so I guess readmemh reads text from the hex file.. thats why we are writing the pgm mem
        # as a string through f.write and not converting the word to int before writing

        # f.write(int(word[6]))
            #     f.write(int(word[6]))
            # TypeError: write() argument must be str, not int
        # I will have to use list append with line.split to get only the hex numbers for ebpf simulations 
# was incorrectly doing # with open('firmware_for_sim.hex', 'wb') as f: before            
        

# last two bytes aren't written in the hex file since they are just two bytes i.e., not multiple of words, so extra two bytes left.
# i'll check that in the code below if that is the case

#print(len(program)) # 92   # each program element is 4 words so total words is 92 x 4 = 368 words = 368 x 4 bytes = 1472 bytes
#sys.exit()
#print(counter)  # 368
#sys.exit()
#y = 0

z1 = 0
            
for i in range(len(program)):  # len program is the total words in the program 
    x = address[i]  # is 0x00 for the first word of instruction 
    x1 = (x&255).to_bytes(1, byteorder='big')   # x&(255=0b11111111) setting other bits other than the 8 LSbs to zero
    
    #print(x1)  # b'\x00' for first loop
    #sys.exit()

    # so the prog counter will be 
    
    #y = y + 1
    
    # if (y == 2):
    #     print(x1)  # b'\x04' # as address has incremented by 4 :3
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

    # ser.write(x1)   # The first 4 bytes sent are address bytes
    # # PC isnt included in the firmware.hex file, just its start @00000000 is included, so we count 
    # # the bytes after it and sent the PC values along with the instructions for the even and odd indices respectively
    # time.sleep(0.001)
    # ser.write(x2)
    # time.sleep(0.001)
    # ser.write(x3)
    # time.sleep(0.001)
    # ser.write(x4)
    time.sleep(0.001)

    #program[i] has 4 bytes of instruction data which is pointed to by the PC (each byte is pointed to by it and PC is incremented by 4 to point to the next 4 bytes)

    x = int(program[i],16)    # print(int("FF", base=16))  #255 # int() converts the input string whose base we mention, into decimal integer
    #https://www.programiz.com/python-programming/methods/built-in/int
        # VVVIIPPPP, hex string is being convereted to int


    #print(nl_rm_line)  # ['37', '01', '00', '10', 'EF', '00', '00', '13', 'EF', '00', '00', '00', '13', '15', '85', '00']
    # THIS ABOVE IS NOT SENT, the little endian style below is what is sent

    #print(program)  #'10000137']  # this is true wrt to program appeend

    # print(x)  # 268,435,767 (integer -> decimal) which is 1000 0137 in hex which is little endian of hex file first 4 bytes
    # sys.exit()

    x1 = (x&255).to_bytes(1, byteorder='big') #Return an array of bytes representing an integer.
    #https://docs.python.org/3/library/stdtypes.html
    # x1 is LSB in the word and this is little endian style

    # SOLVED this issue. I was getting b'7' and in ASCII 7 means 110111 in binary which is 0x37 in HEX
    # but why was it showing it in ASCII yamero BECAUSE  # since to_bytes need an int and not a hex

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
    # with open('firmware_for_sim.hex', 'wb') as f:
    #     f.write(x1) # error TypeError: a bytes-like object is required, not 'str'
    #     f.write("\n")
    # so when writing a hex file, I will write in this order, x4, x3, x2, x1 \n
    #x1 = int(x1) # is giving 7 as string version of 55 int or 37 hex is 7 :3
    #x1 = int(x1.hex())  # retruns 37 :3 should return 55 for int, because default base is 10 
    x1 = int(x1.hex(),16)  # retruns 55 which is the correct value for int of 0x37 hex
    #x1 = int(x1,16)
    #x1 = int(x1,2)
    #print(x1) # is returning 7 while it should return 55 for hex of 37 :3
    #sys.exit("\n x1 testing")
    x2 = int(x2.hex(),16)  # retruns 55 which is the correct value for int of 0x37 hex
    x3 = int(x3.hex(),16)  # retruns 55 which is the correct value for int of 0x37 hex
    x4 = int(x4.hex(),16)  # retruns 55 which is the correct value for int of 0x37 hex

    x_combined = (x4<<24) | (x3<<16) | (x2<<8) | x1  # https://www.geeksforgeeks.org/python-bitwise-operators/ ONLY RUNS on ints
    # This step has already been done in the for loop at the start of this program but Im implementing this
    # to gain further clarity on converting one datatype to another in python
    # with open('firmware_for_sim.hex', 'wb') as f:
    #     f.write(x_combined) # error TypeError: a bytes-like object is required, not 'str'
    #     f.write("\n")


    #x_combined = hex((x4<<24) | (x3<<16) | (x2<<8) | x1)
    #print(x_combined.hex())  #int has not attrib hex

    #print("x_combined is %d \n",x_combined)  # output is 268435767 which in hex is 0x10000137 which is the correct sequence of big endian style machine code we need for our edgetestbed simulation
    #print("x_combined is = ",format(x_combined,"08x"))  # output is 268435767 which in hex is 0x10000137 which is the correct sequence of big endian style machine code we need for our edgetestbed simulation
    #print("x_combined is = ",format(x_combined,"08X"))  # output is 268435767 which in hex is 0x10000137 which is the correct sequence of big endian style machine code we need for our edgetestbed simulation
        # x_combined is =  10000137
        # x_combined is =  130000EF
        # x_combined is =  000000EF
        # x_combined is =  00851513


    #print("x_combined is = ",hex(x_combined))  # output is 268435767 which in hex is 0x10000137 which is the correct sequence of big endian style machine code we need for our edgetestbed simulation
    #x_combined is =  0x10000137
    # x_combined is =  0x130000ef 
   
    # x_combined is =  0xef  # might be a problem here if 0s arent print at the start of the word
   
    # x_combined is =  0x851513


    z1 = z1 + 1
    # if(z1 == 4): 
    #     sys.exit("\n testing done")
    # print(x1)   # b'7'
    # SOLVED this issue. I was getting b'7' and in ASCII 7 means 110111 in binary which is 0x37 in HEX
    # but why was it showing it in ASCII yamero

    # sys.exit()

    # ser.write(x1)
    # time.sleep(0.001)

    # # print(x2)   # b'\x01'
    # # sys.exit()

    # ser.write(x2)
    # time.sleep(0.001)
    
    # # print(x3)   # b'\x00'
    # # sys.exit()

    # ser.write(x3)
    # time.sleep(0.001)

    # # print(x4)   # b'\x10'
    # # sys.exit()

    # # first 4 bytes in hex are 37 01 00 10 
    # # So first 4 bytes are send in Big Endian byte style for the 4 bytes set for each PC

    # ser.write(x4)
    time.sleep(0.001)

print("x_combined is = ",format(x_combined,"08X"))
#x_combined is =  70326932 , so here the last two extra bytes that werent multiple of 4 bytes i.e., word were left out as well.
print ("done")
