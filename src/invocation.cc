#include "invocation.hh"

Invocation::Invocation(int argc, char **argv, int &error)
{
	int option_index= 0;
	int c;

	while ((c= getopt_long(argc, argv, OPTIONS, LONG_OPTIONS, &option_index)) != -1) {
		if (option_setting(c)) continue;
		if (option_various(c)) continue;

		switch (c) {
		case 'c':  {
			had_option_target= true;
			Place place(Place::Type::OPTION, 'c');
			if (*optarg == '\0') {
				place << "expected a non-empty argument";
				exit(ERROR_FATAL);
			}
			deps.push_back
				(std::make_shared <Plain_Dep>
					(0, Place_Target
						(0, Place_Name(optarg, place))));
			break;
		}

		case 'C':  {
			had_option_target= true;
			Parser::add_deps_option_C(deps, optarg);
			break;
		}

		case 'f':
			if (*optarg == '\0') {
				Place(Place::Type::OPTION, 'f') <<
					"expected non-empty argument";
				exit(ERROR_FATAL);
			}

			for (string &filename: filenames) {
				/* Silently ignore duplicate input file on command line */
				if (filename == optarg)  goto end;
			}
			had_option_f= true;
			filenames.push_back(optarg);
			Parser::get_file(optarg, -1, Executor::rule_set,
				target_first, place_first);
		end:
			break;

		case 'F':
			had_option_f= true;
			Parser::get_string(optarg, Executor::rule_set,
				target_first);
			break;

		case 'n':
		case '0':  {
			had_option_target= true;
			Place place(Place::Type::OPTION, c);
			if (*optarg == '\0') {
				place << "expected a non-empty argument";
				exit(ERROR_FATAL);
			}
			deps.push_back
				(std::make_shared <Dynamic_Dep>
					(0, std::make_shared <Plain_Dep>
						(1 << flag_get_index(c),
							Place_Target
							(0, Place_Name(optarg, place)))));
			break;
		}

		case 'o':
		case 'p':  {
			had_option_target= true;
			Place place(Place::Type::OPTION, c);
			if (*optarg == '\0') {
				place << "expected a non-empty argument";
				exit(ERROR_FATAL);
			}
			int index= flag_get_index(c);
			Place places[C_PLACED];
			places[index]= place;
			deps.push_back(std::make_shared <Plain_Dep>
				(1 << flag_get_index(c), places,
					Place_Target(0, Place_Name(optarg, place))));
			break;
		}

		default:
			/* Invalid option -- an error message was
			 * already printed by getopt() */
			string text= show(frmt("%s -h", dollar_zero));
			fprintf(stderr,
				"To get a list of all options, use %s\n",
				text.c_str());
			exit(ERROR_FATAL);
		}
	}

	order_vec= (order == Order::RANDOM);

	if (option_i && option_parallel) {
		Place(Place::Type::OPTION, 'i')
			<< fmt("parallel mode using %s cannot be used in interactive mode",
				show(Flag_View('j')));
		exit(ERROR_FATAL);
	}

	/* Targets passed on the command line, outside of options */
	for (int i= optind; i < argc; ++i) {
		/* The number I may not be the index that the argument had originally,
		 * because getopt() may reorder its arguments. (I.e., using GNU getopt.)
		 * This is why we can't put I into the backtrace. */
		Place place(Place::Type::ARGUMENT);
		if (*argv[i] == '\0') {
			place << fmt("%s: name must not be empty",
				show(argv[i]));
			if (! option_k)
				throw ERROR_LOGICAL;
			error |= ERROR_LOGICAL;
		} else if (option_J) {
			deps.push_back(std::make_shared <Plain_Dep>
				(0, Place_Target(0, Place_Name(argv[i], place))));
		}
	}

	if (! option_J)
		Parser::get_target_arg(deps, argc - optind, argv + optind);

	if (option_I + option_P + option_q >= 2) {
		print_error("options -I/-P/-q must not be used together");
		exit(ERROR_FATAL);
	}

	/* Use the default Stu script if -f/-F are not used */
	if (! had_option_f) {
		filenames.push_back(FILENAME_INPUT_DEFAULT);
		int file_fd= open(FILENAME_INPUT_DEFAULT, O_RDONLY);
		if (file_fd >= 0) {
			Parser::get_file("", file_fd,
				Executor::rule_set, target_first,
				place_first);
		} else if (errno == ENOENT) {
			/* The default file does not exist -- fail if no target is given */
			if (deps.empty() && !had_option_target
				&& !option_P && !option_I) {
				print_error(
					fmt("expected a target or the default file %s",
						show(FILENAME_INPUT_DEFAULT)));

				explain_no_target();
				throw ERROR_LOGICAL;
			}
		} else {
			/* Other errors by open() are fatal */
			print_errno("open", FILENAME_INPUT_DEFAULT);
			exit(ERROR_FATAL);
		}
	}

	if (option_P) {
		Executor::rule_set.print_for_option_P();
		exit(0);
	}
	if (option_I) {
		Executor::rule_set.print_for_option_I();
		exit(0);
	}

	/* If no targets are given on the command line, use the first non-variable target */
	if (deps.empty() && ! had_option_target) {
		if (target_first == nullptr) {
			if (! place_first.empty()) {
				place_first
					<< "expected a rule, because no target is given";
			} else {
				print_error("no rules and no targets given");
			}
			exit(ERROR_FATAL);
		}
		if (target_first->place_target.place_name.is_parametrized()) {
			target_first->place <<
				fmt("the first target %s must not be parametrized if no target is given",
					show(target_first));
			exit(ERROR_FATAL);
		}
		deps.push_back(std::make_shared <Plain_Dep> (*target_first));
	}
}

void Invocation::main_loop()
{
	assert(options_jobs >= 0);
	Root_Executor *root_executor= new Root_Executor(deps);
	int error= 0;
	shared_ptr <const Root_Dep> dep_root= std::make_shared <Root_Dep> ();

	try {
		while (! root_executor->finished()) {
			Proceed proceed;
			do {
				proceed= root_executor->execute(dep_root);
				assert(is_valid(proceed));
			} while (proceed & P_CALL_AGAIN);

			if (proceed & P_WAIT)
				File_Executor::wait();
		}

		assert(root_executor->finished());
		assert(File_Executor::executors_by_pid_size == 0);

		bool success= (root_executor->get_error() == 0);
		assert(option_k || success);

		error= root_executor->get_error();
		assert(error >= 0 && error <= 3);

		if (success) {
			if (! Executor::hide_out_message) {
				if (Executor::out_message_done)
					print_out("Build successful");
				else
					print_out("Targets are up to date");
			}
		} else {
			if (option_k)
				print_error_reminder(
					"targets not up to date because of errors");
		}
	} catch (int e) {
		assert(! option_k);
		assert(e >= 1 && e <= 4);
		if (File_Executor::executors_by_pid_size) {
			terminate_jobs(false);
		}
		assert(e > 0 && e < ERROR_FATAL);
		error= e;
	}

	if (error)
		throw error;
}
