/*
 * timer.h
 *
 *  Created on: Mar 25, 2009
 *      Author: pushkar
 */

#ifndef TIMER_H_
#define TIMER_H_

#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

using namespace std;
namespace Timer {

	/// Get Current Time
	double getCurrentTime();

	/// Subscribe func to the process Timer. func is called when last_time misses to update itself after an interval interval
	void subscribeToTimer(void (*func)(void), double interval, double* last_time);

	/// Timer signal Handler
	void iHandler(int sig);

	/// Start timer here. Only one instance of this timer can be created for every process
	int startTimer();
}

#endif /* TIMER_H_ */
