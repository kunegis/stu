#ifndef BUFFER_HH
#define BUFFER_HH

/* A queue or vector of links */

#include <queue>

#include "link.hh"

/* A random number in [0...n-1]. 
 */
size_t random_number(size_t n) 
{
	assert(n < RAND_MAX); 
	assert(RAND_MAX <= SIZE_MAX); 

	/* rand() % n does not work, as it will give higher probability
	 * to lower numbers (given that RAND_MAX + 1 is not divisible by
	 * n). 
	 */

	size_t r;
	do
		r= rand();
	while (r >= RAND_MAX / n * n);
	return r % n; 
}

class Buffer
{
private:
	queue <Link> q;
	vector <Link> v;

public:

	size_t size() const {
		if (order_vec) 
			return v.size();
		else
			return q.size();
	}

	Link next() {
		if (order_vec) {
			size_t s= v.size();
			size_t k= random_number(s);
			if (k + 1 < s) 
				swap(v.at(k), v.at(s - 1)); 
			Link ret= move(v.at(s - 1));
			v.resize(s - 1); 
			return move(ret); 
		} else {
			Link ret= move(q.front());
			q.pop(); 
			return move(ret); 
		}
	}

	void push(Link &&link) {
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
