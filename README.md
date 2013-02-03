tt2rom
======

Command-line tool to convert ASM charts into Intel ROM files

*Warning*: This file is old and hasn't been updated in a long time.  It probably
contains some things that are stupid and/or wrong.

This archive contains the source code for the 'tt2rom' program, version 2.x.
This program converts an algorithmic state machine (ASM chart), represented as
a truth table, into an Intel HEX format file, for use by commonly-available
EEPROM programmers.

You should have received the following files in this distribution:

<pre>
  README        - this file
  CHANGES       - revision history
  Makefile      - a 'make' script to build tt2rom and its
                  documentation
  rom.{h,c}     - routines for handling ROM images
  text.{h,c}    - routines for processing text input
  tt2rom.c      - the tt2rom driver program (main)
  tt2rom.pod    - manual page in POD format
  test.tt       - a very simple test vector
</pre>

The archive may also have contained a tt2rom.exe file, which is an executable
for a Pentium based PC running Windows 95 or NT.  (But I don't guarantee that
I'll keep including the .EXE in all releases)

To build the program from source, edit the Makefile to make sure that the CC
and CFLAGS options are correctly set for the compiler and platform you are
using, and then type 'make tt2rom'.  Type 'make help' to get a summary of the
things you can do with the Makefile.

If your system does not have the 'make' program, or does not support the
command-line model of compiling programs, you may have to do a bit more work by
hand.  If you are building this program inside an environment such as Microsoft
Visual Studio, you will need to create a new console application workspace
(preferably an "empty" console app, so that you do not get StdAfx.h and other
Windows dependencies).  Import the files 'text.c', 'rom.c', and 'tt2rom.c', and
you should be able to build an executable directly without further inclusion.

<pre>
Note:  The compilers I've tried for Windows (MSVC++ and Borland C++) do not
----   seem to supply the very useful snprintf() function in their standard
       library.  This isn't ENTIRELY unreasonable, since it's not part of the
       ANSI standard.  However, you may find yourself having to change
       snprintf() calls back to normal sprintf() calls.  I disavow any
       responsibility for the unsafe state of affairs you put yourself into, if
       you do this.
</pre>

TT2ROM version 2 was written by Michael J. Fromberger.  The program and its
source are Copyright (C) 1999, 2001, 2002 by The Trustees of Dartmouth College,
Hanover, New Hampshire, USA.  Questions regarding duplication, modification,
and licensing should be directed to the Director of Computing at Thayer School
of Engineering.
