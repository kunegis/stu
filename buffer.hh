#ifndef BUFFER_HH
#define BUFFER_HH

/* 
 * A buffer is a container of normalized dependencies.  It is a queue or
 * vector, depending on the mode in which Stu is run, i.e., whether
 * targets are built in depth-first order (the default), or in random
 * order.  Which is used is determined by the global variable OPTION_VEC
 * defined in global.hh, which is set once before any Buffer object is
 * created.
 */

#include <queue>
#include <random>

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
{
private:
	/* Since we only ever use one of the two, we could use a
	 * union-like data structure, but we don't in this
	 * implementation  */   

	/* All contained dependencies are normalized */

	queue <shared_ptr <const Dep> > q;
	vector <shared_ptr <const Dep> > v;

public:

	size_t size() const {
		if (order_vec) 
			return v.size();
		else
			return q.size();
	}

	shared_ptr <const Dep> next() 
	/* Return the next element, removing it from the buffer at the
	 * same time  */
	{
		if (order_vec) {
			size_t s= v.size();
			size_t k= random_number(s);
			if (k + 1 < s) 
				swap(v[k], v[s - 1]); 
			shared_ptr <const Dep> ret= v[s - 1];
			v.resize(s - 1); 
			return ret; 
		} else {
			shared_ptr <const Dep> ret= q.front();
			q.pop(); 
			return ret; 
		}
	}

	void push(shared_ptr <const Dep> d)
	/* Add to the end of the queue (if sorted, otherwise, just
	 * add) */ 
	{
		assert(d->is_normalized()); 
		if (order_vec) {
			v.emplace_back(d); 
		} else {
			q.push(d); 
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
