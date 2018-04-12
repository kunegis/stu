#ifndef TIMESTAMP_HH
#define TIMESTAMP_HH

/* 
 * Wrapper around time_t or struct timespec.  There are two variants:
 *   - default:  1-second precision.
 *   - mtim:     nanosecond precision (in principle).  Works only on Linux; see below. 
 */

#ifndef USE_MTIM
#   if HAVE_CLOCK_REALTIME_COARSE
#      define USE_MTIM 1
#   else
#      define USE_MTIM 0
#   endif
#endif

#if USE_MTIM

/* 
 * This variant works only on Linux.  Using CLOCK_REALTIME does not work, as:
 *  (1) Two files created in a row have timestamps in the wrong order 
 *  (2) Files create during Stu runtime have a timestamp before the Stu
 *      startup timestamps. 
 * Both errors are on the order of a few milliseconds.  Tested on a
 * system on which the clock resolution as reported by clock_getres() is
 * one nanosecond. 
 */

/* Using CLOCK_REALTIME_COARSE (Linux only) works. */ 

class Timestamp
/* Note also that this implementation has such high precision, that bugs
 * in Stu scripts may emerge which otherwise would have been hidden.  */
{
private:

	struct timespec t;
	/* When undefined, .tv_sec is equal to (time_t) -1, and .tv_nsec
	 * is uninitialized. */ 

	Timestamp(time_t t_) 
	{
		assert(t_ == (time_t) -1); 
		t.tv_sec= t_;
	}

public:

	/* Uninitialized */ 
	Timestamp() { }

	Timestamp(struct stat *buf) 
	{  
		t.tv_sec= buf->st_mtim.tv_sec;
		t.tv_nsec= buf->st_mtim.tv_nsec;
	}
	
	bool defined() const {
		return t.tv_sec != (time_t) -1; 
	}

	bool operator < (const Timestamp &that) const {
		assert(this->defined());
		assert(that.defined());

		return this->t.tv_sec < that.t.tv_sec ||
			(this->t.tv_sec == that.t.tv_sec && this->t.tv_nsec < that.t.tv_nsec); 
	}

	string format() const {
		assert(defined()); 
		return frmt("%lld.%09ld", (long long) t.tv_sec, (long) t.tv_nsec); 
	}

	static const Timestamp UNDEFINED;

	static Timestamp startup;

	static Timestamp now() {
		Timestamp ret;
		int r= clock_gettime(CLOCK_REALTIME_COARSE, & ret.t); 
		if (r != 0) {
			/* If this happens, it is a bug in Stu */
			assert(false);
			print_error_system("clock_gettime(CLOCK_REALTIME_COARSE, ...)");

			/* Do the next best thing:  use time(2).
			 * This may lead to clock skew, as the
			 * nanoseconds are not set correctly */
			ret.t.tv_sec= time(nullptr);
			ret.t.tv_nsec= 0; 
		}
		return ret; 
	}
};

const Timestamp Timestamp::UNDEFINED((time_t) -1); 

Timestamp Timestamp::startup= Timestamp::now();

#else /* Default, 1-second-precision implementation */ 

class Timestamp
{
private:
	time_t t;
	/* (time_t)-1 when undefined */

	Timestamp(time_t t_)
		:  t(t_)
	{ 
		assert(t != (time_t) -1); 
	}

	Timestamp(time_t t_, bool ignore_) 
		:  t(t_)
	{ 
		(void) ignore_; 
	}

public:

	/* Uninitialized */ 
	Timestamp() 
	{ }

	Timestamp(const struct stat *buf) 
	{
		t= buf->st_mtime; 
	}

	bool defined() const {
		return t != (time_t) -1; 
	}

	bool operator < (const Timestamp &that) const {
		assert(this->defined());
		assert(that.defined());
		return this->t < that.t; 
	}

	string format() const {
		return frmt("%ld", (long) t); 
	}

	static const Timestamp UNDEFINED;

	static Timestamp startup;
	/* The time at which Stu was started.  Used to check that no generated
	 * file must be older than that.  */

	static Timestamp now() {
		return Timestamp(time(nullptr)); 
	}
};

const Timestamp Timestamp::UNDEFINED((time_t) -1, true); 

Timestamp Timestamp::startup= Timestamp::now();

#endif /* variant */

#endif /* ! TIMESTAMP_HH */
