#ifndef _SPEEDTRACKING_
#define _SPEEDTRACKING_

#include <pthread.h>
#include <string>
#include <sys/time.h>
#include <cfloat>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
using namespace std;

class SpeedTracking
{
	public:
		SpeedTracking();
		~SpeedTracking();
		
		bool addDownloadedSize(unsigned long long speed);
		bool downloadStart();
		bool downloadEnd();

		bool start();
		bool stop();
		void* threadProc();
		unsigned long long enGetBootTime(unsigned long long *time);
		static void *thread(void *speedTracking);

		void butterFilter(int speed, ofstream& logSpeed);
		string FormatBytes(double i, string unit, int decimal);

		static int _g_speed_track_sampling_rate;
		pthread_t speed_thread;
		pthread_cond_t speed_cond;
		pthread_mutex_t speed_mutex;
		bool _working;
		int _tracking_num;
	    unsigned long long  _size_downloaded;
    	unsigned long long  _start_time;
    	unsigned long long  _end_time;
		double _x[3], _y[3];
		double a[3], b[3];
};

#endif
