#ifndef OPTIONS_HH
#define OPTIONS_HH

#include "package.hh"

/*
 * Global variables of the process.
 *
 * All boolean option variables are FALSE by default.
 */

/*
 * We use getopt(), which means that Stu does only support short options, and not long
 * options.  We avoid getopt_long() as it is a GNU extension, and the short options are
 * sufficient for now.
 *
 * Also, using getopt() means that the exact synytax of Stu depends on the platform:  GNU
 * getopt() will all options to follow arguments, while BSD getopt() does not.
 */
const char OPTIONS[]= "0:ac:C:dEf:F:ghiIj:JkKm:M:n:o:p:PqsVxyYz";

/* The following strings do not contain tabs, but only space characters. */
const char HELP[]=
	"Usage: " PACKAGE " [ -q | -P | -I ] [-f FILENAME] [OPTION]... [TARGET]...\n"
	"By default, build the first target in the file 'main.stu'.\n"
	"TARGET may include the special characters '@[]-'.\n"
	"Options:\n"
	"  -0 FILENAME      Read \\0-separated file targets from the given file\n"
	"  -a               Treat all trivial dependencies as non-trivial\n"
	"  -c FILENAME      Pass a target filename without Stu syntax parsing\n"
	"  -C EXPRESSION    Pass a target in full Stu syntax\n"
	"  -d               Debug mode: show execution information on stderr\n"
	"  -E               Explain error messages\n"
	"  -f FILENAME      The input file to use instead of 'main.stu'\n"
	"  -F RULES         Pass rules in Stu syntax\n"
	"  -g               Treat all optional dependencies as non-optional\n"
	"  -h, --help       Output help\n"
	"  -i               Interactive mode (run jobs in foreground)\n"
	"  -I               Print all buildable file targets as glob patterns\n"
	"  -j K             Run K jobs in parallel\n"
	"  -J               Disable Stu syntax in arguments\n"
	"  -k               Keep on running after errors\n"
	"  -K               Don't delete target files on error or interruption\n"
	"  -m ORDER         Order to run the targets:\n"
	"     dfs           (default) Depth-first order, as in Make\n"
	"     random        Random order\n"
	"  -M STRING        Pseudorandom run order, seeded by given string\n"
	"  -n FILENAME      Read \\n-separated file targets from the given file\n"
	"  -o FILENAME      Build an optional dependency, i.e., build it only if it\n"
	"                   exists and is out of date\n"
	"  -p FILENAME      Build a persistent dependency, i.e., ignore its timestamp\n"
	"  -P               Print the rules\n"
	"  -q               Question mode: check whether targets are up to date\n"
	"  -s               Silent mode: don't use stdout\n"
	"  -V, --version    Output version\n"
	"  -x               Output each line in a command individually\n"
	"  -y               Disable color in output\n"
	"  -Y               Enable color in output\n"
	"  -z               Output run-time statistics on stdout\n"
	"Report bugs to: " PACKAGE_EMAIL "\n"
	"Stu home page: <" PACKAGE_URL ">\n";

static bool option_a= false;
static bool option_d= false;
static bool option_E= false;
static bool option_g= false;
static bool option_i= false;
static bool option_I= false;
static bool option_J= false;
static bool option_k= false;
static bool option_K= false;
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
/* Whether the -j option is used with a value >1 */

static bool order_vec;
/* Whether to use vectors for randomization */

static long options_jobs= 1;
/* Number of free slots for jobs.  This is a long because strtol() gives a
 * long.  Set before calling main() from the -j option, and then changed
 * internally by Executor.  Always nonnegative. */

static const char **envp_global;

static const char *dollar_zero;
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
