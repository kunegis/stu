/*
 * The top-level source code file, which includes the main() function.
 * See the manpage for a description of options, exit codes, etc.  
 */

/* Enable bounds checking when using GNU libc.  Must be defined before
 * including any of the standard headers.  (Only in non-debug mode).  A
 * no-op for non-GNU libc++ libraries. 
 */ 
#ifndef NDEBUG
#    define _GLIBCXX_DEBUG
#endif 

/* Used for all of Stu.
 */
using namespace std; 

#include <unistd.h>
#include <sys/time.h>

#include <memory>
#include <vector>

#include "dependency.hh"
#include "execution.hh"
#include "rule.hh"
#include "timestamp.hh"

/* We use getopt(), which means that Stu does only support short
 * options, and not long options.  We avoid getopt_long() as it is a GNU
 * extension, and the short options are sufficient for now. 
 */
#define STU_OPTIONS "ac:C:Ef:F:ghj:kKm:M:pqsvVwxz"

/* The following strings do not contain tabs, but only space characters */  
#define STU_HELP						       \
	"Usage:   stu [-f FILENAME] [OPTIONS...] [TARGETS...]\n"       \
	"By default, build the first target in the file 'main.stu'.\n" \
	"TARGETS can be specified in full Stu syntax.\n"               \
	"Options:\n"						       \
	"   -a             Treat all trivial dependencies as non-trivial\n"          \
	"   -c FILENAME    Pass a target filename without Stu syntax parsing\n"      \
	"   -C EXPRESSIONS Pass a target in full Stu syntax\n"		             \
	"   -E             Explain error messages\n"                                 \
	"   -f FILENAME    The input file to use instead of 'main.stu'\n"            \
	"   -F RULES       Pass rules in Stu syntax\n"                               \
	"   -g             Treat all optional dependencies as non-optional\n"        \
	"   -h             Output help and exit\n"		                     \
	"   -j K           Run K jobs in parallel\n"			             \
	"   -k             Keep on running after errors\n"		             \
	"   -K             Don't delete target files on error or interruption\n"     \
	"   -m ORDER       Order to run the targets:\n"			             \
	"      dfs         (default) Depth-first order, like in Make\n"	             \
	"      random      Random order\n"				             \
	"   -M STRING      Pseudorandom run order, seeded by given string\n"         \
	"   -p             Print the rules and exit\n"                               \
	"   -q             Question mode; check whether targets are up to date\n"    \
	"   -s             Silent mode; do not output commands\n"	             \
	"   -v             Verbose mode; show execution information on stderr\n"     \
	"   -V             Output version and exit\n"				     \
	"   -w             Short output; show target filenames instead of commands\n"\
	"   -x             Ouput each command statement individually\n"              \
	"   -z             Output runtime statistics on stdout\n"                    \
	"Report bugs to: kunegis@gmail.com\n" \
	"Stu home page: <https:/""/github.com/kunegis/stu>\n"

/* Initialize buffers; called once from main() */ 
void init_buf(); 

/* Parse a string of dependencies and add them to the vector. Used for
 * both the -C option and optionless arguments. 
 */
void add_dependencies_string(vector <shared_ptr <Dependency> > &dependencies,
			     const char *string_);

/* Read in an input file and add the rules to the given rule set.  Used
 * for the -f option and the default input file.  If not yet non-null,
 * set RULE_FIRST to the first rule.  FILE_FD can be -1 or the FD or the
 * filename, if already opened.  If FILENAME is "-", use standard
 * input.  
 */
void read_file(string filename,
	       int file_fd,
	       Rule_Set &rule_set, 
	       shared_ptr <Rule> &rule_first);

void read_string(const char *s,
		 Rule_Set &rule_set, 
		 shared_ptr <Rule> &rule_first);

