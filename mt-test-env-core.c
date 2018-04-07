#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "env-core.h"
#include "stack-comm.h"

#define MAX_DATA_LEN		256
#define PACK_STD_LEN		19
#define SEND_SL_TIME		1

enum conn_id {
	ID_0	= 0,
	ID_1,
	ID_NUM
};

#define test_fail()	fprintf(stderr, "TST-FAIL: %d\n", __LINE__)

void * thread_fn_0(void *args)
{
	uint8_t	recv_data[MAX_DATA_LEN];
	uint8_t	send_data[MAX_DATA_LEN] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0, 0, 0};
	uint16_t crc;

	printf("hello from thread_0!\n");

	crc = stk_checksumm(send_data, PACK_STD_LEN);
	printf("pack crc16: %#x\n", crc);
	stk_set_pack_checksumm(send_data);

	wl_sendPacketTimeout(ID_0, send_data, 0);

	for (int i = 0; i < 100; i++) {
		while (!wl_availableData(ID_0, 0))
			usleep(1000);

		for (int i = 0; i < PACK_STD_LEN; i++)
			recv_data[i] = 0;

		wl_receivePacketTimeout(ID_0, 0, recv_data);
		if (!stk_test_pack_checksumm(recv_data)) {
			crc = stk_checksumm(recv_data, PACK_STD_LEN);
			printf("bad pack crc16: %#x\n", crc);
			stk_set_pack_checksumm(recv_data);
		}

		wl_sendPacketTimeout(ID_0, recv_data, 0);

//		for (int i = 0; i < PACK_STD_LEN; i++)
//			printf("%u, ", recv_data[i]);
//		printf(" (ID_%d)\n", ID_0);
	}

	for (int i = 0; i < PACK_STD_LEN; i++)
		printf("%u, ", recv_data[i]);
	printf("\n");

	for (int i = 0; i < PACK_STD_LEN; i++)
		if (recv_data[i] != send_data[i])
			test_fail();

	printf("result logic hard err number: %u\n", wl_get_err_count());

	printf("bye from thread_0!\n");

	return 0;
}

void * thread_fn_1(void *args)
{
	uint8_t	recv_data[MAX_DATA_LEN];
//	uint8_t	send_data[MAX_DATA_LEN] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0};

	printf("hello from thread_1!\n");

	while (true) {
		while (!wl_availableData(ID_1, 0))
			usleep(1000);
		wl_receivePacketTimeout(ID_1, 0, recv_data);

//		for (int i = 0; i < PACK_STD_LEN; i++)
//			printf("%u, ", recv_data[i]);
//		printf(" (ID_%d)\n", ID_1);

//		sleep(1);

		wl_sendPacketTimeout(ID_1, recv_data, 0);

		for (int i = 0; i < PACK_STD_LEN; i++)
			recv_data[i] = 0;
	}

	printf("result err number: %u\n", wl_get_err_count());

//	sleep(1);

	printf("bye from thread_1!\n");

	return 0;
}

int main(void)
{
	pthread_t thread_0, thread_1;
	int status;

	printf("start multiprocess valid test: %s\n", __func__);

	wl_emul_env_init();

	status = pthread_create(&thread_0, NULL, thread_fn_0, NULL);
	if (status != 0) {
		printf("main error: can't create thread, status = %d\n", status);
		return (-1);
	}
	status = pthread_create(&thread_1, NULL, thread_fn_1, NULL);
	if (status != 0) {
		printf("main error: can't create thread, status = %d\n", status);
		return (-1);
	}

	status = pthread_join(thread_0, NULL);
	if (status != 0) {
		printf("main error: can't join thread, status = %d\n", status);
		return (-2);
	}
	status = pthread_join(thread_1, NULL);
	if (status != 0) {
		printf("main error: can't join thread, status = %d\n", status);
		return (-2);
	}

	printf("joined, result err number: %u\n", wl_get_err_count());

	return 0;
}
