.POSIX:
all: bin/stu 
test: \
    bin/stu.debug \
    man/stu.1 MANPAGE src/version.hh \
    bin/stu \
    log/test_options \
    log/test_unit.debug \
    log/test_clean \
    log/test_unit.ndebug 
all-sani: \
    log/test_unit.sani_undefined
prof:  bin/analysis.prof
.PHONY:  all test all-sani clean prof install 

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
	@mkdir -p bin
	@echo $$(cat conf/CXX) $$(cat conf/CXXFLAGS) $$(cat conf/CXXFLAGS_NDEBUG) src/stu.cc -o bin/stu
	@$$(cat conf/CXX) $$(cat conf/CXXFLAGS) $$(cat conf/CXXFLAGS_NDEBUG) src/stu.cc -o bin/stu
bin/stu.debug:          conf/CXX src/stu.cc src/*.hh src/version.hh 
	@mkdir -p bin
	@echo $$(cat conf/CXX) $$(cat conf/CXXFLAGS) $(CXXFLAGS_DEBUG)            src/stu.cc -o bin/stu.debug
	@$$(cat conf/CXX) $$(cat conf/CXXFLAGS) $(CXXFLAGS_DEBUG)            src/stu.cc -o bin/stu.debug
bin/stu.prof:           conf/CXX src/stu.cc src/*.hh src/version.hh 
	@mkdir -p bin
	@echo $$(cat conf/CXX) $$(cat conf/CXXFLAGS) $(CXXFLAGS_PROF)             src/stu.cc -o bin/stu.prof
	@$$(cat conf/CXX) $$(cat conf/CXXFLAGS) $(CXXFLAGS_PROF)             src/stu.cc -o bin/stu.prof
bin/stu.sani_undefined: conf/CXX src/stu.cc src/*.hh src/version.hh 
	@mkdir -p bin
	@echo $$(cat conf/CXX) $$(cat conf/CXXFLAGS) $(CXXFLAGS_SANI)             src/stu.cc -o bin/stu.sani_undefined
	@$$(cat conf/CXX) $$(cat conf/CXXFLAGS) $(CXXFLAGS_SANI)             src/stu.cc -o bin/stu.sani_undefined

bin/analysis.prof:  bin/gmon.out
	gprof bin/stu.prof bin/gmon.out >bin/analysis.prof

bin/gmon.out:   bin/stu.prof tests/long-1.1-parallel-1/main.stu
	cd bin && ./stu.prof -j10 -f ../tests/long-1.1-parallel-1/main.stu && ../sh/rm_tmps

log/test_options:   sh/test_options src/stu.cc man/stu.1.in
	sh/test_options && mkdir -p log && touch $@
log/test_clean:  src/stu.cc src/*.hh sh/test_clean sh sh/* tests tests/*/* 
	sh/test_clean && mkdir -p log && touch $@

log/test_unit.debug:           bin/stu.debug          sh/mktest tests tests/*/* 
	sh/mktest && mkdir -p log && touch $@
log/test_unit.ndebug:          bin/stu                sh/mktest tests tests/*/* 
	NDEBUG=1 sh/mktest && mkdir -p log && touch $@
log/test_unit.sani_undefined:  bin/stu.sani_undefined sh/mktest tests tests/*/*
	VARIANT=sani_undefined sh/mktest && mkdir -p log && touch $@

MANPAGE:  man/stu.1
	MANWIDTH=80 man man/stu.1 >MANPAGE
man/stu.1:  man/stu.1.in VERSION sh/mkman
	sh/mkman

install:  sh/install bin/stu man/stu.1
	sh/install

clean:
	rm -Rf $$(cat .gitignore)
