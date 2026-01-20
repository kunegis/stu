#include "options.hh"

#include <sys/time.h>

#include "buffer.hh"
#include "color.hh"
#include "format.hh"
#include "job.hh"
#include "package.hh"
#include "show.hh"
#include "timestamp.hh"
#include "trace.hh"
#include "version.hh"

const struct option LONG_OPTIONS[]= {
	{ "file",        required_argument, nullptr, 'f'},
	{ "help",        no_argument,       nullptr, 'h'},
	{ "interactive", no_argument,       nullptr, 'i'},
	{ "jobs",        required_argument, nullptr, 'j'},
	{ "keep-going",  no_argument,       nullptr, 'k'},
	{ "print-rules", no_argument,       nullptr, 'P'},
	{ "question",    no_argument,       nullptr, 'q'},
	{ "quiet",       no_argument,       nullptr, 's'},
	{ "silent",      no_argument,       nullptr, 's'},
	{ "version",     no_argument,       nullptr, 'V'},
	{ nullptr, 0, nullptr, 0}
};

const char HELP[]=
/* The following strings do not contain tabs, but only space characters. */
	"Usage: " PACKAGE " [ -q | -P | -I ] [-f FILENAME] [OPTION]... [TARGET]...\n"
	"By default, build the first target in the file 'main.stu'.\n"
	"TARGET may include the special characters '@[]-'.\n"
	"Options:\n"
	"  -0 FILENAME      Read \\0-separated file targets from the given file\n"
	"  -a               Treat all trivial dependencies as non-trivial\n"
	"  -c FILENAME      Pass a target filename without Stu syntax parsing\n"
	"  -C EXPRESSION    Pass a target in full Stu syntax\n"
	"  -E               Explain error messages\n"
	"  -f FILENAME, --file=FILENAME\n"
	"                   The input file to use instead of 'main.stu'\n"
	"  -F RULES         Pass rules in Stu syntax\n"
	"  -g               Treat all optional dependencies as non-optional\n"
	"  -h, --help       Output help\n"
	"  -i, --interactive\n"
	"                   Interactive mode (run jobs in foreground)\n"
	"  -I               Print all buildable file targets as glob patterns\n"
	"  -j K, --jobs=K   Run K jobs in parallel\n"
	"  -J               Disable Stu syntax in arguments\n"
	"  -k, --keep-going Keep on running after errors\n"
	"  -K               Don't delete target files on error or interruption\n"
	"  -m ORDER         Order to run the targets:\n"
	"     dfs           (default) Depth-first order, as in Make\n"
	"     random        Random order\n"
	"  -M STRING        Pseudorandom run order, seeded by given string\n"
	"  -n FILENAME      Read \\n-separated file targets from the given file\n"
	"  -o FILENAME      Build an optional dependency, i.e., build it only if it\n"
	"                   exists and is out of date\n"
	"  -p FILENAME      Build a persistent dependency, i.e., ignore its timestamp\n"
	"  -P, --print-rules\n"
	"                   Print the rules\n"
	"  -q, --question   Question mode: check whether targets are up to date\n"
	"  -s, --quiet, --silent\n"
	"                   Silent mode: don't use stdout\n"
	"  -U               Ignore %version directives\n"
	"  -V, --version    Output version\n"
	"  -x               Output each line in a command individually\n"
	"  -y               Disable color in output\n"
	"  -Y               Enable color in output\n"
	"  -z               Output run-time statistics on stdout\n"
	"Report bugs to: " PACKAGE_EMAIL "\n"
	"Stu home page: <" PACKAGE_URL ">\n";

bool option_setting(char c)
{
	TRACE_FUNCTION();
	TRACE("c= %s", frmt("'%c'", c));
	switch (c) {
	default:   return false;
	case 'E':  option_E= true;            break;
	case 'U':  option_U= true;            break;
	case 's':  option_s= true;            break;
	case 'x':  option_x= true;            break;
	case 'y':  Color::set(false, false);  break;
	case 'Y':  Color::set(true,  true);   break;
	case 'z':  option_z= true;            break;
	}
	return true;
}

