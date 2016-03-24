#
# This is the makefile of Stu.  No, Stu does *not* have a Stu file.
# Also, there is no configure mechanism, only this Makefile. 
#
# We follow the GNU Coding Standards for the naming of Make targets. 
#
# The invocation of the compiler is written to use G++; it has to be
# changed by hand to use another compiler. 
# 

all: stu.debug stu.ndebug stu.1 stu.text check

.PHONY:  all clean check 

CXX=g++

CXXFLAGS_DEBUG=  -ggdb
CXXFLAGS_NDEBUG= -O3 -DNDEBUG -s
CXXFLAGS_PROF=   -pg -O3 -DNDEBUG 

# Some of the specialized warning flags may not be present in other
# compilers and compiler versions than those used by the authors.  Just
# remove the flags if you use such a compiler. 
CXXFLAGS_OTHER=             \
    -std=c++11              \
    -Wall -Wextra -Wunused -pedantic \
    -Wundef -Wc++11-compat -Wwrite-strings -Wzero-as-null-pointer-constant -Wshadow \
    -Werror                 \
    -fno-gnu-keywords       \
    -D_FILE_OFFSET_BITS=64 
# -lrt would be needed for clock_gettime().  (only enabled with USE_MTIM.)

CXXFLAGS_ALL_DEBUG=  $(CXXFLAGS_DEBUG)  $(CXXFLAGS_OTHER)
CXXFLAGS_ALL_NDEBUG= $(CXXFLAGS_NDEBUG) $(CXXFLAGS_OTHER)
CXXFLAGS_ALL_PROF=   $(CXXFLAGS_PROF)   $(CXXFLAGS_OTHER)

check:  check_options check_test.debug check_test.ndebug

stu.ndebug:  $(wildcard *.cc *.hh) version.hh
	$(CXX) $(CXXFLAGS_ALL_NDEBUG) stu.cc -o stu.ndebug

stu.debug:  $(wildcard *.cc *.hh) version.hh
	$(CXX) $(CXXFLAGS_ALL_DEBUG)  stu.cc -o stu.debug

stu.prof: $(wildcard *.cc *.hh) version.hh
	$(CXX) $(CXXFLAGS_ALL_PROF)   stu.cc -o stu.prof

check_options:  testoptions stu.cc stu.1.in
	./testoptions && touch $@

check_test.debug: stu.debug mktest test test/* test/*/* 
	./mktest && touch $@

check_test.ndebug: stu.ndebug mktest test test/* test/*/* 
	NDEBUG=1 ./mktest && touch $@

# Note:  the ending ".1" indicates that the manpage is in Section 1
# (commands).  It has nothing to do with the version number of Stu. 
stu.1:  stu.1.in VERSION mkman
	./mkman

stu.text:  stu.1
	MANWIDTH=80 man -l stu.1 >stu.text

version.hh:  VERSION mkversion
	./mkversion >version.hh

analysis.prof:  gmon.out 	
	gprof stu.prof gmon.out >analysis.prof

clean:  
	rm -f stu stu.debug stu.ndebug stu.1 version.hh
