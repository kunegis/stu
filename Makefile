#
# This is the makefile of Stu.  No, Stu does *not* have a Stu file.
# Also, there is no configure mechanism, only this Makefile. 
#
# We follow the GNU Coding Standards for the naming of Make targets. 
#
# The arguments to the compiler are meant for GCC; they have to be
# changed by hand to use another compiler. 
# 

all: stu.1 stu.ndebug 

all-test: stu.text test_options stu.debug stu.ndebug test_test.debug test_test.ndebug test_doubleslash

.PHONY:  all all-test clean 

clean:  
	rm -f stu stu.debug stu.ndebug stu.prof stu.1 version.hh stu.text test_*

CXX=c++

CXXFLAGS_DEBUG=  -ggdb \
    -Wall -Wextra -Wunused -pedantic \
    -Wundef -Wc++11-compat -Wwrite-strings -Wzero-as-null-pointer-constant -Wshadow \
    -Werror                 \
    -fno-gnu-keywords       \

CXXFLAGS_NDEBUG= -O3 -DNDEBUG -s

CXXFLAGS_PROF=   -pg -O3 -DNDEBUG 

# Some of the specialized warning flags may not be present in other
# compilers and compiler versions than those used by the authors.  Just
# remove the flags if you use such a compiler. 
CXXFLAGS_OTHER=             \
    -std=c++11              \
    -D_FILE_OFFSET_BITS=64 
# -lrt would be needed for clock_gettime().  (only enabled with USE_MTIM.)

CXXFLAGS_ALL_DEBUG=  $(CXXFLAGS_DEBUG)  $(CXXFLAGS_OTHER)
CXXFLAGS_ALL_NDEBUG= $(CXXFLAGS_NDEBUG) $(CXXFLAGS_OTHER)
CXXFLAGS_ALL_PROF=   $(CXXFLAGS_PROF)   $(CXXFLAGS_OTHER)

stu.ndebug:  $(wildcard *.cc *.hh) version.hh
	$(CXX) $(CXXFLAGS_ALL_NDEBUG) stu.cc -o stu.ndebug

stu.debug:  $(wildcard *.cc *.hh) version.hh
	$(CXX) $(CXXFLAGS_ALL_DEBUG)  stu.cc -o stu.debug

stu.prof: $(wildcard *.cc *.hh) version.hh
	$(CXX) $(CXXFLAGS_ALL_PROF)   stu.cc -o stu.prof

test_options:  testoptions stu.cc stu.1.in
	./testoptions && touch $@

test_test.debug: stu.debug mktest test test/* test/*/* 
	./mktest && touch $@

test_test.ndebug: stu.ndebug mktest test test/* test/*/* 
	NDEBUG=1 ./mktest && touch $@

test_doubleslash:  $(wildcard *.cc *.hh) testdoubleslash
	./testdoubleslash

# Note:  the ending ".1" indicates that the manpage is in Section 1
# (commands).  It has nothing to do with the version number of Stu. 
stu.1:  stu.1.in VERSION mkman
	./mkman

stu.text:  stu.1
	MANWIDTH=80 man ./stu.1 >stu.text

version.hh:  VERSION mkversion
	./mkversion >version.hh

analysis.prof:  gmon.out 	
	gprof stu.prof gmon.out >analysis.prof

