#ifndef FLAGS_HH
#define FLAGS_HH

/*
 * Flags apply to dependencies and represents things like "optional
 * dependency" or "\0-separated file".  Flags are binary option-like,
 * and apply at multiple levels in Stu, from Stu source code where they
 * are represented by a syntax ressembling that of command line flags,
 * to attributes of edges in the dependency graph.  Internally, flags
 * are defined as bit fields.
 *
 * Each edge in the dependency graph is annotated with one object of
 * this type.  This contains bits related to what should be done with
 * the dependency, whether time is considered, etc.  The flags are
 * defined in such a way that the simplest dependency is represented by
 * zero, and each flag enables a specific feature.
 *
 * The transitive bits effectively are set for tasks not to do.
 * Therefore, inverting them gives the bits for the tasks to do.  In
 * particular, the flag fields that store the information which part of
 * a task has been done has inverse semantics: They have a bit set when
 * that part has been done, i.e., when the flag initially was not set.
 */

#include <limits.h>
#include <assert.h>

#include "text.hh"

typedef unsigned Flags; 
/* Declared as integer so arithmetic can be performed on it */

enum 
{
	/* The index of the flags (I_*), used for array indexing.
	 * Variables iterating over these values are usually called
	 * I.  */ 
	
	I_DYNAMIC_TARGET = 0, /* for dynamic bit in Type */

	I_PERSISTENT,          /* -p */    
	I_OPTIONAL,            /* -o */
	I_TRIVIAL,             /* -t */

	I_RESULT_ONLY,         /* -* */

	I_DYNAMIC_LEFT,        /* -/ */
	I_DYNAMIC_RIGHT,       /* -\ */
//	I_COPY_RESULT,         /* -= */
	I_VARIABLE,            /* -$ */
	I_OVERRIDE_TRIVIAL,    /* -T */
	I_NEWLINE_SEPARATED,   /* -n */
	I_NUL_SEPARATED,       /* -0 */

	C_ALL, /* Total number of flags */ 

	/* Base bit and length of the concatenation index numbers */
	C_CONCATENATE_BASE  = C_ALL,
	C_CONCATENATE_COUNT = CHAR_BIT * sizeof(Flags) - C_ALL,
	C_CONCATENATE_MAX = (1 << C_CONCATENATE_COUNT) - 1,

	C_PLACED           = 4,
	/* Only the first C_PLACED flags have a place associated with them */

	C_TRANSITIVE       = 5,
	/* The first C_TRANSITIVE flags are transitive, i.e., inherited
	 * across transient targets  */

	/* What follows are the actual flag bits to be ORed together */ 

	F_DYNAMIC_TARGET       = 1 << I_DYNAMIC_TARGET,  

	/* 
	 * Placed and transitive flags
	 */ 

	F_PERSISTENT       = 1 << I_PERSISTENT,  
	/* (-p) When the dependency is newer than the target, don't rebuild */ 

	F_OPTIONAL         = 1 << I_OPTIONAL,
	/* (-o) Don't create the dependency if it doesn't exist */

	F_TRIVIAL          = 1 << I_TRIVIAL,
	/* (-t) Trivial dependency */

	/* 
	 * Transitive and non-placed flags 
	 */

	F_RESULT_ONLY      = 1 << I_RESULT_ONLY,
	/* Only compute the result list of dependencies associated with
	 * this dependency, rather than building the dependency.  */ 

	/* 
	 * Intransitive flags
	 */ 

	F_DYNAMIC_LEFT     = 1 << I_DYNAMIC_LEFT,
	/* This is the link between a Dynamic_Execution and its left branch */

	F_DYNAMIC_RIGHT    = 1 << I_DYNAMIC_RIGHT,
	/* For right branches of dynamic.  Overrides the otherwise
	 * passed F_DYNAMIC_LEFT flag.  */

//	F_COPY_RESULT      = 1 << I_COPY_RESULT,
//	/* Copy the result list of the child to the parent */

	F_VARIABLE         = 1 << I_VARIABLE,
	/* ($[...]) Content of file is used as variable */ 

	F_OVERRIDE_TRIVIAL = 1 << I_OVERRIDE_TRIVIAL,
	/* Used only in Link.flags in the second pass.  Not used for
	 * dependencies.  Means to override all trivial flags. */ 

	F_NEWLINE_SEPARATED= 1 << I_NEWLINE_SEPARATED,
	/* For dynamic dependencies, the file contains newline-separated
	 * filenames, without any markup  */ 

	F_NUL_SEPARATED=    1 << I_NUL_SEPARATED,
	/* For dynamic dependencies, the file contains NUL-separated
	 * filenames, without any markup  */ 
};

const char *const FLAGS_CHARS= "pot*/\\$Tn0"; 
/* Characters representing the individual flags -- used in verbose mode
 * output */ 

int flag_get_index(char c)
/* 
 * Get the flag index corresponding to a character.
 */ 
{
	switch (c) {

	case 'p':  return I_PERSISTENT;
	case 'o':  return I_OPTIONAL;
	case 't':  return I_TRIVIAL;
	case 'n':  return I_NEWLINE_SEPARATED;
	case '0':  return I_NUL_SEPARATED;
		
	default:
		assert(false);
		return 0;
	}
}

string flags_format(Flags flags) 
/* 
 * Textual representation of a flags value.  To be shown before the
 * argument.  Empty when flags are empty.  This is used only for debug
 * mode output, as of version 2.5.0. 
 */
{
	string ret;
	for (int i= 0;  i < C_ALL;  ++i)
		if (flags & (1 << i)) {
			if (! ret.empty())
				ret += ' ';
			ret += frmt("-%c", FLAGS_CHARS[i]); 
		}
	return ret;
}

#endif /* ! FLAGS_HH */
