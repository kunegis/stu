.POSIX:
all: bin/stu
check: \
    bin/stu.cdebug \
    man/stu.1 MANPAGE src/version.hh \
    bin/stu \
    log/test_unit.cdebug \
    log/test_unit.ndebug
test: \
    bin/stu.debug \
    man/stu.1 MANPAGE src/version.hh \
    bin/stu \
    log/test_options \
    log/test_unit.debug \
    log/test_clean \
    log/test_unit.ndebug \
    log/test_clean_last \
    sani
.PHONY: all clean install check test cov prof sani analyzer

conf/CXX: sh/configure
	sh/configure
src/version.hh: VERSION sh/mkversion
	sh/mkversion >src/version.hh

CXXFLAGS_DEBUG= \
    -ggdb -O0 -Werror -Wall -Wextra -Wpedantic \
    -Wunused -Wundef -Wwrite-strings -Wzero-as-null-pointer-constant -Wshadow \
    -Wnon-virtual-dtor -Wformat-nonliteral -Wsuggest-attribute=format \
    -Wformat-overflow=2 -Wformat=2 -Wformat-signedness -Wformat-truncation=2 \
    -fdelete-null-pointer-checks -Wnull-dereference -Wimplicit-fallthrough \
    -Wignored-attributes -Wswitch-default -Wswitch-enum -Wunused-const-variable=2 \
    -Wuninitialized -Walloc-zero -Wduplicated-branches -Wduplicated-cond -Wunused-macros \
    -Wcast-align -Wrestrict -Wno-parentheses -Wlogical-op -Wredundant-decls \
    -Wno-pessimizing-move -Wsuggest-override \
    -D_GLIBCXX_DEBUG
CXXFLAGS_SANI= \
    -ggdb -O2 -Werror -Wno-unused-result -fsanitize=undefined \
    -fsanitize-undefined-trap-on-error
CXXFLAGS_CDEBUG=   -O0 -D_GLIBCXX_DEBUG -w
CXXFLAGS_SNDEBUG=   -DNDEBUG -O2 -fwhole-program
CXXFLAGS_PROF=     -DNDEBUG -pg -O2
CXXFLAGS_COV=      -DNDEBUG --coverage -lgcov -O0 -DSTU_COV
CXXFLAGS_ANALYZER= -fanalyzer

bin/stu:                conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin log
	@echo $$(cat conf/CXX) $$(cat conf/CXXFLAGS_NDEBUG) $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu
	@     $$(cat conf/CXX) $$(cat conf/CXXFLAGS_NDEBUG) $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu
bin/stu.debug:          conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin log
	@echo $$(cat conf/CXX) $(CXXFLAGS_DEBUG)            $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.debug
	@     $$(cat conf/CXX) $(CXXFLAGS_DEBUG)            $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.debug
bin/stu.cdebug:         conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin log
	@echo $$(cat conf/CXX) $(CXXFLAGS_CDEBUG)           $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.cdebug
	@     $$(cat conf/CXX) $(CXXFLAGS_CDEBUG)           $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.cdebug
bin/stu.sndebug:        conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin log
	@echo $$(cat conf/CXX) $(CXXFLAGS_SNDEBUG)          $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.sndebug
	@     $$(cat conf/CXX) $(CXXFLAGS_SNDEBUG)          $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.sndebug
bin/stu.prof:           conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin log
	@echo $$(cat conf/CXX) $(CXXFLAGS_PROF)             $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.prof
	@     $$(cat conf/CXX) $(CXXFLAGS_PROF)             $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.prof
bin/stu.cov:            conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin log
	@echo $$(cat conf/CXX) $(CXXFLAGS_COV)              $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.cov
	@     $$(cat conf/CXX) $(CXXFLAGS_COV)              $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.cov
bin/stu.sani_undefined: conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin log
	@echo $$(cat conf/CXX) $(CXXFLAGS_SANI)             $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.sani_undefined
	@     $$(cat conf/CXX) $(CXXFLAGS_SANI)             $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.sani_undefined
bin/stu.analyzer:       conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin log
	@echo $$(cat conf/CXX) $(CXXFLAGS_ANALYZER)         $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.analyzer
	@     $$(cat conf/CXX) $(CXXFLAGS_ANALYZER)         $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.analyzer

log/test_options:   sh/test_options src/options.hh man/stu.1.in
	@echo sh/test_options
	@     sh/test_options && mkdir -p log && touch $@
log/test_clean:  src/*.cc src/*.hh sh/test_clean sh sh/* tests tests/*/*
	@echo sh/test_clean
	@     sh/test_clean && mkdir -p log && touch $@
log/test_clean_last:  NEWS src/*.cc src/*.hh sh/test_clean_last sh sh/* tests tests/*/*
	@echo sh/test_clean_last
	@     sh/test_clean_last && mkdir -p log && touch $@

log/test_unit.debug:           bin/stu.debug          sh/test tests tests/*/*
	@echo sh/test
	@     sh/test && mkdir -p log && touch $@
log/test_unit.cdebug:          bin/stu.cdebug         sh/test tests tests/*/*
	@echo VARIANT=cdebug sh/test
	@     VARIANT=cdebug sh/test && mkdir -p log && touch $@
log/test_unit.ndebug:          bin/stu                sh/test tests tests/*/*
	@echo NDEBUG=1 sh/test
	@     NDEBUG=1 sh/test && mkdir -p log && touch $@
log/test_unit.sani_undefined:  bin/stu.sani_undefined sh/test tests tests/*/*
	@echo VARIANT=sani_undefined sh/test
	@     VARIANT=sani_undefined sh/test && mkdir -p log && touch $@

install:  sh/install bin/stu man/stu.1
	sh/install
clean:
	rm -Rf bin/ conf/ log/ cov/ src/version.hh

MANPAGE:  man/stu.1
	MANWIDTH=80 man man/stu.1 >MANPAGE
man/stu.1:  man/stu.1.in VERSION sh/mkman
	sh/mkman

cov:  log/cov
log/cov:  sh/cov_eval cov/OVERVIEW
	sh/cov_eval && touch log/cov
cov/OVERVIEW:  sh/cov_gcov bin/stu.cov-stu.gcda
	sh/cov_gcov
bin/stu.cov-stu.gcda:  bin/stu.cov sh/test tests tests/*/*
	rm -f bin/stu.cov-stu.gcda
	VARIANT=cov nolong=1 sh/test

prof: bin/analysis.prof
bin/analysis.prof:  bin/gmon.out
	gprof bin/stu.prof bin/gmon.out >bin/analysis.prof
bin/gmon.out:   bin/stu.prof tests/long-parallel-1/main.stu
	cd bin && ./stu.prof -j10 -f ../tests/long-parallel-1/main.stu && ../sh/rm_tmps

sani: log/test_unit.sani_undefined
analyzer:  bin/stu.analyzer
