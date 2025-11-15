#ifndef CYCLE_HH
#define CYCLE_HH

#include "executor.hh"

class Cycle
{
public:
	static bool find(
		Executor *parent, Executor *child,
		shared_ptr <const Dep> dep_link);
	/* Find a cycle.  Assuming that the edge parent-->child will be added, find a
	 * directed cycle that would be created.  Start at PARENT and perform a
	 * depth-first search upwards in the hierarchy to find CHILD.  DEPENDENCY_LINK is
	 * the link that would be added between child and parent, and would create a
	 * cycle. */

private:
	static bool find(
		std::vector <Executor *> &path,
		Executor *child,
		shared_ptr <const Dep> dep_link);
	/* Helper function.  PATH is the currently explored path.  PATH[0] is the original
	 * PARENT; PATH[end] is the oldest grandparent found yet. */

	static void print(
		const std::vector <Executor *> &path,
		shared_ptr <const Dep> dep);
	/* Print the error message of a cycle on rule level.
	 * Given PATH = [a, b, c, d, ..., x], the found cycle is
	 * [x <- a <- b <- c <- d <- ... <- x], where A <- B denotes
	 * that A is a dependency of B.  For each edge in this cycle,
	 * output one line.  DEPENDENCY is the link (x <- a), which is not yet
	 * created in the executor objects.  All other link
	 * dependencies are read from the executor objects. */
};

#endif /* ! CYCLE_HH */
