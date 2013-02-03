/*
  tt2rom.c, version 2

  by Michael J. Fromberger <sting@linguist.dartmouth.edu>
  Copyright (C) 1999, 2001 The Trustees of Dartmouth College

  Convert a specification file into a binary ROM image for the Intel
  ROM programmer.

  Syntax:
    tt2rom <file>

  File format:
    Each line of the input file defines a (possibly ambiguous) state
    and the corresponding ROM outputs.  The first line of the file is
    used as a configuration line, specifying how the columns of the
    file are to be interpreted.  An 'A' in a column specifies an input
    state bit (i.e., an address bit).  A digit from '0'-'9' specifies
    which ROM the bits in that column will be assigned to.

    The bits internally sorted out to each ROM, with the MSB on the
    left, LSB on the right.  Values will be left-padded with zeroes if
    necessary to fill out the word size.  All bits specified on the
    configuration line MUST be specified on each data line (this is
    different from the old version, which would assume zeroes)

    Comments beginning with a hash mark (#) will be ignored, as will
    blank lines.  All data bits must be '0', '1', or '-'.  State
    (address) bits may additionally be 'x' (or 'X') which means "don't
    care", and addresses containing don't-care bits will be properly
    coded.  A '-' means "don't care" for an output bit, as well, and
    all such values will be forced high (or low, at the user's option)
    in the output)

  Output:
    One file will be generated for each ROM specified.  The file
    name will be "<file><x>.dat", where <x> is the ROM number, and
    <file> is the the first few characters of the input file name.
    You can override this with the FTEMPLATE environment variable.

  This program is based on the original tt2rom by Anthony Edwards, the
  creation date of which is lost in the mists of antiquity.  This
  version fixes several uncomfortable design problems with the
  original, and expands it to handle ROMs of greater than 64K in size.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "text.h"
#include "rom.h"

#define  MAXLINE     256  /* maximum input string length (bytes) */
#define  MAXFILENAME 32   /* maximum output filename len (bytes) */
#define  PREFIXLEN   6    /* file name prefix length limit       */
#define  MAXBITS     20   /* maximum number of bits in address   */
#define  NUM_ROMS    10   /* maximum number of ROM images        */
#define  VERSION     "2.07"       /* version string              */
#define  FTEMPVAR    "FTEMPLATE"  /* output template environment */

#define  BINARY_FMT  1    /* write binary ROM images             */
#define  TEXT_FMT    2    /* write text format ROM images        */
#define  INTEL_FMT   3    /* write Intel format ROM images       */

#define  OUTPUT_DC   '-'  /* output "don't care" indicator       */

int   g_fmt = INTEL_FMT;        /* default output format     */
char  g_odcv = '1';             /* output don't care value   */
char  g_fname[MAXFILENAME + 1]; /* output filename template  */
char *g_ftmpl = g_fname;        /* which template to use     */

/* Test a output template for correct format */
int template_valid(char *str);

/* Display a help message to the user */
void do_help(void);

/* Generate output file name template */
void make_file_template(char *fname, char *tmpl, int tlen);

/* Process an input stream */
int process_file(FILE *ifp);

/* Parse an individual data line (assumes preprocessing) */
int parse_data(char *str, int line, char *config,
                int abits, byte *accum);

/* Shift arguments leftward to remove an old argument */
int shift_args(int argc, char **argv);

/* Allocate memory for ROM images */
int alloc_roms(byte ***romp, int nroms, char *config, int abits);

/* Release memory used by ROM images */
void free_roms(byte **romp, int nroms);

/* Write ROM images out to files */
int dump_roms(byte **rom, int nroms, int abits, int fmt);

