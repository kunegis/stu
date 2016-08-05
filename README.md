# Stu - Build Automation

This is Stu, a build tool in the spirit of Make, but with two features
that set it apart: 

* Parametrized rules:  Like GNU Make's '%' character, but there can be
  multiple parameters, and they have names.  The syntax is '$NAME',
  where NAME can be any string.  
* Dynamic dependencies:  The dependencies of a target can be generated
  dynamically.  When a dependency is enclosed in square brackets, it means
  that that file is built and dependencies are read from within that
  file. 

See this blog article for a motivation:

https://networkscience.wordpress.com/2016/04/01/stu-build-automation-for-data-mining-projects/

For documentation, compile and read the manpage, or go directly to: 

https://raw.githubusercontent.com/kunegis/stu/master/stu.text

## Installation guide

* See the file INSTALL for compiling Stu yourself
* Package for Arch Linux:  https://aur.archlinux.org/packages/stu-git/

Stu is written in C++11 for POSIX platforms.  You should have no problem
getting it to run on vanilla Linux and other POSIX-compliant platforms.  

## Design Considerations

The design considerations of Stu are:

* Genericity:  Being able to execute long, dynamic lists of identical
  commands should be easy.  Many projects need to run many identical
  tasks only differing by individual parameters.  This is the main
  motivation of Stu, and where virtually all other Make replacements
  fail.  
* Generality:  Don't focus on a particular use case such as compilation,
  but be a generic build tool.  There are no built-in rules for
  compilation or other specific applications.  Instead, allow use
  case-specific rules to be written in the language itself.  
* Files are the central datatype.  Everything is a file.  You can think
  of Stu as "a declarative programming language in which all variables
  are files."  For instance, Stu has no variables like Make; instead, files
  are used.  
* Scalability:  Assume that projects are so large that you can't just
  clean and rebuild everything if there are build inconsistencies.
  Files are sacred; never make the user delete files in order to rebuild
  things.   
* Simplicity:  Do one thing well: We don't include features such as file
  compression   that can be achieved by other tools from within the
  shell commands.   
* Embrace POSIX as an underlying standard. Use the shell as the
  underlying command interpreter. Don't try to create a purportedly
  portable layer on top of it, as POSIX _already is_ a portability
  layer.  Also, don't try to create a new portable language for
  executing commands, as /bin/sh _already is_ one.  
* Keep it simple:  Don't use fancy libraries or hip programming
  languages.  Stu is written in plain C++11 with only standard
  libraries. 
* Reliability:  Stu has extensive unit test coverage.  All published
  versions pass 100% of unit tests.  Stu has 500+ unit tests.  All
  language features and error paths are unit tested. 
* Stability of the interface:  We follow Semantic Versioning
  (semver.org) in order to provide stable syntax and semantics.  Stu
  files written now will still work in the future. 
* Be familiar:  Stu follows the conventions of Make as much as possible,
  to make it easier to make the switch from Make to Stu.  For instance,
  the options -j and -k work like in Make.  

## Comparison to Make

In addition to the properties described above, advantages of Stu over
Make are: 

* Error messages are much better (Make has the dreaded "missing
  separator"); Stu gives full traces like a C compiler.  If you get an
  error, you know which part of the Stu file to fix. 
* Stu has a proper tokenization pass, instead of Make's variable
  substitution syntax.  This makes the whole language much more
  predictable. 
* Variables are not used; instead Stu encourages to put everything in
  files. This makes builds much faster: In a typical Makefile, a lot of
  program logic must be implemented used Make functions (because of a
  lack of dynamic dependencies), making a large piece of code being
  executed on each Make invocation, even if the variables have not
  changed. 
* Stu catches typical Makefile errors such as dependencies that where
  not built.  Make cannot do this because the namespace for files is
  intermixed with namespace for phony targets.  In Stu, both namespaces
  are separate (phony targets are called "transient targets" in Stu),
  and thus Stu can perform all checks properly. 
* Stu has better support for interrupting large builds.  Make will often
  hang or leave processes running. 
* Stu avoids the "inner platform" antipattern present in Make, in which
  a lot of shell functionality is duplicated in Make functions.  For
  instance, why are there text filtering functions in Make when the
  shell already has grep and other tools?  That's because in Make, you
  cannot use shell commands to define dynamic dependencies, so Make
  evolved to have its own mini text filtering language.  Instead, Stu
  allows and encourages all program logic to be implemented in ordinary
  rules, i.e. using a proper shell.  
* Stu supports additional types of dependencies which are essential in
  large, complex projects, such as timestamp-ignoring dependencies with
  the prefix '!', optional dependencies with '?', and trivial
  dependencies with '&'.  These can only be emulated partially with Make
  by using unwieldy constructs. 

## Use Stu

To use Stu, replace your 'Makefile' with a 'main.stu' file, and instead
of calling 'make', call 'stu'. 

The see an example of Stu used in a large data mining project, see the
file 'main.stu' in the KONECT-Analysis project:

https://github.com/kunegis/konect-analysis/blob/master/main.stu

## About 

The name "Stu" follows the precedents of Make replacements Cook and Bake
in referring to kitchen-related verbs, and also honours the author of
the original Unix Make, Stuart Feldman. 

Stu was written to accommodate the Koblenz Network Collection project
(KONECT - http://konect.uni-koblenz.de/) at the University of Koblenz-Landau.
It is thus mainly used for large data mining projects, where it manages
everything from acquiring data from the web, to performing experiments,
to generating plots and compiling the resulting Latex papers.  It is
also used for compiling C/C++ code, and for generating tarballs.  

Stu was written in 2014/2015/2016 by Jérôme Kunegis at the University of
Koblenz-Landau.  

Stu is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free
Software Foundation, either version 3 of the License, or (at your
option) any later version. 

Stu is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details. 

For support, write to Jérôme Kunegis <kunegis@gmail.com>, or go to 

https://github.com/kunegis/stu
