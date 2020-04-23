/* 
 * This is the top-level source code file, which contains the main()
 * function.  See the manpage for a description of options, the exit
 * status, etc.
 */

/* Enable bounds checking when using GNU libc.  Must be defined before
 * including any of the standard headers.  (Only in non-debug mode).  A
 * no-op for non-GNU libc++ libraries.  */ 
#ifndef NDEBUG
#    define _GLIBCXX_DEBUG
#endif 

#include <unistd.h>
#include <sys/time.h>

#include <memory>
#include <vector>

/* Used for all of Stu */
using namespace std; 

#include "dep.hh"
#include "execution.hh" 
#include "rule.hh"
#include "timestamp.hh"
#include "color.hh"

/*
 * Note:  Stu does not call setlocale(), and therefore can make use of
 * isspace() detecting spaces as defined in the C locale.  The same is
 * true for isalnum(), which is used to get ASCII results. 
 */

/* 
 * We use getopt(), which means that Stu does only support short
 * options, and not long options.  We avoid getopt_long() as it is a GNU
 * extension, and the short options are sufficient for now. 
 *
 * Also, using getopt() means that the exact synytax of Stu depends on
 * the platform:  GNU getopt() will all options to follow arguments,
 * while BSD getopt() does not. 
 */
const char OPTIONS[]= "0:ac:C:dEf:F:ghij:JkKm:M:n:o:p:PqsVxyYz"; 

/* The output of the help (-h) option.  The following strings do not
 * contain tabs, but only space characters.  */   
const char HELP[]= 
	"Usage: " PACKAGE " [-f FILENAME] [OPTION]... [TARGET]...\n"           
	"By default, build the first target in the file 'main.stu'.\n" 
	"TARGET may include the special characters '@[]-'.\n"     
	"Options:\n"						       
	"  -0 FILENAME      Read \\0-separated file targets from the given file\n"
	"  -a               Treat all trivial dependencies as non-trivial\n"          
	"  -c FILENAME      Pass a target filename without Stu syntax parsing\n"      
	"  -C EXPRESSIONS   Pass a target in full Stu syntax\n"		              
	"  -d               Debug mode: show execution information on stderr\n"     
	"  -E               Explain error messages\n"                                 
	"  -f FILENAME      The input file to use instead of 'main.stu'\n"            
	"  -F RULES         Pass rules in Stu syntax\n"                               
	"  -g               Treat all optional dependencies as non-optional\n"        
	"  -h               Output help and exit\n"		                      
	"  -i               Interactive mode (run jobs in foreground)\n"
	"  -j K             Run K jobs in parallel\n"			              
	"  -J               Disable Stu syntax in arguments\n"                        
	"  -k               Keep on running after errors\n"		              
	"  -K               Don't delete target files on error or interruption\n"     
	"  -m ORDER         Order to run the targets:\n"			      
	"     dfs           (default) Depth-first order, like in Make\n"	      
	"     random        Random order\n"				              
	"  -M STRING        Pseudorandom run order, seeded by given string\n"         
	"  -n FILENAME      Read \\n-separated file targets from the given file\n"
	"  -o FILENAME      Build an optional dependency, i.e., build it only if it\n"
	"                   exists and is out of date\n"
	"  -p FILENAME      Build a persistent dependency, i.e., ignore its timestamp\n"
	"  -P               Print the rules and exit\n"                               
	"  -q               Question mode: check whether targets are up to date\n"    
	"  -s               Silent mode: don't use stdout\n"
	"  -V               Output version and exit\n"				      
	"  -x               Output each line in a command individually\n"              
	"  -y               Disable color in output\n"                                
	"  -Y               Enable color in output\n"
	"  -z               Output run-time statistics on stdout\n"                   
	"Report bugs to: " PACKAGE_BUGREPORT "\n" 
	"Stu home page: <" PACKAGE_URL ">\n";

const char VERSION_INFO[]=
	PACKAGE " " STU_VERSION "\n"
	"Copyright (C) 2014-2020 Jerome Kunegis, University of Namur\n"
	"License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
	"This is free software: you are free to change and redistribute it.\n"
	"There is NO WARRANTY, to the extent permitted by law.\n";

