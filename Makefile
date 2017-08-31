all:
	gcc fio_crc32c.c Michael_crc32c.c ceph_crc32c.c  test.c  -march=armv8-a+crc+crypto -O3 -DHAVE_ARMV8_CRC_CRYPTO_INTRINSICS -DHAVE_ARMV8_CRYPTO -o crc_test
