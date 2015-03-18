#
# Makefile for the test programs
#

.SUFFIXES: .o .cpp

#
# Compiler and loader definitions
#
PROGRAM = 	testfile

LD =		ld
LDFLAGS =	

CXX =           g++
CXXFLAGS =	-g -Wall

#PURIFY =        purify -collector=/s/ogcc/bin/ld -g++
PURIFY =        purify -collector=/usr/ccs/bin/ld -g++

#
# general definitions
#

MAKEFILE =      Makefile

#
# list of all object and source files
#

OBJS =  db.o buf.o bufHash.o error.o page.o heapfile.o testfile.o 
SRCS =	db.cpp buf.cpp bufHash.cpp error.cpp page.cpp heapfile.cpp testfile.cpp 

all:		$(PROGRAM)

$(PROGRAM):	$(OBJS)
		$(CXX) -o $@ $(OBJS) $(LDFLAGS)

$(PROGRAM).pure:$(OBJS) 
		$(PURIFY) $(CXX) -o $@ $(OBJS) $(LDFLAGS)

.C.o:
		$(CXX) $(CXXFLAGS) -c $<

clean:
		rm -f core *.bak *~ *.o $(PROGRAM) *.pure .pure testpage

depend:
		makedepend -I /s/gcc/include/g++ -f$(MAKEFILE) \
		$(SRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.

