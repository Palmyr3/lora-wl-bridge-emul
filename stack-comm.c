/* TODO: remove */
#include <stdio.h>
#include "env-core.h"

#include "cc-traceinfo.h"
#include "stack-comm.h"

#define STK_SEND_TIMEOUT	(CONFIG_WL_STD_TIME_ON_AIR_MS * 2)

//#define static

static bool stk_mac_has_pack_to_send(struct stl_transmiter *tsm);

static uint16_t stk_crc16(const uint8_t *data_p, uint16_t length)
{
	uint8_t x;
	uint16_t crc = 0xFFFF;

	while (length--) {
		x = crc >> 8 ^ *data_p++;
		x ^= x >> 4;
		crc = (crc << 8) ^ ((uint16_t)(x << 12)) ^ ((uint16_t)(x << 5)) ^ ((uint16_t)x);
	}

	return crc;
}

uint16_t stk_checksumm(const uint8_t *data_p, uint16_t length)
{
	return stk_crc16(data_p, length);
}

bool stk_test_pack_checksumm(const uint8_t *data_p)
{
	union stk_pack_checksum crc;

	/* len = pack length - crc16 length */
	crc.sum_16 = stk_checksumm(data_p, CONFIG_STK_PACK_CONST_LEN - 2);

//	printf("crc 16b: %#x, 8b: %#x : %#x\n", crc.checksum_16, crc.checksum_8.lo, crc.checksum_8.hi);
//	printf("crc 16b: %#x, 8b: %#x : %#x\n", crc.checksum_16, data_p[17], data_p[18]);

	return data_p[STK_P_CRC_LO] == crc.sum_8.lo && data_p[STK_P_CRC_HI] == crc.sum_8.hi;
}

void stk_set_pack_checksumm(uint8_t *data_p)
{
	union stk_pack_checksum crc;

	crc.sum_16 = stk_checksumm(data_p, CONFIG_STK_PACK_CONST_LEN - 2);

	data_p[STK_P_CRC_LO] = crc.sum_8.lo;
	data_p[STK_P_CRC_HI] = crc.sum_8.hi;
}

static enum l1_pack_type stk_get_pack_type(struct wls_pack *pack)
{
	union stk_pack_header_l1 header_l1;

	header_l1.byte = pack->data[STK_P_HEADER_L1];

	return header_l1.generic_pack.type;
}

static bool stk_is_reset_pack(struct wls_pack *pack)
{
	union stk_pack_header_l1 header_l1;

	if (stk_get_pack_type(pack) != TYPE_INTERNAL)
		return false;

	header_l1.byte = pack->data[STK_P_HEADER_L1];

	return !!header_l1.internal_pack.reset;
}

static uint8_t stk_get_trans_id(struct wls_pack *pack)
{
	union stk_pack_header_l1 header_l1;

	header_l1.byte = pack->data[STK_P_HEADER_L1];

	return header_l1.generic_pack.id;
}

uint16_t stk_get_payload_len(uint8_t *data_p)
{
	union stk_pack_header_l1 header_l1;

	header_l1.byte = data_p[STK_P_HEADER_L1];

	if (header_l1.generic_pack.type == TYPE_SINGLE)
		return header_l1.single_pack.payload_len + 1;

	if (header_l1.generic_pack.type == TYPE_MULT)
		return CONFIG_STK_PAYLOAD_LEN;

	/* TODO: add other pack types */
	pr_err("pack type not supportrd yet\n");
	return 0;
}

static int stk_llc_single_prepare(struct stl_transmiter *tsm, struct wls_pack *pack, const uint8_t *payload, uint16_t len)
{
	union stk_pack_header_l1 header_l1;

	pr_assert(len <= CONFIG_STK_PAYLOAD_LEN, "s pack length too big: %u\n", len);
	pr_assert(len != 0, "s pack length too small: %u\n", len);

	header_l1.single_pack.type = TYPE_SINGLE;
	header_l1.single_pack.payload_len = len - 1;
	header_l1.single_pack.id = tsm->trans_id;

	pack->data[STK_P_HEADER_L1] = header_l1.byte;
	for (int i = 0; i < CONFIG_STK_PAYLOAD_LEN; i++)
		pack->data[STK_P_PAYLOAD_L1 + i] = payload[i];

	return 0;
}

