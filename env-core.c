#include <sys/time.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "env-core.h"

#define MAX_WL_DATA_LEN		256

/* configuration options : start */
//      #define CONFIG_WL_LOSE_PACKETS			false
//      #define CONFIG_WL_LOSE_PACKETS_SCORE		10
//      #define CONFIG_WL_ERR_DATA			false
//      #define CONFIG_WL_ERR_DATA_FIRST_SCORE		2	/* first brocken bit in the pack */
//      #define CONFIG_WL_ERR_DATA_MULTIP_SCORE		80	/* if one bit is brocken - probably others are brocken too */
//      #define CONFIG_WL_ADD_PACKETS			false	/* NOTE: not supported */
//      #define CONFIG_WL_ADD_PACKETS_SCORE		2
//      #define CONFIG_WL_RANDOMIZE_TOA			false	/* time on air */

#define CONFIG_WL_SLEEP_GRANULARITY		10 /* milliseconds, sleep in all timeout waiting functions */
#define CONFIG_WL_CHK_MAX_LORA_PACL_LEN		19
/* configuration options : end */

#define __WL_TIME_ON_AIR_NS			((CONFIG_WL_STD_TIME_ON_AIR_MS % 1000) * 1000000ULL)
#define __WL_TIME_ON_AIR_US			((CONFIG_WL_STD_TIME_ON_AIR_MS % 1000) * 1000ULL)
#define __WL_TIME_ON_AIR_S			(CONFIG_WL_STD_TIME_ON_AIR_MS / 1000)


#define pr_fmt(fmt) "rff: " fmt
#define pr_efmt(fmt) "ERR: " fmt
#define pr_wfmt(fmt) "WARN: " fmt
#define pr_trace(fmt, ...)	fprintf(stdout, pr_fmt(fmt), ##__VA_ARGS__)
#define pr_info(fmt, ...)	fprintf(stdout, pr_fmt(fmt), ##__VA_ARGS__)
#define pr_err(fmt, ...)	fprintf(stderr, pr_efmt(fmt), ##__VA_ARGS__)
#define pr_warn(fmt, ...)	fprintf(stderr, pr_wfmt(fmt), ##__VA_ARGS__)
//#define rff_warn(fmt, ...) ({if (g_args.verbose) rff_err(fmt, ##__VA_ARGS__);})

enum conn_id {
	ID_0	= 0,
	ID_1,
	ID_NUM
};

/* single buffer: input */
struct wl_emul_buff_i {
	struct	timeval recv_time;
	uint8_t	recv_data[MAX_WL_DATA_LEN];
	bool	is_recieved;
};

/* single buffer: output */
struct wl_emul_buff_o {
	struct	timeval send_time;
	uint8_t	send_data[MAX_WL_DATA_LEN];
	bool	is_send;
};

/* single client: one of LORA transmitter */
struct wl_emul_side_client {
	struct wl_emul_buff_i input_buff;
	struct wl_emul_buff_o output_buff;
};

/* all devices with buffers protected by lock */
struct wl_emul_global {
	struct wl_emul_side_client dev[ID_NUM];
	/* thread-safe seed for random */
	unsigned int seedp[ID_NUM];
	/* global wl emulator lock */
	pthread_spinlock_t lock;
	/* error statistic */
	uint32_t err;
};

/* per-emul globals */
static struct wl_emul_global devs = {};

int wl_emul_env_init(void)
{
	pthread_spin_init(&devs.lock, 0);

	pthread_spin_lock(&devs.lock);

	devs.err = 0;

	devs.dev[ID_0].output_buff.is_send = true;
	devs.dev[ID_1].output_buff.is_send = true;

	devs.dev[ID_0].input_buff.is_recieved = false;
	devs.dev[ID_1].input_buff.is_recieved = false;

	devs.seedp[ID_0] = 0;
	devs.seedp[ID_1] = 1;

	pthread_spin_unlock(&devs.lock);

	return 0;
}

