/*
  text.h

  by Michael J. Fromberger <sting@linguist.dartmouth.edu>
  Copyright (C) 1999 The Trustees of Dartmouth College

  Text handling routines for tt2rom version 2
*/

#ifndef _H_TEXT_
#define _H_TEXT_

#include <stdio.h>

/* Remove line-end comments from a zero-terminated string */
void strip_comment(char *line);

/* A line is blank if it consists only of whitespace */
int is_blank(char *line);

/* Is string 'str' a prefix of string 'of'? */
int is_prefix(char *str, char *of);

/* Parse an option of the form --name[=value].  Yields pointers to the
   name and value in static storage which is overwritten with each
   call.  Returns true if the given string is an option, and fills in
   the name and value fields; false otherwise.

   Either 'name' or 'value' may be passed as NULL; if so, the
   parameters will be discarded, and the return value may be used
   simply to determine if the option is valid.

   Both option names and value strings are limited to a fixed maximum
   size (defined as OPTBUF_SIZE in text.c), and will be truncated to
   that maximum.
 */
int parse_option(char *opt, char **name, char **value);

/* Read in a line from the given input source and chop off the newline
   Returns true if the line was read; false otherwise
 */
int read_line(FILE *ifp, char *buf, int len);

/* Allocate a buffer big enough to hold the given string, and copy
   the input string into it; returns a pointer to that buffer. It
   is the caller's responsibility to free this memory
 */
char *copy_string(char *str);

/* Remove all whitespace from the given source string, packing all
   non-whitespace characters to the left end
 */
void strip_whitespace(char *line);

/* Check if 'str' consists only of characters from 'comp'.  If so,
   returns true; otherwise returns false
 */
int valid_string(char *str, char *comp);

/* Translate all occurrences of 'from' to 'to' in the given string */
void translate(char *str, char from, char to);

/* Count how many distinct ROM images are specified in the header */
int count_roms(char *hdr);

/* Count how many address bits are specified in the header */
int count_addr(char *hdr);

#endif /* end _H_TEXT_ */
