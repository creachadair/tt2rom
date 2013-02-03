/*
  rom.c

  by Michael J. Fromberger <sting@linguist.dartmouth.edu>
  Copyright (C) 1999 The Trustees of Dartmouth College

  Routines for writing data to ROM images in memory, for tt2rom version 2.
 */

#include "rom.h"

#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

/* Get bits 16-19 out of the given address, and left-justify */
#define SEGMENT(A) (((A >> 16) & 0xF) << 12)

/* Write individual records out to a file */
static void write_data_record(byte *data, int len, address addr, FILE *ofp);
static void write_offset_record(address offset, FILE *ofp);
static void write_end_record(FILE *ofp);

/* The checksum used by the Intel ROM programmer is the two's
   complement of the sum of the bytes of the data being checked.
 */
byte compute_data_checksum(byte count, address addr, byte *data)
{
  int ix;
  unsigned int sum = count; /* sum includes record size */
  byte out;

  /* Include address field and record type in checksum */
  sum += (addr >> CHAR_BIT) & UCHAR_MAX;
  sum += addr & UCHAR_MAX;
  sum += DATA_REC;

  /* Include data field in checksum */
  for(ix = 0; ix < count; ix++)
    sum += data[ix];

  out = sum & UCHAR_MAX;  /* strip low-order byte  */
#ifdef DEBUG
  fprintf(stderr, "compute_data_checksum: sum = %X, out = %02X\n",
          sum, ~out + 1);
#endif

  return ~out + 1;        /* take two's complement */

} /* end of compute_data_checksum() */


byte compute_offset_checksum(address offset)
{
  unsigned int sum = 0;
  byte out;

  sum += 2; /* size of offset record */
  sum += (offset >> CHAR_BIT) & UCHAR_MAX;
  sum += offset & UCHAR_MAX;
  sum += OFFSET_REC;

  out = sum & UCHAR_MAX;
#ifdef DEBUG
  fprintf(stderr, "compute_offset_checksum: sum = %X, out = %02X\n",
          sum, ~out + 1);
#endif

  return ~out + 1;

} /* end of compute_offset_checksum() */


byte compute_end_checksum(void)
{
  return ~END_REC + 1;
}


/* Using 'addr' as a base address, possibly including "don't care"
   designators, write byte value 'val' to all the memory addresses
   indicated by the address itself.  The ROM image is presumed to have
   enough storage available for this to work.

   The 'addr' parameter is a string of '0', '1', and 'x' characters
   denoting the address to be considered.  An 'x' denotes a don't care
   value.  The 'alen' parameter is the length of the address, which is
   not presumed to be zero-terminated.

   This algorithm stolen from the original tt2rom by Anthony Edwards
 */
void write_range(byte *rom, char *addr, int alen, byte val)
{
  char *wild;  /* array indicating positions of don't-care bits */
  int ix, jx, kx, count = 0;
  address base = 0;

  if((wild = calloc(alen, sizeof(char))) == NULL)
    return;

  /* Construct base address with all don't-care bits set to zero */
  for(ix = 0; ix < alen; ix++) {
    if(tolower(addr[ix]) == 'x') {
      wild[ix] = 1;
      ++count; /* keep count of # of don't-care bits */
    }

    base <<= 1;
    if(addr[ix] == '1')
      base |= 1;
  }

  /* Now iterate over all possible combinations of don't-care bits.
     The outer loop acts as a binary counter, and the inner loop
     constructs an offset by moving the bits of this counter to the
     locations corresponding to the positions of the don't-care bits
     in the original pattern.  The third index, kx, insures we get the
     right bit of the binary counter each time.

     This works because the 'base' state contains the address with all
     zeroes in the "don't care" positions, so we can just add the
     offset to the base to get the effective address.
   */
  count = (1 << count);

  for(ix = 0; ix < count; ix++) {
    address off = 0;   /* offset from base address */

    /*  Construct the offset for the next counter value */
    for(jx = 0, kx = 0; jx < alen; jx++) {
      off <<= 1;

      if(wild[jx])
        off |= (ix >> kx++) & 1;
    }

    /*  Write the byte value to this location in the ROM image */
    *(rom + base + off) = val;
  }

  free(wild);

} /* end of write_range() */


void dump_raw(byte *rom, int rlen, FILE *ofp)
{
  fwrite(rom, sizeof(byte), rlen, ofp);

} /* end of dump_raw() */


void dump_text(byte *rom, int rlen, FILE *ofp)
{
  int pos, brk = 0;

  for(pos = 0; pos < rlen; pos++) {
    if(brk == 0)
      fprintf(ofp, "%05X:", pos);

    fprintf(ofp, " %02X", rom[pos]);

    brk = (brk + 1) & 15;
    if(brk == 0)
      fputc('\n', ofp);
  }
  if(brk != 0)
    fputc('\n', ofp);

} /* end of dump_text() */


void dump_intel(byte *rom, int rlen, FILE *ofp)
{
  address cur = 0, seg = 0;

  /* Begin by priming the segment register */
  write_offset_record(seg, ofp);

  /* Write out data records in CHUNK_SIZE blocks, until the whole ROM
     has been written, or a smaller chunk remains
   */
  while(cur + CHUNK_SIZE <= rlen)  {
    /* If the segment register has changed, update it, and issue a new
       offset record
     */
    if(SEGMENT(cur) != seg) {
      seg = SEGMENT(cur);
      write_offset_record(seg, ofp);
    }

    write_data_record(rom + cur, CHUNK_SIZE, cur & 0xFFFF, ofp);
    cur += CHUNK_SIZE;
  }

  /* If there's a small block at the end, write it out now ... */
  if(cur < rlen) {
    if(SEGMENT(cur) != seg) {
      seg = SEGMENT(cur);
      write_offset_record(seg, ofp);
    }

    write_data_record(rom + cur, rlen - cur, cur & 0xFFFF, ofp);
  }

  /* Conclude with an end record ... */
  write_end_record(ofp);

} /* end of dump_intel() */


/*------------------------------------------------------------------------*/

/* These functions do the work for dump_intel() above, for the various
   types of records it needs to put into the output stream
 */
void write_data_record(byte *data, int len, address addr, FILE *ofp)
{
  byte chk;
  int ix;

  /* Output start character and data length */
  fprintf(ofp, ":%02X", len);

  /* Output address field and record type */
  fprintf(ofp, "%04X%02X", addr, DATA_REC);

  /* Output data field ... */
  for(ix = 0; ix < len; ix++)
    fprintf(ofp, "%02X", data[ix]);

  /* Compute and output checksum byte, and terminate record */
  chk = compute_data_checksum(len, addr, data);
  fprintf(ofp, "%02X\n", chk);

} /* end write_data_record() */


void write_offset_record(address offset, FILE *ofp)
{
  byte chk;

  /* Output start character, data length, address, and record type */
  fprintf(ofp, ":020000%02X", OFFSET_REC);

  /* Output offset value ... */
  fprintf(ofp, "%04X", offset);

  /* Compute and output checksum byte, and terminate record */
  chk = compute_offset_checksum(offset);
  fprintf(ofp, "%02X\n", chk);

} /* end write_offset_record() */


void write_end_record(FILE *ofp)
{
  byte chk;

  chk = compute_end_checksum();
  fprintf(ofp, ":000000%02X%02X\n", END_REC, chk);

} /* end write_end_record() */

/* Here there be dragons */
