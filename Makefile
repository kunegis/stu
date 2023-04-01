all: bin/stu 
all-devel: \
    bin/stu.debug \
    man/stu.1 MANPAGE src/version.hh \
    bin/stu \
    all-test
all-test: \
    test_options test_sed \
    test_unit.debug \
    test_comments \
    test_unit.ndebug 
all-sani: \
    test_unit.sani_undefined
prof:  tmp-prof/analysis.prof
.PHONY:  all all-devel all-test all-sani clean prof install

src/version.hh:  VERSION sh/mkversion
	sh/mkversion >src/version.hh
conf/CXX:  sh/conf
	sh/conf
CXXFLAGS_DEBUG=  -ggdb -O0 -Werror -Wall -Wextra -Wpedantic \
    -Wunused -Wundef -Wwrite-strings -Wzero-as-null-pointer-constant -Wshadow \
    -Wnon-virtual-dtor -Wformat-nonliteral -Wsuggest-attribute=format \
    -Wlogical-op -Wredundant-decls -fno-gnu-keywords \
    -Wno-unknown-warning-option -Wno-pessimizing-move
CXXFLAGS_PROF= -pg -O2 -DNDEBUG
CXXFLAGS_SANI= -pg -O2 -Werror -Wno-unused-result -fsanitize=undefined \
    -fsanitize-undefined-trap-on-error 

bin/stu:                conf/CXX src/stu.cc src/*.hh src/version.hh
	mkdir -p bin
	@echo $$(cat conf/CXX) $$(cat conf/CXXFLAGS) $$(cat conf/CXXFLAGS_NDEBUG) src/stu.cc -o bin/stu
	@$$(cat conf/CXX) $$(cat conf/CXXFLAGS) $$(cat conf/CXXFLAGS_NDEBUG) src/stu.cc -o bin/stu
bin/stu.debug:          conf/CXX src/stu.cc src/*.hh src/version.hh 
	mkdir -p bin
	@echo $$(cat conf/CXX) $$(cat conf/CXXFLAGS) $(CXXFLAGS_DEBUG)            src/stu.cc -o bin/stu.debug
	@$$(cat conf/CXX) $$(cat conf/CXXFLAGS) $(CXXFLAGS_DEBUG)            src/stu.cc -o bin/stu.debug
bin/stu.prof:           conf/CXX src/stu.cc src/*.hh src/version.hh 
	mkdir -p bin
	@echo $$(cat conf/CXX) $$(cat conf/CXXFLAGS) $(CXXFLAGS_PROF)             src/stu.cc -o bin/stu.prof
	@$$(cat conf/CXX) $$(cat conf/CXXFLAGS) $(CXXFLAGS_PROF)             src/stu.cc -o bin/stu.prof
bin/stu.sani_undefined: conf/CXX src/stu.cc src/*.hh src/version.hh 
	mkdir -p bin
	@echo $$(cat conf/CXX) $$(cat conf/CXXFLAGS) $(CXXFLAGS_SANI)             src/stu.cc -o bin/stu.sani_undefined
	@$$(cat conf/CXX) $$(cat conf/CXXFLAGS) $(CXXFLAGS_SANI)             src/stu.cc -o bin/stu.sani_undefined

tmp-prof/analysis.prof:  tmp-prof/gmon.out
	gprof bin/stu.prof tmp-prof/gmon.out >tmp-prof/analysis.prof

tmp-prof/gmon.out:   bin/stu.prof test/long-1.1-parallel-1/main.stu
	mkdir -p tmp-prof && cd tmp-prof && ../bin/stu.prof -j10 -f ../test/long-1.1-parallel-1/main.stu 

test_options:   sh/testoptions src/stu.cc man/stu.1.in
	sh/testoptions && touch $@
test_sed:       sh/testsed test test/* test/*/* sh 
	sh/testsed && touch $@
test_comments:  src/stu.cc src/*.hh sh/testcomments sh sh/* test test/* test/*/* 
	sh/testcomments && touch $@

test_unit.debug:           bin/stu.debug          sh/mktest test test/* test/*/* 
	sh/mktest && touch $@
test_unit.ndebug:          bin/stu                sh/mktest test test/* test/*/* 
	NDEBUG=1 sh/mktest && touch $@
test_unit.sani_undefined:  bin/stu.sani_undefined sh/mktest test test/* test/*/*
	VARIANT=sani_undefined sh/mktest && touch $@

MANPAGE:  man/stu.1
	MANWIDTH=80 man man/stu.1 >MANPAGE
man/stu.1:  man/stu.1.in VERSION sh/mkman
	sh/mkman

install:  sh/install bin/stu man/stu.1
	sh/install

clean:
	rm -Rf $$(cat .gitignore)
