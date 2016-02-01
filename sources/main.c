#include <stdio.h>
#include <signal.h>
#include <math.h>
#include <pthread.h>

#include <candev/node.h>
#include <candev/ceac124.h>

#ifdef __XENO__
#define __REALTIME__
#endif // __XENO__

void printDevStatus(void *cookie, const CEAC124_DevStatus *st) {
	printf("device_status = {\n");
	printf(" dev_mode = 0x%x,\n", (int) st->dev_mode);
	printf(" label  = %d,\n", (int) st->label);
	printf(" pADC = %d,\n", (int) st->padc);
	printf(" file_ident = %d,\n", (int) st->file_ident);
	printf(" pDAC = %d,\n", (int) st->pdac);
	printf("}\n");
}

int main(int argc, char *argv[]) {
	int status;
	int done = 0;
	CAN_Node node;
	CEAC124 dev;
	
	//signal(SIGTERM, sig_handler);
	//signal(SIGINT, sig_handler);
	
	const char *ifname;
#ifdef __XENO__
	ifname = "rtcan0";
#else
	ifname = "can0";
#endif
	status = CAN_createNode(&node, ifname);
	if(status != 0)
		return 1;

	printf("Node created\n");
	
	CEAC124_setup(&dev, 0x1F, &node);
	
	dev.cb_cookie = (void *) &dev;
	dev.cb_dev_status = printDevStatus;
	
	/*
	if(CEAC124_getDevStatus(&dev) != 0) {
		return 2;
	}
	CEAC124_listen(&dev, &done);
	*/
	
#ifdef __REALTIME__
	struct sched_param param;
	param.sched_priority = 80;
	pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
#endif // __REALTIME__
	
	double t = 0.0;
	struct timespec lts;
	double freq = 200.0; // Hz
	long delay = 99999; // ns
	long min_delay = 90000;
	long period = 1e4;
	
	long ns;
	long tuned_delay = delay;
	double avg_ns = 0;
	long counter = 0;
	
	clock_gettime(CLOCK_MONOTONIC, &lts);
	while(!done) {
		struct timespec ts;
		
		CEAC124_DACWriteProp wp;
		wp.channel_number = 0;
		wp.use_code = 0;
		wp.voltage = 7.4*sin(t);
		if(CEAC124_dacWrite(&dev, &wp) != 0) {
			fprintf(stderr, "error dac write\n");
			return 2;
		}
		
		clock_gettime(CLOCK_MONOTONIC, &ts);
		ns = 1000000000*(ts.tv_sec - lts.tv_sec) + ts.tv_nsec - lts.tv_nsec;
		
		if(ns < tuned_delay) {
			ts.tv_sec = 0;
			ts.tv_nsec = tuned_delay - ns;
			nanosleep(&ts, NULL);
		}
		
		clock_gettime(CLOCK_MONOTONIC, &ts);
		ns = 1000000000*(ts.tv_sec - lts.tv_sec) + ts.tv_nsec - lts.tv_nsec;
		lts = ts;
		
		t += freq*(2*M_PI*1e-9*ns);
		
		tuned_delay = delay + tuned_delay - ns;
		if(tuned_delay < min_delay)
			tuned_delay = 2*delay;
		
		avg_ns += ns;
		++counter;
		if(!(counter % period)) {
			avg_ns /= period;
			printf("%lf\t%ld\n", avg_ns, tuned_delay);
			avg_ns = 0;
		}
	}
	
	CAN_destroyNode(&node);
	
	printf("exiting...\n");
	
	return 0;
}