static void wl_pack_copy(uint16_t len, uint8_t *dst, uint8_t *src)
{
	for (uint16_t i = 0; i < len; i++)
		dst[i] = src[i];
}

static uint32_t rand_pack_bit(unsigned int *seedp)
{
	/* Return bit from 0 to CONFIG_WL_PACK_CONST_LEN * 8 */
	return ((rand_r(seedp) / (RAND_MAX / (CONFIG_WL_PACK_CONST_LEN * 8))));
}

/* Called in locked context */
static uint32_t wl_rand_pack_bit(uint8_t id_src)
{
	return rand_pack_bit(&devs.seedp[id_src]);
}

static bool b_rand(unsigned int *seedp, int true_score)
{
	return ((rand_r(seedp) / (RAND_MAX / 100)) < true_score);
}

/* Called in locked context */
static bool wl_b_rand(uint8_t id_src, int true_score)
{
	return b_rand(&devs.seedp[id_src], true_score);
}

/* Write to pack CONFIG_WL_PACK_CONST_LEN bytes of random data
 * Called in locked context */
static __attribute__((unused)) void wl_gen_random_pack(uint8_t id_src, uint8_t *pack)
{
	for (uint16_t i = 0; i < CONFIG_WL_PACK_CONST_LEN; i++)
		pack[i] = (rand_r(&devs.seedp[id_src]) / (RAND_MAX / 255));
}

uint32_t wl_get_err_count(void)
{
	uint32_t ret;

	pthread_spin_lock(&devs.lock);
	ret = devs.err;
	pthread_spin_unlock(&devs.lock);

	return ret;
}

void wl_emul_env_destroy(void)
{
	pthread_spin_destroy(&devs.lock);
}

/* Called in locked context */
static bool wl_is_packet_lost(uint8_t id_src)
{
	if (CONFIG_WL_LOSE_PACKETS)
		return wl_b_rand(id_src, CONFIG_WL_LOSE_PACKETS_SCORE);

	return false;
}

static void wl_pack_bit_flip(uint32_t bit, uint8_t *pack)
{
	uint8_t byte_n, bit_n;
	uint8_t tmp;

	byte_n = bit / 8;
	bit_n = bit % 8;

	tmp = pack[byte_n];
	pack[byte_n] ^= (1 << bit_n);

	pr_info("Flip bit %u: pack[%u].%u; pack was: %#x is %#x\n", bit, byte_n,
		bit_n, tmp, pack[byte_n]);
}

/* Called in locked context */
static uint32_t wl_pack_break(uint8_t id_src, uint8_t *pack)
{
	uint32_t ret = 0;
	uint32_t bit_to_break, bit_to_break_prev;

	if (!CONFIG_WL_ERR_DATA || !wl_b_rand(id_src, CONFIG_WL_ERR_DATA_FIRST_SCORE))
		return ret;

	bit_to_break_prev = wl_rand_pack_bit(id_src);
	wl_pack_bit_flip(bit_to_break_prev, pack);
	ret++;

	/* TODO: FIXME: we can flip same bit twice - so no error will be generated
	 * instead of 2 errors */
	while (wl_b_rand(id_src, CONFIG_WL_ERR_DATA_MULTIP_SCORE)) {
		/* Try to avoid flipping the same bit twice
		 * NOTE: this implementation will work for error number <= 2 */
		do {
			bit_to_break = wl_rand_pack_bit(id_src);
		} while (bit_to_break == bit_to_break_prev);

		wl_pack_bit_flip(bit_to_break, pack);

		ret++;
	}

	return ret;
}

