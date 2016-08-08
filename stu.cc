/*
 * This is the top-level source code file, which contains the main()
 * function.  See the manpage for a description of options, exit codes,
 * etc.   
 */

/*
 * Enable bounds checking when using GNU libc.  Must be defined before
 * including any of the standard headers.  (Only in non-debug mode).  A
 * no-op for non-GNU libc++ libraries. 
 */ 
#ifndef NDEBUG
#    define _GLIBCXX_DEBUG
#endif 

#include <unistd.h>
#include <sys/time.h>

#include <memory>
#include <vector>

/* Used for all of Stu */
using namespace std; 

#include "dependency.hh"
#include "execution.hh"
#include "rule.hh"
#include "timestamp.hh"
#include "color.hh"

/* We use getopt(), which means that Stu does only support short
 * options, and not long options.  We avoid getopt_long() as it is a GNU
 * extension, and the short options are sufficient for now. 
 */
const char OPTIONS[]= "ac:C:Ef:F:ghj:JkKm:M:pqQsvVwxyz"; 

/* The output of the help (-h) option.  The following strings do not
 * contain tabs, but only space characters.  */   
const char HELP[]= 
	"Usage: stu [-f FILENAME] [OPTION]... [TARGET]...\n"           
	"By default, build the first target in the file 'main.stu'.\n" 
	"TARGETS may include the special characters '!?@[]'.\n"        
	"Options:\n"						       
	"  -a               Treat all trivial dependencies as non-trivial\n"          
	"  -c FILENAME      Pass a target filename without Stu syntax parsing\n"      
	"  -C EXPRESSIONS   Pass a target in full Stu syntax\n"		              
	"  -E               Explain error messages\n"                                 
	"  -f FILENAME      The input file to use instead of 'main.stu'\n"            
	"  -F RULES         Pass rules in Stu syntax\n"                               
	"  -g               Treat all optional dependencies as non-optional\n"        
	"  -h               Output help and exit\n"		                      
	"  -j K             Run K jobs in parallel\n"			              
	"  -J               Disable Stu syntax in arguments\n"                        
	"  -k               Keep on running after errors\n"		              
	"  -K               Don't delete target files on error or interruption\n"     
	"  -m ORDER         Order to run the targets:\n"			      
	"     dfs           (default) Depth-first order, like in Make\n"	      
	"     random        Random order\n"				              
	"  -M STRING        Pseudorandom run order, seeded by given string\n"         
	"  -p               Print the rules and exit\n"                               
	"  -q               Question mode; check whether targets are up to date\n"    
	"  -Q               Quiet mode; suppress special stdout messages\n"           
	"  -s               Silent mode; do not output commands\n"	              
	"  -v               Verbose mode; show execution information on stderr\n"     
	"  -V               Output version and exit\n"				      
	"  -w               Short output; show target filenames instead of commands\n"
	"  -x               Ouput each command statement individually\n"              
	"  -y               Disable color in output\n"                                
	"  -z               Output run-time statistics on stdout\n"                   
	"Report bugs to: kunegis@gmail.com\n" 
	"Stu home page: <https:/""/github.com/kunegis/stu>\n";

const char VERSION_INFO[]=
	"stu " STU_VERSION "\n"
	"Copyright (C) 2016 Jerome Kunegis\n"
	"License GPLv3+: GNU GPL version 3 or later <http:/""/gnu.org/licenses/gpl.html>\n"
	"This is free software: you are free to change and redistribute it.\n"
	"There is NO WARRANTY, to the extent permitted by law.\n";

/* Initialize buffers; called once from main() */ 
void init_buf(); 

/* Parse a string of dependencies and add them to the vector. Used for
 * the -C option.
 */
void add_dependencies_option_C(vector <shared_ptr <Dependency> > &dependencies,
			       const char *string_);

/* Add a single dependency from the given STRING, in syntax used for
 * optionless arguments  
 */
void add_dependencies_argument(vector <shared_ptr <Dependency> > &dependencies,
			       const char *string_);

/* Read in an input file and add the rules to the given rule set.  Used
 * for the -f option and the default input file.  If not yet non-null,
 * set RULE_FIRST to the first rule.  FILE_FD can be -1 or the FD or the
 * filename, if already opened.  If FILENAME is "-", use standard
 * input.   If FILENAME is "", use the default file ('main.stu'). 
 */
void read_file(string filename,
	       int file_fd,
	       Rule_Set &rule_set, 
	       shared_ptr <Rule> &rule_first,
	       Place &place_first); 

/* Read rules from the argument to the -F option */ 
void read_option_F(const char *s,
		   Rule_Set &rule_set, 
		   shared_ptr <Rule> &rule_first);

