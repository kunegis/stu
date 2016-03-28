/*
 * The main entry point of Stu.  See the manpage for a description of
 * options, exit codes, etc.  
 */

/* Enable bounds checking when using GNU libc.  Must be defined before
 * including any of the standard headers.  (Only in non-debug mode).  A
 * no-op for non-GNU compilers. 
 */ 
#ifndef NDEBUG
#    define _GLIBCXX_DEBUG
#endif 

/* Used for all of Stu.
 */
using namespace std; 

#include "token.hh"
#include "rule.hh"
#include "error.hh"
#include "parse.hh"
#include "build.hh"
#include "execution.hh"
#include "version.hh"

#include <sys/time.h>

/* We use getopt(), which means that Stu does only support short
 * options, and not long options.  We avoid getopt_long() as it is a GNU
 * extension, and the short options are sufficient for now. 
 */
#define STU_OPTIONS "aC:df:ghj:km:M:svVw"

#ifdef NDEBUG
#    define STU_HELP_VERBOSE ""
#else 
#    define STU_HELP_VERBOSE "   -d            Enable debug info on standard error output (only in debug version of Stu)\n"
#endif

/* Note: the following strings do not contain tabs, but only space
 * characters */ 
#define STU_HELP \
	"Usage:   stu [TARGETS...] [OPTIONS...]\n" \
	"By default, build the first target in the file 'main.stu'.\n" \
	"Options:\n" \
	"   -a            Treat all trivial dependencies as non-trivial\n" \
	"   -C FILENAME   Pass a target filename without Stu syntax parsing\n" \
	STU_HELP_VERBOSE \
	"   -f FILENAME   The input file to use instead of 'main.stu'\n" \
	"   -g            Treat all optional dependencies as non-optional\n" \
	"   -h            Output help\n" \
	"   -j K          Run K jobs in parallel\n"	  \
	"   -k            Keep on running after errors\n" \
	"   -m ORDER      Order to run the targets:\n" \
	"      dfs        (default) Depth-first order, like in Make\n" \
	"      random     Random order\n" \
	"   -M STRING     Pseudorandom run order, seeded by given string\n" \
	"   -s            Silent mode; do not output commands\n" \
	"   -v            Verbose mode; print additional info on standard error output\n" \
	"   -V            Output version\n" \
	"   -w            Short output; don't show the commands, only the target filenames\n"

/* Initialize buffers; called once from main() */ 
void init_buf(); 

