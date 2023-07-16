#include "buffer.hh"

std::default_random_engine buffer_generator;

size_t random_number(size_t n)
{
	std::uniform_int_distribution <size_t> distribution(0, n - 1);
	return distribution(buffer_generator);
}

void Buffer::push(shared_ptr <const Dep> d)
{
	assert(d->is_normalized());
	if (order_vec)
		v.emplace_back(d);
	else
		q.push(d);
}

shared_ptr <const Dep> Buffer::pop()
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
