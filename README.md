This is Stu, a build tool similar to make, but with two additional features: 

* Parametrized rules:  Like Make's '%' character, but there can be
  multiple parameters, and they have names.  The syntax is '$NAME',
  where NAME can be any name. 
* Dynamic dependencies:  The dependencies of a target can be generated
  dynamically.  When a dependency is enclosed in square brackets, it means
  that that file is built and dependencies are read from within that
  file. 

See the manpage for more information, in the stu.text, or go directly to:

https://raw.githubusercontent.com/kunegis/stu/master/stu.text

Other advantages over Make are:

* Error messages are much better (Make has the dreaded "missing
  separator"); Stu gives full traces like a C compiler 
* Stu has a proper tokenization pass, instead of Make's variable
  substitution syntax.  
* Variables are not used; instead Stu encourages to put everything in
  files. This makes builds much faster: In a typical Makefile, a lot of
  program logic must be implemented used Make functions (because of a
  lack of dynamic dependencies), making a large piece of code being
  executed on each Make invocation, even if the variables have not
  changed. 
* Stu catches typical Makefile errors such as dependencies that where
  not built.
* Stu has better support for interrupting large builds (Make will often
  hang or leave processes running) 
* Stu avoids the "inner platform" antipattern present in Make, in which
  a lot of shell functionality is duplicated in Make functions.  Stu
  encourages all program logic to be implemented in rules, i.e. using a
  proper shell.  

The name "Stu" follows the precedents of Make replacements Cook and Bake
in referring to kitchen-related verbs, and also honours the author of
the original Unix Make, Stuart Feldman. 

Stu is written in C++11 for POSIX platforms.  You should have no problem
compiling it on vanilla Linux and other POSIX-compliant platforms.  See
the file INSTALL for installation instructions.  The Makefile contains
specific compilation commands for G++; they have to be changed to use
another compiler. 

Stu was written to accommodate the Koblenz Network Collection project
(KONECT - http://konect.uni-koblenz.de/) at the University of Koblenz-Landau.
It is thus mainly used for large data mining projects, where it manages
everything from acquiring data from the web, to performing experiments,
to generating plots and compiling the resulting Latex papers.  It is
also used for compiling C/C++ code, and for generating tarballs.  

Stu was written in 2014/2015 by Jérôme Kunegis at the University of
Koblenz-Landau.

Stu is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your
option) any later version. 

Stu is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details. 