int main(int argc, char **argv, char **envp)
{
	/* Initialization */
	dollar_zero= argv[0]; 
	envp_global= (const char **)envp; 

	/* Refuse to run when $STU_STATUS is set */ 
	const char *const stu_status= getenv("STU_STATUS");
	if (stu_status != nullptr) {
		print_error(frmt("Refusing to run recursive Stu; unset %s$STU_STATUS%s to circumvent",
				 Color::word, Color::end));
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
		/* Place of first file when no rule is contained */ 
		Place place_first;

		bool had_option_c= false; /* Both lower and upper case */
		bool had_option_f= false; /* Both lower and upper case */

		for (int c; (c= getopt(argc, argv, OPTIONS)) != -1;) {
			switch (c) {

			case 'a': option_nontrivial= true;     break;
			case 'E': option_explain= true;        break;
			case 'g': option_nonoptional= true;    break;
			case 'h': fputs(HELP, stdout);         exit(0);
			case 'J': option_literal= true;        break;
			case 'k': option_keep_going= true;     break;
			case 'K': option_no_delete= true;      break;
			case 'p': option_print= true;          break; 
			case 'q': option_question= true;       break;
			case 'Q': option_quiet= true;          break; 
			case 's': output_mode= Output::SILENT; break;
			case 'v': option_verbose= true;        break;
			case 'w': output_mode= Output::SHORT;  break;
			case 'y': Color::set(false, false);    break;
			case 'x': option_individual= true;     break;
			case 'z': option_statistics= true;     break;

			case 'c': 
				{
					had_option_c= true; 
					if (*optarg == '\0') {
						print_error(frmt("Option %s-c%s must have non-empty argument",
								 Color::word, Color::end)); 
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
					add_dependencies_option_C(dependencies, optarg);
					break;
				}

			case 'f':
				if (*optarg == '\0') {
					print_error(frmt("Option %s-f%s must have non-empty argument",
							 Color::word, Color::end)); 
					exit(ERROR_FATAL);
				}

				for (string &filename:  filenames) {
					if (filename == optarg)  goto end;
				}
				had_option_f= true;
				filenames.push_back(optarg); 
				read_file(optarg, -1, Execution::rule_set, rule_first, place_first);
			end:
				break;

			case 'F':
				had_option_f= true;
				read_option_F(optarg, Execution::rule_set, rule_first);
				break;

			case 'j':
				errno= 0;
				char *endptr;
				Execution::jobs= strtol(optarg, &endptr, 0);
				if (errno != 0 || *endptr != '\0') {
					print_error(fmt("Invalid argument %s to option %s-j%s",
							name_format_word(optarg),
							Color::word, Color::end)); 
					exit(ERROR_FATAL); 
				}
				if (Execution::jobs < 1) {
					print_error(fmt("Argument %s to option %s-j%s must be positive",
							name_format_word(optarg),
							Color::word, Color::end)); 
						    
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
						print_error_system("gettimeofday");
						exit(ERROR_FATAL); 
					}
					buffer_generator.seed(tv.tv_sec + tv.tv_usec); 
				}
				else if (!strcmp(optarg, "dfs"))     /* Default */ ;
				else {
					print_error(fmt("Invalid argument %s for option %s-m%s; valid values are %s and %s", 
							name_format_word(optarg),
							Color::word, Color::end,
							name_format_word("random"),
							name_format_word("dfs"))); 
					exit(ERROR_FATAL); 
				}
				break;

			case 'M':
				order= Order::RANDOM;
				buffer_generator.seed(hash <string> ()(string(optarg)));
				break;

			case 'V': 
				fputs(VERSION_INFO, stdout); 
				printf("USE_MTIM = %u\n", USE_MTIM); 
				if (ferror(stdout)) {
					print_error_system("puts"); 
					exit(ERROR_FATAL);
				}
				exit(0);

			default:  
				/* Invalid option -- an error message was
				 * already printed by getopt() */   
				string text= name_format_word(frmt("%s -h", dollar_zero));
				fprintf(stderr, 
					"To get a list of all options, use %s\n", 
					text.c_str()); 
				exit(ERROR_FATAL); 
			}
		}

		order_vec= (order == Order::RANDOM); 

		/* Targets passed as-is on the command line, outside of options */ 
		for (int i= optind;  i < argc;  ++i) {

			/* With GNU getopt(), I is not the index that the argument had
			 * originally, because getopt() reorders its arguments.
			 * This is why we can't put I into the trace. */ 
			if (! option_literal) {
				add_dependencies_argument(dependencies, argv[i]); 
			} else {
				dependencies.push_back
					(make_shared <Direct_Dependency>
					 (0, Place_Param_Target
					  (Type::FILE, 
					   Place_Param_Name
					   (argv[i],
					    Place(Place::Type::ARGV)))));
			}
		}

		/* Use the default Stu file if -f/-F are not used */ 
		if (! had_option_f) {
			filenames.push_back(FILENAME_INPUT_DEFAULT); 
			int file_fd= open(FILENAME_INPUT_DEFAULT, O_RDONLY); 
			if (file_fd >= 0) {
				read_file("", file_fd, 
					  Execution::rule_set, rule_first, place_first); 
			} else {
				if (errno == ENOENT) { 
					/* The default file does not exist --
					 * fail if no target is given */  
					if (dependencies.empty() && ! had_option_c 
					    && ! option_print) {
						print_error(fmt("Expected a target or the default file %s",
								name_format_word(FILENAME_INPUT_DEFAULT))); 

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
		if (dependencies.empty() && ! had_option_c) {

			if (rule_first == nullptr) {
				if (! place_first.empty()) {
					place_first
						<< "expected a rule, because no target is given";
				} else {
					print_error("No rules and no targets given");
				}
				exit(ERROR_FATAL);
			}

			if (rule_first->is_parametrized()) {
				rule_first->place <<
					fmt("the first target %s must not be parametrized if no target is given",
					    rule_first->place_param_targets[0]->format_word());
				exit(ERROR_FATAL);
			}

			dependencies.push_back
				(make_shared <Direct_Dependency> (0, *(rule_first->place_param_targets[0])));  
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
	/* Set STDOUT to line buffered, so that the
	 * output of command lines always happens before the output of
	 * commands themselves */ 
	/* Note:  Setting the buffering like this is only possible if we
	 * have not written anything yet.  */  
	if (0 != setvbuf(stdout, nullptr, _IOLBF, 0)) {
		print_error_system("setvbuf"); 
		exit(ERROR_FATAL); 
	}
		
	{
		/* Set STDOUT to append mode; this is also done by GNU Make */ 
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

void add_dependencies_option_C(vector <shared_ptr <Dependency> > &dependencies,
			       const char *string_)
{
	vector <shared_ptr <Token> > tokens;
	Place place_end;
				
	Parse::parse_tokens_string(tokens, 
				   Parse::OPTION_C,
				   place_end, string_,
				   Place(Place::Type::OPTION_C));

	vector <shared_ptr <Dependency> > dependencies_option;
	Place_Param_Name input; /* remains empty */ 
	Place place_input; /* remains empty */ 

	Build::get_expression_list(dependencies_option, tokens, 
				   place_end, input, place_input);

	for (auto &j:  dependencies_option) {
		dependencies.push_back(j); 
	}
}

void add_dependencies_argument(vector <shared_ptr <Dependency> > &dependencies,
			       const char *string_)
{
	shared_ptr <Dependency> dep= Build::get_target_dependency(string_);
	dependencies.push_back(dep); 
}

void read_file(string filename,
	       int file_fd,
	       Rule_Set &rule_set, 
	       shared_ptr <Rule> &rule_first,
	       Place &place_first)
{
	assert(file_fd == -1 || file_fd > 1); 

	Place place_diagnostic= filename == "" 
		? Place()
		: Place(Place::Type::OPTION_f);

	if (filename == "")
		filename= FILENAME_INPUT_DEFAULT;

	string filename_passed= filename;
	if (filename_passed == "-")  filename_passed= ""; 

	/* Tokenize */ 
	vector <shared_ptr <Token> > tokens;
	Place place_end;
	Parse::parse_tokens_file(tokens, 
				 Parse::SOURCE,
				 place_end, filename_passed, 
				 place_diagnostic, 
				 file_fd); 

	/* Build rules */
	vector <shared_ptr <Rule> > rules;
	Build::get_rule_list(rules, tokens, place_end); 

	/* Add to set */
	rule_set.add(rules);

	/* Set the first one */
	if (rule_first == nullptr) {
		auto i= rules.begin();
		if (i != rules.end()) {
			rule_first= *i; 
		}
	}

	if (rules.empty() && place_first.empty()) {
		place_first= place_end;
	}
}

void read_option_F(const char *s,
		   Rule_Set &rule_set, 
		   shared_ptr <Rule> &rule_first)
{
	/* Tokenize */ 
	vector <shared_ptr <Token> > tokens;
	Place place_end;
	Parse::parse_tokens_string(tokens, 
				   Parse::OPTION_F,
				   place_end, s,
				   Place(Place::Type::OPTION_F));

	/* Build rules */
	vector <shared_ptr <Rule> > rules;
	Build::get_rule_list(rules, tokens, place_end);

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
