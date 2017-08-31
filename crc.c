#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
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

uint32_t Michael_crc32c_aarch64(unsigned char const *buffer, uint32_t len);
uint32_t fio_crc32c_arm64(unsigned char const *data, uint32_t length);

int test_one(uint32_t size_of_buffes, uint32_t num_of_buffes, uint32_t num_of_loops,
	     uint8_t *buf, uint32_t (*fn)(unsigned char const *,  uint32_t))
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
               	 crc += fn(&buf[size_of_buffes * j], size_of_buffes);

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
	test_one(size_of_buffes, num_of_buffes, num_of_loops, data_buf, fio_crc32c_arm64);
     
     return 0;

}

#define CRC32CX(crc, value) __asm__("crc32cx %w[c], %w[c], %x[v]":[c]"+r"(crc):[v]"r"(value))
#define CRC32CW(crc, value) __asm__("crc32cw %w[c], %w[c], %w[v]":[c]"+r"(crc):[v]"r"(value))
#define CRC32CH(crc, value) __asm__("crc32ch %w[c], %w[c], %w[v]":[c]"+r"(crc):[v]"r"(value))
#define CRC32CB(crc, value) __asm__("crc32cb %w[c], %w[c], %w[v]":[c]"+r"(crc):[v]"r"(value))

uint32_t __attribute__ ((noinline)) Michael_crc32c_aarch64(unsigned char const *buffer, uint32_t len)
{
        int64_t length = len;
        uint32_t crc = CRC;
        while ((length -= sizeof(uint64_t)) >= 0) {
                        CRC32CX(crc, *(uint64_t *)buffer);
                        buffer += sizeof(uint64_t);

                }

                /* The following is more efficient than the straight loop */
                if (length & sizeof(uint32_t)) {
                        CRC32CW(crc, *(uint32_t *)buffer);
                        buffer += sizeof(uint32_t);
                }
                if (length & sizeof(uint16_t)) {
                        CRC32CH(crc, *(uint16_t *)buffer);
                        buffer += sizeof(uint16_t);
                }
                if (length & sizeof(uint8_t))
                        CRC32CB(crc, *buffer);

        return crc;
}



#define CRC32C3X8(ITR) \
	crc1 = __crc32cd(crc1, *((const uint64_t *)data + 42*1 + (ITR)));\
	crc2 = __crc32cd(crc2, *((const uint64_t *)data + 42*2 + (ITR)));\
	crc0 = __crc32cd(crc0, *((const uint64_t *)data + 42*0 + (ITR)));

#define CRC32C7X3X8(ITR) do {\
	CRC32C3X8((ITR)*7+0) \
	CRC32C3X8((ITR)*7+1) \
	CRC32C3X8((ITR)*7+2) \
	CRC32C3X8((ITR)*7+3) \
	CRC32C3X8((ITR)*7+4) \
	CRC32C3X8((ITR)*7+5) \
	CRC32C3X8((ITR)*7+6) \
	} while(0)

#ifndef HWCAP_CRC32
#define HWCAP_CRC32             (1 << 7)
#endif /* HWCAP_CRC32 */

#include <sys/auxv.h>
#include <arm_acle.h>
#include <arm_neon.h>


/*
 * Function to calculate reflected crc with PMULL Instruction
 * crc done "by 3" for fixed input block size of 1024 bytes
 */
uint32_t fio_crc32c_arm64(unsigned char const *data, uint32_t length)
{
	int64_t len = length;
	uint32_t crc = CRC;
	uint32_t crc0, crc1, crc2;
	/* Load two consts: K1 and K2 */
	const poly64_t k1 = 0xe417f38a, k2 = 0x8f158014;
	uint64_t t0, t1;

	while ((len -= 1024) >= 0) {
		/* Do first 8 bytes here for better pipelining */
		crc0 = __crc32cd(crc, *(const uint64_t *)data);
		crc1 = 0;
		crc2 = 0;
		data += sizeof(uint64_t);

		/* Process block inline
		   Process crc0 last to avoid dependency with above */
		CRC32C7X3X8(0);
		CRC32C7X3X8(1);
		CRC32C7X3X8(2);
		CRC32C7X3X8(3);
		CRC32C7X3X8(4);
		CRC32C7X3X8(5);

		data += 42*3*sizeof(uint64_t);

		/* Merge crc0 and crc1 into crc2
		   crc1 multiply by K2
		   crc0 multiply by K1 */

		t1 = (uint64_t)vmull_p64(crc1, k2);
		t0 = (uint64_t)vmull_p64(crc0, k1);
		crc = __crc32cd(crc2, *(const uint64_t *)data);
		crc1 = __crc32cd(0, t1);
		crc ^= crc1;
		crc0 = __crc32cd(0, t0);
		crc ^= crc0;

		data += sizeof(uint64_t);
	}

	if (!(len += 1024))
		return crc;

	while ((len -= sizeof(uint64_t)) >= 0) {
                crc = __crc32cd(crc, *(const uint64_t *)data);
                data += sizeof(uint64_t);
        }

        /* The following is more efficient than the straight loop */
        if (len & sizeof(uint32_t)) {
                crc = __crc32cw(crc, *(const uint32_t *)data);
                data += sizeof(uint32_t);
        }
        if (len & sizeof(uint16_t)) {
                crc = __crc32ch(crc, *(const uint16_t *)data);
                data += sizeof(uint16_t);
        }
        if (len & sizeof(uint8_t)) {
                crc = __crc32cb(crc, *(const uint8_t *)data);
        }

	return crc;
}
