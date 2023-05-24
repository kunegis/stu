#include "options.hh"

#include <sys/time.h>

#include "buffer.hh"
#include "color.hh"
#include "format.hh"
#include "job.hh"
#include "package.hh"
#include "text.hh"
#include "timestamp.hh"
#include "version.hh"

bool option_setting(char c)
{
	switch (c) {
	default:   return false;
	case 'E':  option_E= true;            break;
	case 's':  option_s= true;            break;
	case 'x':  option_x= true;            break;
	case 'y':  Color::set(false, false);  break;
	case 'Y':  Color::set(true,  true);   break;
	case 'z':  option_z= true;            break;
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
		      "Interactive mode cannot be used because no TTY is available");
}

void set_option_j(const char *value)
{
	errno= 0;
	char *endptr;
	options_jobs= strtol(value, &endptr, 10);
	Place place(Place::Type::OPTION, 'j');
	if (errno != 0 || *endptr != '\0') {
		place << fmt("expected the number of jobs, not %s",
			     show(value));
		exit(ERROR_FATAL);
	}
	if (options_jobs < 1) {
		place << fmt("expected a positive number of jobs, not %s",
			     show(value));
		exit(ERROR_FATAL);
	}
	option_parallel= options_jobs > 1;
}

void set_option_m(const char *value)
{
	if (!strcmp(value, "random"))  {
		order= Order::RANDOM;
		/* Use gettimeofday() instead of time() to get millisecond
		 * instead of second precision  */
		struct timeval tv;
		if (gettimeofday(&tv, nullptr) != 0) {
			print_errno("gettimeofday");
			exit(ERROR_FATAL);
		}
		buffer_generator.seed(tv.tv_sec + tv.tv_usec);
	} else if (!strcmp(value, "dfs")) {
		/* Default */ ;
	} else {
		print_error(fmt("Invalid argument %s for option %s; valid values are %s and %s",
				show(value),
				show("-m"),
//				Color::stderr_highlight_on, Color::stderr_highlight_off,
				show("random"),
				show("dfs")));
		exit(ERROR_FATAL);
	}
}

void set_option_M(const char *value)
{
	order= Order::RANDOM;
	buffer_generator.seed(hash <string> ()(string(value)));
}

void set_option_V()
{
	printf(PACKAGE " " STU_VERSION "\n"
	       "Copyright (C) 2014-2023 Jerome Kunegis\n"
	       "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
	       "This is free software: you are free to change and redistribute it.\n"
	       "There is NO WARRANTY, to the extent permitted by law.\n"
	       "USE_MTIM = %u\n",
	       (unsigned)USE_MTIM);
}

void set_env_options()
{
	const char *stu_options= getenv("STU_OPTIONS");
	if (stu_options != nullptr) {
		while (*stu_options) {
			char c= *stu_options++;
			if (c == '-' || isspace(c))
				continue;
			if (! option_setting(c)) {
				Place place(Place::Type::ENV_OPTIONS);
				place << fmt("invalid option %s",
					     show(frmt("-%c", c)));
				exit(ERROR_FATAL);
			}
		}
	}
}

void check_status()
{
	const char *const stu_status= getenv("STU_STATUS");
	if (stu_status != nullptr) {
		print_error(frmt("Refusing to run recursive Stu; "
				 "unset %s$STU_STATUS%s to circumvent",
				 Color::stderr_highlight_on, Color::stderr_highlight_off));
		exit(ERROR_FATAL);
	}
}
