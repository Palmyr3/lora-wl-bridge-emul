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

void * thread_fn_0(void *args)
{
	struct stl_transmiter tsmt;
//	uint8_t	send_data[MAX_DATA_LEN] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0, 0, 0};

//	tsmt.id = ID_0;
//	tsmt.state = M_IDLE;

	printf("hello from thread_0!\n");

	sleep(1);

//	for (int i = 0; i < 100; i++) {
//		stk_m_send_single(&tsmt, send_data, 16);
//	}

	stk_m_comm_loop(&tsmt);

	printf("result logic hard err number: %u\n", wl_get_err_count());

	printf("bye from thread_0!\n");

	return 0;
}

void * thread_fn_1(void *args)
{
	struct stl_transmiter tsmt;

//	tsmt.id = ID_1;

	printf("hello from thread_1!\n");

	stk_s_comm_loop(&tsmt);

	printf("result err number: %u\n", wl_get_err_count());
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