static bool stk_mac_has_pack_to_send(struct stl_transmiter *tsm)
{
	return !list_empty(&tsm->send_list);
}

static struct wls_pack * stk_mac_get_first_to_send(struct stl_transmiter *tsm)
{
	pr_assert(stk_mac_has_pack_to_send(tsm), "%s\n", __func__);

	return list_first_entry(&tsm->send_list, struct wls_pack, mac_node);
}

static void stk_mac_mark_as_send(struct stl_transmiter *tsm)
{
	struct wls_pack *first;

	pr_assert(stk_mac_has_pack_to_send(tsm), "%s\n", __func__);

	first = stk_mac_get_first_to_send(tsm);

	list_del(&first->mac_node);
}

/* add to send queue */
static void stk_llc_pack_commit(struct stl_transmiter *tsm, struct wls_pack *pack)
{
	pr_assert(!stk_mac_has_pack_to_send(tsm), "%s\n", __func__); //temp, due to lack of support pack chains

	list_add_tail(&pack->mac_node, &tsm->send_list);
}

static bool stk_llc_has_received_pack(struct stl_transmiter *tsm)
{
	return !list_empty(&tsm->recv_list);
}

/* add to received queue */
static void stk_mac_pack_commit(struct stl_transmiter *tsm, struct wls_pack *pack)
{
	pr_assert(!stk_llc_has_received_pack(tsm), "%s\n", __func__); //temp, due to lack of support pack chains

	list_add_tail(&pack->mac_node, &tsm->recv_list);
}

static struct wls_pack * stk_llc_get_first_received(struct stl_transmiter *tsm)
{
	pr_assert(stk_llc_has_received_pack(tsm), "%s\n", __func__);

	return list_first_entry(&tsm->recv_list, struct wls_pack, mac_node);
}

static void stk_put_recv_packholder(struct stl_transmiter *tsm, struct wls_pack *pack)
{
	/* retrieve to pool */
	list_move_tail(&pack->owner_node, &tsm->recv_list_pool);
	tsm->llc_recv_owned_counter--;
}

static void stk_llc_mark_as_received(struct stl_transmiter *tsm)
{
	struct wls_pack *first;

	pr_assert(stk_llc_has_received_pack(tsm), "%s\n", __func__);

	first = stk_llc_get_first_received(tsm);

	/* delete from recieve list */
	list_del(&first->mac_node);

	/* retrieve to pool */
	stk_put_recv_packholder(tsm, first);
}

static void stk_mac_do_send(struct stl_transmiter *tsm)
{
	struct wls_pack *first;

	pr_assert(stk_mac_has_pack_to_send(tsm), "try to send, but no pack to send\n");

	first = stk_mac_get_first_to_send(tsm);

	stk_set_pack_checksumm(first->data);
	wl_sendPacketTimeout(tsm->id, first->data, STK_SEND_TIMEOUT);

	stk_mac_mark_as_send(tsm);
}

/* allocate wls_pack from recv pool  */
static struct wls_pack * stk_get_recv_packholder(struct stl_transmiter *tsm)
{
	struct wls_pack *pack;

	pr_assert(!list_empty(&tsm->recv_list_pool), "no more entries in recv_list_pool? recv_owned %d\n",
		  tsm->llc_recv_owned_counter);

	pack = list_first_entry(&tsm->recv_list_pool, struct wls_pack, owner_node);
	list_move_tail(&pack->owner_node, &tsm->llc_recv_owned);
	tsm->llc_recv_owned_counter++;

	return pack;
}

static struct wls_pack * stk_get_send_packholder(struct stl_transmiter *tsm)
{
	struct wls_pack *pack;

	pr_assert(!list_empty(&tsm->send_list_pool), "no more entries in send_list_pool? send_owned %d\n",
		  tsm->llc_send_owned_counter);

