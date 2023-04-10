/* 
 * This is the top-level source code file, which contains the main()
 * function.  See the manpage for a description of options, the exit
 * status, etc.
 */

#define PACKAGE           "stu"
#define PACKAGE_URL       "https://github.com/kunegis/stu"
#define PACKAGE_BUGREPORT "kunegis@gmail.com"

/* Enable bounds checking when using GNU libc.  Must be defined before
 * including any of the standard headers.  (Only in non-debug mode).  A
 * no-op for non-GNU libc++ libraries.  */ 
#ifndef NDEBUG
#    define _GLIBCXX_DEBUGa
#endif 

#include <unistd.h>
#include <sys/time.h>

#include <memory>
#include <vector>

/* Used for all of Stu */
using namespace std; 

#include "dep.hh"
#include "execution.hh"
#include "main_loop.hh"
#include "options_init.hh"
#include "rule.hh"
#include "timestamp.hh"

/*
 * Note:  Stu does not call setlocale(), and therefore can make use of
 * isspace() detecting spaces as defined in the C locale.  The same is
 * true for isalnum(), which is used to get ASCII results. 
 */

void init_buf(); 
/* Initialize buffers; called once from main() */ 

void add_deps_option_C(vector <shared_ptr <const Dep> > &deps,
		       const char *string_);
/* Parse a string of dependencies and add them to the vector. Used for
 * the -C option.  Support the full Stu syntax.  */