int main(int argc, char **argv, char **envp)
{
	/* Initialization codes */
	dollar_zero= argv[0]; 
	envp_global= (const char **)envp; 

	/* Refuse to run when $STU_STATUS is set */ 
	const char *const stu_status= getenv("STU_STATUS");
	if (stu_status != nullptr) {
		print_error("Refusing to run recursive Stu; unset $STU_STATUS to circumvent");
		exit(ERROR_FATAL); 
	}

	init_buf();

	try {
		/* Filenames passed using the -f option.  Entries are
		 * unique and sorted as they were given, except
		 * duplicates. */   
		vector <string> filenames;

		/* Assemble targets here */ 
		vector <shared_ptr <Dependency> > dependencies; 

		/* Set to the first rule when there is one */ 
		shared_ptr <Rule> rule_first;

		bool had_option_c= false; /* Both lower and upper case */
		bool had_option_f= false; /* Both lower and upper case */
		bool had_option_F= false; /* Only -F */

		for (int c; (c= getopt(argc, argv, STU_OPTIONS)) != -1;) {
			switch (c) {

			case 'a': option_nontrivial= true;     break;
			case 'E': option_explain= true;        break;
			case 'g': option_nonoptional= true;    break;
			case 'h': fputs(STU_HELP, stdout);     exit(0);
			case 'k': option_keep_going= true;     break;
			case 'K': option_no_delete= true;      break;
			case 'p': option_print= true;          break; 
			case 'q': option_question= true;       break;
			case 's': output_mode= Output::SILENT; break;
			case 'v': option_verbose= true;        break;
			case 'w': output_mode= Output::SHORT;  break;
			case 'x': option_individual= true;     break;
			case 'z': option_statistics= true;     break;

			case 'c': 
				{
					had_option_c= true; 
					if (*optarg == '\0') {
						print_error("Option -c must take non-empty argument"); 
						exit(ERROR_FATAL);
					}
					const char *const name= optarg;
					Type type= Type::FILE;
					Place place(Place::Type::ARGV, name, 1, 0);
					dependencies.push_back
						(make_shared <Direct_Dependency>
						 (0, Place_Param_Target
						  (type, Place_Param_Name(name, place))));
					break;
				}

			case 'C': 
				{
					had_option_c= true; 
					add_dependencies_string(dependencies, optarg);
					break;
				}

			case 'f':
				if (*optarg == '\0') {
					print_error("Option -f must take non-empty argument");
					exit(ERROR_FATAL);
				}

				for (string &filename:  filenames) {
					if (filename == optarg)  goto end;
				}
				had_option_f= true;
				filenames.push_back(optarg); 
				read_file(optarg, -1, Execution::rule_set, rule_first);
			end:
				break;

			case 'F':
				had_option_f= true;
				had_option_F= true; 
				read_string(optarg, Execution::rule_set, rule_first);
				break;

			case 'j':
				errno= 0;
				char *endptr;
				Execution::jobs= strtol(optarg, &endptr, 0);
				if (errno != 0 || *endptr != '\0') {
					print_error("Invalid argument to -j");
					exit(ERROR_FATAL); 
				}
				if (Execution::jobs < 1) {
					print_error("Argument to -j must be positive");
					exit(ERROR_FATAL); 
				}
				break;

			case 'm':
				if (!strcmp(optarg, "random"))  {
					order= Order::RANDOM;
					/* Use gettimeofday() instead of time()
					 * to get millisecond instead of second
					 * precision */ 
					struct timeval tv;
					if (gettimeofday(&tv, nullptr) != 0) {
						perror("gettimeofday");
						exit(ERROR_FATAL); 
					}
					srand(tv.tv_sec + tv.tv_usec);
				}
				else if (!strcmp(optarg, "dfs"))     /* Default */ ;
				else {
					print_error(frmt("Invalid order '%s' for option -m", optarg));
					exit(ERROR_FATAL); 
				}
				break;

			case 'M':
				order= Order::RANDOM;
				srand(hash <string> ()(string(optarg))); 
				break;

			case 'V': 
				puts("stu " STU_VERSION);    
				puts("Copyright (C) 2016 Jerome Kunegis");
				puts("License GPLv3+: GNU GPL version 3 or later <http:/""/gnu.org/licenses/gpl.html>");
				puts("This is free software: you are free to change and redistribute it.");
				puts("There is NO WARRANTY, to the extent permitted by law.");
				if (ferror(stdout)) {
					perror("puts"); 
					exit(ERROR_FATAL);
				}
				exit(0);

			default:  
				/* Invalid option -- an error message was
				 * already printed by getopt() */   
				fprintf(stderr, 
					"To get a list of all options, use '%s -h'\n", 
					dollar_zero); 
				exit(ERROR_FATAL); 
			}
		}

		order_vec= (order == Order::RANDOM); 

		/* Targets passed as-is on the command line, outside of options */ 
		for (int i= optind;  i < argc;  ++i) {
			/* With GNU getopt(), I is not the index that the argument had
			 * originally, because getopt() reorders its arguments.
			 * This is why we can't put I into the trace. 
			 */ 
			add_dependencies_string(dependencies, argv[i]);
		}

		/* Use the default Stu file if -f/-F are not used */ 
		if (! had_option_f) {
			filenames.push_back(FILENAME_INPUT_DEFAULT); 
			int file_fd= open(FILENAME_INPUT_DEFAULT, O_RDONLY); 
			if (file_fd >= 0) {
				read_file(FILENAME_INPUT_DEFAULT, file_fd, Execution::rule_set, rule_first); 
			} else {
				if (errno == ENOENT) { 
					/* The default file does not exist --
					 * fail if no target is given */  
					if (dependencies.empty() && ! had_option_c && ! option_print) {
						print_error("No target given and no default file "
							    "'" FILENAME_INPUT_DEFAULT "' present");
						explain_no_target(); 
						throw ERROR_LOGICAL; 
					}
				} else { 
					/* Other errors by open() are fatal */ 
					perror(FILENAME_INPUT_DEFAULT);
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
		if (dependencies.empty() && ! had_option_c) {

			if (rule_first == nullptr) {
				print_error
					((filenames.size() == 1 && ! had_option_F)
					 ? fmt("Input file '%s' does not contain any rules and no target given", 
					       filenames.at(0))
					 : "No rules and no targets given");
				exit(ERROR_FATAL);
			}

			if (rule_first->place_param_target.place_param_name.get_n() != 0) {
				rule_first->place <<
					"the first rule given must not be parametrized if no target is given";
				exit(ERROR_FATAL);
			}

			dependencies.push_back
				(make_shared <Direct_Dependency> (0, rule_first->place_param_target));  
		}

		/* Execute */
		Execution::main(dependencies);

	} catch (int error) {
		assert(error >= 1 && error <= 3); 
		exit(error); 
	}

	exit(0); 
}

void init_buf()
{
	/* If -s is not given, set STDOUT to line buffered, so that the
	 * output of command lines always happens before the output of
	 * commands themselves */ 
	if (output_mode > Output::SILENT) {
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

void add_dependencies_string(vector <shared_ptr <Dependency> > &dependencies,
			     const char *string_)
{
	vector <shared_ptr <Token> > tokens;
	Place place_end;
				
	Parse::parse_tokens_string(tokens, false, place_end, string_);

	auto iter= tokens.begin(); 
	vector <shared_ptr <Dependency> > dependencies_option;
	Build build(tokens, iter, place_end); 
	Place_Param_Name input; /* remains empty */ 
	Place place_input; /* remains empty */ 
	build.build_expression_list
		(dependencies_option, input, place_input); 
	if (iter != tokens.end()) {
		(*iter)->get_place() << "expected a dependency"; 
		throw ERROR_LOGICAL;
	}

	for (auto &j:  dependencies_option) {
		dependencies.push_back(j); 
	}
}

void read_file(string filename,
	       int file_fd,
	       Rule_Set &rule_set, 
	       shared_ptr <Rule> &rule_first)
{
	assert(file_fd == -1 || file_fd > 1); 
	assert(filename != "");
	if (filename == "-")  filename= ""; 

	/* Tokenize */ 
	vector <shared_ptr <Token> > tokens;
	Place place_end;
	Parse::parse_tokens_file(tokens, true, place_end, filename, file_fd); 

	/* Build rules */
	auto iter= tokens.begin(); 
	Build build(tokens, iter, place_end);
	vector <shared_ptr <Rule> > rules;
	build.build_rule_list(rules); 
	if (iter != tokens.end()) {
		(*iter)->get_place() << "expected a rule"; 
		throw ERROR_LOGICAL;
	}

	/* Add to set */
	rule_set.add(rules);

	/* Set the first one */
	if (rule_first == nullptr) {
		auto i= rules.begin();
		if (i != rules.end()) {
			rule_first= *i; 
		}
	}
}

void read_string(const char *s,
		 Rule_Set &rule_set, 
		 shared_ptr <Rule> &rule_first)
{
	/* Tokenize */ 
	vector <shared_ptr <Token> > tokens;
	Place place_end;
	Parse::parse_tokens_string(tokens, true, place_end, s);

	/* Build rules */
	auto iter= tokens.begin(); 
	Build build(tokens, iter, place_end);
	vector <shared_ptr <Rule> > rules;
	build.build_rule_list(rules); 
	if (iter != tokens.end()) {
		(*iter)->get_place() << "expected a rule"; 
		throw ERROR_LOGICAL;
	}

	/* Add to set */
	rule_set.add(rules);

	/* Set the first one */
	if (rule_first == nullptr) {
		auto i= rules.begin();
		if (i != rules.end()) {
			rule_first= *i; 
		}
	}
}
