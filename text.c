/*
  text.c

  by Michael J. Fromberger <sting@linguist.dartmouth.edu>
  Copyright (C) 1999 The Trustees of Dartmouth College

  Text handling routines for tt2rom version 2
 */

#include "text.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define COMMENT_CHAR '#'
#define OPTBUF_SIZE 256

/* Remove line-end comments from a zero-terminated string */
void strip_comment(char *line) {
  if (line == NULL) return;

  while (*line) {
    if (*line == COMMENT_CHAR) {
      *line = '\0';
      return;
    }

    ++line;
  }

} /* end strip_comment() */

/* A line is blank if it consists only of whitespace */
int is_blank(char *line) {
  if (line == NULL) return 1; /* we will consider a NULL pointer a blank line */

  while (isspace((int)*line)) ++line;

  /*
    If the first character after all the whitespace is the NUL terminator,
    then the line is all whitespace; otherwise it's not
  */
  return (*line == '\0');

} /* end is_blank() */

int is_prefix(char *str, char *of) {
  if (str == NULL || of == NULL) return 0;

  /* Empty string is a prefix of any string */
  if (*str == '\0') return 1;

  /* A non-empty string is a prefix iff all its characters
     match a left subset of the target
   */
  while (*str && *of && *str == *of) {
    ++str;
    ++of;
  }

  /* Did we match all the characters in the alleged prefix? */
  return (*str == '\0');

} /* end is_prefix() */

int parse_option(char *opt, char **name, char **value) {
  static char namebuf[OPTBUF_SIZE];
  static char valbuf[OPTBUF_SIZE];
  int pos = 0;

  if (!is_prefix("--", opt)) return 0;

  /* Preset outputs to NULL so we don't have to do it in multiple
     places later on
   */
  if (name) *name = NULL;
  if (value) *value = NULL;

  opt += 2; /* skip past leading '--' */

  /* Scan name into name buffer, stopping at end of string or
     when an '=' is encountered (denoting a value is next)
   */
  while (*opt && *opt != '=' && pos < sizeof(namebuf) - 1) {
    namebuf[pos++] = *opt;
    ++opt;
  }
  namebuf[pos] = '\0'; /* make sure name is terminated */

  /* Send name to output, if possible */
  if (name) *name = (char *)&namebuf;

  /* Is there a value to follow? */
  if (*opt == '\0') return 1; /* no, just return a name */

  ++opt; /* skip past the '=' */
  pos = 0;

  /* Scan value into value buffer, stopping at end of string.
     Note that it is legal for the value to be an empty string;
     this will be returned as a zero-length string, rather than
     as a NULL
   */
  while (*opt && pos < sizeof(valbuf) - 1) {
    valbuf[pos++] = *opt;
    ++opt;
  }
  valbuf[pos] = '\0'; /* make sure value is terminated */

  /* Send value to output, if possible */
  if (value) *value = (char *)&valbuf;

  return 1;

} /* end parse_option() */

/* Read in a line from the given input source and chop off the newline
   Returns true if the line was read; false otherwise
 */
int read_line(FILE *ifp, char *buf, int len) {
  int alen;

  if (fgets(buf, len, ifp) == NULL) return 0; /* no more lines available */

  alen = strlen(buf);
  if (buf[alen - 1] == '\n') buf[alen - 1] = '\0';

  return 1;

} /* end read_line() */

char *copy_string(char *str) {
  char *copy;
  int len;

  if (str == NULL || (len = strlen(str)) == 0) return NULL;

  if ((copy = malloc(len + 1)) == NULL) return NULL;

  memcpy(copy, str, len);
  copy[len] = '\0';

  return copy;

} /* end copy_string() */

void strip_whitespace(char *line) {
  char *spc, *non;

  if (line == NULL) return;

  spc = line;

  /*    Find the first whitespace character in the line, if any */
  while (*spc && !isspace((int)*spc)) ++spc;

  non = spc;

  while (1) {
    /*  Find the first nonspace character after that */
    while (*non && isspace((int)*non)) ++non;

    /*  If there is none, attenuate the string, and we're finished */
    if (*non == '\0') {
      *spc = '\0';
      return;
    }

    /*  Otherwise, pack non-space characters leftward 'til we find a
        space again, then start over */
    while (!isspace((int)*non) && *non != '\0') {
      *spc = *non;
      ++spc;
      ++non;
    }
  }

} /* end strip_whitespace() */

int valid_string(char *str, char *comp) {
  int len, span;

  if (str == NULL || comp == NULL) return 0;

  /* If str consists only of characters from comp, then the prefix of
     str returned by strspn() should be the same length as the string
     itself; otherwise, there are alloying characters, and the string
     is not "valid" according to our definition
   */
  len = strlen(str);
  span = strspn(str, comp);

  return (span == len);

} /* end valid_string() */

void translate(char *str, char from, char to) {
  if (str == NULL) return;

  while (*str) {
    if (*str == from) *str = to;

    ++str;
  }

} /* end translate() */

/* Count how many distinct ROM images are specified in the header */
int count_roms(char *hdr) {
  int maxrom = -1;

  if (hdr == NULL) return 0;

  /* ROM numbers are specified by single digits */
  while (*hdr) {
    if (isdigit((int)*hdr)) {
      int off = *hdr - '0';

      if (off > maxrom) maxrom = off;
    }

    ++hdr;
  }

  return maxrom + 1;

} /* end count_roms() */

/* Count how many address bits are specified in the header */
int count_addr(char *hdr) {
  int nbits = 0;

  if (hdr == NULL) return 0;

  while (*hdr) {
    if (tolower((int)*hdr) == 'a') ++nbits;

    ++hdr;
  }

  return nbits;

} /* end count_addr() */

/* Here there be dragons */
