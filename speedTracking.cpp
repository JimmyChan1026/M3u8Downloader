#include "speedTracking.h"
#include <iostream>

using namespace std;

SpeedTracking::SpeedTracking():
	speed_thread((pthread_t) 0), 
    _working(false),
    _tracking_num(0),
    _size_downloaded(0),
    _start_time(0),
    _end_time(0)
{
	_x[0] = _x[1] = _x[2] = DBL_MAX;
	_y[0] = _y[1] = _y[2] = DBL_MAX;
	a[0] = a[1] = a[2] = 123;
	b[0] = b[1] = b[2] = 123;
	start();
}

SpeedTracking::~SpeedTracking()
{
    stop();
}

bool SpeedTracking::addDownloadedSize(unsigned long long size)
{
	if(_working)
	{
		pthread_mutex_lock(&speed_mutex);
        _size_downloaded += size;
        // signal a sampling for a large size downloaded, usually it's a very high speed network
        if (_size_downloaded >= 1024*1024)  
//            cout << "mutex signal " << endl;
			pthread_cond_signal(&speed_cond);
		pthread_mutex_unlock(&speed_mutex);
    }
    return true;
}

bool SpeedTracking::downloadStart()
{
    if (_working)
    {
        //reset the time counter of tracking download speed
		pthread_mutex_lock(&speed_mutex);
        if (_tracking_num == 0){
			struct timeval st;
			gettimeofday(&st, NULL);
			_start_time = st.tv_sec * 1e6 + st.tv_usec;
			//cout << "downloadStart(): " << _start_time << endl;
		}
        ++_tracking_num;
		pthread_cond_signal(&speed_cond);
		pthread_mutex_unlock(&speed_mutex);
    }
    return true;
}

bool SpeedTracking::downloadEnd()
{
    bool ret = true;
    if (_working)
    {
        ret = false;
		pthread_mutex_lock(&speed_mutex);

        if (_tracking_num > 0)
        {
            --_tracking_num;
            if (_tracking_num == 0){
				struct timeval et;
				gettimeofday(&et, NULL);
				_end_time = et.tv_sec * 1e6 + et.tv_usec;
				//cout << "downloadEnd(): " << _end_time << endl;
			}
			pthread_cond_signal(&speed_cond);
            ret = true;
        }
		pthread_mutex_unlock(&speed_mutex);
    }
    return ret;
}

bool SpeedTracking::stop()
{
	cout << "SpeedTracking() stop!!!" << endl;
    if(speed_thread != (pthread_t) 0)
    {
		pthread_mutex_lock(&speed_mutex);
        _working = false;
		pthread_cond_signal(&speed_cond);
		pthread_mutex_unlock(&speed_mutex);
        pthread_join(speed_thread, NULL);
        cout << "pthread_join() success on download speed tracking" << endl;
        speed_thread = (pthread_t) 0;
    }
    return true;
}

bool SpeedTracking::start()
{
	cout << "SpeedTracking() start!!!" << endl;
	if(speed_thread == (pthread_t) 0)
	{
		_working = true;
    	int ret = pthread_create(&speed_thread, NULL, thread, this); //Calling speed thread 
    	if (ret != 0){ 
        	cout << "Failed to create pthread" << endl;
			_working = false;
        	return false; 
    	}
	}
	return true;
}

void* SpeedTracking::thread(void *speedTracking){
	return reinterpret_cast<SpeedTracking*>(speedTracking)->threadProc();
}