	pack = list_first_entry(&tsm->send_list_pool, struct wls_pack, owner_node);
	list_move_tail(&pack->owner_node, &tsm->llc_send_owned);
	tsm->llc_send_owned_counter++;

	return pack;
}

static void stk_mac_do_receive(struct stl_transmiter *tsm)
{
	struct wls_pack *pack;

	pack = stk_get_recv_packholder(tsm);
	pr_assert(pack, "no place in recv pool?\n");

	if (wl_receivePacketTimeout(tsm->id, tsm->mac_recv_timeout, pack->data))
		goto recv_fail;

	if (!stk_test_pack_checksumm(pack->data)) {
		pr_info("[%s:%u] %s: bad crc\n", tsm->id ? "s" : "m", tsm->trans_id, __func__);
		goto recv_fail;
	}

	stk_mac_pack_commit(tsm, pack);

	return;

recv_fail:
	stk_put_recv_packholder(tsm, pack);
}

static void stk_c_init_pools(struct stl_transmiter *tsm)
{
	INIT_LIST_HEAD(&tsm->send_list_pool);
	INIT_LIST_HEAD(&tsm->recv_list_pool);

	tsm->llc_send_owned_counter = 0;
	tsm->llc_recv_owned_counter = 0;

	for (int i = 0; i < STK_PACKET_POOL_SIZE; i++)
		list_add_tail(&(tsm->send_list_pool_buff[i].owner_node), &tsm->send_list_pool);

	for (int i = 0; i < STK_PACKET_POOL_SIZE; i++)
		list_add_tail(&(tsm->recv_list_pool_buff[i].owner_node), &tsm->recv_list_pool);
}

static void stk_c_tsm_init(struct stl_transmiter *tsm)
{
	INIT_LIST_HEAD(&tsm->send_list);
	INIT_LIST_HEAD(&tsm->recv_list);
	INIT_LIST_HEAD(&tsm->llc_resend_list);
	INIT_LIST_HEAD(&tsm->llc_send_owned);
	INIT_LIST_HEAD(&tsm->llc_recv_owned);
	tt_timer_init(&tsm->llc_timer);

	stk_c_init_pools(tsm);

	tsm->retries_num = 0;
}

static void stk_s_tsm_init(struct stl_transmiter *tsm)
{
	stk_c_tsm_init(tsm);

	/* always use 1 as slave id */
	tsm->id = 1;

	tsm->mac_state = MAC_RECEIVING_CONT;
	tsm->llc_state_s = S_LLC_IDLE;

	tsm->trans_id = TRANS_ID_NONE;
	tsm->mac_recv_timeout = CONFIG_WL_STD_TIME_ON_AIR_MS * 1000;
}

static void stk_m_tsm_init(struct stl_transmiter *tsm)
{
	stk_c_tsm_init(tsm);

	/* always use 0 as master id */
	tsm->id = 0;

	tsm->mac_state = MAC_IDLE;
	tsm->llc_state_m = M_LLC_IDLE;

	tsm->trans_id = TRANS_ID_START;
	tsm->mac_recv_timeout = CONFIG_WL_STD_TIME_ON_AIR_MS * 3;
}

static void stk_llc_resend_clear_all(struct stl_transmiter *tsm)
{
	struct wls_pack *child, *_next;

	list_for_each_entry_safe(child, _next, &tsm->llc_resend_list, owner_node) {
		list_move_tail(&child->owner_node, &tsm->send_list_pool);
		tsm->llc_send_owned_counter--;
	}

	/* TODO: chech do we need to init list here? */
	INIT_LIST_HEAD(&tsm->llc_resend_list);

}

static void stk_llc_resend_add(struct stl_transmiter *tsm, struct wls_pack *pack)
{
	list_move_tail(&pack->owner_node, &tsm->llc_resend_list);
}

static bool stk_llc_resend_get(struct stl_transmiter *tsm, struct wls_pack **pack)
{
	if (!list_empty(&tsm->llc_resend_list)) {
		*pack = list_first_entry(&tsm->llc_resend_list, struct wls_pack, owner_node);
		return true;
	}

	return false;
}