int main(int argc, char *argv[])
{
  FILE *ifp;
  int res = 0, ix = 0;
  char *name, *value;

  /* Parse command line options.  This uses a custom mechanism,
     because the Unix getopt() is not readily available for DOS,
     as far as I can tell
   */
  while(argc >= 2 && parse_option(argv[1], &name, &value)) {
    /* Print help message summarizing command line options */
    if(strcmp(name, "help") == 0) {
      do_help();
      return 0;

    /* Print out a version message and exit the program    */
    } else if(strcmp(name, "version") == 0) {
      fprintf(stderr, "tt2rom v. %s by Michael J. Fromberger\n"
              "Copyright (C) 1999 The Trustees of Dartmouth College\n\n",
              VERSION);
      return 0;

    /* Set the default output "don't care" value           */
    } else if(strcmp(name, "output-dc") == 0) {
      long bitval;
      char *endp;

      if(value == NULL || value[0] == '\0') {
        fprintf(stderr,
                "Default output value must be specified as 0 or 1\n");
        return 1;
      }

      bitval = strtol(value, &endp, 2);
      if(bitval < 0 || bitval > 1) {
        fprintf(stderr, "Output value out of range: 0 or 1 expected\n");
        return 1;
      } else if(*endp != '\0') {
        fprintf(stderr, "Unrecognized junk in option value: '%s'\n",
                endp);
        return 1;
      }

      g_odcv = (bitval ? '1' : '0');

    /* Set output format                                   */
    } else if(strcmp(name, "output-fmt") == 0) {
      if(value == NULL) {
        fprintf(stderr,
                "Output format must be 'raw', 'text', or 'intel'\n");
        return 1;
      }

      if(strcmp(value, "intel") == 0) {
        g_fmt = INTEL_FMT;
      } else if(strcmp(value, "raw") == 0) {
        g_fmt = BINARY_FMT;
      } else if(strcmp(value, "text") == 0) {
        g_fmt = TEXT_FMT;
      } else {
        fprintf(stderr,
                "Output format must be 'raw', 'text', or 'intel'\n");
        return 1;
      }
    /* A blank name signals end of option processing       */
    } else if(name[0] == '\0') {
      argc = shift_args(argc, argv);
      break;

    /* Anything else is garbage, and an error              */
    } else {
      fprintf(stderr, "Unrecognized option: '%s'\n", name);
      return 1;
    }

    /* Assuming we got here, we successfully processed an option.  Now
       we need to shift the options down so that we can pretend it was
       never here...
     */
    argc = shift_args(argc, argv);

  } /* end option parsing */

  /* Print a welcome banner (so people know what version they have) */
  fprintf(stderr, "This is tt2rom version %s\n\n", VERSION);

  /* Make sure we at least got a file name */
  if(argc < 2) {
    fprintf(stderr, "Usage: %s <file> (or '--help' for assistance)\n",
            argv[0]);
    return 1;
  }

  /* Get filename template from environment, if available.  If none
     is provided, or if it is not valid, we'll ignore it and use the
     built in version, to avoid format string attacks */

  snprintf(g_fname, MAXFILENAME, "source%%d.hex"); /* default value */

  if((name = getenv(FTEMPVAR)) != NULL) {
    if(template_valid(name))
      g_ftmpl = name;
    else
      fprintf(stderr,
              "%s: warning: file name template is invalid, ignoring it\n",
              argv[0]);
  }

  for(ix = 1; ix < argc; ix++) {
    /* Attempt to open the input file specified */
    if((ifp = fopen(argv[ix], "r")) == NULL) {
      fprintf(stderr, "Unable to open file '%s' for reading\n", argv[1]);
      return 1;
    }

    /* Set up output file name */
    if(g_ftmpl == g_fname)
      make_file_template(argv[ix], g_fname, MAXFILENAME);

    /* Do the deed ... */
    res = process_file(ifp);

    fclose(ifp);
  }

  return res;
}


/*
  The task of this function is to insure that whatever was passed in
  as a file name template from the environment is actually valid for
  use as a format string.  If it passes this test, it should be okay
  and not expose us to any format string vulnerabilities.
 */
int template_valid(char *str)
{
  int has_num = 0;

  while(*str) {
    if(*str == '%') {
      switch(*(str + 1)) {
      case 'd':
        /* A single %d is good, because it means we have a place to put
           the ROM number.  Having zero or more than one of these is,
           however, a bad thing, so we'll consider it an error.
         */
        if(has_num)
          return 0;
        else
          has_num = 1;
        break;

      case '%':
        /* Okay, an escaped percent sign.  Just skip over it and continue */
        break;

      default:
        /* Anything else with a percent sign in front of it is definitely
           Considered Harmful; this string will be rejected in that case,
           without any further processing.
         */
        return 0;
      }
      ++str;
    }

    ++str;
  }

  return (has_num == 1); /* all's well */

} /* end template_valid() */


