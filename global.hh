#ifndef GLOBAL_HH
#define GLOBAL_HH

/* 
 * Global variables of the process.  All boolean option variables are
 * FALSE by default. 
 */ 

/* The -a option (consider all trivial dependencies to be non-trivial) */ 
static bool option_nontrivial= false;

/* The -g option (consider all optional dependencies to be non-optional) */
static bool option_nonoptional= false;

/* The -k option (keep going) */ 
static bool option_keep_going= false;

/* The -v option (verbose mode) */ 
static bool option_verbose= false;

/* The -z option (output statistics) */
static bool option_statistics= false;

/* The -K option (don't delete partially built files) */
static bool option_no_delete= false;

/* The -q option (question mode) */
static bool option_question= false; 

/* The -x option (use sh -x) */ 
static bool option_individual= false;


/* Determines how commands are output
 */
enum class Output {
	SILENT  = -2, /* No output */
	SHORT   = -1, /* Only target name */
	LONG    =  0  /* Output the command */
};
static Output output_mode= Output::LONG; 

enum class Order {
	DFS   = 0,
	RANDOM= 1,
	
	/* -M mode is coded as Order::RANDOM */ 
};
static Order order= Order::DFS; 

/* Whether to use vectors for randomization */ 
static bool order_vec; 

/* The envp variable.  Set in main(). 
 */
const char **envp_global;

/* Does the same as program_invocation_name (which is a GNU extension,
 * so we don't use it); the value of argv[0], set in main() */ 
static const char *dollar_zero;

#endif /* ! GLOBAL_HH */