bool option_various(char c)
{
	TRACE_FUNCTION();
	TRACE("c= %s", frmt("'%c'", c));
	switch (c) {
	default:   return false;
	case 'a':  option_a= true;         break;
	case 'g':  option_g= true;         break;
	case 'h':  fputs(HELP, stdout);    exit(0);
	case 'i':  set_option_i();         break;
	case 'I':  option_I= true;         break;
	case 'j':  set_option_j(optarg);   break;
	case 'J':  option_J= true;         break;
	case 'k':  option_k= true;         break;
	case 'K':  option_K= true;         break;
	case 'm':  set_option_m(optarg);   break;
	case 'M':  set_option_M(optarg);   break;
	case 'P':  option_P= true;         break;
	case 'q':  option_q= true;         break;
	case 'V':  print_option_V();       exit(0);
	}
	return true;
}

void set_option_i()
{
	TRACE_FUNCTION();
	option_i= true;
	if (Job::get_fd_tty() < 0) {
		Place place(Place::Type::OPTION, 'i');
		place << "interactive mode cannot be used because no TTY is available";
		exit(ERROR_FATAL);
	}
}

void set_option_j(const char *value)
{
	TRACE_FUNCTION();
	errno= 0;
	char *endptr;
	options_jobs= strtol(value, &endptr, 10);
	Place place(Place::Type::OPTION, 'j');
	if (*endptr != '\0') {
		place << fmt("invalid number of jobs %s", show(value));
		exit(ERROR_FATAL);
	}
	if (errno != 0 || *endptr != '\0') {
		place << fmt("invalid number of jobs %s: %s", show(value), strerror(errno));
		exit(ERROR_FATAL);
	}
	if (options_jobs < 1) {
		place << fmt("expected a positive number of jobs, not %s", show(value));
		exit(ERROR_FATAL);
	}
	option_parallel= options_jobs > 1;
}

void set_option_m(const char *value)
{
	TRACE_FUNCTION();
	TRACE("value= %s", value);
	if (!strcmp(value, "random")) {
		order= Order::RANDOM;
		struct timespec ts;
		if (clock_gettime(CLOCK_REALTIME, &ts) < 0) {
			print_errno("clock_gettime");
			exit(ERROR_FATAL);
		}
		buffer_generator.seed(ts.tv_sec + ts.tv_nsec);
	} else if (!strcmp(value, "dfs")) {
		/* Default */ ;
	} else {
		print_error(
			fmt("invalid argument %s for option %s; valid values are %s and %s",
				show(value), show_prefix("-", "m"),
				show("random"), show("dfs")));
		exit(ERROR_FATAL);
	}
}

void set_option_M(const char *value)
{
	TRACE_FUNCTION();
	order= Order::RANDOM;
	buffer_generator.seed(std::hash <string> ()(string(value)));
}

void print_option_V()
{
	TRACE_FUNCTION();
	printf(PACKAGE " " STU_VERSION "\n"
		"Copyright (C) Jerome Kunegis\n"
		"License GPLv3+: GNU GPL version 3 or later "
		"<http://gnu.org/licenses/gpl.html>\n"
		"This is free software: you are free to change and redistribute it.\n"
		"There is NO WARRANTY, to the extent permitted by law.\n"
#ifndef NDEBUG
		"NDEBUG is not defined\n"
#endif
#ifdef STU_COV
		"This version is built for coverage analysis.\n"
#endif
		"USE_MTIM = %u\n",
		(unsigned)USE_MTIM);
}

void set_env_options()
{
	TRACE_FUNCTION();
	const char *stu_options= getenv(ENV_STU_OPTIONS);
	if (!stu_options)
		return;
	while (*stu_options) {
		char c= *stu_options++;
		if (c == '-' || isspace(c))
			continue;
		if (! option_setting(c)) {
			Place place(Place::Type::ENV_OPTIONS);
			place << fmt("invalid option %s",
				     show_prefix("-", frmt("%c", c)));
			exit(ERROR_FATAL);
		}
	}
}

void check_status()
{
	TRACE_FUNCTION();
	const char *const stu_status= getenv(ENV_STU_STATUS);
	if (!stu_status)
		return;
	print_error(fmt("refusing to run recursive Stu; unset %s to circumvent",
			show_operator("$" ENV_STU_STATUS)));
	exit(ERROR_FATAL);
}