/* Called in locked context */
static void wl_shadow_worker_targ(uint8_t id_src, uint8_t id_dst)
{
	struct	timeval curr_time;
	struct	timeval check_time;

	/* TODO: add support of time interval >= 1sec */
	struct	timeval send_time = {
		.tv_sec = __WL_TIME_ON_AIR_S,
		.tv_usec = __WL_TIME_ON_AIR_US
	};

	/* As there is no data which is not send we have nothing to do */
	if (devs.dev[id_src].output_buff.is_send)
		return;

	/* Check send time */
	gettimeofday(&curr_time, NULL);
	timeradd(&devs.dev[id_src].output_buff.send_time, &send_time, &check_time);
	/* As send time + time_on_air <= current time we
	 * successfuly send packet, and other device probably recieved
	 * it */
	if (timercmp(&check_time, &curr_time, <=)) {
		uint32_t bits_corrupted;
//		pr_trace("Trans send from ID_%u\n", id_src);
		pr_trace("%ld.%06ld: Trans sent from ID_%u\n", check_time.tv_sec,
			 check_time.tv_usec, id_src);

		devs.dev[id_src].output_buff.is_send = true;

		/* check if we lose packets while we send it
		 * IE: (reciever didn't recognized packet preamble) */
		if (wl_is_packet_lost(id_src)) {
			pr_info("%ld.%06ld: [L] packet from ID_%u was lost\n",
				check_time.tv_sec, check_time.tv_usec, id_src);

			/* Nothing to do here */
			return;
		}

		/* corrupt data in output_buff, if needed
		 * (emulation of data corruption while pack sending) */
		bits_corrupted = wl_pack_break(id_src, devs.dev[id_src].output_buff.send_data);
		if (bits_corrupted)
			pr_info("%ld.%06ld: [C] %u bits corrupted while sending\n",
				check_time.tv_sec, check_time.tv_usec, bits_corrupted);

		if (devs.dev[id_dst].input_buff.is_recieved) {
			devs.err++;
			pr_err("%ld.%06ld: New pack came to ID_%u, but previous was not read!\n",
			       check_time.tv_sec, check_time.tv_usec, id_dst);

			/* As reciever HW input buffer is full - it can't recieve
			 * this message. So nothing to do here */
			return;
		}

//		pr_trace("New pack came to ID_%u\n", id_dst);
		pr_trace("%ld.%06ld: New pack came to ID_%u\n", check_time.tv_sec,
			 check_time.tv_usec, id_dst);

		wl_pack_copy(MAX_WL_DATA_LEN,
			devs.dev[id_dst].input_buff.recv_data,
			devs.dev[id_src].output_buff.send_data);

		devs.dev[id_dst].input_buff.is_recieved = true;
	}
}

/* Called in locked context */
static void wl_shadow_worker_chanel_check(void)
{
	/* Both transceivers are sending smth simultaneously */
	if (!devs.dev[ID_0].output_buff.is_send && !devs.dev[ID_1].output_buff.is_send) {
		devs.err++;
		pr_err("Both transceivers are sending smth simultaneously!\n");
	}
}

/* called before each lib call */
static void wl_shadow_worker(void)
{
	pthread_spin_lock(&devs.lock);

	wl_shadow_worker_targ(ID_0, ID_1);
	wl_shadow_worker_targ(ID_1, ID_0);

	wl_shadow_worker_chanel_check();

	pthread_spin_unlock(&devs.lock);
}

/* called after each lib call */
static void wl_shadow_checker(void)
{
	pthread_spin_lock(&devs.lock);

	wl_shadow_worker_chanel_check();

	pthread_spin_unlock(&devs.lock);
}

