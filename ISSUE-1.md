Hey Jerome,

since issues are disabled, I'll use a PR for reporting this.

- The test suite fails, with the attached error.
- The tests take a LOT of time, b/c of all those sleeps. How about removing them from the default make target?

```
hartmann@HHXPS:/home/hartmann/git/stu git:(master)
$ git pull
git pull
Already up-to-date.
hartmann@HHXPS:/home/hartmann/git/stu git:(master)
$ git rev-parse HEAD
git rev-parse HEAD
56f2a1387e5e8aa5dc54deaf9765a4ddf0c8a95e
hartmann@HHXPS:/home/hartmann/git/stu git:(master)
$ date
date
Sa 2. Apr 14:39:40 CEST 2016
hartmann@HHXPS:/home/hartmann/git/stu git:(master)
$ make
make
./mktest && touch check_test.debug
__________________________________
cd a
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd accents
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd already-present-dynamic
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd although
../../stu 
EXITCODE...1 correct
INSTDERR...correct
__________________________________
cd although-2
../../stu 
EXITCODE...1 correct
STDOUT...correct
STDERR...correct
__________________________________
cd backtick-command
../../stu 
EXITCODE...1 correct
INSTDERR...correct
__________________________________
cd bare-exist
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd bare-noexist
../../stu 
EXITCODE...1 correct
INSTDERR...correct
__________________________________
cd both-dependencies
../../stu -j2
EXITCODE...0 correct
../../stu -j2
Second invocation...correct
__________________________________
cd both-dynamic-and-dynamic-variable
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd catch-all
../../stu A
EXITCODE...0 correct
../../stu A
Second invocation...correct
__________________________________
cd chain
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd chain-2
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd chain-phony-command
./EXEC
../../stu
: CORRECT 
__________________________________
cd clash
./EXEC
LINE: A:  !? B { touch A }      B { touch B }
rm -f ? list.err
../../stu -f tmp.stu
LINE: A:  ?! B { touch A }      B { touch B }
rm -f ? list.err
../../stu -f tmp.stu
LINE: A:  !(? B) { touch A }    B { touch B }
rm -f ? list.err
../../stu -f tmp.stu
LINE: A:  ?(! B) { touch A }    B { touch B }
rm -f ? list.err
../../stu -f tmp.stu
LINE: A:  !((? B)) { touch A }  B { touch B }
rm -f ? list.err
../../stu -f tmp.stu
LINE: A:  ?((! B)) { touch A }  B { touch B }
rm -f ? list.err
../../stu -f tmp.stu
LINE: A:  !?((B)) { touch A }   B { touch B }
rm -f ? list.err
../../stu -f tmp.stu
LINE: A:  ?!((B)) { touch A }   B { touch B }
rm -f ? list.err
../../stu -f tmp.stu
LINE: A:  !?[B] { touch A }     >B { echo C }   
rm -f ? list.err
../../stu -f tmp.stu
LINE: A:  ?![B] { touch A }     >B { echo C }
rm -f ? list.err
../../stu -f tmp.stu
LINE: A:  ![B] { touch A }   >B { echo '?C' } 
rm -f ? list.err
../../stu -f tmp.stu
>B: echo '?C' 
LINE: A:  ?[B] { touch A }   >B { echo '!C' } 
rm -f B list.err
../../stu -f tmp.stu
>B: echo '!C' 
__________________________________
cd clash-parentheses
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd clash-phony
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd clean
./EXEC
rm -f A
../../stu
echo aaa >A
../../stu
../../stu @clean
rm -f A
../../stu A
echo aaa >A
../../stu A
../../stu @clean
rm -f A
__________________________________
cd colon-instead-of-command
../../stu 
EXITCODE...2 correct
__________________________________
cd command-outside-of-rule
../../stu 
EXITCODE...2 correct
__________________________________
cd command-place-1
../../stu 
EXITCODE...1 correct
STDOUT...correct
STDERR...correct
__________________________________
cd command-place-2
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd command-place-3
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd command-place-4
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd command-place-5
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd command-place-6
../../stu 
EXITCODE...1 correct
STDOUT...correct
STDERR...correct
__________________________________
cd command-place-7
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd command-syntax
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd command-trace
../../stu 
EXITCODE...1 correct
STDOUT...correct
STDERR...correct
__________________________________
cd comment
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd complex-weak
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
../../stu  -j 1
PARALLEL...correct
../../stu  -j 2
PARALLEL...correct
../../stu  -j 3
PARALLEL...correct
../../stu  -j 4
PARALLEL...correct
../../stu  -j 5
PARALLEL...correct
../../stu  -j 6
PARALLEL...correct
../../stu  -j 7
PARALLEL...correct
../../stu  -j 8
PARALLEL...correct
../../stu  -j 9
PARALLEL...correct
__________________________________
cd contiguous-parameters
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd continue
./EXEC
rm -f ?
../../stu -k
sleep 1
cat skjafhsoewiuowi >B
cat: skjafhsoewiuowi: No such file or directory
main.stu:13:4: command for 'B' failed with exit code 1
main.stu:8:4: 'B' is needed by 'A'
../../stu: *** Removing file 'B' because command failed
sleep 2
echo Cee >C
sleep 1
cat skjfdksjhfsjkhfdsjkfh >D
cat: skjfdksjhfsjkhfdsjkfh: No such file or directory
main.stu:23:2: command for 'D' failed with exit code 1
main.stu:8:8: 'D' is needed by 'A'
../../stu: *** Removing file 'D' because command failed
sleep 2
echo Eeee >E
cat sldfslkjfdkllsjdf >F
cat: sldfslkjfdkllsjdf: No such file or directory
main.stu:33:4: command for 'F' failed with exit code 1
main.stu:8:12: 'F' is needed by 'A'
../../stu: *** Removing file 'F' because command failed
../../stu: *** Targets not rebuilt because of errors
rm -f C E
../../stu -j2 -k
sleep 1
cat skjafhsoewiuowi >B
sleep 2
echo Cee >C
cat: skjafhsoewiuowi: No such file or directory
main.stu:13:4: command for 'B' failed with exit code 1
main.stu:8:4: 'B' is needed by 'A'
../../stu: *** Removing file 'B' because command failed
sleep 1
cat skjfdksjhfsjkhfdsjkfh >D
sleep 2
echo Eeee >E
cat: skjfdksjhfsjkhfdsjkfh: No such file or directory
main.stu:23:2: command for 'D' failed with exit code 1
main.stu:8:8: 'D' is needed by 'A'
../../stu: *** Removing file 'D' because command failed
cat sldfslkjfdkllsjdf >F
cat: sldfslkjfdkllsjdf: No such file or directory
main.stu:33:4: command for 'F' failed with exit code 1
main.stu:8:12: 'F' is needed by 'A'
../../stu: *** Removing file 'F' because command failed
../../stu: *** Targets not rebuilt because of errors
rm -f C E
../../stu -j 3 -k
sleep 1
cat skjafhsoewiuowi >B
sleep 2
echo Cee >C
sleep 1
cat skjfdksjhfsjkhfdsjkfh >D
cat: skjafhsoewiuowi: No such file or directory
main.stu:13:4: command for 'B' failed with exit code 1
main.stu:8:4: 'B' is needed by 'A'
../../stu: *** Removing file 'B' because command failed
sleep 2
echo Eeee >E
cat: skjfdksjhfsjkhfdsjkfh: No such file or directory
main.stu:23:2: command for 'D' failed with exit code 1
main.stu:8:8: 'D' is needed by 'A'
../../stu: *** Removing file 'D' because command failed
cat sldfslkjfdkllsjdf >F
cat: sldfslkjfdkllsjdf: No such file or directory
main.stu:33:4: command for 'F' failed with exit code 1
main.stu:8:12: 'F' is needed by 'A'
../../stu: *** Removing file 'F' because command failed
../../stu: *** Targets not rebuilt because of errors
__________________________________
cd continue-dependency-not-found
./EXEC
rm -f ?
../../stu
rm -f B
../../stu -k
__________________________________
cd continue-not-found
./EXEC
__________________________________
cd control-1
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd control-del
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd copy
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd createandfail
../../stu 
EXITCODE...1 correct
__________________________________
cd cross-dependency
../../stu -j2
EXITCODE...0 correct
../../stu -j2
Second invocation...correct
__________________________________
cd cycl-1
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd cycl-1a
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd cycl-2
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd cycl-2a
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd cycle-3
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd cycle-error-phony
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd cyclestrong
../../stu 
EXITCODE...2 correct
__________________________________
cd cyclestrong-rule-1
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd cyclestrong-rule-2
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd cyclestrong1
../../stu 
EXITCODE...2 correct
__________________________________
cd cyclestrong2
../../stu 
EXITCODE...2 correct
__________________________________
cd cyclic-detail-1
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd cyclic-detail-12
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd cyclic-detail-4
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd cyclic-detail-5
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd cyclic-detail-6
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd cyclic-detail-9
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd cyclic-phony
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd cyclic-phony-1
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd cyclic-phony-2
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd dangling
./EXEC
rm -f ?
ln -s W B
../../stu
STDOUT:
echo 'Executing B'
echo CORRECT >B
Executing B
cp B A
_______
STDERR:
_______
rm -f A B W
__________________________________
cd default-dynamic
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd dependency-trace
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd different-trivial
./EXEC
A:
_____
CORRECT1
CORRECT2
_____
__________________________________
cd dollar-semicolon
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd dominate-tail
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd domination
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd double-at
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd double-dependency
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd double-dynamic-dependency
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd double-exclam
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd double-exclam-variable
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd double-indirection
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd double-optional
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd double-parenthesis
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd double-quote
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd double-trivial
./EXEC
__________________________________
cd doubly-dynamic-phony
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd doubly-dynamic3
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd doubly-error
../../stu 
EXITCODE...2 correct
STDOUT...correct
STDERR...correct
__________________________________
cd doubly-within
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd doubly-within2
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd duplicate-parameter
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd duplicate-parameter-in-dependency
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd duplicate-rule
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd dynamic
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd dynamic-clash
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd dynamic-concurrent
../../stu -j3
EXITCODE...0 correct
../../stu -j3
Second invocation...correct
__________________________________
cd dynamic-cycle
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd dynamic-empty
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd dynamic-input
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd dynamic-mismatch-phony
../../stu 
EXITCODE...1 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd dynamic-multiple
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd dynamic-no-rebuild
./EXEC
rm -f ?
../../stu
set -e
echo '#! /bin/sh' >D 
echo 'echo C >B' >>D 
chmod u+x D
./D
echo ccc >C
set -e
ln -s notexistingfile W # This fails if executed twice
cp C A
rm D
sleep 2
../../stu
set -e
echo '#! /bin/sh' >D 
echo 'echo C >B' >>D 
chmod u+x D
./D
__________________________________
cd dynamic-no-rebuild-update
./EXEC
FIRST RUN
set -e
echo '#! /bin/sh' >D 
echo 'echo C >B' >>D 
chmod u+x D
./D
echo ccc >C
set -e
ln -s notexistingfile W # This fails if executed twice
cp C A
SECOND RUN
./D
__________________________________
cd dynamic-noclash
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd dynamic-paren-phony
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd dynamic-phony
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd dynamic-phony-k
./EXEC
../../stu -k
>B: echo bbb
__________________________________
cd dynamic-phony-trace
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd dynamic-phony-trace2
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd dynamic-phony2
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd dynamic-phony3
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd dynamic-spaces
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd dynamic-strong1
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd dynamic-strong2
../../stu 
EXITCODE...2 correct
STDOUT...correct
INSTDERR...correct
STDERR...correct
__________________________________
cd dynamic-target
../../stu 
EXITCODE...2 correct
__________________________________
cd dynamic-twist
../../stu -j2
EXITCODE...0 correct
../../stu -j2
Second invocation...correct
__________________________________
cd dynamic-variable-dependency
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd dynamic-variable-equal-parameter
../../stu 
EXITCODE...2 correct
INSTDERR...correct
INSTDERR...correct
__________________________________
cd dynamic-variable-in-dynamic-dependency
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd empty
./EXEC
__________________________________
cd empty-command
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd empty-filename-1
../../stu 
EXITCODE...2 correct
INSTDERR...correct
INSTDERR...correct
__________________________________
cd empty-filename-2
../../stu 
EXITCODE...2 correct
INSTDERR...correct
INSTDERR...correct
__________________________________
cd empty-filename-phony
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd empty-parameter
../../stu 
EXITCODE...2 correct
__________________________________
cd empty-parameter2
../../stu 
EXITCODE...2 correct
__________________________________
cd empty0
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd empty1
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd emptycommand
../../stu 
EXITCODE...1 correct
__________________________________
cd emptydep0
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd emptydep1
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd error-3
../../stu -k
EXITCODE...3 correct
STDERR...correct
__________________________________
cd error-b
../../stu 
EXITCODE...2 correct
__________________________________
cd error-child
../../stu 
EXITCODE...1 correct
__________________________________
cd error-upcase
../../stu -k
EXITCODE...1 correct
INSTDERR...correct
__________________________________
cd escape-backslash
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd exclam-question
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd existence-no-rebuild
./EXEC
FIRST RUN
echo bbb >B
ls -l B >A
ln -s nonexistingfile W # This will fail when run a second time
SECOND RUN
echo bbb >B
__________________________________
cd existence-norebuild
./EXEC
rm -f ?
touch A
sleep 2
../../stu
touch B 
__________________________________
cd existence-only-dependency
./EXEC
rm -rf data
../../stu
mkdir data 
[ -r data/B ] && exit 1
echo Hello >data/B
cp data/B A
touch data/X
../../stu
__________________________________
cd existence-only-dependency-parameters
./EXEC
rm -rf data-X-dir
../../stu
N=X: mkdir data-$N-dir 
N=X: echo Hello $N >data-$N-dir/list
N=X: cp data-$N-dir/list list.$N
cp list.X A
touch data-X-dir/unrelatedfile
../../stu
rm -rf data-X-dir list.output A list.X list.output
__________________________________
cd existence-variable
./EXEC
rm -f ? list.*
../../stu
>B: echo CORRECT 
>A: echo $B
sleep 2
touch B
../../stu
rm -f A B list.out
__________________________________
cd existence-variable-dependency
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd existing-dependency
../../stu 
EXITCODE...0 correct
NOTINSTDERR...correct
../../stu 
Second invocation...correct
__________________________________
cd expected
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd expected-a-dependency
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd explain-1
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd explain-2
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd exponential
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd fail
../../stu 
EXITCODE...1 correct
__________________________________
cd fail-dynamic
../../stu 
EXITCODE...1 correct
STDERR...correct
NOTINSTDERR...correct
__________________________________
cd fail-j-k
./EXEC
RUN (1)
rm -f ?
../../stu
sleep 1
exit 1
main.stu:18:5: command for 'B' failed with exit code 1
main.stu:13:5: 'B' is needed by 'A'
RUN (2)
rm -f ?
../../stu -k
sleep 2
sleep 1
exit 1
main.stu:18:5: command for 'B' failed with exit code 1
main.stu:13:5: 'B' is needed by 'A'
sleep 74634275
exit 0
kill -TERM 474
sleep 1
RUN (3)
rm -f ?
../../stu -j2
sleep 1
exit 1
sleep 74634275
exit 0
main.stu:18:5: command for 'B' failed with exit code 1
main.stu:13:5: 'B' is needed by 'A'
../../stu: *** Terminating all running jobs
RUN (4)
rm -f ?
../../stu -j2 -k
sleep 2
sleep 1
exit 1
sleep 74634275
exit 0
main.stu:18:5: command for 'B' failed with exit code 1
main.stu:13:5: 'B' is needed by 'A'
kill -TERM 499
__________________________________
cd false-lead
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd file-target-without-command
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd filename-after-dollar-bracket-exclamation
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd first-rule-parametrized
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd flags
./EXEC
== run A
NAME=a: echo ballsdlakjds >$NAME.c 
echo -g -Wall >CFLAGS 
echo -L >LDFLAGS 
echo Compiling ; echo $CFLAGS $LDFLAGS >a 
Compiling
NAME=b: echo ballsdlakjds >$NAME.c 
echo Compiling ; echo $CFLAGS $LDFLAGS >b 
Compiling
== run B
___
echo -g -Wall >CFLAGS 
___
== grep
__________________________________
cd flags-1
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd flags-2
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd flags-outside-variable
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd flags-outside-variable2
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd flags-outside-variable3
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd flags-shift
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd flat-1
../../stu 
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd flat-2
../../stu 
EXITCODE...2 correct
STDOUT...correct
STDERR...correct
__________________________________
cd flat-3
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd flat-4
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd flat-5
../../stu 
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
__________________________________
cd flat-6
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd flat-7
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd followed-by-parameter-name
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd future-created
./EXEC
__________________________________
cd future-existing
./EXEC
cp B A
__________________________________
cd group-0
../../stu 
EXITCODE...0 correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd group-1
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd group-2
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd group-3
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd group-4
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd group-5
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd group-6
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd group-7
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd group-8
./EXEC
>B: echo CORRECT 
<B:
[ "$FAIL" ] && exit 1
cat >A 
__________________________________
cd group-9
./EXEC
>B: echo CORRECT 
[ "$FAIL" ] && exit 1
cp B A
__________________________________
cd help-newline
./EXEC
__________________________________
cd help-notab
./EXEC
__________________________________
cd higher-order-nonedge
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd in-target
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd include
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd include-diamond
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd include-diamond2
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd include-dynamic
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd include-end
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd include-error
../../stu 
EXITCODE...2 correct
INSTDERR...correct
INSTDERR...correct
STDERR...correct
__________________________________
cd include-in-dynamic-dependency
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd include-inc
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd include-nofile
../../stu 
EXITCODE...4 correct
INSTDERR...correct
__________________________________
cd include-nofile2
../../stu 
EXITCODE...4 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd include-nofilename
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd include-noinclude
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd include-parametrized
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd include-recursion-direct
../../stu 
EXITCODE...2 correct
INSTDERR...correct
INSTDERR...correct
STDERR...correct
__________________________________
cd include-recursion-indirect
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd include-recursion-indirect2
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd infinite-dynamic-descent
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd input-correct
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd input-dynamic
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd input-exclam-variable
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd input-existence-only
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd input-included
../../stu 
EXITCODE...2 correct
STDOUT...correct
STDERR...correct
__________________________________
cd input-multiple
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd input-nocommand
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd input-output
../../stu 
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd input-outputand
../../stu 
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd input-outputand2
../../stu 
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd input-override1
../../stu 
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd input-override2
../../stu 
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd input-phony
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd input-unreadable
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd input-variable
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd int-remove-files
./EXEC
rm -f ?
../../stu -k -j99
sleep 1
echo >B ; sleep 31415926535 ; echo Bla >>B 
echo >C ; sleep 31415926535 ; echo Bla >>C 
echo >D ; sleep 31415926535 ; echo Bla >>D 
echo >E ; sleep 31415926535 ; echo Bla >>E 
echo >F ; sleep 31415926535 ; echo Bla >>F 
echo >G ; sleep 31415926535 ; echo Bla >>G 
echo >H ; sleep 31415926535 ; echo Bla >>H 
echo >I ; sleep 31415926535 ; echo Bla >>I 
echo >J ; sleep 31415926535 ; echo Bla >>J 
echo >K ; sleep 31415926535 ; echo Bla >>K 
echo >L ; sleep 31415926535 ; echo Bla >>L 
echo >M ; sleep 31415926535 ; echo Bla >>M 
echo >N ; sleep 31415926535 ; echo Bla >>N 
echo >O ; sleep 31415926535 ; echo Bla >>O 
echo >P ; sleep 31415926535 ; echo Bla >>P 
echo >Q ; sleep 31415926535 ; echo Bla >>Q 
echo >R ; sleep 31415926535 ; echo Bla >>R 
echo >S ; sleep 31415926535 ; echo Bla >>S 
echo >T ; sleep 31415926535 ; echo Bla >>T 
echo >U ; sleep 31415926535 ; echo Bla >>U 
echo >V ; sleep 31415926535 ; echo Bla >>V 
echo >W ; sleep 31415926535 ; echo Bla >>W 
echo >X ; sleep 31415926535 ; echo Bla >>X 
echo >Y ; sleep 31415926535 ; echo Bla >>Y 
echo >Z ; sleep 31415926535 ; echo Bla >>Z 
kill -TERM 1981
sleep 1
../../stu: *** Removing partially built files
rm -f ?
__________________________________
cd intermediary-phony
./EXEC
CORRECT
: ALL HEADERS BUILT 
: ALL HEADERS BUILT 
__________________________________
cd invalid-equal
../../stu 
EXITCODE...2 correct
__________________________________
cd invalid-escape
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd invalid-token-after-at
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd invalid-token-after-questionmark
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd invalid-token-at-end-of-dynamic-variable
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd invalid-token-at-end-of-rule
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd invalid-token-in-phony
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd isdir
../../stu 
EXITCODE...4 correct
STDERR...correct
__________________________________
cd jobs
../../stu -j lsdkjflskdjf
EXITCODE...4 correct
STDERR...correct
__________________________________
cd jobs2
../../stu -j -1000
EXITCODE...4 correct
STDERR...correct
__________________________________
cd jobs3
../../stu -j 0
EXITCODE...4 correct
STDERR...correct
__________________________________
cd jobs4
../../stu -j 5X
EXITCODE...4 correct
STDERR...correct
__________________________________
cd k-single-error
../../stu -k
EXITCODE...1 correct
STDERR...correct
__________________________________
cd killed
../../stu 
EXITCODE...1 correct
STDOUT...correct
STDERR...correct
__________________________________
cd layered-phonies
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd length-zero-cycle
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd lone-colon
../../stu 
EXITCODE...2 correct
__________________________________
cd lone-equal
../../stu 
EXITCODE...2 correct
__________________________________
cd lone-langle
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd lone-rangle
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd lone-rbracket
../../stu 
EXITCODE...2 correct
__________________________________
cd lone-rbracket2
../../stu 
EXITCODE...2 correct
__________________________________
cd long-help
../../stu --help
EXITCODE...4 correct
STDERR...correct
__________________________________
cd midbuild
./EXEC
echo ccc >C
cp C B
cp B A
cp C B
__________________________________
cd midbuild-indirect
./EXEC
echo ccc >C
cp C B
cp B A
cp C B
__________________________________
cd missing-dependency-after-questionmark
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd missing-filename-after-langle
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd missing-name-in-phony
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd missing-name-of-phony-dependency
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd missing-rangle
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd missing-rangle-after-dollar-langle
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd multi
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd multiarg-dynamic
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd multilayer
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd multiline
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd multiple-error
../../stu -k
EXITCODE...1 correct
STDERR...correct
__________________________________
cd multiple-parameter-parses
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd multiple-parametrized-rules-with-same-number-of-parameters
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd multiply-dynamic-dependency
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd no-command-no-exist
../../stu 
EXITCODE...1 correct
__________________________________
cd no-command-older
./EXEC
Nothing to be done
__________________________________
cd no-default-file-no-target
../../stu 
EXITCODE...2 correct
__________________________________
cd no-dependency
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd no-dependency-or-command
../../stu 
EXITCODE...2 correct
__________________________________
cd no-equal-in-dynamic-variable
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd no-file-exists-message
./EXEC
rm -f A
../../stu
__________________________________
cd no-filename-after-dollar-langle
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd no-future-existence
./EXEC
__________________________________
cd no-nothing
../../stu this-file-does-not-exist
EXITCODE...1 correct
INSTDERR...correct
__________________________________
cd no-output-variable
../../stu 
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd no-remake
./EXEC
rm -rf ?
../../stu A
echo Hello >B
echo B >C
cp B A
rm -rf C
../../stu A
echo B >C
rm -rf C
../../stu A
__________________________________
cd no-rm-old
./EXEC
touch A
sleep 2
../../stu
__________________________________
cd no-rule-but-file-exists
../../stu code.h
EXITCODE...0 correct
INSTDOUT...correct
../../stu code.h
Second invocation...correct
__________________________________
cd no-rules-but-file-exists
../../stu code.h
EXITCODE...0 correct
INSTDOUT...correct
../../stu code.h
Second invocation...correct
__________________________________
cd no-side-effect
../../stu 
EXITCODE...1 correct
INSTDERR...correct
__________________________________
cd no-such-variable
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd no-target
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd no-variable-set-parent
../../stu 
./mktest: *** Test 'no-variable-set-parent' should have returned 0, but returned 1
./mktest: *** Target 'A' was not built
__________________________________
cd nocommandaftertarget
../../stu 
EXITCODE...2 correct
__________________________________
cd nocreatedwith-trace
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd nodependencies-1
../../stu 
EXITCODE...1 correct
STDOUT...correct
STDERR...correct
__________________________________
cd nodependencies-2
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd non-parameter-bracket
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd nonexisting
../../stu 
EXITCODE...1 correct
__________________________________
cd nonexisting-cmd
../../stu ljhskdjfhskjhfsdjkdfh
EXITCODE...1 correct
__________________________________
cd nonexisting-long
../../stu 
EXITCODE...1 correct
__________________________________
cd nonexisting-long3
../../stu 
EXITCODE...1 correct
__________________________________
cd nonexisting-long4
../../stu 
EXITCODE...1 correct
INSTDERR...correct
__________________________________
cd nonexisting-phony
../../stu 
EXITCODE...1 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd nonexisting-phony-argv
./EXEC
__________________________________
cd nonoptional
../../stu -g
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
../../stu -g
Second invocation...correct
__________________________________
cd nonoptional-dynamic
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd nontrivial
./EXEC
rm -f ?
../../stu -a
sleep 2
touch B C
../../stu -a
__________________________________
cd norebuild
./EXEC
rm -f ?
../../stu
>B: echo C   
>C: echo D E 
>D: echo ddd 
>E: echo eee 
>A: cat D E  
rm -f B C
../../stu
__________________________________
cd nosuchfile
../../stu 
EXITCODE...4 correct
STDERR...correct
__________________________________
cd not-built-twice
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd not-cycle
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd notbuilt
../../stu 
EXITCODE...1 correct
__________________________________
cd notbuilt-trace
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd nothing-at-all
../../stu 
EXITCODE...2 correct
__________________________________
cd null-quoted
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd null-unquoted
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd one-parameter-over-two
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd openat
../../stu -f ddd
EXITCODE...0 correct
CONTENT...correct
../../stu -f ddd
Second invocation...correct
__________________________________
cd openat2
../../stu 
EXITCODE...4 correct
STDERR...correct
__________________________________
cd openat3
../../stu -f ddd/
EXITCODE...4 correct
STDERR...correct
__________________________________
cd openat4
../../stu -f ddd/
EXITCODE...0 correct
CONTENT...correct
../../stu -f ddd/
Second invocation...correct
__________________________________
cd option-c-1
../../stu -c A
EXITCODE...0 correct
../../stu -c A
Second invocation...correct
__________________________________
cd option-c-10
../../stu -c 'A#'
EXITCODE...0 correct
../../stu -c 'A#'
Second invocation...correct
__________________________________
cd option-c-11
../../stu -c 'C B A'
EXITCODE...0 correct
CONTENT...correct
../../stu -c 'C B A'
Second invocation...correct
__________________________________
cd option-c-12
./EXEC
__________________________________
cd option-c-13
../../stu -c '!A'
EXITCODE...0 correct
../../stu -c '!A'
Second invocation...correct
__________________________________
cd option-c-14
../../stu -c '%include "xxx.stu"'
EXITCODE...2 correct
STDERR...correct
__________________________________
cd option-c-15
./EXEC
__________________________________
cd option-c-16
./EXEC
__________________________________
cd option-c-17
./EXEC
__________________________________
cd option-c-18
./EXEC
__________________________________
cd option-c-19
../../stu [X]
EXITCODE...0 correct
CONTENT...correct
../../stu [X]
Second invocation...correct
__________________________________
cd option-c-2
../../stu -c '"list.B "'
EXITCODE...0 correct
../../stu -c '"list.B "'
Second invocation...correct
__________________________________
cd option-c-3
../../stu -c [B]
EXITCODE...0 correct
../../stu -c [B]
Second invocation...correct
__________________________________
cd option-c-4
../../stu -c [[B]]
EXITCODE...0 correct
../../stu -c [[B]]
Second invocation...correct
__________________________________
cd option-c-5
../../stu -c @x
EXITCODE...0 correct
../../stu -c @x
Second invocation...correct
__________________________________
cd option-c-6
../../stu -c 'A:'
EXITCODE...2 correct
__________________________________
cd option-c-7
../../stu -c 'A;'
EXITCODE...2 correct
STDERR...correct
__________________________________
cd option-c-8
../../stu -c '[A'
EXITCODE...2 correct
STDERR...correct
__________________________________
cd option-c-9
../../stu -c '@'
EXITCODE...2 correct
STDERR...correct
__________________________________
cd option-f-empty
./EXEC
__________________________________
cd option-f-nosuchfile
../../stu -f askjdhkjashskjahskdjh
EXITCODE...4 correct
STDOUT...correct
STDERR...correct
__________________________________
cd optional-1
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-2
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-3
./EXEC
[ -r B ] || exit 1
cp B A
exitcode=0
__________________________________
cd optional-4
./EXEC
cp B A 
exitcode=0
__________________________________
cd optional-5a
../../stu 
EXITCODE...0 correct
__________________________________
cd optional-5b
../../stu -j2
EXITCODE...0 correct
__________________________________
cd optional-6a
../../stu 
EXITCODE...0 correct
__________________________________
cd optional-6b
../../stu -j2
EXITCODE...0 correct
__________________________________
cd optional-7
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-8
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-direct-dynamic
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-double-dynamic
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-dynamic
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-dynamic-phony
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-dynamic-rebuild
./EXEC
touch A 
sleep 2
echo X >C
sleep 2
touch X 
touch A 
__________________________________
cd optional-dynamic2
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-indirect
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-input-variable
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-nonoptional
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-nonoptional-dynamic
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-transitive-phony
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-twice
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd optional-variable
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd options
./EXEC
rm -f files list.*
../../stu
rm -f files list.1 list.err list.out
../../stu -k
rm -f files list.1 list.3 list.err list.out
../../stu -j5
rm -f files list.1 list.3 list.err list.out
../../stu -j5 -k
rm -f files list.1 list.3 list.err list.out
__________________________________
cd order
../../stu 
EXITCODE...0 correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd order-dfs
../../stu -m dfs
EXITCODE...0 correct
STDOUT...correct
../../stu -m dfs
Second invocation...correct
__________________________________
cd order-random
./EXEC
../../stu
../../stu -m random
list.out stdout-correct differ: byte 2, line 1
../../stu -m random
list.out list.out2 differ: byte 2, line 1
__________________________________
cd order-seed
./EXEC
../../stu
../../stu -M Hello
list.out stdout-correct differ: byte 2, line 1
../../stu -M Hello
__________________________________
cd order-xxx
../../stu -m ksjhfckwuhef
EXITCODE...4 correct
STDERR...correct
__________________________________
cd out-z
../../stu list.widths.z
EXITCODE...0 correct
../../stu list.widths.z
Second invocation...correct
__________________________________
cd output
../../stu 
EXITCODE...0 correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd output-nocommand
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd outside-optional
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd override-existence-only
./EXEC
rm -f ? list.*
../../stu -v
VERBOSE  main.next
VERBOSE     ROOT execute  {}
VERBOSE     ROOT deploy Link('A', , {})
VERBOSE        'A' Rule('A':  !'B', 'B')
VERBOSE        'A' push Link(!'B', !, {!})
VERBOSE        'A' push Link('B', , {})
VERBOSE        'A' execute  {}
VERBOSE        'A' deploy Link(!'B', !, {!})
VERBOSE           'B' Rule('B':  )
VERBOSE           'B' execute ! {!}
echo CORRECT >B
VERBOSE           'B' execute pid = 5580
VERBOSE  wait
VERBOSE  wait pid = 5580
VERBOSE  main.next
VERBOSE     ROOT execute  {}
VERBOSE        'A' execute  {}
VERBOSE           'B' execute ! {!}
VERBOSE           'B' finished
VERBOSE        'A' unlink 'B' {!?&}
VERBOSE        'A' deploy Link('B', , {})
VERBOSE           'B' execute  {}
VERBOSE           'B' finished
VERBOSE        'A' unlink 'B' {!?&}
VERBOSE        'A' deploy Link(!'B', !*, {!})
VERBOSE           'B' execute !* {!}
VERBOSE           'B' finished
VERBOSE        'A' unlink 'B' {!?&}
VERBOSE        'A' deploy Link('B', *, {})
VERBOSE           'B' execute * {}
VERBOSE           'B' finished
VERBOSE        'A' unlink 'B' {!?&}
cp B A
VERBOSE        'A' execute pid = 5581
VERBOSE  wait
VERBOSE  wait pid = 5581
VERBOSE  main.next
VERBOSE     ROOT execute  {}
VERBOSE        'A' execute  {}
VERBOSE        'A' finished
VERBOSE     ROOT unlink 'A' {!?&}
sleep 2
touch B
../../stu -v
VERBOSE  main.next
VERBOSE     ROOT execute  {}
VERBOSE     ROOT deploy Link('A', , {})
VERBOSE        'A' Rule('A':  !'B', 'B')
VERBOSE        'A' push Link(!'B', !, {!})
VERBOSE        'A' push Link('B', , {})
VERBOSE        'A' execute  {}
VERBOSE        'A' deploy Link(!'B', !, {!})
VERBOSE           'B' Rule('B':  )
VERBOSE           'B' execute ! {!}
VERBOSE        'A' unlink 'B' {?&}
VERBOSE        'A' deploy Link('B', , {})
VERBOSE           'B' execute  {}
VERBOSE        'A' unlink 'B' {!?&}
VERBOSE        'A' deploy Link(!'B', !*, {!})
VERBOSE           'B' execute !* {!}
VERBOSE           'B' finished
VERBOSE        'A' unlink 'B' {!?&}
VERBOSE        'A' deploy Link('B', *, {})
VERBOSE           'B' execute * {}
VERBOSE           'B' finished
VERBOSE        'A' unlink 'B' {!?&}
VERBOSE        'A' execute pid = 5587
VERBOSE  wait
cp B A
VERBOSE  wait pid = 5587
VERBOSE  main.next
VERBOSE     ROOT execute  {}
VERBOSE        'A' execute  {}
VERBOSE        'A' finished
VERBOSE     ROOT unlink 'A' {!?&}
rm -f A B list.out
__________________________________
cd parallel
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
../../stu  -j 1
PARALLEL...correct
../../stu  -j 2
PARALLEL...correct
../../stu  -j 3
PARALLEL...correct
../../stu  -j 4
PARALLEL...correct
../../stu  -j 5
PARALLEL...correct
../../stu  -j 6
PARALLEL...correct
../../stu  -j 7
PARALLEL...correct
../../stu  -j 8
PARALLEL...correct
../../stu  -j 9
PARALLEL...correct
../../stu  -j 10
PARALLEL...correct
../../stu  -j 11
PARALLEL...correct
__________________________________
cd param-000
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd param-001
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd param-002
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd param-003
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd param-004
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd param-005
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd param-006
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd param-007
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd param-008
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd parameter-dynamic
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd parameter-dynamic-2
../../stu -k
EXITCODE...2 correct
STDERR...correct
__________________________________
cd parameter-match-dominates-two
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd parameter-match-no-domination
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd parameter-match-order
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd parameter-match-substring
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd parameter-not-used
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd parameter-not-used-compact
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd parameter-not-used2
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd parameter-order
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd parameter-trace
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd parameters
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd parameters-dynamic
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd parametrized-dynamics
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd parametrized-phony-without-command
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd partial
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd partial-no-rule
../../stu 
EXITCODE...1 correct
INSTDERR...correct
__________________________________
cd pass
./EXEC
list.file.stu: A: !B ?C D E {touch A} B{touch B} C{touch C} D{touch D} E{touch E}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: !(B) ?(C) (D) (E) {touch A} B{touch B} C{touch C} D{touch D} E{touch E}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: (!B) (?C) (D) (E) {touch A} B{touch B} C{touch C} D{touch D} E{touch E}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: (!B ?C D E) {touch A} B{touch B} C{touch C} D{touch D} E{touch E}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: !((B)) ?((C)) ((D)) ((E)) {touch A} B{touch B} C{touch C} D{touch D} E{touch E}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: ((!B)) ((?C)) ((D)) ((E)) {touch A} B{touch B} C{touch C} D{touch D} E{touch E}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: ((!B ?C D E)) {touch A} B{touch B} C{touch C} D{touch D} E{touch E}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: !!B ??C D E {touch A} B{touch B} C{touch C} D{touch D} E{touch E}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: !B ?C {touch A}	B: D {touch B}	C: E {touch C} D{touch D} E{touch E}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: !@b ?@c D E {touch A}	@b: B;	@c: C; B{touch B} C{touch C} D{touch D} E{touch E}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: !@b ?@c D E {touch A}	@b: @b2;	@b2: B;	@c: @c2;	@c2: C; B{touch B} C{touch C} D{touch D} E{touch E}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: [!B] [?C] D E {touch A}	>B { echo D }	>C { echo E } D{touch D} E{touch E} X{touch X}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: [!B ?C] D E {touch A}	>B { echo D }	>C { echo E } D{touch D} E{touch E} X{touch X}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
list.file.stu: A: ![D] ?[E] {touch A}	>D { echo B }	>E { echo C } B{touch B} C{touch C}  X{touch X}
../../stu -f list.file.stu
olden
touch B
../../stu -f list.file.stu
olden
echo X >C
../../stu -f list.file.stu
olden
echo X >D
../../stu -f list.file.stu
__________________________________
cd past-unchanged
./EXEC
ls -l B
-rw-rw-r-- 1 hartmann hartmann 0 Jan  1  2014 B
echo ccc >C
# Do nothing 
__________________________________
cd pgid
./EXEC
rm -f X Y Z
../../stu -j2 &
sleep 2
echo begin >X
sleep 1
echo end >>X
echo begin >Y
sleep 2837415
echo end >>Y
echo begin >Z
sleep 2837415s
echo end >>Z
kill -TERM 7180
sleep 1
../../stu: *** Removing partially built files
rm -f X
__________________________________
cd phonies-dynamics-parallelism
./EXEC
../../stu -j 20
    for i in $(seq 1 100) ; do
		echo "list.$i"
    done >All
N=1:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=2:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=3:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=4:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=5:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=6:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=7:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=8:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=9:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=10:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=11:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=12:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=13:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=14:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=15:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=16:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=17:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=18:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=19:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=20:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=21:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=22:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=23:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=24:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=25:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=26:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=27:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=28:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=29:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=30:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=31:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=32:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=33:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=34:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=35:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=36:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=37:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=38:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=39:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=40:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=41:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=42:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=43:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=44:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=45:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=46:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=47:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=48:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=49:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=50:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=51:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=52:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=53:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=54:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=55:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=56:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=57:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=58:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=59:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=60:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=61:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=62:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=63:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=64:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=65:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=66:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=67:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=68:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=69:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=70:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=71:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=72:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=73:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=74:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=75:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=76:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=77:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=78:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=79:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=80:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=81:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=82:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=83:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=84:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=85:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=86:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=87:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=88:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=89:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=90:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=91:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=92:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=93:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=94:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=95:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=96:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=97:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=98:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=99:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=100:
sleep 1
echo ${N}${N}${N} >"list.$N"
../../stu -j 30
    for i in $(seq 1 100) ; do
		echo "list.$i"
    done >All
N=1:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=2:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=3:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=4:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=5:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=6:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=7:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=8:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=9:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=10:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=11:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=12:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=13:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=14:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=15:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=16:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=17:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=18:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=19:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=20:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=21:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=22:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=23:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=24:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=25:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=26:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=27:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=28:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=29:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=30:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=31:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=32:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=33:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=34:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=35:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=36:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=37:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=38:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=39:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=40:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=41:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=42:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=43:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=44:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=45:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=46:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=47:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=48:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=49:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=50:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=51:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=52:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=53:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=54:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=55:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=56:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=57:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=58:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=59:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=60:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=61:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=62:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=63:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=64:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=65:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=66:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=67:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=68:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=69:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=70:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=71:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=72:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=73:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=74:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=75:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=76:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=77:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=78:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=79:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=80:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=81:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=82:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=83:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=84:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=85:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=86:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=87:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=88:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=89:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=90:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=91:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=92:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=93:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=94:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=95:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=96:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=97:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=98:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=99:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=100:
sleep 1
echo ${N}${N}${N} >"list.$N"
../../stu -j 50
    for i in $(seq 1 100) ; do
		echo "list.$i"
    done >All
N=1:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=2:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=3:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=4:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=5:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=6:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=7:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=8:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=9:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=10:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=11:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=12:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=13:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=14:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=15:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=16:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=17:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=18:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=19:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=20:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=21:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=22:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=23:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=24:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=25:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=26:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=27:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=28:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=29:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=30:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=31:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=32:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=33:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=34:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=35:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=36:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=37:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=38:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=39:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=40:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=41:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=42:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=43:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=44:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=45:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=46:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=47:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=48:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=49:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=50:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=51:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=52:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=53:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=54:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=55:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=56:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=57:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=58:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=59:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=60:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=61:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=62:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=63:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=64:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=65:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=66:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=67:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=68:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=69:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=70:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=71:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=72:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=73:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=74:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=75:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=76:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=77:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=78:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=79:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=80:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=81:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=82:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=83:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=84:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=85:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=86:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=87:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=88:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=89:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=90:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=91:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=92:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=93:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=94:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=95:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=96:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=97:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=98:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=99:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=100:
sleep 1
echo ${N}${N}${N} >"list.$N"
../../stu -j 90
    for i in $(seq 1 100) ; do
		echo "list.$i"
    done >All
N=1:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=2:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=3:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=4:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=5:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=6:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=7:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=8:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=9:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=10:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=11:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=12:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=13:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=14:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=15:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=16:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=17:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=18:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=19:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=20:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=21:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=22:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=23:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=24:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=25:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=26:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=27:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=28:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=29:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=30:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=31:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=32:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=33:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=34:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=35:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=36:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=37:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=38:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=39:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=40:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=41:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=42:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=43:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=44:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=45:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=46:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=47:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=48:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=49:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=50:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=51:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=52:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=53:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=54:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=55:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=56:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=57:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=58:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=59:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=60:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=61:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=62:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=63:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=64:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=65:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=66:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=67:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=68:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=69:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=70:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=71:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=72:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=73:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=74:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=75:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=76:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=77:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=78:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=79:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=80:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=81:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=82:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=83:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=84:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=85:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=86:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=87:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=88:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=89:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=90:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=91:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=92:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=93:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=94:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=95:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=96:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=97:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=98:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=99:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=100:
sleep 1
echo ${N}${N}${N} >"list.$N"
../../stu -j 99
    for i in $(seq 1 100) ; do
		echo "list.$i"
    done >All
N=1:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=2:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=3:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=4:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=5:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=6:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=7:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=8:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=9:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=10:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=11:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=12:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=13:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=14:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=15:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=16:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=17:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=18:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=19:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=20:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=21:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=22:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=23:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=24:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=25:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=26:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=27:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=28:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=29:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=30:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=31:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=32:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=33:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=34:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=35:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=36:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=37:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=38:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=39:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=40:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=41:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=42:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=43:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=44:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=45:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=46:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=47:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=48:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=49:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=50:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=51:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=52:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=53:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=54:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=55:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=56:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=57:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=58:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=59:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=60:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=61:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=62:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=63:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=64:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=65:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=66:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=67:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=68:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=69:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=70:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=71:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=72:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=73:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=74:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=75:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=76:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=77:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=78:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=79:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=80:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=81:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=82:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=83:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=84:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=85:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=86:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=87:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=88:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=89:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=90:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=91:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=92:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=93:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=94:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=95:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=96:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=97:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=98:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=99:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=100:
sleep 1
echo ${N}${N}${N} >"list.$N"
../../stu -j 100
    for i in $(seq 1 100) ; do
		echo "list.$i"
    done >All
N=1:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=2:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=3:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=4:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=5:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=6:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=7:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=8:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=9:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=10:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=11:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=12:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=13:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=14:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=15:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=16:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=17:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=18:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=19:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=20:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=21:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=22:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=23:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=24:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=25:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=26:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=27:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=28:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=29:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=30:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=31:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=32:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=33:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=34:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=35:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=36:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=37:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=38:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=39:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=40:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=41:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=42:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=43:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=44:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=45:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=46:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=47:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=48:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=49:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=50:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=51:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=52:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=53:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=54:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=55:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=56:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=57:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=58:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=59:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=60:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=61:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=62:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=63:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=64:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=65:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=66:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=67:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=68:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=69:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=70:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=71:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=72:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=73:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=74:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=75:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=76:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=77:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=78:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=79:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=80:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=81:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=82:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=83:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=84:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=85:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=86:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=87:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=88:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=89:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=90:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=91:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=92:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=93:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=94:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=95:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=96:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=97:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=98:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=99:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=100:
sleep 1
echo ${N}${N}${N} >"list.$N"
../../stu -j 101
    for i in $(seq 1 100) ; do
		echo "list.$i"
    done >All
N=1:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=2:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=3:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=4:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=5:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=6:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=7:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=8:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=9:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=10:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=11:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=12:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=13:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=14:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=15:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=16:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=17:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=18:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=19:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=20:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=21:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=22:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=23:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=24:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=25:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=26:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=27:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=28:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=29:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=30:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=31:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=32:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=33:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=34:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=35:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=36:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=37:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=38:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=39:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=40:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=41:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=42:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=43:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=44:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=45:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=46:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=47:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=48:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=49:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=50:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=51:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=52:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=53:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=54:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=55:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=56:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=57:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=58:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=59:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=60:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=61:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=62:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=63:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=64:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=65:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=66:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=67:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=68:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=69:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=70:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=71:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=72:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=73:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=74:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=75:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=76:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=77:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=78:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=79:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=80:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=81:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=82:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=83:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=84:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=85:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=86:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=87:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=88:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=89:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=90:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=91:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=92:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=93:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=94:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=95:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=96:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=97:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=98:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=99:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=100:
sleep 1
echo ${N}${N}${N} >"list.$N"
../../stu -j 1234
    for i in $(seq 1 100) ; do
		echo "list.$i"
    done >All
N=1:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=2:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=3:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=4:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=5:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=6:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=7:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=8:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=9:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=10:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=11:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=12:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=13:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=14:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=15:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=16:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=17:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=18:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=19:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=20:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=21:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=22:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=23:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=24:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=25:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=26:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=27:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=28:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=29:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=30:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=31:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=32:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=33:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=34:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=35:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=36:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=37:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=38:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=39:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=40:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=41:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=42:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=43:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=44:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=45:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=46:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=47:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=48:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=49:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=50:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=51:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=52:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=53:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=54:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=55:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=56:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=57:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=58:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=59:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=60:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=61:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=62:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=63:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=64:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=65:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=66:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=67:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=68:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=69:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=70:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=71:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=72:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=73:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=74:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=75:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=76:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=77:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=78:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=79:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=80:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=81:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=82:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=83:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=84:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=85:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=86:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=87:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=88:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=89:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=90:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=91:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=92:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=93:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=94:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=95:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=96:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=97:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=98:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=99:
sleep 1
echo ${N}${N}${N} >"list.$N"
N=100:
sleep 1
echo ${N}${N}${N} >"list.$N"
__________________________________
cd phony
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd phony-conflict-message
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd phony-dynamic-indirect
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd phony-file-mismatch
../../stu 
EXITCODE...1 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd phony-in-dynamics
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd phony-in-dynamics2
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd phony-input
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd phony-not-twice
./EXEC
../../stu
# This will fail if executed multiple times 
ln -s nonexistingfile L 
__________________________________
cd phony-nothing
./EXEC
Nothing to be done
__________________________________
cd phony-optional
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd phony-place
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd phony-print
./EXEC
../../stu
: HELLO 
__________________________________
cd phony-quote
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd place-deponly-dependency
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd preexisting-dynamic
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd preexisting-optional-dynamic
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd pretty-command
../../stu 
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd question-exclam
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd question-redirect
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd random-restart
./EXEC
N=2 X=q:
sleep 1
touch list.$X.$N 
N=2 X=o:
sleep 1
touch list.$X.$N 
N=2 X=x:
sleep 1
touch list.$X.$N 
N=2 X=v:
sleep 1
touch list.$X.$N 
N=2 X=i:
sleep 1
touch list.$X.$N 
N=2 X=n:
sleep 1
touch list.$X.$N 
N=1 X=d:
sleep 1
touch list.$X.$N 
N=1 X=g:
sleep 1
touch list.$X.$N 
N=2 X=z:
sleep 1
touch list.$X.$N 
N=1 X=k:
sleep 1
touch list.$X.$N 
N=1 X=m:
sleep 1
touch list.$X.$N 
N=2 X=f:
sleep 1
touch list.$X.$N 
N=2 X=c:
sleep 1
touch list.$X.$N 
N=2 X=r:
sleep 1
touch list.$X.$N 
N=2 X=l:
sleep 1
touch list.$X.$N 
N=2 X=h:
sleep 1
touch list.$X.$N 
N=1 X=y:
sleep 1
touch list.$X.$N 
N=1 X=e:
sleep 1
touch list.$X.$N 
N=2 X=a:
sleep 1
touch list.$X.$N 
N=1 X=p:
sleep 1
touch list.$X.$N 
N=1 X=j:
sleep 1
touch list.$X.$N 
N=2 X=u:
sleep 1
touch list.$X.$N 
N=2 X=w:
sleep 1
touch list.$X.$N 
N=1 X=s:
sleep 1
touch list.$X.$N 
N=2 X=b:
sleep 1
touch list.$X.$N 
N=1 X=t:
sleep 1
touch list.$X.$N 
N=2 X=j:
sleep 1
touch list.$X.$N 
N=1 X=r:
sleep 1
touch list.$X.$N 
N=2 X=e:
sleep 1
touch list.$X.$N 
N=2 X=g:
sleep 1
touch list.$X.$N 
N=1 X=o:
sleep 1
touch list.$X.$N 
N=1 X=n:
sleep 1
touch list.$X.$N 
N=1 X=q:
sleep 1
touch list.$X.$N 
N=1 X=u:
sleep 1
touch list.$X.$N 
N=1 X=i:
sleep 1
touch list.$X.$N 
N=2 X=s:
sleep 1
touch list.$X.$N 
N=1 X=h:
sleep 1
touch list.$X.$N 
N=1 X=x:
sleep 1
touch list.$X.$N 
N=2 X=p:
sleep 1
touch list.$X.$N 
N=1 X=v:
sleep 1
touch list.$X.$N 
N=1 X=f:
sleep 1
touch list.$X.$N 
N=1 X=c:
sleep 1
touch list.$X.$N 
N=1 X=w:
sleep 1
touch list.$X.$N 
N=1 X=z:
sleep 1
touch list.$X.$N 
N=2 X=y:
sleep 1
touch list.$X.$N 
N=1 X=b:
sleep 1
touch list.$X.$N 
N=2 X=d:
sleep 1
touch list.$X.$N 
N=2 X=m:
sleep 1
touch list.$X.$N 
N=1 X=a:
sleep 1
touch list.$X.$N 
N=2 X=t:
sleep 1
touch list.$X.$N 
N=1 X=l:
sleep 1
touch list.$X.$N 
N=2 X=k:
sleep 1
touch list.$X.$N 
touch A 
LINE='q'
LINE='o'
LINE='x'
LINE='v'
LINE='i'
LINE='n'
LINE='d'
LINE='g'
LINE='z'
LINE='k'
LINE='m'
LINE='f'
LINE='c'
LINE='r'
LINE='l'
LINE='h'
LINE='y'
LINE='e'
LINE='a'
LINE='p'
LINE='j'
LINE='u'
LINE='w'
LINE='s'
LINE='b'
LINE='t'
LINE='j'
LINE='r'
LINE='e'
LINE='g'
LINE='o'
LINE='n'
LINE='q'
LINE='u'
LINE='i'
LINE='s'
LINE='h'
LINE='x'
LINE='p'
LINE='v'
LINE='f'
LINE='c'
LINE='w'
LINE='z'
LINE='y'
LINE='b'
LINE='d'
LINE='m'
LINE='a'
LINE='t'
LINE='l'
LINE='k'
COUNT='0'
__________________________________
cd recursive-dynamic-dependency
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd recursive-include-different-filenames
./EXEC
xxx/../main.stu:5:10: recursive inclusion of 'xxx/../main.stu'
__________________________________
cd redirect-lone-output
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd redirect-missing-filename
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd redirect-noredirect
../../stu 
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
STDERR...correct
../../stu 
Second invocation...correct
__________________________________
cd redirect-output
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd redirect-output-phony
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd redirect-phony
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd redirect-verbose
../../stu 
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd redirection-in-dynamic
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd remove-incomplete
./EXEC
rmm
touch list.A
sleep 2
../../stu data.A
echo xxx >X
exit 1
rmm
touch list.B
sleep 2
../../stu data.B
echo xxx >X
echo foo >list.B
exit 1
../../stu: *** Removing file 'list.B' because command failed
rmm
touch list.C
sleep 2
../../stu data.C
echo xxx >X
sleep 2222
exit 1
rmm
touch list.D
sleep 2
../../stu data.D
echo xxx >X
echo foo >list.D
sleep 2222
exit 1
../../stu: *** Removing file 'list.D' because command failed
__________________________________
cd removed-variable
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd rm-redirect
./EXEC
>A:
echo Hello
exit 1
main.stu:5:2: command for 'A' failed with exit code 1
../../stu: *** Removing file 'A' because command failed
__________________________________
cd self-include
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd self-including-dynamic
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd sh-line
./EXEC
../../stu
>A: wefvunwoiefuvuviwwve
__________________________________
cd shell-e
../../stu 
EXITCODE...1 correct
__________________________________
cd shell-minus
../../stu 
EXITCODE...1 correct
INSTDERR...correct
INSTDERR...correct
__________________________________
cd shell-plus
../../stu 
EXITCODE...1 correct
INSTDERR...correct
INSTDERR...correct
__________________________________
cd short-err
../../stu -w
EXITCODE...0 correct
STDOUT...correct
STDERR...correct
../../stu -w
Second invocation...correct
__________________________________
cd short-out
../../stu -w
EXITCODE...0 correct
STDOUT...correct
../../stu -w
Second invocation...correct
__________________________________
cd silent
../../stu -s
EXITCODE...0 correct
CONTENT...correct
STDOUT...correct
../../stu -s
Second invocation...correct
__________________________________
cd simple-dynamic
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd single-quote
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd space-parameter
../../stu 
EXITCODE...2 correct
__________________________________
cd statement-missing
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd statement-newline
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd statement-nonexisting
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd statement-short
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd status-fail
./EXEC
__________________________________
cd status-hack
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd status-recursive
../../stu 
EXITCODE...1 correct
INSTDERR...correct
__________________________________
cd status-set
../../stu 
EXITCODE...0 correct
__________________________________
cd stronger-dynamic
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd stu_shell
./EXEC
__________________________________
cd subsequent1
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd subsequent2
./EXEC
rm -f ?
../../stu
>B: echo bbb 
cp B A
sleep 2
echo ddd >B
sleep 2
../../stu
__________________________________
cd subsequent3
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd subsequent4
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd symlink
./EXEC
rm -f ? list.*
ln -s W B
../../stu
STDOUT:
cp B A 
____
STDERR:
____
sleep 2
echo bbb >W
../../stu
STDOUT:
cp B A 
____
STDERR:
____
rm -f A B W list.err list.out
__________________________________
cd symlink-not-created
./EXEC
ln -s X A
__________________________________
cd symlink-to-nonexisting
./EXEC
rm -f A B
ln -s nonexisting B
../../stu
__________________________________
cd syntax-error-in-dynamic
../../stu 
EXITCODE...2 correct
STDOUT...correct
INSTDERR...correct
STDERR...correct
__________________________________
cd syntax-error-in-dynamic-k
./EXEC
rm -f ? ??
rm -f ? B1
__________________________________
cd target-twice-in-cmd
../../stu A A -j2
EXITCODE...0 correct
../../stu A A -j2
Second invocation...correct
__________________________________
cd test-cmd-quote
../../stu -C 'list.B '
EXITCODE...0 correct
../../stu -C 'list.B '
Second invocation...correct
__________________________________
cd testest
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
../../stu  -j 3
PARALLEL...correct
../../stu  -j 4
PARALLEL...correct
../../stu  -j 5
PARALLEL...correct
../../stu  -j 67
PARALLEL...correct
__________________________________
cd too-old
../../stu 
EXITCODE...1 correct
INSTDERR...correct
INSTDERR...correct
INSTDERR...correct
INSTDERR...correct
__________________________________
cd trace-1
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd trace-10
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd trace-11
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd trace-13
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd trace-14
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd trace-15
../../stu 
EXITCODE...1 correct
STDOUT...correct
STDERR...correct
__________________________________
cd trace-16
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd trace-2
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd trace-3
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd trace-7
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd trace-8
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd trace-doubly-dynamic
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd trace-placement
../../stu 
EXITCODE...1 correct
INSTDERR...correct
__________________________________
cd transient-phony
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd triangle
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd triply-dynamic
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd triv-1
./EXEC
rm -f ?
touch A
sleep 2
touch B
__________________________________
cd triv-2
../../stu 
EXITCODE...0 correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd triv-3
../../stu 
EXITCODE...0 correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd triv-4
../../stu 
EXITCODE...0 correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd trivial
./EXEC
rm -f ?
../../stu
>B: echo bbb
>D: echo ddd 
cp D C
>A: cat C B 
sleep 2
touch D
../../stu
sleep 2
rm D
../../stu
sleep 2
touch B
../../stu
sleep 2
touch B
../../stu
__________________________________
cd trivial-cycle
./EXEC
type=a
cp a.stu main.stu
<main.stu >main2.stu sed -re 's,&, ,g'
cas=A--ABIDJ
list_old=A
list_new=
list_res=A B I D J
rm -f A B D I J X Y
echo xxx >A
touch_old A
../../stu -f main.stu
cas=ABIDJ--
list_old=A B I D J
list_new=
list_res=
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
touch_old I
echo xxx >D
touch_old D
echo xxx >J
touch_old J
../../stu -f main.stu
cas=ADJ-BI-
list_old=A D J
list_new=B I
list_res=
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >D
touch_old D
echo xxx >J
touch_old J
echo xxx >B
echo xxx >I
../../stu -f main.stu
cas=ABDJ-I-
list_old=A B D J
list_new=I
list_res=
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >D
touch_old D
echo xxx >J
touch_old J
echo xxx >I
../../stu -f main.stu
cas=AIDJ-B-
list_old=A I D J
list_new=B
list_res=
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >I
touch_old I
echo xxx >D
touch_old D
echo xxx >J
touch_old J
echo xxx >B
../../stu -f main.stu
cas=ABI-DJ-A
list_old=A B I
list_new=D J
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
touch_old I
echo xxx >D
echo xxx >J
../../stu -f main.stu
cas=ABID-J-AD
list_old=A B I D
list_new=J
list_res=A D
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
touch_old I
echo xxx >D
touch_old D
echo xxx >J
../../stu -f main.stu
cas=ABIJ-D-A
list_old=A B I J
list_new=D
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
touch_old I
echo xxx >J
touch_old J
echo xxx >D
../../stu -f main.stu
cas=A-BIDJ-A
list_old=A
list_new=B I D J
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
echo xxx >I
echo xxx >D
echo xxx >J
../../stu -f main.stu
cas=AB-IDJ-AB
list_old=A B
list_new=I D J
list_res=A B
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
echo xxx >D
echo xxx >J
../../stu -f main.stu
cas=AI-BDJ-A
list_old=A I
list_new=B D J
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >I
touch_old I
echo xxx >B
echo xxx >D
echo xxx >J
../../stu -f main.stu
cas=AD-BIJ-AD
list_old=A D
list_new=B I J
list_res=A D
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >D
touch_old D
echo xxx >B
echo xxx >I
echo xxx >J
../../stu -f main.stu
cas=ABD-IJ-ABD
list_old=A B D
list_new=I J
list_res=A B D
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >D
touch_old D
echo xxx >I
echo xxx >J
../../stu -f main.stu
cas=AID-BJ-AD
list_old=A I D
list_new=B J
list_res=A D
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >I
touch_old I
echo xxx >D
touch_old D
echo xxx >B
echo xxx >J
../../stu -f main.stu
cas=AJ-BID-A
list_old=A J
list_new=B I D
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >J
touch_old J
echo xxx >B
echo xxx >I
echo xxx >D
../../stu -f main.stu
cas=ABJ-ID-AB
list_old=A B J
list_new=I D
list_res=A B
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >J
touch_old J
echo xxx >I
echo xxx >D
../../stu -f main.stu
cas=AIJ-BD-A
list_old=A I J
list_new=B D
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >I
touch_old I
echo xxx >J
touch_old J
echo xxx >B
echo xxx >D
../../stu -f main.stu
type=b
cp b.stu main.stu
<main.stu >main2.stu sed -re 's,&, ,g'
cas=A--ABIDJ
list_old=A
list_new=
list_res=A B I D J
rm -f A B D I J X Y
echo xxx >A
touch_old A
../../stu -f main.stu
cas=ABIDJ--
list_old=A B I D J
list_new=
list_res=
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
touch_old I
echo xxx >D
touch_old D
echo xxx >J
touch_old J
../../stu -f main.stu
cas=ADJ-BI-
list_old=A D J
list_new=B I
list_res=
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >D
touch_old D
echo xxx >J
touch_old J
echo xxx >B
echo xxx >I
../../stu -f main.stu
cas=ABDJ-I-
list_old=A B D J
list_new=I
list_res=
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >D
touch_old D
echo xxx >J
touch_old J
echo xxx >I
../../stu -f main.stu
cas=AIDJ-B-
list_old=A I D J
list_new=B
list_res=
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >I
touch_old I
echo xxx >D
touch_old D
echo xxx >J
touch_old J
echo xxx >B
../../stu -f main.stu
cas=ABI-DJ-A
list_old=A B I
list_new=D J
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
touch_old I
echo xxx >D
echo xxx >J
../../stu -f main.stu
cas=ABID-J-AD
list_old=A B I D
list_new=J
list_res=A D
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
touch_old I
echo xxx >D
touch_old D
echo xxx >J
../../stu -f main.stu
cas=ABIJ-D-A
list_old=A B I J
list_new=D
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
touch_old I
echo xxx >J
touch_old J
echo xxx >D
../../stu -f main.stu
cas=A-BIDJ-A
list_old=A
list_new=B I D J
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
echo xxx >I
echo xxx >D
echo xxx >J
../../stu -f main.stu
cas=AB-IDJ-AB
list_old=A B
list_new=I D J
list_res=A B
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
echo xxx >D
echo xxx >J
../../stu -f main.stu
cas=AI-BDJ-A
list_old=A I
list_new=B D J
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >I
touch_old I
echo xxx >B
echo xxx >D
echo xxx >J
../../stu -f main.stu
cas=AD-BIJ-AD
list_old=A D
list_new=B I J
list_res=A D
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >D
touch_old D
echo xxx >B
echo xxx >I
echo xxx >J
../../stu -f main.stu
cas=ABD-IJ-ABD
list_old=A B D
list_new=I J
list_res=A B D
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >D
touch_old D
echo xxx >I
echo xxx >J
../../stu -f main.stu
cas=AID-BJ-AD
list_old=A I D
list_new=B J
list_res=A D
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >I
touch_old I
echo xxx >D
touch_old D
echo xxx >B
echo xxx >J
../../stu -f main.stu
cas=AJ-BID-A
list_old=A J
list_new=B I D
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >J
touch_old J
echo xxx >B
echo xxx >I
echo xxx >D
../../stu -f main.stu
cas=ABJ-ID-AB
list_old=A B J
list_new=I D
list_res=A B
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >J
touch_old J
echo xxx >I
echo xxx >D
../../stu -f main.stu
cas=AIJ-BD-A
list_old=A I J
list_new=B D
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >I
touch_old I
echo xxx >J
touch_old J
echo xxx >B
echo xxx >D
../../stu -f main.stu
type=c
cp c.stu main.stu
<main.stu >main2.stu sed -re 's,&, ,g'
cas=A--ABIDJ
list_old=A
list_new=
list_res=A B I D J
rm -f A B D I J X Y
echo xxx >A
touch_old A
../../stu -f main.stu
cas=ABIDJ--
list_old=A B I D J
list_new=
list_res=
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
touch_old I
echo xxx >D
touch_old D
echo xxx >J
touch_old J
../../stu -f main.stu
cas=ADJ-BI-
list_old=A D J
list_new=B I
list_res=
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >D
touch_old D
echo xxx >J
touch_old J
echo xxx >B
echo xxx >I
../../stu -f main.stu
cas=ABDJ-I-
list_old=A B D J
list_new=I
list_res=
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >D
touch_old D
echo xxx >J
touch_old J
echo xxx >I
../../stu -f main.stu
cas=AIDJ-B-
list_old=A I D J
list_new=B
list_res=
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >I
touch_old I
echo xxx >D
touch_old D
echo xxx >J
touch_old J
echo xxx >B
../../stu -f main.stu
cas=ABI-DJ-A
list_old=A B I
list_new=D J
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
touch_old I
echo xxx >D
echo xxx >J
../../stu -f main.stu
cas=ABID-J-AD
list_old=A B I D
list_new=J
list_res=A D
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
touch_old I
echo xxx >D
touch_old D
echo xxx >J
../../stu -f main.stu
cas=ABIJ-D-A
list_old=A B I J
list_new=D
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
touch_old I
echo xxx >J
touch_old J
echo xxx >D
../../stu -f main.stu
cas=A-BIDJ-A
list_old=A
list_new=B I D J
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
echo xxx >I
echo xxx >D
echo xxx >J
../../stu -f main.stu
cas=AB-IDJ-AB
list_old=A B
list_new=I D J
list_res=A B
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >I
echo xxx >D
echo xxx >J
../../stu -f main.stu
cas=AI-BDJ-A
list_old=A I
list_new=B D J
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >I
touch_old I
echo xxx >B
echo xxx >D
echo xxx >J
../../stu -f main.stu
cas=AD-BIJ-AD
list_old=A D
list_new=B I J
list_res=A D
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >D
touch_old D
echo xxx >B
echo xxx >I
echo xxx >J
../../stu -f main.stu
cas=ABD-IJ-ABD
list_old=A B D
list_new=I J
list_res=A B D
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >D
touch_old D
echo xxx >I
echo xxx >J
../../stu -f main.stu
cas=AID-BJ-AD
list_old=A I D
list_new=B J
list_res=A D
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >I
touch_old I
echo xxx >D
touch_old D
echo xxx >B
echo xxx >J
../../stu -f main.stu
cas=AJ-BID-A
list_old=A J
list_new=B I D
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >J
touch_old J
echo xxx >B
echo xxx >I
echo xxx >D
../../stu -f main.stu
cas=ABJ-ID-AB
list_old=A B J
list_new=I D
list_res=A B
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >B
touch_old B
echo xxx >J
touch_old J
echo xxx >I
echo xxx >D
../../stu -f main.stu
cas=AIJ-BD-A
list_old=A I J
list_new=B D
list_res=A
rm -f A B D I J X Y
echo xxx >A
touch_old A
echo xxx >I
touch_old I
echo xxx >J
touch_old J
echo xxx >B
echo xxx >D
../../stu -f main.stu
rm -f A B D I J X Y main.stu main2.stu
__________________________________
cd trivial-existence
./EXEC
rm -f ?
../../stu
sleep 2
touch B
../../stu
__________________________________
cd trivial-existence2
./EXEC
rm -f ?
../../stu
sleep 2
touch B
../../stu
__________________________________
cd trivial10
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd trivial11
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd trivial2
./EXEC
rm -f ?
../../stu
>B: echo bbb
>D: echo ddd 
cp D C
>A:
echo "$C"
cat B
sleep 2
touch D
../../stu
sleep 2
rm D
../../stu
sleep 2
touch B
../../stu
sleep 2
touch B
../../stu
__________________________________
cd trivial3
./EXEC
rm -f ?
../../stu
touch C
../../stu
sleep 2
touch D
../../stu
sleep 2
rm D
../../stu
sleep 2
touch B
../../stu
sleep 2
touch B
../../stu
__________________________________
cd trivial4
./EXEC
rm -f ?
../../stu
touch C
../../stu
sleep 2
touch D
../../stu
sleep 2
rm D
../../stu
sleep 2
touch B
../../stu
sleep 2
touch B
../../stu
__________________________________
cd trivial5
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd trivial6
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd trivial7
../../stu 
EXITCODE...0 correct
STDOUT...correct
__________________________________
cd trivial8
../../stu 
EXITCODE...0 correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd trivial9
./EXEC
rm -f ?
../../stu
>B: echo bbb
>D: echo ddd 
cp D C
>A:
echo "$C"
cat B
sleep 2
touch D
../../stu
sleep 2
rm D
../../stu
sleep 2
touch B
../../stu
sleep 2
touch B
../../stu
__________________________________
cd two
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd twodep
../../stu 
EXITCODE...1 correct
INSTDERR...correct
__________________________________
cd twoflags
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd twoflags-mixed
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd twoinputs
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd twolevel-flags
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd unbuilt-partial
../../stu 
EXITCODE...1 correct
STDERR...correct
__________________________________
cd unclosed-parameter
../../stu 
EXITCODE...2 correct
__________________________________
cd unexpected-rbrace
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd unexpected-rbracket
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd unfinished-double-quote
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd unfinished-single-quote
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd unicode
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd unknown-phony-k
./EXEC
../../stu: *** No rule to build @lakndlkwande
../../stu: *** Targets not rebuilt because of errors
__________________________________
cd unmatched-langle
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd unmatched-lbrace
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd unmatched-twoparens
../../stu 
EXITCODE...2 correct
__________________________________
cd unparametrized-precedence
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd unquoted-target-1
../../stu -C
EXITCODE...4 correct
STDERR...correct
__________________________________
cd unquoted-target-2
./EXEC
__________________________________
cd unquoted-target-3
./EXEC
__________________________________
cd update
./EXEC
rm -rf ?
echo Hello >B
../../stu
cat B B >A
-rw-rw-r-- 1 hartmann hartmann 12 Apr  2 14:42 A
-rw-rw-r-- 1 hartmann hartmann 45 Apr  2 14:42 B
../../stu
cat B B >A
A
rm -rf A B
__________________________________
cd update-dynamic
./EXEC
rm -rf ?
echo Hello >B
../../stu
echo B >dep.A 
cat B B >A
-rw-rw-r-- 1 hartmann hartmann 12 Apr  2 14:42 A
-rw-rw-r-- 1 hartmann hartmann 45 Apr  2 14:42 B
../../stu
cat B B >A
A
rm -rf A B
__________________________________
cd update-through-phony
./EXEC
rm -rf BBB A
../../stu
cat BBB >A
../../stu
../../stu
../../stu
__________________________________
cd variable
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd variable-double-phony
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd variable-doubly-dynamic
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd variable-exclamation
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd variable-input
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd variable-input2
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd variable-output
../../stu 
EXITCODE...0 correct
STDOUT...correct
../../stu 
Second invocation...correct
__________________________________
cd variable-trivial
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd varpar-0
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd varpar-1
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd varpar-10
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd varpar-11
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd varpar-12
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd varpar-13
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd varpar-14
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd varpar-15
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd varpar-2
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd varpar-3
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd varpar-4
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd varpar-5
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd varpar-6
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd varpar-7
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd varpar-8
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd varpar-9
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd verbose
./EXEC
__________________________________
cd version
./EXEC
./EXEC:  TESTING 2 2.0
	exitcode=2
tmp.stu:1:10: requested version 2.0 is incompatible with this Stu's version 1.9.22
./EXEC:  TESTING 2 2.0.0
	exitcode=2
tmp.stu:1:10: requested version 2.0.0 is incompatible with this Stu's version 1.9.22
./EXEC:  TESTING 2 0.0
	exitcode=2
tmp.stu:1:10: requested version 0.0 is incompatible with this Stu's version 1.9.22
./EXEC:  TESTING 2 0.0.0
	exitcode=2
tmp.stu:1:10: requested version 0.0.0 is incompatible with this Stu's version 1.9.22
./EXEC:  TESTING 2 1.10
	exitcode=2
tmp.stu:1:10: requested version 1.10 is incompatible with this Stu's version 1.9.22
./EXEC:  TESTING 2 1.10.0
	exitcode=2
tmp.stu:1:10: requested version 1.10.0 is incompatible with this Stu's version 1.9.22
./EXEC:  TESTING 0 1.8
echo CORRECT >A 
	exitcode=0
CORRECT
./EXEC:  TESTING 0 1.8.9999
echo CORRECT >A 
	exitcode=0
CORRECT
./EXEC:  TESTING 2 1.9.23
	exitcode=2
tmp.stu:1:10: requested version 1.9.23 is incompatible with this Stu's version 1.9.22
./EXEC:  TESTING 0 1.9.21
echo CORRECT >A 
	exitcode=0
CORRECT
./EXEC:  TESTING 0 1.9.22
echo CORRECT >A 
	exitcode=0
CORRECT
__________________________________
cd version-double
./EXEC
./EXEC:  TESTING 2 2.0 2.0
	exitcode=2
./EXEC:  TESTING 2 2.0 1.10
	exitcode=2
./EXEC:  TESTING 2 2.0 1.10.0
	exitcode=2
./EXEC:  TESTING 2 2.0 1.9.23
	exitcode=2
./EXEC:  TESTING 2 2.0 1.9.22
	exitcode=2
./EXEC:  TESTING 2 2.0 0.0
	exitcode=2
./EXEC:  TESTING 2 2.0 0.0.0
	exitcode=2
./EXEC:  TESTING 2 2.0 1.8
	exitcode=2
./EXEC:  TESTING 2 2.0 1.8.99999
	exitcode=2
./EXEC:  TESTING 2 2.0 1.9.21
	exitcode=2
./EXEC:  TESTING 2 1.10 2.0
	exitcode=2
./EXEC:  TESTING 2 1.10 1.10
	exitcode=2
./EXEC:  TESTING 2 1.10 1.10.0
	exitcode=2
./EXEC:  TESTING 2 1.10 1.9.23
	exitcode=2
./EXEC:  TESTING 2 1.10 1.9.22
	exitcode=2
./EXEC:  TESTING 2 1.10 0.0
	exitcode=2
./EXEC:  TESTING 2 1.10 0.0.0
	exitcode=2
./EXEC:  TESTING 2 1.10 1.8
	exitcode=2
./EXEC:  TESTING 2 1.10 1.8.99999
	exitcode=2
./EXEC:  TESTING 2 1.10 1.9.21
	exitcode=2
./EXEC:  TESTING 2 1.10.0 2.0
	exitcode=2
./EXEC:  TESTING 2 1.10.0 1.10
	exitcode=2
./EXEC:  TESTING 2 1.10.0 1.10.0
	exitcode=2
./EXEC:  TESTING 2 1.10.0 1.9.23
	exitcode=2
./EXEC:  TESTING 2 1.10.0 1.9.22
	exitcode=2
./EXEC:  TESTING 2 1.10.0 0.0
	exitcode=2
./EXEC:  TESTING 2 1.10.0 0.0.0
	exitcode=2
./EXEC:  TESTING 2 1.10.0 1.8
	exitcode=2
./EXEC:  TESTING 2 1.10.0 1.8.99999
	exitcode=2
./EXEC:  TESTING 2 1.10.0 1.9.21
	exitcode=2
./EXEC:  TESTING 2 1.9.23 2.0
	exitcode=2
./EXEC:  TESTING 2 1.9.23 1.10
	exitcode=2
./EXEC:  TESTING 2 1.9.23 1.10.0
	exitcode=2
./EXEC:  TESTING 2 1.9.23 1.9.23
	exitcode=2
./EXEC:  TESTING 2 1.9.23 1.9.22
	exitcode=2
./EXEC:  TESTING 2 1.9.23 0.0
	exitcode=2
./EXEC:  TESTING 2 1.9.23 0.0.0
	exitcode=2
./EXEC:  TESTING 2 1.9.23 1.8
	exitcode=2
./EXEC:  TESTING 2 1.9.23 1.8.99999
	exitcode=2
./EXEC:  TESTING 2 1.9.23 1.9.21
	exitcode=2
./EXEC:  TESTING 2 1.9.22 2.0
	exitcode=2
./EXEC:  TESTING 2 1.9.22 1.10
	exitcode=2
./EXEC:  TESTING 2 1.9.22 1.10.0
	exitcode=2
./EXEC:  TESTING 2 1.9.22 1.9.23
	exitcode=2
./EXEC:  TESTING 0 1.9.22 1.9.22
echo CORRECT >B 
cp B A 
	exitcode=0
./EXEC:  TESTING 2 1.9.22 0.0
	exitcode=2
./EXEC:  TESTING 2 1.9.22 0.0.0
	exitcode=2
./EXEC:  TESTING 0 1.9.22 1.8
cp B A 
	exitcode=0
./EXEC:  TESTING 0 1.9.22 1.8.99999
cp B A 
	exitcode=0
./EXEC:  TESTING 0 1.9.22 1.9.21
cp B A 
	exitcode=0
./EXEC:  TESTING 2 0.0 2.0
	exitcode=2
./EXEC:  TESTING 2 0.0 1.10
	exitcode=2
./EXEC:  TESTING 2 0.0 1.10.0
	exitcode=2
./EXEC:  TESTING 2 0.0 1.9.23
	exitcode=2
./EXEC:  TESTING 2 0.0 1.9.22
	exitcode=2
./EXEC:  TESTING 2 0.0 0.0
	exitcode=2
./EXEC:  TESTING 2 0.0 0.0.0
	exitcode=2
./EXEC:  TESTING 2 0.0 1.8
	exitcode=2
./EXEC:  TESTING 2 0.0 1.8.99999
	exitcode=2
./EXEC:  TESTING 2 0.0 1.9.21
	exitcode=2
./EXEC:  TESTING 2 0.0.0 2.0
	exitcode=2
./EXEC:  TESTING 2 0.0.0 1.10
	exitcode=2
./EXEC:  TESTING 2 0.0.0 1.10.0
	exitcode=2
./EXEC:  TESTING 2 0.0.0 1.9.23
	exitcode=2
./EXEC:  TESTING 2 0.0.0 1.9.22
	exitcode=2
./EXEC:  TESTING 2 0.0.0 0.0
	exitcode=2
./EXEC:  TESTING 2 0.0.0 0.0.0
	exitcode=2
./EXEC:  TESTING 2 0.0.0 1.8
	exitcode=2
./EXEC:  TESTING 2 0.0.0 1.8.99999
	exitcode=2
./EXEC:  TESTING 2 0.0.0 1.9.21
	exitcode=2
./EXEC:  TESTING 2 1.8 2.0
	exitcode=2
./EXEC:  TESTING 2 1.8 1.10
	exitcode=2
./EXEC:  TESTING 2 1.8 1.10.0
	exitcode=2
./EXEC:  TESTING 2 1.8 1.9.23
	exitcode=2
./EXEC:  TESTING 0 1.8 1.9.22
cp B A 
	exitcode=0
./EXEC:  TESTING 2 1.8 0.0
	exitcode=2
./EXEC:  TESTING 2 1.8 0.0.0
	exitcode=2
./EXEC:  TESTING 0 1.8 1.8
cp B A 
	exitcode=0
./EXEC:  TESTING 0 1.8 1.8.99999
cp B A 
	exitcode=0
./EXEC:  TESTING 0 1.8 1.9.21
cp B A 
	exitcode=0
./EXEC:  TESTING 2 1.8.99999 2.0
	exitcode=2
./EXEC:  TESTING 2 1.8.99999 1.10
	exitcode=2
./EXEC:  TESTING 2 1.8.99999 1.10.0
	exitcode=2
./EXEC:  TESTING 2 1.8.99999 1.9.23
	exitcode=2
./EXEC:  TESTING 0 1.8.99999 1.9.22
cp B A 
	exitcode=0
./EXEC:  TESTING 2 1.8.99999 0.0
	exitcode=2
./EXEC:  TESTING 2 1.8.99999 0.0.0
	exitcode=2
./EXEC:  TESTING 0 1.8.99999 1.8
cp B A 
	exitcode=0
./EXEC:  TESTING 0 1.8.99999 1.8.99999
cp B A 
	exitcode=0
./EXEC:  TESTING 0 1.8.99999 1.9.21
cp B A 
	exitcode=0
./EXEC:  TESTING 2 1.9.21 2.0
	exitcode=2
./EXEC:  TESTING 2 1.9.21 1.10
	exitcode=2
./EXEC:  TESTING 2 1.9.21 1.10.0
	exitcode=2
./EXEC:  TESTING 2 1.9.21 1.9.23
	exitcode=2
./EXEC:  TESTING 0 1.9.21 1.9.22
cp B A 
	exitcode=0
./EXEC:  TESTING 2 1.9.21 0.0
	exitcode=2
./EXEC:  TESTING 2 1.9.21 0.0.0
	exitcode=2
./EXEC:  TESTING 0 1.9.21 1.8
cp B A 
	exitcode=0
./EXEC:  TESTING 0 1.9.21 1.8.99999
cp B A 
	exitcode=0
./EXEC:  TESTING 0 1.9.21 1.9.21
cp B A 
	exitcode=0
__________________________________
cd version-invalid
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd version-long
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd version-partial
../../stu 
EXITCODE...2 correct
STDERR...correct
__________________________________
cd weak-and-strong-cycle
../../stu 
EXITCODE...2 correct
__________________________________
cd weakcycle
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
../../stu  -j 2
PARALLEL...correct
__________________________________
cd weird-filename
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd whitespace-command
../../stu 
EXITCODE...0 correct
../../stu 
Second invocation...correct
__________________________________
cd wrong-quote
../../stu 
EXITCODE...2 correct
INSTDERR...correct
__________________________________
cd wrong-rangle
../../stu 
EXITCODE...2 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd wrong-type-in-dynamic
../../stu 
EXITCODE...1 correct
INSTDERR...correct
STDERR...correct
__________________________________
cd xargs
../../stu 
EXITCODE...0 correct
CONTENT...correct
../../stu 
Second invocation...correct
__________________________________
cd zerolength-command
../../stu 
EXITCODE...1 correct
__________________________________
./mktest: *** The following tests failed:
no-variable-set-parent
Makefile:52: recipe for target 'check_test.debug' failed
make: *** [check_test.debug] Error 1
hartmann@HHXPS:/home/hartmann/git/stu git:(master)
date
date
Sa 2. Apr 14:44:01 CEST 2016
```