int process_file(FILE *ifp)
{
  char *ibuf, *config = NULL;
  byte **rom = NULL;       /* pointers to ROM images */
  byte *data = NULL;      /* data accumulators      */
  int line = 0, first = 1, nroms = 0, abits = 0, length = 0, res = 0;
  int ix;

  /* Allocate space to read strings into */
  if((ibuf = calloc(MAXLINE + 1, sizeof(char))) == NULL) {
    fprintf(stderr, "Insufficient memory to process file\n");
    return 1; /* out of memory */
  }

  /* Read strings from the input file */
  while(read_line(ifp, ibuf, MAXLINE)) {
    ++line;
    strip_comment(ibuf);

    /* Blank lines are skipped in all cases */
    if(is_blank(ibuf))
      continue;

    strip_whitespace(ibuf);

    /*
      When we see the first line, we need to accumulate some other
      info we're going to need later.  This includes the number of
      address bits (abits), and the number of ROMs (nroms).  We also
      need to save a copy of the configuration line for later, since
      ibuf will be overwritten with each line that is consumed.

      We also range-check the values of abits and nroms, and allocate
      memory for the ROM images we will build during the reading of
      the rest of the file.  We save the length of the configuration
      line (length) so that we can syntax check subsequent lines in
      an efficent manner.
     */
    if(first) {

      /* Check structural validity of configuration line */
      if(!valid_string(ibuf, "0123456789Aa")) {
        fprintf(stderr,
                "Line %d: invalid character in configuration\n",
                line);
        res = 1;
        goto CLEANUP;
      }

      /* Count number of ROM cells and address (state) bits */
      nroms = count_roms(ibuf);
      abits = count_addr(ibuf);

      /* Complain if we didn't get at least 1 ROM, or if we got too many */
      if(nroms < 1) {
        fprintf(stderr, "Line %d: must specify at least 1 ROM number\n",
                line);
        res = 2;
        goto CLEANUP;
      } else if(nroms > NUM_ROMS) {
        fprintf(stderr, "Line %d: cannot specify more than %d ROMs\n",
                line, NUM_ROMS);
        res = 2;
        goto CLEANUP;
      }

      /* Complain if we got no address bits, or more than MAXBITS */
      if(abits < 1 || abits > MAXBITS) {
        fprintf(stderr, "Line %d: must have between 1-%d state bits\n",
                line, MAXBITS);
        res = 3;
        goto CLEANUP;
      }

      /* Okay, the config is alright, save it for later ... */
      if((config = copy_string(ibuf)) == NULL) {
        fprintf(stderr, "Insufficient memory to process file\n");
        res = 1;
        goto CLEANUP;
      }

      /* Hang onto the length, we'll need it later */
      length = strlen(config);

      /* Allocate space for the ROMs and their accumulators */
      if(!alloc_roms(&rom, nroms, config, abits)) {
        fprintf(stderr, "Insufficient memory to process file\n");
        res = 1;
        goto CLEANUP;
      }
      if((data = calloc(nroms, sizeof(byte))) == NULL) {
        fprintf(stderr, "Insufficient memory to process file\n");
        res = 1;
        goto CLEANUP;
      }

      first = 0;
      continue;
    } /* end if(first) */

    /* Translate output "don't care" values to regular bits */
    translate(ibuf, OUTPUT_DC, g_odcv);

    /* Anything bad left in the string? */
    if(!valid_string(ibuf, "01Xx")) {
      fprintf(stderr, "Line %d: invalid character in data\n", line);
      res = 1;
      goto CLEANUP;
    }

    /* Make sure we got enough fields to satisfy the template */
    if(strlen(ibuf) != length) {
      fprintf(stderr,
              "Line %d: wrong number of fields (wanted %u, got %u)\n",
              line, (unsigned) length, (unsigned) strlen(ibuf));
      res = 4;
      goto CLEANUP;
    }

    /* Grab all the data out of the line, escaping on error */
    if(!parse_data(ibuf, line, config, abits, data)) {
      res = 5;
      goto CLEANUP;
    }

    /* Write the data into the ROM images.  We know which ROMs
       to use by checking the pointers in the ROM image array,
       and the accumulator is already set to go
     */
    for(ix = 0; ix < nroms; ix++) {
      if(rom[ix]) {
        write_range(rom[ix], ibuf, abits, data[ix]);
      }
    }

    /* Clear out accumulators for the next round */
    memset(data, 0, nroms);

  } /* end while(read_line(...)) */

  /* If we didn't get a first line at all, the file was logically
     empty (i.e., not even a configuration!) */
  if(first) {
    fprintf(stderr, "No configuration line was found\n");
    res = 7;

  } else {

    /* Having accumulated all the data into the ROM images, we now
       will dump them out into the appropriate files */
    if(!dump_roms(rom, nroms, abits, g_fmt))
      res = 6;
  }

 CLEANUP:
  free(ibuf);

  if(config)
    free(config);

  if(rom) {
    free_roms(rom, nroms);
    free(rom);
  }

  if(data)
    free(data);

  return res;

} /* end process_file() */


/*
  We know, a priori, that the strings passed in to this function
  consist only of valid characters (we checked in process_file() by
  calling the valid_string() function).  Thus, we can just grobble
  through the string assigning bits to the accumulators.
 */
