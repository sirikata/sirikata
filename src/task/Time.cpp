#include "Time.hpp"

#include <sys/time.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
// probably something like this function

Sirikata::Task::AbsTime Sirikata::task::AbsTime::now() {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	ULARGE_INTEGER uli;
	uli.LowPart = ft.dwLowDateTime;
	uli.HighPart = ft.dwHighDateTime;
	__int64 time64 = uli.QuadPart;

	return AbsTime(((double)time64)/10000000); // 100-nanosecond.
}

#else
Sirikata::Task::AbsTime Sirikata::Task::AbsTime::now() {
	struct timeval tv = {0, 0};
	gettimeofday(&tv, NULL);

	return AbsTime((double)tv.tv_sec + ((double)tv.tv_usec)/1000000);
}
#endif
