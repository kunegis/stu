.POSIX:
all: bin/stu
test: \
    bin/stu.debug \
    man/stu.1 MANPAGE src/version.hh \
    bin/stu \
    log/test_options \
    log/test_unit.debug \
    log/test_clean \
    log/test_unit.ndebug \
    log/test_clean_last
.PHONY: all clean install test cov prof sani

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
CXXFLAGS_PROF= -pg -O2 -DNDEBUG
CXXFLAGS_COV=  --coverage -lgcov -O0 -DNDEBUG -DSTU_COV
CXXFLAGS_SANI= -pg -O2 -Werror -Wno-unused-result -fsanitize=undefined \
    -fsanitize-undefined-trap-on-error

bin/stu:                conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin
	@echo $$(cat conf/CXX) $$(cat conf/CXXFLAGS_NDEBUG) $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu
	@     $$(cat conf/CXX) $$(cat conf/CXXFLAGS_NDEBUG) $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu
bin/stu.debug:          conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin
	@echo $$(cat conf/CXX) $(CXXFLAGS_DEBUG)            $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.debug
	@     $$(cat conf/CXX) $(CXXFLAGS_DEBUG)            $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.debug
bin/stu.prof:           conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin
	@echo $$(cat conf/CXX) $(CXXFLAGS_PROF)             $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.prof
	@     $$(cat conf/CXX) $(CXXFLAGS_PROF)             $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.prof
bin/stu.cov:           conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin
	@echo $$(cat conf/CXX) $(CXXFLAGS_COV)              $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.cov
	@     $$(cat conf/CXX) $(CXXFLAGS_COV)              $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.cov
bin/stu.sani_undefined: conf/CXX src/*.cc src/*.hh src/version.hh
	@mkdir -p bin
	@echo $$(cat conf/CXX) $(CXXFLAGS_SANI)             $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.sani_undefined
	@     $$(cat conf/CXX) $(CXXFLAGS_SANI)             $$(cat conf/CXXFLAGS) src/stu.cc -o bin/stu.sani_undefined

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
log/test_unit.ndebug:          bin/stu                sh/test tests tests/*/*
	@echo NDEBUG=1 sh/test
	@     NDEBUG=1 sh/test && mkdir -p log && touch $@
log/test_unit.sani_undefined:  bin/stu.sani_undefined sh/test tests tests/*/*
	@echo VARIANT=sani_undefined sh/test
	@     VARIANT=sani_undefined sh/test && mkdir -p log && touch $@

install:  sh/install bin/stu man/stu.1
	sh/install
clean:
	rm -Rf bin/ conf/ log/ cov/

MANPAGE:  man/stu.1
	MANWIDTH=80 man man/stu.1 >MANPAGE
man/stu.1:  man/stu.1.in VERSION sh/mkman
	sh/mkman

cov:  bin/stu.cov sh/cov bin/stu.cov-stu.gcda
	sh/cov
bin/stu.cov-stu.gcda:  bin/stu.cov sh/test tests tests/*/*
	rm -f bin/stu.cov-stu.gcda
	VARIANT=cov sh/test

prof: bin/analysis.prof
bin/analysis.prof:  bin/gmon.out
	gprof bin/stu.prof bin/gmon.out >bin/analysis.prof
bin/gmon.out:   bin/stu.prof tests/long-parallel-1/main.stu
	cd bin && ./stu.prof -j10 -f ../tests/long-parallel-1/main.stu && ../sh/rm_tmps

sani: log/test_unit.sani_undefined
