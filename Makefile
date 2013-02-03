#
# Makefile for tt2rom version 2
#
# by Michael J. Fromberger <sting@linguist.dartmouth.edu>
# Copyright (C) 1999 The Trustees of Dartmouth College
#
# $Id: Makefile,v 1.1 2005/05/25 19:51:46 sting Exp $
#

CC=gcc
CFLAGS=-ansi -pedantic -Wall -O2

HDRS=text.h rom.h
SRCS=text.c rom.c tt2rom.c
OBJS=text.o rom.o

VERS=2.08
SECT=1

HCC=pod2man
HFLAGS=--release="Version $(VERS)" --center "TT2ROM" --section=$(SECT)

help:
	@ echo ""
	@ echo "The following targets may be built with this Makefile:"
	@ echo ""
	@ echo "tt2rom    - the tt2rom program itself (see README)"
	@ echo "doc       - the manual page"
	@ echo "clean     - remove objects and cores"
	@ echo "distclean - clean up for distribution"
	@ echo "dist      - build distribution archive"
	@ echo ""

.c.o:
	$(CC) $(CFLAGS) -c $<

tt2rom: $(HDRS) $(OBJS) tt2rom.c
	$(CC) $(CFLAGS) -o tt2rom $(OBJS) tt2rom.c

doc: tt2rom.pod
	$(HCC) $(HFLAGS) tt2rom.pod > tt2rom.$(SECT)

clean:
	rm -f *.o
	rm -f *~
	rm -f core

distclean: clean
	rm -f rom?.img source?.hex
	rm -f tt2rom
	rm -f *.1

dist: $(HDRS) $(SRCS) tt2rom.pod test.tt Makefile README CHANGES
	zip tt2rom-$(VERS).zip README CHANGES Makefile $(HDRS) $(SRCS) \
		tt2rom.pod test.tt tt2rom.exe