int main(int argc, char **argv, char **envp)
{
	/* Initialization */
	dollar_zero= argv[0]; 
	envp_global= (const char **) envp; 
	init_buf();
	Job::init_tty();
	Color::set();
	int error= 0;

	/* Refuse to run when $STU_STATUS is set */ 
	const char *const stu_status= getenv("STU_STATUS");
	if (stu_status != nullptr) {
		print_error(frmt("Refusing to run recursive Stu; "
				 "unset %s$STU_STATUS%s to circumvent",
				 Color::word, Color::end));
		exit(ERROR_FATAL); 
	}

	try {
		vector <string> filenames;
		/* Filenames passed using the -f option.  Entries are
		 * unique and sorted as they were given, except for
		 * duplicates. */   

		vector <shared_ptr <const Dep> > deps; 
		/* Assemble targets here */ 

		shared_ptr <const Place_Param_Target> target_first; 
		/* Set to the first rule when there is one */ 

		Place place_first;
		/* Place of first file when no rule is contained */ 

		bool had_option_target= false;   
		/* Whether any target(s) was passed through one of the
		 * options -c, -C, -o, -p, -n, -0.  Also set when zero
		 * targets are passed through one of these, e.g., when
		 * -n is used on an empty file.  */

		bool had_option_f= false; /* Both lower and upper case */

		/* Parse $STU_OPTIONS */ 
		const char *stu_options= getenv("STU_OPTIONS");
		if (stu_options != nullptr) {
			while (*stu_options) {
				char c= *stu_options++;
				if (c == '-' || isspace(c))
					continue; 
				if (! option_setting(c)) {
					Place place(Place::Type::ENV_OPTIONS);
					place << fmt("invalid option %s",
						     multichar_format_err(frmt("-%c", c)));
					exit(ERROR_FATAL); 
				}
			}
		}
		
		for (int c; (c= getopt(argc, argv, OPTIONS)) != -1;) {
			if (option_setting(c))
				continue;

			switch (c) {
			case 'a':  option_nontrivial= true;     break;
			case 'd':  option_debug= true;          break;
			case 'g':  option_nonoptional= true;    break;
			case 'h':  fputs(HELP, stdout);         exit(0);
			case 'i':  option_i();                  break;
			case 'j':  option_j();                  break;
			case 'J':  option_literal= true;        break;
			case 'k':  option_keep_going= true;     break;
			case 'K':  option_no_delete= true;      break;
			case 'm':  option_m();                  break;
			case 'M':  option_M();                  break;
			case 'P':  option_print= true;          break;  
			case 'q':  option_question= true;       break;
			case 'V':  option_V();  		exit(0);


			case 'c':  {
				had_option_target= true; 
				Place place(Place::Type::OPTION, 'c');
				if (*optarg == '\0') {
					place << "expected a non-empty argument"; 
					exit(ERROR_FATAL);
				}
				deps.push_back
					(make_shared <Plain_Dep>
					 (0, Place_Param_Target
					  (0, Place_Name(optarg, place))));
				break;
			}

			case 'C':  {
				had_option_target= true; 
				add_deps_option_C(deps, optarg);
				break;
			}

			case 'f':
				if (*optarg == '\0') {
					Place(Place::Type::OPTION, 'f') <<
						"expected non-empty argument"; 
					exit(ERROR_FATAL);
				}

				for (string &filename:  filenames) {
					/* Silently ignore duplicate input file on command line */
					if (filename == optarg)  goto end;
				}
				had_option_f= true;
				filenames.push_back(optarg); 
				Parser::get_file(optarg, -1, Execution::rule_set,
						 target_first, place_first);
			end:
				break;

			case 'F':
				had_option_f= true;
				Parser::get_string(optarg, Execution::rule_set, target_first);
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
					(make_shared <Dynamic_Dep>
					 (0, make_shared <Plain_Dep>
					  (1 << flag_get_index(c), 
					   Place_Param_Target
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
				Place places[C_PLACED];
				places[c == 'p' ? I_PERSISTENT : I_OPTIONAL]= place; 
				deps.push_back
					(make_shared <Plain_Dep>
					 (c == 'p' ? F_PERSISTENT : F_OPTIONAL, places,
					  Place_Param_Target(0, Place_Name(optarg, place))));
				break; 
			}

			default:  
				/* Invalid option -- an error message was
				 * already printed by getopt() */   
				string text= name_format_err(frmt("%s -h", dollar_zero));
				fprintf(stderr, 
					"To get a list of all options, use %s\n", 
					text.c_str()); 
				exit(ERROR_FATAL); 
			}
		}

		order_vec= (order == Order::RANDOM);

		if (option_interactive && option_parallel) {
			Place(Place::Type::OPTION, 'i')
				<< fmt("parallel mode using %s cannot be used in interactive mode",
				       multichar_format_err("-j")); 
			exit(ERROR_FATAL); 
		}

		/* Targets passed on the command line, outside of options */ 
		for (int i= optind;  i < argc;  ++i) {
			/* The number I may not be the index that the
			 * argument had originally, because getopt() may
			 * reorder its arguments. (I.e., using GNU
			 * getopt.)  This is why we can't put I into the
			 * trace.  */ 
			Place place(Place::Type::ARGUMENT); 
			if (*argv[i] == '\0') {
				place << fmt("%s: name must not be empty",
					     name_format_err(argv[i])); 
				if (! option_keep_going) 
					throw ERROR_LOGICAL;
				error |= ERROR_LOGICAL; 
			} else if (option_literal)
				deps.push_back(make_shared <Plain_Dep> 
					       (0, Place_Param_Target
						(0, Place_Name(argv[i], place))));
		}

		if (! option_literal) 
			Parser::get_target_arg(deps, argc - optind, argv + optind); 

		/* Use the default Stu script if -f/-F are not used */ 
		if (! had_option_f) {
			filenames.push_back(FILENAME_INPUT_DEFAULT); 
			int file_fd= open(FILENAME_INPUT_DEFAULT, O_RDONLY); 
			if (file_fd >= 0) {
				Parser::get_file("", file_fd, 
						 Execution::rule_set, target_first,
						 place_first); 
			} else {
				if (errno == ENOENT) { 
					/* The default file does not exist --
					 * fail if no target is given */  
					if (deps.empty() && ! had_option_target 
					    && ! option_print) {
						print_error(fmt("Expected a target or the default file %s",
								name_format_err(FILENAME_INPUT_DEFAULT))); 

						explain_no_target(); 
						throw ERROR_LOGICAL; 
					}
				} else { 
					/* Other errors by open() are fatal */ 
					print_error_system(FILENAME_INPUT_DEFAULT);
					exit(ERROR_FATAL);
				}
			}
		}

		if (option_print) {
			Execution::rule_set.print(); 
			exit(0); 
		}

		/* If no targets are given on the command line,
		 * use the first non-variable target */ 
		if (deps.empty() && ! had_option_target) {
			if (target_first == nullptr) {
				if (! place_first.empty()) {
					place_first
						<< "expected a rule, because no target is given";
				} else {
					print_error("No rules and no targets given");
				}
				exit(ERROR_FATAL);
			}
			if (target_first->place_name.is_parametrized()) {
				target_first->place <<
					fmt("the first target %s must not be parametrized if no target is given",
					    target_first->format_err());
				exit(ERROR_FATAL);
			}
			deps.push_back(make_shared <Plain_Dep> (*target_first));  
		}

		main_loop(deps);
	} catch (int e) {
		assert(e >= 1 && e <= 3); 
		error= e;
	}

	/*
	 * Code executed before exiting:  This must be executed even if
	 * Stu fails (but not for fatal errors).
	 */
	
	if (option_statistics) 
		Job::print_statistics();

	if (fclose(stdout)) {
		perror("fclose(stdout)");
		exit(ERROR_FATAL);
	}
	/* No need to flush stderr, because it is line buffered, and if
	 * we used it, it means there was an error anyway, so we're not
	 * losing any information  */
	exit(error); 
}

void init_buf()
{
	/* Set STDOUT to line buffered, so that the
	 * output of command lines always happens before the output of
	 * commands themselves */ 
	/* Note:  Setting the buffering like this is only possible if we
	 * have not written anything yet.  */  
	if (0 != setvbuf(stdout, nullptr, _IOLBF, 0)) {
		print_error_system("setvbuf"); 
		exit(ERROR_FATAL); 
	}
		
	/* Set STDOUT to append mode; this is also done by GNU Make */ 
	{
		int flags= fcntl(fileno(stdout), F_GETFL, 0);
		if (flags >= 0)
			fcntl(fileno(stdout), F_SETFL, flags | O_APPEND);
	}

	/* Set STDERR to append mode; this is also done by GNU Make */ 
	{
		int flags= fcntl(fileno(stderr), F_GETFL, 0);
		if (flags >= 0)
			fcntl(fileno(stderr), F_SETFL, flags | O_APPEND);
	}
}

void add_deps_option_C(vector <shared_ptr <const Dep> > &deps,
		       const char *string_)
{
	vector <shared_ptr <Token> > tokens;
	Place place_end;
				
	Tokenizer::parse_tokens_string
		(tokens, 
		 Tokenizer::OPTION_C,
		 place_end, string_,
		 Place(Place::Type::OPTION, 'C'));

	vector <shared_ptr <const Dep> > deps_option;
	Place_Name input; /* remains empty */ 
	Place place_input; /* remains empty */ 

	Parser::get_expression_list(deps_option, tokens, 
				    place_end, input, place_input);

	for (auto &j:  deps_option) {
		deps.push_back(j); 
	}
}
