#ifndef STACK_COMM_H
#define STACK_COMM_H

#include <stdint.h>
#include <stdbool.h>
#include "list.h"
#include "timeout-timer.h"

#define CONFIG_STK_PACK_CONST_LEN		19
#define CONFIG_STK_PAYLOAD_LEN			CONFIG_STK_PACK_CONST_LEN - 3

#define STK_PACKET_POOL_SIZE	10

/* packet structure */
#define STK_P_HEADER_L1		0
#define STK_P_HEADER_L2		1
#define STK_P_PAYLOAD_L1	1
#define STK_P_CRC_LO		17
#define STK_P_CRC_HI		18

enum l1_pack_type {
	TYPE_INTERNAL = 0,
	TYPE_SINGLE,
	TYPE_MULT,
	TYPE_MULT_LAST
};

union stk_pack_header_l1 {
	struct {
		uint8_t type: 2, id: 2, pad: 4;
	} generic_pack;
	struct {
		uint8_t type: 2, id: 2, payload_len: 4;
	} single_pack;
	struct {
		uint8_t type: 2, id: 2, pack_num: 4;
	} modbus_pack;
	struct {
		uint8_t type: 2, id: 2, reset: 1, part_ack: 1, config: 2;
	} internal_pack;
	uint8_t byte;
};

union stk_pack_checksum {
	uint16_t sum_16;
	struct {
		/* LE expected */
		uint8_t hi;
		uint8_t lo;
	} sum_8;
};

enum trans_id {
	TRANS_ID_START	= 0,
	TRANS_ID_MAX	= 3,
	TRANS_ID_NONE	= 4,
};

enum mac_state {
	MAC_IDLE = 0,
	MAC_SENDING,
	MAC_RECEIVING_CONT
};

//enum llc_state {
//	LLC_IDLE = 0,
///*	LLC_RECEIVED, */	/* used only in multi-pack link */
//	LLC_RECEIVED_ALL,
///*	LLC_SEND, */		/* used only in multi-pack link */
//	LLC_SEND_ALL
//};

enum llc_state_s {
	S_LLC_IDLE = 0,
	S_LLC_RECEIVED_ALL,
	S_LLC_SEND_ALL,
	S_LLC_RESEND
};

enum llc_state_m {
	M_LLC_IDLE = 0,
	M_LLC_RECEIVED_ALL,
	M_LLC_SEND_ALL,
	M_LLC_RESEND
};


struct wls_pack {
	uint8_t data[CONFIG_STK_PACK_CONST_LEN];
	struct list_head mac_node;	/* send / recieve queue */

	struct list_head owner_node;	/* pool / resend pool / etc */
};

struct stl_transmiter {
	uint8_t id;

	enum mac_state		mac_state;
	enum llc_state_s	llc_state_s;
	enum llc_state_m	llc_state_m;

	uint8_t			test_trans_id;		// temp
	uint8_t			retries_num;		// temp
	uint32_t		mac_recv_timeout;	// temp

	struct list_head	send_list;
	struct list_head	recv_list;

	struct wls_pack		send_list_pool_buff[STK_PACKET_POOL_SIZE];
	struct list_head	send_list_pool;
	struct wls_pack		recv_list_pool_buff[STK_PACKET_POOL_SIZE];
	struct list_head	recv_list_pool;

	/* TODO: could we use common pool? */
	int8_t			llc_send_owned_counter;
	struct list_head	llc_send_owned;
	int8_t			llc_recv_owned_counter;
	struct list_head	llc_recv_owned;

	struct list_head	llc_resend_list;
	struct tt_timer		llc_timer;

	uint8_t			trans_id; /* 2 bit of transaction id field */
};

uint16_t stk_checksumm(const uint8_t *data_p, uint16_t length);
bool stk_test_pack_checksumm(const uint8_t *data_p);
void stk_set_pack_checksumm(uint8_t *data_p);

void stk_m_send_single(struct stl_transmiter *tsm, const uint8_t *payload, uint16_t len);
void stk_m_wait_for_pack(struct stl_transmiter *tsm);

void stk_s_wait_for_pack(struct stl_transmiter *tsm);
void stk_s_process_pack_generic(struct stl_transmiter *tsm, uint8_t *pack);
void stk_s_process_pack_single(struct stl_transmiter *tsm, uint8_t *pack);

void stk_s_comm_loop(struct stl_transmiter *tsm);
void stk_m_comm_loop(struct stl_transmiter *tsm);

#endif /* STACK_COMM_H */
