#ifndef EXPLAIN_HH
#define EXPLAIN_HH

/* 
 * Explanation functions: they output an explanation of a feature of Stu
 * on standard error output.  This is used after certain non-trivial
 * error messages, and is enabled by the -E option.
 *
 * Many of these texts echo parts of the manpage. 
 *
 * The line lengths in the printed output are set by hand and are
 * approximate.   
 */

void explain_clash() 
{
	if (! option_explain)  return;
	fputs("Explanation: A dependency cannot be declared as persistent (with '-p') and\n"
	      "optional (with '-o') at the same time, as that would mean that its command\n"
	      "is never executed.\n",
	      stderr); 
}

void explain_file_without_command_with_dependencies()
{
	if (! option_explain)  return;
	fputs("Explanation: If a file rule has no command, this means that the file\n"
	      "is always up-to-date whenever its dependencies are up to date.  In general,\n"
	      "this means that the file is generated in conjunction with its dependencies.\n",
	      stderr); 
}

void explain_file_without_command_without_dependencies()
{
	if (! option_explain)  return;
	fputs("Explanation: A filename followed by a semicolon declares a file that is\n"
	      "always present.\n",
	      stderr); 
}

void explain_no_target()
{
	if (! option_explain)  return;
	fputs("Explanation: There must be either a target given as an argument to Stu\n"
	      "invocation, one of the target-specifying options -c/-C/-p/-o/-n/-0,\n"
	      "an -f option with a default target, a file 'main.stu' with a default\n"
	      "target, or an -F option.\n",
	      stderr); 
}

void explain_parameter_character()
{
	if (! option_explain)  return;
	fputs("Explanation: Parameter names can only include alphanumeric characters\n"
	      "and underscores.\n",
	      stderr); 
}

void explain_cycle()
{
	if (! option_explain)  return;
	fputs("Explanation: A cycle in the dependency graph is an error.  Cycles are \n"
	      "verified on the rule level, not on the target level.\n", 
	      stderr);
}

void explain_startup_time()
{
	if (! option_explain)  return;
	fputs("Explanation: If a created file has a timestamp older than the startup of Stu,\n"
	      "a clock skew is likely.\n",
	      stderr); 
}

void explain_variable_equal()
{
	if (! option_explain)  return;
	fputs("Explanation: The name of an environment variable cannot contain the\n"
	      "equal sign '=', because the operating system uses '=' as a delimiter\n"
	      "when passing environment variables to child processes.\n"
	      "The syntax $[VARIABLENAME = FILENAME] can be used to use a different\n"
	      "name for the variable.\n",
	      stderr); 
}

void explain_version()
{
	if (! option_explain)  return;
	fputs("Explanation: Each Stu script can declare a version to which it is compatible\n"
	      "using the syntax '% version X.Y' or '% version X.Y.Z'.  Stu will then fail at\n"
	      "runtime if (a) 'X' does not equal the major version number of Stu,\n"
	      "(b) 'Y' is larger than Stu's minor version number, or (c) 'X' equals Stu's\n"
	      "minor version number and 'Z' is larger than Stu's patch level.  These rules\n"
	      "correspond to the SemVer.org version semantics.\n",
	      stderr); 
}

void explain_minimal_matching_rule()
{
	if (! option_explain)  return;
	fputs("Explanation: There must by a minimal matching rule for every target.  If multiple\n"
	      "rules match a target, then Stu chooses the one that dominates all other ones.\n"
	      "A rule (x) is defined to dominate another rule (y) for a given name if every\n"
	      "character in the name that is part of a matched parameter in rule (x) is also\n"
	      "inside a matched parameter in rule (y), and at least one character of the name\n"
	      "is part of a matched parameter in rule (y) but not in rule (x).  It is an error when\n"
	      "there is no single matching rule that dominates all other matching rules.\n",
	      stderr); 
}

void explain_separated_parameters()
{
	if (! option_explain)  return;
	fputs("Explanation: When a target contains two contiguous parameters, it is\n"
	      "impossible to match a target name to it as there are multiple ways to split the\n"
	      "text matching the two parameters as a whole into two parts.  Therefore, there\n"
	      "must always be at least one character between any two parameters in a target name.\n"
	      "This restriction is not used with parameters in dependencies, as they are not matched.\n",
	      stderr); 
}

void explain_flags() 
{
	if (! option_explain)  return;
	fputs("Explanation: The valid flags are -p (persistent dependency), -o (optional dependency),\n"
	      "and -t (trivial dependency).\n",
	      stderr); 
}

void explain_quoted_characters()
{
	if (! option_explain)  return;
	fputs("Explanation: The following characters must always be quoted when appearing in names:\n"
	      "\t#%\'\":;-$@<>={}()[]*\\&|!?,\n",
	      stderr); 
}

void explain_missing_optional_copy_source()
{
	if (! option_explain)  return;
	 fputs("Explanation: In copy rules whose source file is declared as optional\n"
	       "using the -o option, the source file may be missing only if the target file\n"
	       "is present.  It is an error if both the source and the target files\n"
	       "are missing.\n",
	       stderr); 
}

void explain_parameter_syntax()
{
	if (! option_explain)  return;
	fputs("Explanation: Parameters are introduced by the dollar sign, followed by the\n"
	      "parameter name, optionally surrounded by braces, and optionally enclosed in\n"
	      "double quotes.  Thus, valid ways to write a parameter are:\n"
	      "\t$name    ${name}    \"...$name...\"    \"...${name}...\"\n", 
	      stderr); 
}

#endif /* ! EXPLAIN_HH */