int main(int argc, char **argv, char **envp)
{
	/*
	 * Initialization codes 
	 */
	dollar_zero= argv[0]; 
	envp_global= (const char **)envp; 
	process_init(); 

	/* Either FILE_FD is initialized to a file that should be read
	 * (the default file), or FILENAME is set by the -f option.  We
	 * already open() the file here to save a stat() call. 
	 * If FILE_FD is set, then FILENAME is set to the name of the
	 * corresponding filename. 
	 */ 
	int file_fd= -1;
	string filename; 

	init_buf();

	/* Refuse to run when $STU_STATUS is set */ 
	const char *const stu_status= getenv("STU_STATUS");
	if (stu_status != nullptr) {
		print_error("Refusing to run recursive Stu; unset $STU_STATUS to circumvent");
		exit(ERROR_LOGICAL); 
	}

	/* Assemble targets here */ 
	vector <shared_ptr <Dependency> > dependencies; 
//	vector <Target> targets;
//	vector <Place> places;

	for (int c; (c= getopt(argc, argv, STU_OPTIONS)) != -1;) {
		switch (c) {

		case 'a': option_nontrivial= true;     break;
		case 'g': option_nonoptional= true;    break;
		case 'h': fputs(STU_HELP, stdout);     exit(0);
		case 'k': option_continue= true;       break;
		case 's': verbosity= VERBOSITY_SILENT; break;
		case 'v': verbosity= VERBOSITY_VERBOSE;break;
		case 'V': puts("stu " STU_VERSION);    exit(0);
		case 'w': verbosity= VERBOSITY_SHORT;  break;

		case 'C': 
			{
				if (*optarg == '\0') {
					print_error("Option -C must take non-empty argument"); 
					exit(ERROR_LOGICAL);
				}
				const char *const name= optarg;
				Type type= T_FILE;
				Place place(name);
				dependencies.push_back
					(shared_ptr <Dependency>
					 (new Direct_Dependency
					  (0, Place_Param_Target(type, Place_Param_Name(name, place)))));
//				targets.push_back(Target(type, name)); 
//				places.push_back(place); 
				break;
			}

		case 'd': 
#ifndef NDEBUG
			option_debug= true;          
#else
			print_error("Option -d is only available in debug mode; this is a non-debug version of Stu");
			exit(ERROR_LOGICAL); 
#endif 
			break;

		case 'f':
			if (filename != "") {
				print_error("Option -f must be used at most once"); 
				exit(ERROR_LOGICAL);
			}
			if (*optarg == '\0') {
				print_error("Option -f must take non-empty argument");
				exit(ERROR_LOGICAL);
			}
			filename= string(optarg); 
			break;

		case 'j':
			errno= 0;
			char *endptr;
			Execution::jobs= strtol(optarg, &endptr, 0);
			if (errno != 0 || *endptr != '\0') {
				print_error("Invalid argument to -j");
				exit(ERROR_LOGICAL); 
			}
			if (Execution::jobs < 1) {
				print_error("Argument to -j must be positive");
				exit(ERROR_LOGICAL); 
			}
			break;

		case 'm':
			if (!strcmp(optarg, "random"))  {
				order= ORDER_RANDOM;
				/* Use gettimeofday() instead of time()
				 * to get millisecond instead of second
				 * precision */ 
				struct timeval tv;
				if (gettimeofday(&tv, nullptr) != 0) {
					perror("gettimeofday");
					exit(ERROR_SYSTEM); 
				}
				srand(tv.tv_sec + tv.tv_usec);
			}
			else if (!strcmp(optarg, "dfs"))     /* Default */ ;
			else {
				print_error(frmt("Invalid order '%s' for option -m", optarg));
				exit(ERROR_LOGICAL); 
			}
			break;

		case 'M':
			order= ORDER_RANDOM;
			srand(hash <string> ()(string(optarg))); 
			break;

		default:  
			/* Invalid option -- an error message was
			 * already printed by getopt() */   
			exit(ERROR_LOGICAL); 
		}
	}

	order_vec= (order == ORDER_RANDOM); 

	/* Assemble targets from the command line */ 
	for (int i= optind;  i < argc;  ++i) {
		/* Note:  I is not the index that the argument had
		 * originally, because getopt() reorders its arguments.
		 * This is why we can't put I into Argv_Trace. 
		 */ 
		const char *name= argv[i];
		Type type= T_FILE;
		if (*name == '@') {
			type= T_PHONY;
			++name;
		}
		Place place(name);
		dependencies.push_back
			(shared_ptr <Dependency>
			 (new Direct_Dependency
			  (0, Place_Param_Target(type, Place_Param_Name(name, place)))));
//		targets.push_back(Target(type, name)); 
//		places.push_back(place); 
	}

	/* Use the default Stu file, if it exists */ 
	if (filename == "") {
		file_fd= open(FILENAME_INPUT_DEFAULT, O_RDONLY); 
		if (file_fd >= 0) {
			filename= FILENAME_INPUT_DEFAULT;
		} else {
			if (errno == ENOENT) { 
				/* The default file does not exist --
				 * fail if no target is given */  
				if (dependencies.empty()) {
					print_error("No target given and no default file "
						    "'" FILENAME_INPUT_DEFAULT "' present");
					explain_no_target(); 
					exit(ERROR_LOGICAL); 
				}
			} else { 
				/* Other errors by open() are system errors */ 
				perror(FILENAME_INPUT_DEFAULT);
				exit(ERROR_SYSTEM);
			}
		}
	}

	try {
		/* Read input file */ 
		if (filename != "") {

			/* Tokenize */ 
			vector <shared_ptr <Token> > tokens;
			vector <Trace> traces;
			vector <string> filenames; 
			Place place_end;
			Parse::parse(tokens, place_end, filename, true, traces, filenames, file_fd); 

			/* Build rules */
			auto iter= tokens.begin(); 
			Build build(tokens, iter, place_end);
			vector <shared_ptr <Rule> > rules;
			build.build_rule_list(rules); 
			Execution::rule_set.add(rules);

			/* If no targets are given on the command line,
			 * use the first non-variable target */ 
			if (dependencies.empty()) {
				Place_Param_Target place_param_target(T_ROOT); 
//				Target target(T_ROOT); 
//				Place place; 

				for (auto i= rules.begin();  i != rules.end();  ++i) {
					if (dynamic_pointer_cast <Rule> (*i)) {
//						Place_Param_Target 
							place_param_target= 
							dynamic_pointer_cast <Rule> (*i)->place_param_target;
						if (place_param_target.place_param_name.get_n() != 0) {
							place_param_target.place <<
								"the first rule must not be parametrized "
								"if it is used by default";
							throw ERROR_LOGICAL;
						}
//						place_param_target= place_param_target_i; 
//						target= place_param_target.unparametrized(); 
//						place= place_param_target.place; 

						break;
					}
				}

				if (place_param_target.type == T_ROOT) {
					print_error
						(fmt("Input file '%s' does not contain rules and no target given", 
						     filename)); 
					exit(ERROR_LOGICAL);
				}

				dependencies.push_back
					(shared_ptr <Dependency>
					 (new Direct_Dependency(0, place_param_target))); 

//				targets.push_back(target); 
//				places.push_back(place);
			}
		} else {
			/* We checked earlier that when no input file is
			 * specified, there are targets given. */ 
			assert(dependencies.size()); 
		}

		/* Execute */
		int error= Execution::main(dependencies);
		if (error != 0) {
			/* We don't have to output any error here, because the
			 * error(s) was/were already printed when it
			 * occurred */  
			assert(error >= 1 && error <= 4); 
			exit(error);
		}
	}
	catch (int e) {
		assert(e >= 1 && e <= 3); 
		exit(e); 
	}

	exit(0); 
}

void init_buf()
{
	/* If -s is not given, set STDOUT to line buffered, so that the
	 * output of command lines always happens before the output of
	 * commands themselves */ 
	if (verbosity > VERBOSITY_SILENT) {
		/* Note:  only possible if we have not written anything yet */ 
		setvbuf(stdout, nullptr, _IOLBF, 0); 

		/* Set STDOUT to append mode; this is also done by GNU Make */ 
		int flags= fcntl(fileno(stdout), F_GETFL, 0);
		if (flags >= 0)
			fcntl(fileno(stdout), F_SETFL, flags | O_APPEND);
	}

	/* Set STDERR to append mode; this is also done by GNU Make */ 
	int flags= fcntl(fileno(stderr), F_GETFL, 0);
	if (flags >= 0)
		fcntl(fileno(stderr), F_SETFL, flags | O_APPEND);
}

