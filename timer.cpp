/*
 * timer.cpp
 *
 *  Created on: Mar 25, 2009
 *      Author: pushkar
 */

#include <stdlib.h>
#include "timer.h"

namespace Timer {

	double min_frequency = 500;

	int total_handles = 0;

	void (*_callback[10])(void);
	double _intervals[10];
	double * _last_time[10];
	int _handle_time_index[10];

	struct sigaction sact;
	struct itimerval value, ovalue, pvalue;
	struct timespec current_time;
	double _init_time, _cur_time;

	int sigcount = 0, timer_set = 0;

	double getCurrentTime() {
		double t;
		clock_gettime(CLOCK_REALTIME, &current_time);
		t = (double) current_time.tv_sec + (double) current_time.tv_nsec / 1000000000;
		return t;
	}

	void subscribeToTimer(void (*func)(void), double interval, double* last_time) {
		_callback[total_handles] = func;
		_handle_time_index[total_handles] = interval/min_frequency;
		_intervals[total_handles] = interval / 1000000;
		_last_time[total_handles] = last_time;
		total_handles++;
	}

	double _diff_time;

	void iHandler(int sig) {
		sigcount++;
		int i = 0;
		sig = 0;
#ifdef DEBUG
		_diff_time = Timer::getCurrentTime() - _cur_time;
#endif
		_cur_time = Timer::getCurrentTime();

		if(timer_set == 1 && sigcount > 1) {
			while(i < total_handles) {
				if(((_cur_time - *_last_time[i]) > _intervals[i])
						&& _handle_time_index[i] == sigcount % 10) {
					_callback[i]();
					getitimer(ITIMER_REAL, &pvalue);

					value.it_interval.tv_sec = 0;
					value.it_interval.tv_usec = 0;

					setitimer(ITIMER_REAL, &value, NULL);

				}
				i++;
			}
		}
	}

	int startTimer() {
		if(timer_set == 1)
			return 1;

		for(int i = 0; i < 10; i++) {
			_last_time[i] = (double*) malloc (sizeof(double));
		}

		_init_time = Timer::getCurrentTime();
		sigemptyset(&sact.sa_mask);
		sact.sa_flags = 0;
		sact.sa_handler = iHandler;
		sigaction( SIGALRM, &sact, NULL);

		getitimer(ITIMER_REAL, &pvalue);

		value.it_interval.tv_sec = 0;
		value.it_interval.tv_usec = min_frequency;
		value.it_value.tv_sec = 0;
		value.it_value.tv_usec = 10000;

		int result = setitimer(ITIMER_REAL, &value, &ovalue);

		if( ovalue.it_interval.tv_sec != pvalue.it_interval.tv_sec  ||
			ovalue.it_interval.tv_usec != pvalue.it_interval.tv_sec ||
			ovalue.it_value.tv_sec != pvalue.it_value.tv_sec ||
			ovalue.it_value.tv_usec != pvalue.it_value.tv_usec ) {
			fprintf(stderr, "Real time interval timer mismatch\n" );
			result = -1;
		}
		timer_set++;
		return result;
	}
}


