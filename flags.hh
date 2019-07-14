#ifndef FLAGS_HH
#define FLAGS_HH

/*
 * Flags are bit fields that apply to dependencies and represents things
 * like "optional dependency" or "\0-separated file".  Flags are binary
 * option-like, and apply at multiple levels in Stu, from Stu source
 * code where they are represented by a syntax ressembling that of
 * command line flags, to attributes of edges in the dependency graph.
 * Internally, flags are defined as bit fields.
 *
 * Each edge in the dependency graph is annotated with one object of
 * this type.  This contains bits related to what should be done with
 * the dependency, whether time is considered, etc.  The flags are
 * defined in such a way that the simplest dependency is represented by
 * zero, and each flag enables a specific feature.
 *
 * The transitive bits are effectively set for tasks not to do.
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
	/* 
	 * The index of the flags (I_*), used for array indexing.
	 * Variables iterating over these values are usually called
	 * I.  
	 */ 
	I_PERSISTENT= 0,	/* -p  \                  \                     */
	I_OPTIONAL,		/* -o   | placed flags     |                    */
	I_TRIVIAL,		/* -t  /                   | target word flags  */
	I_TARGET_DYNAMIC,	/* [ ] \ target flags      |                    */
	I_TARGET_TRANSIENT,	/* @   /                   |                    */
	I_VARIABLE,		/* $                       |                    */
	I_NEWLINE_SEPARATED,	/* -n  \ attribute flags   |                    */
	I_NUL_SEPARATED,	/* -0  /                  /                     */
	I_INPUT,		/* <                                            */
	I_RESULT_NOTIFY,        /* -*                                           */
	I_RESULT_COPY,          /* -%                                           */

	C_ALL,                 
	C_PLACED           	= 3,  /* Flags for which we store a place in Dep */
	C_WORD			= 8,  /* Flags used for caching; they are stored in Target */
#define C_WORD			  8 /* Used statically */
	/* The last #define can be replaced with template trickery, yes,
	 * but it makes it much longer.  Accept the duplicate constant
	 * for now.  */

	/* 
	 * What follows are the actual flag bits to be ORed together 
	 */ 

	F_PERSISTENT		= 1 << I_PERSISTENT,  
	/* (-p) When the dependency is newer than the target, don't rebuild */ 

	F_OPTIONAL		= 1 << I_OPTIONAL,
	/* (-o) Don't create the dependency if it doesn't exist */

	F_TRIVIAL		= 1 << I_TRIVIAL,
	/* (-t) Trivial dependency */

	F_TARGET_DYNAMIC	= 1 << I_TARGET_DYNAMIC,
	/* A dynamic target */

	F_TARGET_TRANSIENT	= 1 << I_TARGET_TRANSIENT,
	/* A transient target */

	F_VARIABLE		= 1 << I_VARIABLE,
	/* ($[...]) Content of file is used as variable */ 

	F_NEWLINE_SEPARATED	= 1 << I_NEWLINE_SEPARATED,
	/* For dynamic dependencies, the file contains newline-separated
	 * filenames, without any markup  */ 

	F_NUL_SEPARATED		= 1 << I_NUL_SEPARATED,
	/* For dynamic dependencies, the file contains NUL-separated
	 * filenames, without any markup  */ 

	F_INPUT 		= 1 << I_INPUT,
	/* A dependency is annotated with the input redirection flag '<' */

	F_RESULT_NOTIFY		= 1 << I_RESULT_NOTIFY,
	/* The link A ---> B between two executions annotated with this
	 * flags means that A is notified of B's results.  */

	F_RESULT_COPY		= 1 << I_RESULT_COPY,
	/* The link A ---> B between two executions annotated with this
	 * flags means that the results of B will be copied into A's result  */

	/*
	 * Aggregates
	 */
	F_PLACED	= (1 << C_PLACED) - 1,
	F_TARGET_BYTE	= (1 << C_WORD) - 1,
	F_TARGET	= F_TARGET_DYNAMIC | F_TARGET_TRANSIENT,
	F_ATTRIBUTE	= F_NEWLINE_SEPARATED | F_NUL_SEPARATED,
};

/* 
 * A done variable denotes which "aspects" of an execution have been
 * done.  Every Execution has one variable called "done".  This is a
 * different way to encode the three placed flags.  
 *
 * The first two flags correspond to the first two flags (persistent and
 * optional).  These two are duplicated in order to accommodate trivial
 * dependencies. 
 */
typedef unsigned Done;
enum 
{
	D_NONPERSISTENT_TRANSIENT 	= 1 << 0,
	D_NONOPTIONAL_TRANSIENT		= 1 << 1,
	D_NONPERSISTENT_NONTRANSIENT	= 1 << 2,
	D_NONOPTIONAL_NONTRANSIENT	= 1 << 3,

	D_ALL                     	= (1 << 4) - 1,
	D_ALL_OPTIONAL		  	= D_NONPERSISTENT_TRANSIENT | D_NONPERSISTENT_NONTRANSIENT,
};

const char *const FLAGS_CHARS= "pot[@$n0<*%"; 
/* Characters representing the individual flags -- used in debug mode
 * output, and in other cases  */ 

const char *FLAGS_PHRASES[C_PLACED]= {"persistent", "optional", "trivial"};

unsigned flag_get_index(char c)
/* 
 * Get the flag index corresponding to a character.
 * Not all cases are implemented 
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
	for (unsigned i= 0;  i < C_ALL;  ++i)
		if (flags & (1 << i)) {
			ret += FLAGS_CHARS[i]; 
		}
	if (! ret.empty())
		ret= '-' + ret; 
	return ret;
}

string done_format(Done done) 
{
	char ret[7]= "[0000]";
	for (int i= 0;  i < 4;  ++i) 
		ret[1+i] |= 1 & done << i;
	return ret; 
}

Done done_from_flags(Flags flags) 
/* Only placed flags are kept */
{
	return (~flags & (F_PERSISTENT | F_OPTIONAL)) * (1 | (~flags & F_TRIVIAL));
}

#endif /* ! FLAGS_HH */
