#ifndef OPTIONS_HH
#define OPTIONS_HH

#include <getopt.h>

#include "package.hh"

/*
 * Global variables of the process.
 *
 * All boolean option variables are FALSE by default.
 */

const char OPTIONS[]= "0:ac:C:dEf:F:ghiIj:JkKm:M:n:o:p:PqsUVxyYz";

extern const struct option LONG_OPTIONS[];

extern const char HELP[];

#define ENV_STU_CP         "STU_CP"
#define ENV_STU_OPTIONS    "STU_OPTIONS"
#define ENV_STU_SHELL      "STU_SHELL"
#define ENV_STU_STATUS     "STU_STATUS"

static bool option_a= false;
static bool option_E= false;
static bool option_g= false;
static bool option_i= false;
static bool option_I= false;
static bool option_J= false;
static bool option_k= false;
static bool option_K= false;
static bool option_U= false;
static bool option_P= false;
static bool option_q= false;
static bool option_s= false;
static bool option_x= false;
static bool option_z= false;

enum class Order {
	DFS   = 0,
	RANDOM= 1,
	/* -M mode is coded as Order::RANDOM */
};
static Order order= Order::DFS;

static bool option_parallel= false;
/* Option -j is used with value >1 */

static bool order_vec= false;
/* Use vectors for randomization */

static long options_jobs= 1;
/* Number of free slots for jobs.  This is a long because strtol() gives a
 * long.  Set before calling main() from the -j option, and then changed
 * internally by Executor.  Always nonnegative. */

static const char **envp_global= nullptr;

static const char *dollar_zero= nullptr;
/* Does the same as program_invocation_name (which is a GNU extension,
 * so we don't use it); the value of argv[0], set in main() */

/* Return whether this was a valid settings option */
bool option_setting(char c);
bool option_various(char c);

void set_option_i();
void set_option_j(const char *value);
void set_option_m(const char *value);
void set_option_M(const char *value);
void print_option_V();
void set_env_options();
void check_status();

#endif /* ! OPTIONS_HH */
