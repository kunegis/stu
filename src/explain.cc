#include "explain.hh"

void explain_clash()
{
	if (! option_E) return;
	fputs("Explanation: A dependency cannot be declared as persistent (with '-p') and\n"
		"optional (with '-o') at the same time, as that would mean that its\n"
		"command is never executed.\n",
		stderr);
}

void explain_cycle()
{
	if (! option_E) return;
	fputs("Explanation: A cycle in the dependency graph is an error.  Cycles are \n"
		"verified on the rule level, not on the target level.\n",
		stderr);
}

void explain_dynamic_no_param()
{
	if (! option_E) return;
	fputs("Explanation: Dynamic dependencies cannot contain parameters introduced\n"
		"with '$s'.\n",
		stderr);
}

void explain_file_without_command_with_dependencies()
{
	if (! option_E) return;
	fputs("Explanation: If a file rule has no command, this means that the file\n"
		"is always up-to-date whenever its dependencies are up to date.  In\n"
		"general, this means that the file is generated in conjunction with its\n"
		"dependencies.\n",
		stderr);
}

void explain_file_without_command_without_dependencies()
{
	if (! option_E) return;
	fputs("Explanation: A filename followed by a semicolon declares a file that is\n"
		"always present.\n",
		stderr);
}

void explain_flags()
{
	if (! option_E) return;
	fputs("Explanation: The valid flags are -p (persistent dependency), -o (optional\n"
		"dependency), and -t (trivial dependency).\n",
		stderr);
}

void explain_minimal_matching_rule()
{
	if (! option_E) return;
	fputs("Explanation: There must by a minimal matching rule for every target.  If\n"
		"multiple rules match a target, then Stu chooses the one that dominates\n"
		"all other ones.  A rule (x) is defined to dominate another rule (y) for\n"
		"a given name if every character in the name that is part of a matched\n"
		"parameter in rule (x) is also inside a matched parameter in rule (y),\n"
		"and at least one character of the name is part of a matched parameter\n"
		"in rule (y) but not in rule (x).  It is an error when there is no\n"
		"single matching rule that dominates all other matching rules.\n",
		stderr);
}

void explain_missing_optional_copy_source()
{
	if (! option_E) return;
	fputs("Explanation: In copy rules whose source file is declared as optional\n"
		"using the -o option, the source file may be missing only if the target\n"
		"file is present.  It is an error if both the source and the target files\n"
		"are missing.\n",
		stderr);
}

void explain_no_target()
{
	if (! option_E) return;
	fputs("Explanation: There must be either a target given as an argument to Stu\n"
		"invocation, one of the target-specifying options -c/-C/-p/-o/-n/-0,\n"
		"an -f option with a default target, a file 'main.stu' with a default\n"
		"target, or an -F option.\n",
		stderr);
}

void explain_parameter_character()
{
	if (! option_E) return;
	fputs("Explanation: Parameter names can only include alphanumeric characters\n"
		"and underscores.\n",
		stderr);
}

void explain_parameter_syntax()
{
	if (! option_E) return;
	fputs("Explanation: Parameters are introduced by the dollar sign, followed by the\n"
		"parameter name, optionally surrounded by braces, and optionally enclosed\n"
		"in double quotes.  Thus, valid ways to write a parameter are:\n"
		"\t$name    ${name}    \"...$name...\"    \"...${name}...\"\n",
		stderr);
}

void explain_quoted_characters()
{
	if (! option_E) return;
	fputs("Explanation: The following characters must always be quoted when appearing\n"
		"in names:\n"
		"\t#%\'\":;-$@<>={}()[]*\\&|!?,\n",
		stderr);
}

void explain_separated_parameters()
{
	if (! option_E) return;
	fputs("Explanation: When a target contains two contiguous parameters, it is\n"
		"impossible to match a target name to it as there are multiple ways to\n"
		"split the text matching the two parameters as a whole into two parts.\n"
		"Therefore, there must always be at least one character between any two\n"
		"parameters in a target name.  This restriction is not used with\n"
		"parameters in dependencies, as they are not matched.\n",
		stderr);
}
void explain_startup_time()
{
	if (! option_E) return;
	fputs("Explanation: If a created file has a timestamp older than the startup of\n"
		"Stu, a clock skew is likely.\n",
		stderr);
}

void explain_target_flags()
{
	if (! option_E) return;
	fputs("Explanation: Only the flags -p and -o can be used before targets of a\n"
		"rule.  In that case, the flags will always apply to that target.  Flags\n"
		"cannot be used before phony targets.  If a rule has multiple targets,\n"
		"each flag only applies to the target immediately following it.\n",
		stderr);
}

void explain_variable_equal()
{
	if (! option_E) return;
	fputs("Explanation: The name of an environment variable cannot contain the\n"
		"equal sign '=', because the operating system uses '=' as a delimiter\n"
		"when passing environment variables to child processes.\n"
		"The syntax $[VARIABLENAME = FILENAME] can be used to use a different\n"
		"name for the variable.\n",
		stderr);
}

void explain_version()
{
	if (! option_E) return;
	fputs("Explanation: Each Stu script can declare a version to which it is\n"
		"compatible using the syntax '% version X.Y' or '% version X.Y.Z'.  Stu\n"
		"will then fail at runtime if (a) 'X' does not equal the major version\n"
		"number of Stu, (b) 'Y' is larger than Stu's minor version number, or\n"
		"(c) 'X' equals Stu's minor version number and 'Z' is larger than Stu's\n"
		"patch level.  These rules correspond to the SemVer.org version\n"
		"semantics.\n",
		stderr);
}