static void stk_mac_worker(struct stl_transmiter *tsm, enum mac_state *mac_state)
{
	/* don't support MAC_IDLE as of today */
	/* TODO: support send delay slot */
	/* TODO: non-blocking funcs support (basically add state magic) */

	if (stk_mac_has_pack_to_send(tsm)) {
		*mac_state = MAC_SENDING;
		stk_mac_do_send(tsm);
	} else {
		*mac_state = MAC_RECEIVING_CONT;
		stk_mac_do_receive(tsm);
	}

	/*
	 * If we have another pack to send - send it in the next iteration
	 * Otherwise - try to recieve smth in the next iteration.
	 */
	if (!stk_mac_has_pack_to_send(tsm)) {
		*mac_state = MAC_RECEIVING_CONT;
	}
}

static uint8_t send_data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 0, 0, 0};

void stk_s_llc_worker_v3(struct stl_transmiter *tsm, enum llc_state_s *llc_state_s)
{
	struct wls_pack *pack;
	enum llc_state_s llc_state_s_prev;

respin:
	llc_state_s_prev = *llc_state_s;

	switch (llc_state_s_prev) {
	case S_LLC_IDLE: {
		if (stk_llc_has_received_pack(tsm)) {
			pack = stk_llc_get_first_received(tsm);

			if (stk_is_reset_pack(pack)) {
				/* TODO: probably we need to treat _all_
				 * TYPE_INTERNAL != ACK packets (like config)
				 * unconditionnaly */
				/* Do some reset actions */
				/* TODO: send ACK to master */
				pr_assert(false, "\n");
			} else if (stk_get_trans_id(pack) == tsm->trans_id) {
				/* we need to resend smth */
				if (stk_get_pack_type(pack) == TYPE_SINGLE) {
					*llc_state_s = S_LLC_RESEND;
				} else {
					pr_assert(false, "\n");
				}
			} else {
				/* new transaction packet came */
				stk_llc_resend_clear_all(tsm);
				tsm->retries_num = 0;

				if (stk_get_pack_type(pack) == TYPE_SINGLE) {
					tsm->trans_id = stk_get_trans_id(pack);
					*llc_state_s = S_LLC_RECEIVED_ALL;
				} else {
					pr_assert(false, "\n");
				}
			}

			stk_llc_mark_as_received(tsm);
		}

		break;
	}
	case S_LLC_RECEIVED_ALL: {
		pack = stk_get_send_packholder(tsm);
		stk_llc_single_prepare(tsm, pack, send_data, 16);

		stk_llc_pack_commit(tsm, pack);
		stk_llc_resend_add(tsm, pack);

		*llc_state_s = S_LLC_SEND_ALL;

		break;
	}
	case S_LLC_SEND_ALL: {
		pr_info("[s:%u] recieved & send\n", tsm->trans_id);

		*llc_state_s = S_LLC_IDLE;

		break;
	}
	case S_LLC_RESEND: {
		/* resend last packet (or series) */
		pr_assert(stk_llc_resend_get(tsm, &pack), "empty resend list?\n");

		stk_llc_resend_get(tsm, &pack);
		stk_llc_pack_commit(tsm, pack);

		tsm->retries_num++;

		pr_info("[s:%u] resending response, due to master pack, retries %u\n", tsm->trans_id, tsm->retries_num);

		*llc_state_s = S_LLC_IDLE;

		break;
	}

	default:
		pr_assert(false, "\n");
	}

	/* as current state changed - run FSM again as we can process another state */
	if (llc_state_s_prev != *llc_state_s)
		goto respin;
}

void stk_s_comm_loop(struct stl_transmiter *tsm)
{
	enum mac_state l_mac_state;
	enum llc_state_s l_llc_state_s;

	stk_s_tsm_init(tsm);

	l_mac_state = tsm->mac_state;
	l_llc_state_s = tsm->llc_state_s;

	while (true) {
		stk_s_llc_worker_v3(tsm, &l_llc_state_s);
		tsm->llc_state_s = l_llc_state_s;

		stk_mac_worker(tsm, &l_mac_state);
		tsm->mac_state = l_mac_state;
	}
}

