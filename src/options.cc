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

bool option_setting(char c)
{
	TRACE_FUNCTION();
	TRACE("c= %s", frmt("%c", c));
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
	switch (c) {
	default:   return false;
	case 'a':  option_a= true;         break;
	case 'd':  option_d= true;         break;
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
	option_i= true;
	if (Job::get_tty() >= 0)
		return;
	Place place(Place::Type::OPTION, 'i');
	print_warning(place,
		      "interactive mode cannot be used because no TTY is available");
}

void set_option_j(const char *value)
{
	errno= 0;
	char *endptr;
	options_jobs= strtol(value, &endptr, 10);
	Place place(Place::Type::OPTION, 'j');
	if (errno != 0 || *endptr != '\0') {
		place << fmt("expected the number of jobs, not %s", show(value));
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
	order= Order::RANDOM;
	buffer_generator.seed(std::hash <string> ()(string(value)));
}

void print_option_V()
{
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
	const char *const stu_status= getenv(ENV_STU_STATUS);
	if (!stu_status)
		return;
	print_error(fmt("refusing to run recursive Stu; unset %s to circumvent",
			show_operator("$" ENV_STU_STATUS)));
	exit(ERROR_FATAL);
}
