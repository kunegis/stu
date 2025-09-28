#ifndef EXPLAIN_HH
#define EXPLAIN_HH

/*
 * Explanation functions: they output an explanation of a feature of Stu on
 * standard error output.  This is used after certain non-trivial error
 * messages, and is enabled by the -E option.
 *
 * Many of these texts echo parts of the manpage.
 *
 * The line lengths in the printed output are set by hand and are approximate.
 */

void explain_clash();
void explain_dynamic_no_param();
void explain_file_without_command_with_dependencies();
void explain_file_without_command_without_dependencies();
void explain_no_target();
void explain_parameter_character();
void explain_cycle();
void explain_startup_time();
void explain_variable_equal();
void explain_version();
void explain_minimal_matching_rule();
void explain_separated_parameters();
void explain_flags();
void explain_quoted_characters();
void explain_missing_optional_copy_source();
void explain_parameter_syntax();

#endif /* ! EXPLAIN_HH */
