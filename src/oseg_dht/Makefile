#
# Copyright (c) 2001-2003 Regents of the University of California.
# All rights reserved.
#
# See the file LICENSE included in this distribution for details.
#




CC = g++


#Note: we probably don't actually need the wx-config stuff anymore.
#CPREFLAG = `wx-config --cppflags` `pkg-config --cflags opencv`
#CPOSTFLAG = `wx-config --libs` `pkg-config --libs opencv`

CPREFLAG = 
CPOSTFLAG = -Wno-write-strings
TESTOUTPUT = dhtTester
all: class_test



#####################Makefile test for all##########################


class_test: Bamboo_class_test_main.cpp gateway_prot_clnt.c gateway_prot_xdr.c gateway_prot.h Bamboo.hpp Bamboo.cpp 
	$(CC) $(CPREFLAG) Bamboo_class_test_main.cpp gateway_prot_clnt.c gateway_prot_xdr.c Bamboo.cpp -o $(TESTOUTPUT) $(CPOSTFLAG)


clean:
	rm -rvf *.o $(TESTOUTPUT) 

