/*
 * pTimer.h
 *
 *  Created on: Mar 25, 2009
 *      Author: pushkar
 */

#ifndef PTIMER_H_
#define PTIMER_H_

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

namespace PTimer {

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

	pthread_t timert;

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
		//printf("Subscribed with interval %lf\n", _intervals[total_handles]);
		total_handles++;
	}

	double _diff_time;

	void *pHandler(void*) {
		while(1) {
			usleep(450);
			sigcount++;
			int i = 0;
			_diff_time = PTimer::getCurrentTime() - _cur_time;
			_cur_time = PTimer::getCurrentTime();

			fprintf(stderr, "%d. %lf\n", sigcount, _diff_time);
			if(timer_set == 1 && sigcount > 1) {
				while(i < total_handles) {
					//fprintf(stderr, "\n%d. In handler D:%lf (%lf) > %lf", sigcount, _diff_time,(_cur_time - *_last_time[i]), _intervals[i]);
					if(((_cur_time - *_last_time[i]) > _intervals[i])
							&& _handle_time_index[i] == sigcount % 10) {
						_callback[i]();
						pthread_exit(NULL);

					}
					i++;
				}
			}
		}
		return NULL;
	}

	int startTimer() {
		if(timer_set == 1)
			return 1;

		for(int i = 0; i < 10; i++) {
			_last_time[i] = (double*) malloc (sizeof(double));
		}

		_init_time = PTimer::getCurrentTime();

		int rs = pthread_create(&timert, NULL, pHandler, NULL);
		fprintf(stderr, "Thread created\n");
		timer_set++;
		return rs;
	}
}


#endif /* PTIMER_H_ */