static void stk_m_prep_trans_id(struct stl_transmiter *tsm)
{
	tsm->trans_id++;

	if (tsm->trans_id > TRANS_ID_MAX)
		tsm->trans_id = TRANS_ID_START;
}

bool stk_m_has_pack_to_send(struct stl_transmiter *tsm)
{
	return true;
}

static void stk_m_llc_worker_v3(struct stl_transmiter *tsm, enum llc_state_m *llc_state_m)
{
	struct wls_pack *pack;
	enum llc_state_m llc_state_m_prev;

respin:
	llc_state_m_prev = *llc_state_m;

	switch (llc_state_m_prev) {
	case M_LLC_IDLE: {
		if (stk_m_has_pack_to_send(tsm)) {
			stk_llc_resend_clear_all(tsm);
			tsm->retries_num = 0;
			stk_m_prep_trans_id(tsm);

			pack = stk_get_send_packholder(tsm);

			stk_llc_single_prepare(tsm, pack, send_data, 16);

			stk_llc_pack_commit(tsm, pack);
			stk_llc_resend_add(tsm, pack);

			tt_timer_start(&tsm->llc_timer, CONFIG_WL_STD_TIME_ON_AIR_MS * 3);

			*llc_state_m = M_LLC_SEND_ALL;
		}

		break;
	}
	case M_LLC_SEND_ALL: {
		if (stk_llc_has_received_pack(tsm)) {
			pack = stk_llc_get_first_received(tsm);

			pr_assert(stk_get_pack_type(pack) == TYPE_SINGLE,
				  "unknown pack, type %u\n", stk_get_pack_type(pack));

			pr_assert(stk_get_trans_id(pack) == tsm->trans_id,
				  "trans_id mismatch, got %u\n", stk_get_trans_id(pack));

			tsm->test_trans_id = stk_get_trans_id(pack);

			stk_llc_mark_as_received(tsm);
			*llc_state_m = M_LLC_RECEIVED_ALL;

		} else if (tt_timer_is_timeouted(&tsm->llc_timer)) {
			/* timeouted, resend last packet */
			*llc_state_m = M_LLC_RESEND;
		}

		break;
	}
	case M_LLC_RECEIVED_ALL: {
		pr_assert(tsm->trans_id == tsm->test_trans_id, "%s trans_id mismatch\n", __func__);

		pr_info("[m:%u] send & recieved ACK, got id %u, retries %u\n", tsm->trans_id, tsm->test_trans_id, tsm->retries_num);

		*llc_state_m = M_LLC_IDLE;

		break;
	}
	case M_LLC_RESEND: {
		/* resend last packet (or series) */
		pr_assert(stk_llc_resend_get(tsm, &pack), "%s empty resend list?\n", __func__);

		stk_llc_resend_get(tsm, &pack);
		stk_llc_pack_commit(tsm, pack);

		tt_timer_start(&tsm->llc_timer, CONFIG_WL_STD_TIME_ON_AIR_MS * 3);

		tsm->retries_num++;

		pr_info("[m:%u] resending pack, due to timeout, retries %u\n", tsm->trans_id, tsm->retries_num);

		*llc_state_m = M_LLC_SEND_ALL;

		break;
	}

	default:
		pr_assert(false, "\n");
	}

	/* as current state changed - run FSM again as we can process another state */
	if (llc_state_m_prev != *llc_state_m)
		goto respin;
}

void stk_m_comm_loop(struct stl_transmiter *tsm)
{
	enum mac_state l_mac_state;
	enum llc_state_m l_llc_state_m;

	stk_m_tsm_init(tsm);

	l_mac_state = tsm->mac_state;
	l_llc_state_m = tsm->llc_state_m;

	while (true) {
		stk_m_llc_worker_v3(tsm, &l_llc_state_m);
		tsm->llc_state_m = l_llc_state_m;

		stk_mac_worker(tsm, &l_mac_state);
		tsm->mac_state = l_mac_state;
	}
}