int parse_data(char *str, int line, char *config, int abits, byte *accum)
{
  int pos = abits;

  while(isdigit((int)str[pos])) {
    int  bit = str[pos] - '0';     /* get bit value  */
    int  rnum = config[pos] - '0'; /* get ROM number */

    /* Write this bit into the accumulator for its ROM */
    accum[rnum] = (accum[rnum] << 1) | bit;
    ++pos;
  }

  if(str[pos] != '\0') {
    fprintf(stderr, "Line %d: illegal don't-care bit in data\n",
            line);
    return 0;
  }

  return 1;

} /* end parse_data() */


int shift_args(int argc, char **argv)
{
  int pos = 2;

  while(pos < argc) {
    argv[pos - 1] = argv[pos];
    pos++;
  }

  return argc - 1;

} /* end shift_args() */


void do_help(void)
{
  fprintf(stderr,
          "Help for tt2rom version %s:\n\n"

          "Usage is:  tt2rom [options] <file>\n\n"

          "The input file is processed, and any errors are reported.\n"
          "Assuming no errors are encountered, the completed ROM\n"
          "images are written out to files named file0.hex, file1.hex,\n"
          "etc., up to the number of ROMs specified in the source.\n\n",

          VERSION);

  fprintf(stderr,
          "Options:\n"
          " --help         - displays this message\n"
          " --version      - print the version number of tt2rom\n"
          " --output-dc=X  - set default value for don't-care bits\n"
          "                  in the output to X, where X is 0 or 1\n"
          "                  The current default is %c\n"
          " --output-fmt=X - set output format to X, where X is one\n"
          "                  of 'raw', 'text', or 'intel'.\n\n"

          "Report bugs to <admin@thayer.dartmouth.edu>\n\n",

          g_odcv);

} /* end do_help() */


void make_file_template(char *fname, char *tmpl, int tlen)
{
  static char *tag = "%d.hex";
  int          ix, pos = 0;

  if(fname[0] == '\0' || fname[0] == '.') {
    ix = snprintf(tmpl, tlen, "output");

  } else {

    for(ix = 0; ix < PREFIXLEN && fname[ix] && fname[ix] != '.'; ix++)
      tmpl[ix] = fname[ix];
  }

  while(ix < tlen - 1)
    tmpl[ix++] = tag[pos++];

  tmpl[ix] = '\0';

} /* end make_file_template() */


int  alloc_roms(byte ***romp, int nroms, char *config, int abits)
{
  unsigned int size = (1 << abits);  /* ROM size */
  int res = 1; /* innocent 'til proven guilty */

  if((*romp = calloc(nroms, sizeof(byte *))) == NULL)
    return 0;

  while(*config) {
    if(isdigit((int)*config)) {
      int  rnum = (*config - '0');

      /* If we haven't gotten this one already, allocate it */
      if((*romp)[rnum] == NULL) {
        if(((*romp)[rnum] = calloc(size, sizeof(byte))) == NULL) {
          fprintf(stderr, "Unable to allocate ROM #%d image\n", rnum);
          res = 0;
          break; /* out of the while() */
        }
      }
    }

    ++config;
  }

  /* If an allocation failed, clean up any we already got */
  if(res == 0)
    free_roms(*romp, nroms);

  return res;

} /* end alloc_roms() */


void free_roms(byte **romp, int nroms)
{
  int ix;

  for(ix = 0; ix < nroms; ix++)
    if(romp[ix]) {
      free(romp[ix]);
      romp[ix] = NULL;
    }

} /* end free_roms() */


int dump_roms(byte **rom, int nroms, int abits, int fmt)
{
  char fname[MAXFILENAME];
  int ix;
  FILE *ofp;
  unsigned int romsize = (1 << abits);

  fprintf(stderr, "%d ROM images to be written, %u bytes per image\n",
          nroms, romsize);

  for(ix = 0; ix < nroms; ix++) {
    if(rom[ix] != NULL) {
      sprintf(fname, g_ftmpl, ix);

      if((ofp = fopen(fname, "w")) == NULL) {
        fprintf(stderr, "Unable to open output file '%s' for writing\n",
                fname);
        return 0;
      }

      fprintf(stderr, "Writing ROM #%d to file '%s'\n", ix, fname);
      switch(fmt) {
      case BINARY_FMT:
        dump_raw(rom[ix], romsize, ofp);
        break;
      case TEXT_FMT:
        dump_text(rom[ix], romsize, ofp);
        break;
      default:
        dump_intel(rom[ix], romsize, ofp);
        break;
      }
      fclose(ofp);
    }
  }

  return 1;

} /* end dump_roms() */

/* Here there be dragons */
