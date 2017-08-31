#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include "crc32c.h"

#define CRC 0xdebb20e3

// gcc crc.c  -march=armv8-a+crc+crypto -O3 -o crc_test
clockid_t clockid;

int get_clockid(void)
{
	 clockid_t types[] = { CLOCK_REALTIME, CLOCK_MONOTONIC,
		 CLOCK_PROCESS_CPUTIME_ID, CLOCK_THREAD_CPUTIME_ID};
	 int i = 0;
	 struct timespec spec = {0, 0};
	 uint64_t min_nsec = (uint64_t)-1;
	 uint64_t nsec;
	 for (i = 0; i < (sizeof(types) / sizeof(types[0])); i++ ){
	 	if (clock_getres(types[i], &spec)){
			continue;
		}
		nsec = 1000000000ULL * spec.tv_sec + spec.tv_nsec;
		if (min_nsec > nsec){
			min_nsec = nsec;
			clockid = types[i];
		}
	 }

	 if (min_nsec == (uint64_t)-1)
		 return -1;
	 
	 return 0;
}

int get_time_now(struct timespec *tp)
{
	return clock_gettime(clockid, tp);
}

uint64_t ntime_since_now(struct timespec *tp)
{
	int64_t sec, nsec;
	struct timespec now;
	uint64_t ret;
	if (get_time_now(&now))
		return 0;
	sec = now.tv_sec - tp->tv_sec;
	nsec = now.tv_nsec - tp->tv_nsec;
	if (sec < 0 || (sec == 0 && nsec < 0))
		return 0;
	return 1000000000ULL * sec + nsec;

}

int test_one(uint32_t size_of_buffes, uint32_t num_of_buffes, uint32_t num_of_loops,
	     uint8_t *buf, uint32_t (*fn)(uint32_t, unsigned char const *,  uint32_t))
{

     struct timespec new_time;
     uint32_t i, j;
     uint32_t crc = 0;
     uint64_t nsec = 0;
     uint64_t sum_bytes = (uint64_t)size_of_buffes * num_of_buffes * num_of_loops;

     if (get_time_now(&new_time)){
     	printf("Cann't get time\n");
	return 0;
     }

     for(i = 0; i < num_of_loops; i++)
	for(j = 0; j < num_of_buffes; j++)
               	 crc += fn(CRC, &buf[size_of_buffes * j], size_of_buffes);

     if ((nsec = ntime_since_now(&new_time)) == 0){
     	printf("Cann't calc time\n");
	return 0;
     }

     
     printf("processed %f MB of data in %f msec, at rate %f MB per second,CRC = %x\n",
        (double)sum_bytes / 0x100000, (double)nsec / 1000000, (double)sum_bytes / ((double)nsec / 1000000000) / 0x100000 ,crc);
     
     return 0;

}

int main(int argc, void **argv)
{
     uint8_t *data_buf;
     uint32_t num_of_loops;
     uint32_t num_of_buffes;
     uint32_t size_of_buffes;

     if (argc < 4){
     	printf("./crc_test buffer_size buffer_num loop_num\n");
	return 0;
     }

     size_of_buffes = (uint32_t)atol(argv[1]); 
     num_of_buffes = (uint32_t)atol(argv[2]);
     num_of_loops = (uint32_t)atol(argv[3]); 

     if (get_clockid()){
     	printf("Cann't get clockid\n");
	return 0;
     }

     data_buf = malloc(size_of_buffes * num_of_buffes);
     if (!data_buf){
     	printf("Cann't malloc %dB memory \n", size_of_buffes);
	return 0;	
     }

     printf(" test with size_of_buffes = %u num_of_buffes = %u num_of_loops =%u\n", size_of_buffes, num_of_buffes, num_of_loops);
	test_one(size_of_buffes, num_of_buffes, num_of_loops, data_buf, Michael_crc32c_aarch64);
	test_one(size_of_buffes, num_of_buffes, num_of_loops, data_buf, fio_crc32c_aarch64);
	test_one(size_of_buffes, num_of_buffes, num_of_loops, data_buf, ceph_crc32c_aarch64);
     
     return 0;

}
