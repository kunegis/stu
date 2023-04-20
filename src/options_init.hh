#ifndef OPTIONS_INIT_HH
#define OPTIONS_INIT_HH

bool option_setting(char c)
/* Set one of the "setting options", i.e., of of those that can appear
 * in $STU_OPTIONS.  Return whether this was a valid settings option.  */
{
	switch (c) {
	default:  return false;

	case 'E':  option_explain= true;        break;
	case 's':  option_silent= true;         break;
	case 'x':  option_individual= true;     break;
	case 'y':  Color::set(false);           break;
	case 'Y':  Color::set(true);            break;
	case 'z':  option_statistics= true;     break;
	}

	return true;
}

void option_i()
{
	option_interactive= true;
	if (Job::get_tty() < 0) {
		Place place(Place::Type::OPTION, 'i');
		print_warning(place, "Interactive mode cannot be used because no TTY is available");
	}
}

void option_j()
{
	errno= 0;
	char *endptr;
	options_jobs= strtol(optarg, &endptr, 10);
	Place place(Place::Type::OPTION, 'j');
	if (errno != 0 || *endptr != '\0') {
		place << fmt("expected the number of jobs, not %s",
			     name_format_err(optarg));
		exit(ERROR_FATAL);
	}
	if (options_jobs < 1) {
		place << fmt("expected a positive number of jobs, not %s",
			     name_format_err(optarg));
		exit(ERROR_FATAL);
	}
	option_parallel= options_jobs > 1;
}

void option_m()
{
	if (!strcmp(optarg, "random"))  {
		order= Order::RANDOM;
		/* Use gettimeofday() instead of time() to get millisecond
		 * instead of second precision  */
		struct timeval tv;
		if (gettimeofday(&tv, nullptr) != 0) {
			print_error_system("gettimeofday");
			exit(ERROR_FATAL);
		}
		buffer_generator.seed(tv.tv_sec + tv.tv_usec);
	} else if (!strcmp(optarg, "dfs")) {
		/* Default */ ;
	} else {
		print_error(fmt("Invalid argument %s for option %s-m%s; valid values are %s and %s",
				name_format_err(optarg),
				Color::word, Color::end,
				name_format_err("random"),
				name_format_err("dfs")));
		exit(ERROR_FATAL);
	}
}

void option_M()
{
	order= Order::RANDOM;
	buffer_generator.seed(hash <string> ()(string(optarg)));
}

void option_V()
{
	printf(PACKAGE " " STU_VERSION "\n"
	       "Copyright (C) 2014-2023 Jerome Kunegis\n"
	       "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
	       "This is free software: you are free to change and redistribute it.\n"
	       "There is NO WARRANTY, to the extent permitted by law.\n"
	       "USE_MTIM = %u\n",
	       USE_MTIM);
}

#endif /* ! OPTIONS_INIT_HH */
