#include "buffer.hh"

default_random_engine buffer_generator;

size_t random_number(size_t n)
{
	uniform_int_distribution <size_t> distribution(0, n - 1);
	return distribution(buffer_generator);
}