uint8_t wl_sendPacketTimeout(uint8_t id, uint8_t *payload, uint32_t timeout)
{
	uint16_t pack_len;
	struct timeval check_time __attribute__((unused));

	wl_shadow_worker();

	if (!devs.dev[id].output_buff.is_send) {
		pthread_spin_lock(&devs.lock);
		devs.err++;
		pthread_spin_unlock(&devs.lock);
		pr_err("[%u] try to send new pack, while previous packet is not sent!\n", id);
	}

//	pack_len = (uint16_t)strlen(payload);
	pack_len = CONFIG_WL_PACK_CONST_LEN;

	if (pack_len > CONFIG_WL_CHK_MAX_LORA_PACL_LEN)
		pr_warn("packet length is too high: %u\n", pack_len);

	pthread_spin_lock(&devs.lock);

	gettimeofday(&devs.dev[id].output_buff.send_time, NULL);
	check_time.tv_sec = devs.dev[id].output_buff.send_time.tv_sec;
	check_time.tv_usec = devs.dev[id].output_buff.send_time.tv_usec;
	wl_pack_copy(pack_len, devs.dev[id].output_buff.send_data, payload);
	devs.dev[id].output_buff.is_send = false;

	pthread_spin_unlock(&devs.lock);

	pr_trace("%ld.%06ld: Pack sending from ID_%u\n", check_time.tv_sec,
		 check_time.tv_usec, id);

	/* TODO: check if timeout < TOA */
	if (timeout) {
		struct timespec send_time;

		/* TODO: add nanosleep reminder check, add sec setting */
		send_time.tv_sec = __WL_TIME_ON_AIR_S;
		send_time.tv_nsec = __WL_TIME_ON_AIR_NS;
		nanosleep(&send_time, NULL);

		wl_shadow_worker();
	}

	wl_shadow_checker();

	return 0;
}

static bool wl_is_data(uint8_t id)
{
	bool ret;

	wl_shadow_worker();

	pthread_spin_lock(&devs.lock);
	ret = devs.dev[id].input_buff.is_recieved;
	pthread_spin_unlock(&devs.lock);

	wl_shadow_checker();

	return ret;
}

bool wl_availableData(uint8_t id, uint32_t timeout)
{
	struct timespec send_time;
	uint32_t time_sleep = 0;
	uint32_t i = 0;

	send_time.tv_sec = 0;
	send_time.tv_nsec = CONFIG_WL_SLEEP_GRANULARITY * 1000 * 1000;

//	if (timeout)
//		pr_warn("waiting in %s is not supported!\n", __func__);

	if (!timeout)
		return wl_is_data(id);

	while (timeout > time_sleep) {
		if (wl_is_data(id))
			return true;

		nanosleep(&send_time, NULL);

		time_sleep += CONFIG_WL_SLEEP_GRANULARITY;
		i++;
	}

	pr_trace("wait for data, %u iterations\n", i);

	return false;
}

uint8_t wl_receivePacketTimeout(uint8_t id, uint32_t timeout, uint8_t *payload)
{
	struct timeval check_time __attribute__((unused));
	bool ready;

	wl_shadow_worker();

	gettimeofday(&check_time, NULL);
//	pr_trace("%ld.%06ld: [%u] try to recieve new pack, timeout=%u\n",
//		check_time.tv_sec, check_time.tv_usec, id, timeout);

//	if (timeout)
//		pr_warn("waiting in %s is not supported!\n", __func__);
//
//	pthread_spin_lock(&devs.lock);
//	ready = devs.dev[id].input_buff.is_recieved;
//	pthread_spin_unlock(&devs.lock);

	ready = wl_availableData(id, timeout);

	/* it is ok for func with timeout */
	if (!ready && !timeout) {
		gettimeofday(&check_time, NULL);

		pthread_spin_lock(&devs.lock);
		devs.err++;
		pthread_spin_unlock(&devs.lock);

		pr_err("%ld.%06ld: [%u] try to read new pack, but it is not recieved yet!\n",
			check_time.tv_sec, check_time.tv_usec, id);
	}

	if (!ready)
		return -1;

	pthread_spin_lock(&devs.lock);

	wl_pack_copy(CONFIG_WL_PACK_CONST_LEN, payload, devs.dev[id].input_buff.recv_data);
	devs.dev[id].input_buff.is_recieved = false;

	pthread_spin_unlock(&devs.lock);

	wl_shadow_checker();

	return 0;
}
