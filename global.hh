#ifndef GLOBAL_HH
#define GLOBAL_HH

/* 
 * Global variables used by Stu
 */ 

/* The -a option (consider all trivial dependencies to be non-trivial) */ 
static bool option_nontrivial= false;

/* The -g option (consider all optional dependencies to be non-optional) */
static bool option_nonoptional= false;

/* The -k option (continue when encountering errors) */ 
static bool option_continue= false;

/* Determines how commands are output
 */
enum {
	VERBOSITY_SILENT  = -2, /* No output */
	VERBOSITY_SHORT   = -1, /* Only target name */
	VERBOSITY_LONG    =  0, /* Output the command */
	VERBOSITY_VERBOSE = +1  /* More information */ 
};

static int verbosity= VERBOSITY_LONG; 

enum Mode {
	MODE_DFS   = 0,
	MODE_RANDOM= 1,
	
	/* -M mode is coded as MODE_RANDOM */ 
};

static int order= MODE_DFS; 
static bool order_vec; 

const char **envp_global;

/* Does the same as program_invocation_name (which is a GNU extension,
 * so we don't use it); the value of argv[0], set in main() */ 
static const char *dollar_zero;

#endif /* ! GLOBAL_HH */
