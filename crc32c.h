#ifndef _______CRC___H____
#define _______CRC___H____
uint32_t Michael_crc32c_aarch64(uint32_t, unsigned char const *buffer, uint32_t len);
uint32_t fio_crc32c_aarch64(uint32_t, unsigned char const *data, uint32_t length);
uint32_t ceph_crc32c_aarch64(uint32_t, unsigned char const *data, uint32_t length);


#endif