void *SpeedTracking::threadProc()
{
    cout << "Speed Thread In" << endl;
	//cout << endl;
    struct timespec ts;
    struct timeval tp, tp_2, tt;
    int ret = ETIMEDOUT;

    long looping_interval_us = 50000; // 50 ms
    int _g_speed_track_sampling_rate = 5;
    long sampling_interval_us = 1e6 / _g_speed_track_sampling_rate; 
    long time_since_last_sample_us = sampling_interval_us;
    long waited_time_us = 0;
	ofstream logForSpeed("Speed_log.txt");

	pthread_mutex_lock(&speed_mutex);
    do
    {
        time_since_last_sample_us += waited_time_us;
        if((time_since_last_sample_us >= sampling_interval_us) && (ret != ETIMEDOUT)) // 200,000 
        {
            // to calculate the download speed for every sampling_interval_us
            double time = 0;
            unsigned long long time_tmp;
			gettimeofday(&tt, NULL);
			time_tmp = tt.tv_sec * 1e6 + tt.tv_usec;
            if (_tracking_num == 0) // finish download one packet and not start a new one yet
            {
                if (_end_time < _start_time){
                    time = sampling_interval_us / 1000; // nothing is downloaded but the _start_time is updated already
                }else{ // download finished
                    time = _end_time - _start_time;
				}
            }
            else // still downloading
            {
                time = time_tmp - _start_time;
            }

            time /= 1e6;
			double speed = 0;

			if((_size_downloaded != 0) && (time != 0)){
            	speed = _size_downloaded / time;
			}
            _start_time = time_tmp;
            if ((speed > 0.f) && (time > 0.f))
            {
	//			cout << "Size: " << _size_downloaded << endl; 
                butterFilter(speed, logForSpeed); // butterFilter(int, double)
            }
            _size_downloaded = 0;
            time_since_last_sample_us=0;
        }

        gettimeofday(&tp, NULL);
        ts.tv_sec  = tp.tv_sec + ((tp.tv_usec + looping_interval_us) / 1e6);
        ts.tv_nsec = ((tp.tv_usec + looping_interval_us) % (long) 1e6) * 1000;
        ret = pthread_cond_timedwait(&speed_cond, &speed_mutex, &ts);
        if (_working == false)
        {
            cout << "leaving... ret = " << ret << endl;
            break;
        }
        else
        {
            gettimeofday(&tp_2, NULL);
            waited_time_us = (tp_2.tv_sec * 1e6 + tp_2.tv_usec) - (tp.tv_sec * 1e6 + tp.tv_usec);
        }
    }
    while (1);
	logForSpeed.close();
	pthread_mutex_unlock(&speed_mutex);
	pthread_exit(NULL);
}

void SpeedTracking::butterFilter(int speed, ofstream& logSpeed)
{
    double GAIN = 7.485478157e+01;
    double A[3] = { 1, 2, 1}; 
    double B[2] = {-0.7008967812, 1.6474599811};
	
	if(_x[0] == DBL_MAX)
    {   
        _x[0] = _x[1] = _x[2] = speed;
        _y[0] = _y[1] = _y[2] = speed;
    }   
    else
    {   
        _x[0] = _x[1]; _x[1] = _x[2];
        _x[2] = speed;
        _y[0] = _y[1]; _y[1] = _y[2];
        _y[2] = A[0] * _x[0] / GAIN
            + A[1] * _x[1] / GAIN
            + A[2] * _x[1] /GAIN
            + B[0] * _y[0]
            + B[1] * _y[1];
    }
/*
	if(a[0] == 123){
		a[0] = a[1] = a[2] = sp; 
        b[0] = b[1] = b[2] = sp;
	}else{
		a[0] = a[1]; a[1] = a[2];
        a[2] = sp; 
        b[0] = b[1]; b[1] = b[2];
        b[2] = A[0] * a[0] / GAIN
            + A[1] * a[1] / GAIN
            + A[2] * a[1] /GAIN
            + B[0] * b[0]
            + B[1] * b[1];
    }
*/
//    int filterResult = _y[2] > 0 ? _y[2] : 0;
	int filterResult = _y[2];

	cout << "Speed = " << speed << "(" << FormatBytes(speed * 8, "bps", 2).c_str() << "), FilterResult = " << filterResult << "(" << FormatBytes(filterResult * 8, "bps", 2).c_str() << ")" << endl;
	//logSpeed << "Speed = " << speed << "(" << FormatBytes(speed * 8, "bps", 2).c_str() << "), FilterResult = " << filterResult << "(" << FormatBytes(filterResult * 8, "bps", 2).c_str() << ")" << endl;
	//cout << "(Double) Speed = " << sp << "(" << FormatBytes(sp * 8, "bps", 2).c_str() << "), FilterResult = " << fi << "(" << FormatBytes(fi * 8, "bps", 2).c_str() << ")" << endl;
	//cout << endl;
}

string SpeedTracking::FormatBytes(double i, string unit, int decimal)
{
    stringstream ss; 
    if (i >= (1ll<<40))
        ss << setprecision(decimal) << std::fixed << (double) i / (1ll<<40) << "G" << unit;
    else if (i >= (1ll<<20))
        ss << setprecision(decimal) << std::fixed << (double) i / (1ll<<20) << "M" << unit;
    else if (i >= (1ll<<10))
        ss << setprecision(decimal) << std::fixed << (double) i / (1ll<<10) << "K" << unit;
    else
        ss << i << unit;
    return ss.str();
}

