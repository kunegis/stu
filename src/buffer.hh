#ifndef BUFFER_HH
#define BUFFER_HH

/*
 * A buffer is a container of normalized dependencies.  It is a queue or vector,
 * depending on the mode in which Stu is run, i.e., whether targets are built in
 * depth-first order (the default), or in random order.  Which is used is
 * determined by the global variable OPTION_VEC defined in global.hh, which is
 * set once before any Buffer object is created.
 */

#include <memory>
#include <queue>
#include <random>

#include "dep.hh"

extern std::default_random_engine buffer_generator;

size_t random_number(size_t n);
/* Random number in [0...n-1] */

class Buffer
{
private:
	/* Since we only ever use one of the two, we could use a union-like data
	 * structure, but we don't. */

	/* All contained dependencies are normalized */
	std::queue <shared_ptr <const Dep> > q;
	std::vector <shared_ptr <const Dep> > v;

public:
	size_t size() const {
		if (order_vec)
			return v.size();
		else
			return q.size();
	}

	bool empty() const {
		if (order_vec)
			return v.empty();
		else
			return q.empty();
	}

	void push(shared_ptr <const Dep> d);
	shared_ptr <const Dep> pop();
};

#endif /* ! BUFFER_HH */
