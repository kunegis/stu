#ifndef GLOBAL_HH
#define GLOBAL_HH

/* 
 * Global variables used by Stu
 */ 

/* The -k option (continue when encountering errors) */ 
static bool option_continue= false;

/* The -s option (silent mode) */
static bool option_silent= false; 

/* The -v option (verbose mode) */ 
static bool option_verbose= false; 

const char **envp_global;

/* Does the same as program_invocation_name (which is a GNU extension,
 * so we don't use it); the value of argv[0], set in main() */ 
static const char *dollar_zero;

#endif /* ! GLOBAL_HH */
