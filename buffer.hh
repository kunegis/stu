#ifndef BUFFER_HH
#define BUFFER_HH

/* 
 * A buffer is a container of Link objects.  It is a queue or vector,
 * depending on the mode in which Stu is run, i.e., whether targets are
 * built in depth-first order (the default), or in random order.  Which
 * is used is determined by the global variable OPTION_VEC defined in
 * global.hh, which is set once before any Buffer object is created. 
 */

#include <queue>
#include <random>

#include "link.hh"

static default_random_engine buffer_generator;

/* 
 * A random number in [0...n-1]. 
 */
size_t random_number(size_t n) 
{
	uniform_int_distribution <size_t> distribution(0, n - 1);

	return distribution(buffer_generator); 
}

class Buffer
/*
 * All contained Links contain non-compound dependencies.
 */
{
private:
	/* Since we only ever use one of the two, we could use a
	 * union-like data structure, but we don't in this
	 * implementation  */   

	// TODO investigate whether we need a link here, or whether it
	// could be replaced by a Dependency.   

	queue <Link> q;
	vector <Link> v;

public:

	size_t size() const {
		if (order_vec) 
			return v.size();
		else
			return q.size();
	}

	Link next() 
	/* Return the next element, removing it from the buffer at the
	 * same time  */
	{
		if (order_vec) {
			size_t s= v.size();
			size_t k= random_number(s);
			if (k + 1 < s) 
				swap(v[k], v[s - 1]); 
			Link ret= move(v[s - 1]);
			v.resize(s - 1); 
			return move(ret); 
		} else {
			Link ret= move(q.front());
			q.pop(); 
			return move(ret); 
		}
	}

	void push(Link &&link) 
	/* Add to the end of the queue (if sorted, otherwise, just
	   add) */ 
	{
		assert(dynamic_pointer_cast <Compound_Dependency> (link.dependency) == nullptr); 
		if (order_vec) {
			v.emplace_back(link); 
		} else {
			q.push(move(link)); 
		}
	}

	bool empty() const {
		if (order_vec) {
			return v.empty();
		} else {
			return q.empty(); 
		}
	}
};

#endif /* ! BUFFER_HH */
