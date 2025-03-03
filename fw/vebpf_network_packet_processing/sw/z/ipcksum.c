////////////////////////////////////////////////////////////////////////////////
//
// Filename: 	ipsum.c
//
// Project:	ZipVersa, Versa Brd implementation using ZipCPU infrastructure
//
// Purpose:	To calculate (and return) an IP checksum on a section of data.
//		The data must be contiguous in memory, and the checksum field
//	(which is usually a part of it) must be blank when calling this
//	function.
//
// Creator:	Dan Gisselquist, Ph.D.
//		Gisselquist Technology, LLC
//
////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2019, Gisselquist Technology, LLC
//
// This program is free software (firmware): you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTIBILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  (It's in the $(ROOT)/doc directory.  Run make with no
// target there if the PDF file isn't present.)  If not, see
// <http://www.gnu.org/licenses/> for a copy.
//
// License:	GPL, v3, as defined and found on www.gnu.org,
//		http://www.gnu.org/licenses/gpl.html
//
//
////////////////////////////////////////////////////////////////////////////////
//
//
#include "ipcksum.h"

unsigned	ipcksum(int len, char *ptr) {
	unsigned	checksum = 0;
	unsigned char	*ucp = (unsigned char *)ptr;

	for(int i=0; i<len; i+=2) {
		checksum = checksum + ((ucp[i] & 0x0ff)<<8)
			+ (ucp[i+1] & 0x0ff);
	}
	while(checksum & ~0x0ffff)
		checksum = (checksum & 0x0ffff) + (checksum >> 16);
	return checksum ^ 0x0ffff;
}

/*

A checksum is a value that represents the number of bits in a transmission message and is used by IT professionals to detect high-level errors within data transmissions. Prior to transmission, every piece of data or file can be assigned a checksum value after running a cryptographic hash function.
Checksum is an error detection method.

Error detection using checksum method involves the following steps-
https://www.gatevidyalay.com/checksum-checksum-example-error-detection/

*/