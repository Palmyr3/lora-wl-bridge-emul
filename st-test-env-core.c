#include <stdio.h>
#include <unistd.h>

#include "env-core.h"

enum conn_id {
	ID_0	= 0,
	ID_1,
	ID_NUM
};

#define MAX_DATA_LEN		256
#define PACK_STD_LEN		19
#define SEND_SL_TIME		1

#define test_fail()	fprintf(stderr, "TST-FAIL: %d\n", __LINE__)

int main(void)
{
	uint32_t err_prev = 0, err_curr = 0;
	uint8_t	recv_data[MAX_DATA_LEN];
	uint8_t	send_data[MAX_DATA_LEN] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0, 0, 0};

	wl_emul_env_init();

//// valid test
	printf("start valid test: %s\n", __func__);

	// send from 0 to 1
	wl_sendPacketTimeout(ID_0, send_data, 0);
	if (wl_availableData(ID_1, 0))
		test_fail();

	sleep(SEND_SL_TIME);

	if (!wl_availableData(ID_1, 0))
		test_fail();

	for (int i = 0; i < PACK_STD_LEN; i++)
		recv_data[i] = 0;
	wl_receivePacketTimeout(ID_1, 0, recv_data);
	for (int i = 0; i < PACK_STD_LEN; i++)
		if (recv_data[i] != send_data[i])
			test_fail();

	// send from 1 to 0
	wl_sendPacketTimeout(ID_1, send_data, 0);
	if (wl_availableData(ID_0, 0))
		test_fail();

	sleep(SEND_SL_TIME);
	if (!wl_availableData(ID_0, 0))
		test_fail();

	for (int i = 0; i < PACK_STD_LEN; i++)
		recv_data[i] = 0;
	wl_receivePacketTimeout(ID_0, 0, recv_data);
	for (int i = 0; i < PACK_STD_LEN; i++)
		if (recv_data[i] != send_data[i])
			test_fail();

	if (wl_get_err_count() > 0)
		test_fail();


	wl_emul_env_destroy();

//// invalid test
	printf("start invalid test: %s\n", __func__);


	// send before previuous send is ready
	wl_emul_env_init();
	err_prev = wl_get_err_count();
	wl_sendPacketTimeout(ID_0, send_data, 0);
	wl_sendPacketTimeout(ID_0, send_data, 0);
	err_curr = wl_get_err_count();
	if (err_curr <= err_prev)
		test_fail();
	wl_emul_env_destroy();

	// both transceivers send simultaneously
	wl_emul_env_init();
	err_prev = wl_get_err_count();
	wl_sendPacketTimeout(ID_0, send_data, 0);
	wl_sendPacketTimeout(ID_1, send_data, 0);
	err_curr = wl_get_err_count();
	if (err_curr <= err_prev)
		test_fail();
	wl_emul_env_destroy();

	// send again before previuous data was read
	wl_emul_env_init();
	err_prev = wl_get_err_count();
	wl_sendPacketTimeout(ID_0, send_data, 0);
	sleep(SEND_SL_TIME);
	wl_sendPacketTimeout(ID_0, send_data, 0);
	sleep(SEND_SL_TIME);
	wl_sendPacketTimeout(ID_0, send_data, 0);
	err_curr = wl_get_err_count();
	if (err_curr <= err_prev)
		test_fail();
	wl_emul_env_destroy();

	return 0;
}
