/*
  rom.h

  by Michael J. Fromberger <sting@linguist.dartmouth.edu>
  Copyright (C) 1999 The Trustees of Dartmouth College

  Routines for writing data to ROM images in memory, for tt2rom
  version 2.
 */

#ifndef _H_ROM_
#define _H_ROM_

#include <stdio.h>

typedef unsigned char	byte;
typedef unsigned int	address;

/* Record type codes */
#define DATA_REC		0
#define END_REC			1
#define OFFSET_REC		2
#define START_REC		3   /* not used here */

#define	CHUNK_SIZE		16  /* data written in chunks this big */

/* Compute two's complement checksum byte for an output record
     count   - number of bytes in data field
     addr    - source address for data storage
     offset  - offset value (for extended address records)
     data    - record data ('count' bytes of data)

   These quantities are those used to construct an output record
   for the Intel ROM programmer.
 */
byte compute_data_checksum(byte count, address addr, byte *data);
byte compute_offset_checksum(address offset);
byte compute_end_checksum(void);

/* Using 'addr' as a base address, possibly including "don't care"
   designators, write byte value 'val' to all the memory addresses
   indicated by the address itself.  The ROM image is presumed to have
   enough storage available for this to work.

   The 'addr' parameter is a string of '0', '1', and 'x' characters
   denoting the address to be considered.  An 'x' denotes a don't care
   value.  The 'alen' parameter is the length of the address, which is
   not presumed to be zero-terminated.
 */
void write_range(byte *rom, char *addr, int alen, byte val);

/* Dump a ROM image out in various formats:

     dump_raw()	  - raw bytes of the ROM, in binary
     dump_text()  - literal bytes of the ROM, human-readable
     dump_intel() - Intel ROM programmer format, with addresses

   The dump_intel() function accounts for ROM sizes larger than 64K by
   writing extended address records (OFFSET_REC) whenever appropriate
 */
void dump_raw(byte *rom, int rlen, FILE *ofp);
void dump_text(byte *rom, int rlen, FILE *ofp);
void dump_intel(byte *rom, int rlen, FILE *ofp);

#endif /* end _H_ROM_ */
