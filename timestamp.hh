#ifndef TIMESTAMP_HH
#define TIMESTAMP_HH

/* Wrapper around time_t or struct timespec.  
 */

/* Not enabled because common file systems seem to give wrong results,
 * e.g., creating two files in a row may give the second file an older
 * timestamp. */
/* #define USE_MTIM 1 */

#ifdef USE_MTIM

/* This variant is not used */ 

/*
 * This needs to be linked with -ltr. 
 */

class Timestamp
{
private:

	/* When undefined, .tv_sec is equal to (time_t) -1, and .tv_nsec
	 * is uninitialized. */ 
	struct timespec t;

	Timestamp(time_t t_) 
	{
		assert(t_ == (time_t) -1); 
		t.tv_sec= t_;
	}

public:

	/* Uninitialized */ 
	Timestamp() { }

	Timestamp(struct stat *buf) 
		:  t(buf->st_mtim) 
	{  }
	
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
		return frmt("%ld.%ld", (long) t.tv_sec, (long) t.tv_nsec); 
	}

	static const Timestamp UNDEFINED;

	static Timestamp startup;

	static Timestamp now() {
		Timestamp ret;
		/* Note:  clock_gettime() needs to be linked with -lrt */ 
		int r= clock_gettime(CLOCK_REALTIME, & ret.t); 
		if (r < 0) {
			/* If this happens, it is a bug in Stu */
			assert(false);
			perror("clock_gettime(CLOCK_REALTIME, ...)");

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

#else /* old, 1-second-precision implementation */ 

class Timestamp
{
private:
	/* (time_t)-1 when undefined. 
	 */
	time_t t;

	Timestamp(time_t t_)
		:  t(t_)
	{ 
		assert(t != (time_t) -1); 
	}

	Timestamp(time_t t_, bool ignore) 
		:  t(t_)
	{ 
		(void) ignore; 
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

	/* The time at which Stu was started.  Used to check that no generated
	 * file must be older than that.  
	 */
	static Timestamp startup;

	static Timestamp now() {
		return Timestamp(time(nullptr)); 
	}
};

const Timestamp Timestamp::UNDEFINED((time_t) -1, true); 

Timestamp Timestamp::startup= Timestamp::now();

#endif /* variant */

#endif /* ! TIMESTAMP_HH */
