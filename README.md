This is Stu, a build tool similar to make, but with two additional features: 

* Parametrized rules:  Like Make's '%' character, but there can be
  multiple parameters, and they have names.  The syntax is '$NAME',
  where NAME can be any name. 
* Dynamic dependencies:  The dependencies of a target can be generated
  dynamically.  When a dependency is enclosed in square brackets, it means
  that that file is built and dependencies are read from within that
  file. 

See the manpage for more information:  https://raw.githubusercontent.com/kunegis/stu/master/stu.text

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