void init_buf(); 
/* Initialize buffers; called once from main() */ 

void add_deps_option_C(vector <shared_ptr <const Dep> > &deps,
		       const char *string_);
/* Parse a string of dependencies and add them to the vector. Used for
 * the -C option.  Support the full Stu syntax.  */

/* Set one of the "setting options", i.e., of of those that can appear
 * in $STU_OPTIONS.  Return whether this was a valid settings option.  */ 
bool stu_setting(char c)
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
		if (stu_options != NULL) {
			while (*stu_options) {
				char c= *stu_options++;
				if (c == '-' || isspace(c))
					continue; 
				if (! stu_setting(c)) {
					Place place(Place::Type::ENV_OPTIONS);
					place << fmt("invalid option %s",
						     multichar_format_err(frmt("-%c", c)));
					exit(ERROR_FATAL); 
				}
			}
		}
		
		for (int c; (c= getopt(argc, argv, OPTIONS)) != -1;) {

			if (stu_setting(c))
				continue;

			switch (c) {

			case 'a': option_nontrivial= true;     break;
			case 'd': option_debug= true;          break;
			case 'g': option_nonoptional= true;    break;
			case 'h': fputs(HELP, stdout);         exit(0);
			case 'J': option_literal= true;        break;
			case 'k': option_keep_going= true;     break;
			case 'K': option_no_delete= true;      break;
			case 'P': option_print= true;          break;  
			case 'q': option_question= true;       break;

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

			case 'i':
				option_interactive= true;
				if (Job::get_tty() < 0) {
					Place place(Place::Type::OPTION, 'i');
					print_warning(place, "Interactive mode cannot be used because no TTY is available"); 
				}
				break;
		
			case 'j':  {
				errno= 0;
				char *endptr;
				Execution::jobs= strtol(optarg, &endptr, 10);
				Place place(Place::Type::OPTION, c); 
				if (errno != 0 || *endptr != '\0') {
					place << fmt("expected the number of jobs, not %s",
						     name_format_err(optarg)); 
					exit(ERROR_FATAL); 
				}
				if (Execution::jobs < 1) {
					place << fmt("expected a positive number of jobs, not %s",
						     name_format_err(optarg));
					exit(ERROR_FATAL); 
				}
				option_parallel= Execution::jobs > 1; 
				break;
			}

			case 'm':
				if (!strcmp(optarg, "random"))  {
					order= Order::RANDOM;
					/* Use gettimeofday() instead of time()
					 * to get millisecond instead of second
					 * precision */ 
					struct timeval tv;
					if (gettimeofday(&tv, nullptr) != 0) {
						print_error_system("gettimeofday");
						exit(ERROR_FATAL); 
					}
					buffer_generator.seed(tv.tv_sec + tv.tv_usec); 
				}
				else if (!strcmp(optarg, "dfs"))     /* Default */ ;
				else {
					print_error(fmt("Invalid argument %s for option %s-m%s; valid values are %s and %s", 
							name_format_err(optarg),
							Color::word, Color::end,
							name_format_err("random"),
							name_format_err("dfs"))); 
					exit(ERROR_FATAL); 
				}
				break;

			case 'M':
				order= Order::RANDOM;
				buffer_generator.seed(hash <string> ()(string(optarg)));
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

			case 'V': 
				fputs(VERSION_INFO, stdout); 
				printf("USE_MTIM = %u\n", USE_MTIM); 
				exit(0);

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

		if (! option_literal) {
			Parser::get_target_arg(deps, argc - optind, argv + optind); 
		} 

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

		/* Execute */
		Execution::main(deps);

	} catch (int e) {
		assert(e >= 1 && e <= 3); 
		error= e;
	}

	/*
	 * Code executed before exiting:  This must be executed even if
	 * Stu fails (but not for fatal errors).
	 */
	
	if (option_statistics) {
		Job::print_statistics();
	}

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
