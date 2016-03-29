#ifndef GLOBAL_HH
#define GLOBAL_HH

/* 
 * Global variables of the process. 
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
	VERBOSITY_LONG    =  0  /* Output the command */
};

static int verbosity= VERBOSITY_LONG; 

/* The -d option (debug mode) */ 
static bool option_verbose= false;

enum Order {
	ORDER_DFS   = 0,
	ORDER_RANDOM= 1,
	
	/* -M mode is coded as ORDER_RANDOM */ 
};

static int order= ORDER_DFS; 

/* Whether to use vectors for randomization */ 
static bool order_vec; 

/* The envp variable.  Set in main(). 
 */
const char **envp_global;

/* Does the same as program_invocation_name (which is a GNU extension,
 * so we don't use it); the value of argv[0], set in main() */ 
static const char *dollar_zero;

#endif /* ! GLOBAL_HH */
