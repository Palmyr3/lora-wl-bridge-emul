/* TODO: remove */
#include <stdio.h>
#include "env-core.h"

#include "cc-traceinfo.h"
#include "stack-comm.h"

#define STK_SEND_TIMEOUT	(CONFIG_WL_STD_TIME_ON_AIR_MS * 2)

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

/* Master part */
//bool stk_transmiter_busy(struct stl_transmiter *tsm)
//{
//	return tsm->state != M_IDLE;
//}

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

#define WFA_ATTEMPTS 3

//void stk_m_send_single(struct stl_transmiter *tsm, const uint8_t *payload, uint16_t len)
//{
//	uint8_t ack_pack[CONFIG_STK_PACK_CONST_LEN];
//	int ret, attempts = WFA_ATTEMPTS;
//
//	if (tsm->state != M_IDLE) {
//		pr_err("transmiter is not idle\n");
//		return;
//	}
//
//	ret = stk_m_single_prepare(tsm, payload, len);
//	if (ret)
//		return;
//
//	while (attempts--) {
//		tsm->state = M_SEND_SINGLE;
//
//		ret = wl_sendPacketTimeout(tsm->id, tsm->ts_pack, STK_SEND_TIMEOUT);
//
//		tsm->state = M_WAIT_ACK_SINGLE;
//		/* Wait for ACK */
//		ret = wl_receivePacketTimeout(tsm->id, CONFIG_WL_STD_TIME_ON_AIR_MS * 2, ack_pack);
//		if (ret)
//			continue;
//
//		if (!stk_test_pack_checksumm(ack_pack)) {
//			pr_warn("%s: bad CRC\n", __func__);
//			continue;
//		}
//
//		if (stk_get_pack_type(ack_pack) != TYPE_INTERNAL) {
//			pr_warn("%s: unknown pack\n", __func__);
//			continue;
//		}
//
//		/* TODO: check that pack is ACK exactly */
//
//		tsm->state = M_IDLE;
//		pr_trace("sended and recieved ACK\n");
//		return;
//	}
//
//	tsm->state = M_IDLE;
//	pr_warn("failed to send and rec ACK after %d attempts\n", WFA_ATTEMPTS);
//}

//void stk_m_wait_for_pack(struct stl_transmiter *tsm)
//{
//	uint8_t ack_pack[CONFIG_STK_PACK_CONST_LEN] = {};
//	union stk_pack_header_l1 header_l1;
//	int ret;
//
//	while (true) {
//		ret = wl_receivePacketTimeout(tsm->id, CONFIG_WL_STD_TIME_ON_AIR_MS * 1000, ack_pack);
//		if (ret)
//			continue;
//
//		if (!stk_test_pack_checksumm(ack_pack)) {
//			pr_warn("%s: bad CRC\n", __func__);
//			continue;
//		}
//
//		if (stk_get_pack_type(ack_pack) != TYPE_SINGLE) {
//			pr_warn("%s: unknown pack\n", __func__);
//			continue;
//		}
//
//		header_l1.single_pack.type = TYPE_INTERNAL;
//		/* TODO: add freecounter */
//
//		tsm->ts_pack[STK_P_HEADER_L1] = header_l1.byte;
//		stk_set_pack_checksumm(tsm->ts_pack);
//
//		ret = wl_sendPacketTimeout(tsm->id, tsm->ts_pack, STK_SEND_TIMEOUT);
//
//		pr_info("send ACK\n");
//	}
//}

//void stk_s_wait_for_pack(struct stl_transmiter *tsm)
//{
//	uint8_t ack_pack[CONFIG_STK_PACK_CONST_LEN] = {};
//	uint32_t free_wait_timeout = CONFIG_WL_STD_TIME_ON_AIR_MS * 1000;
//	int ret;
//
//	tsm->state = S_WAIT_FOR_PACK;
//
//	while (tsm->state == S_WAIT_FOR_PACK) {
//		ret = wl_receivePacketTimeout(tsm->id, free_wait_timeout, ack_pack);
//		if (ret)
//			continue;
//
//		if (!stk_test_pack_checksumm(ack_pack)) {
//			pr_warn("%s: bad CRC\n", __func__);
//			continue;
//		}
//
//		stk_s_process_pack_generic(tsm, ack_pack);
//	}
//}

//void stk_s_process_pack_generic(struct stl_transmiter *tsm, uint8_t *pack)
//{
//	union stk_pack_header_l1 header_l1;
//
//	tsm->state = S_PREPARE_RESP;
//
//	header_l1.byte = pack[STK_P_HEADER_L1];
//
//	switch (header_l1.generic_pack.type) {
//		case TYPE_SINGLE:
//			stk_s_process_pack_single(tsm, pack);
//			break;
//		case TYPE_INTERNAL:
//		case TYPE_MULT:
//		case TYPE_MULT_LAST:
//		default:
//			tsm->state = S_WAIT_FOR_PACK;
//			pr_warn("Unsupported packet type: 0x%x", header_l1.generic_pack.type);
//			break;
//	}
//}

//void stk_s_process_pack_single(struct stl_transmiter *tsm, uint8_t *pack)
//{
//	union stk_pack_header_l1 header_l1;
//
//	header_l1.single_pack.type = TYPE_INTERNAL;
//	/* TODO: add freecounter */
//
//	tsm->ts_pack[STK_P_HEADER_L1] = header_l1.byte;
//	stk_set_pack_checksumm(tsm->ts_pack);
//
//	wl_sendPacketTimeout(tsm->id, tsm->ts_pack, STK_SEND_TIMEOUT);
//
//	pr_info("send ACK\n");
//
//	tsm->state = S_WAIT_FOR_PACK;
//}

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

static void stk_llc_mark_as_received(struct stl_transmiter *tsm)
{
	struct wls_pack *first;

	pr_assert(stk_llc_has_received_pack(tsm), "%s\n", __func__);

	first = stk_llc_get_first_received(tsm);

	list_del(&first->mac_node);
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
	//temporary use first place as we don't support pack chains
	return &(tsm->recv_list_pool[0]);
}

static void stk_mac_do_receive(struct stl_transmiter *tsm)
{
	struct wls_pack *pack;

	pack = stk_get_recv_packholder(tsm);
	pr_assert(pack, "no place in recv pool?\n");

	if (wl_receivePacketTimeout(tsm->id, tsm->mac_recv_timeout, pack->data))
		return;

	if (!stk_test_pack_checksumm(pack->data)) {
		pr_warn("%s: bad CRC\n", __func__);
		return;
	}

	stk_mac_pack_commit(tsm, pack);
}

static void stk_c_tsm_init(struct stl_transmiter *tsm)
{
	INIT_LIST_HEAD(&tsm->send_list);
	INIT_LIST_HEAD(&tsm->recv_list);
	INIT_LIST_HEAD(&tsm->llc_resend_list);
	tt_timer_init(&tsm->llc_timer);

	tsm->retries_num = 0;
}

static void stk_s_tsm_init(struct stl_transmiter *tsm)
{
	stk_c_tsm_init(tsm);

	/* always use 1 as slave id */
	tsm->id = 1;

	tsm->mac_state = MAC_RECEIVING_CONT;
	tsm->llc_state = LLC_IDLE;
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
	tsm->llc_state = LLC_IDLE;

	tsm->trans_id = TRANS_ID_START;
	tsm->mac_recv_timeout = CONFIG_WL_STD_TIME_ON_AIR_MS * 3;
}

static void stk_llc_resend_clear(struct stl_transmiter *tsm)
{
	struct wls_pack *child, *_next;

	list_for_each_entry_safe(child, _next, &tsm->llc_resend_list, owner_node) {
		list_del_init(&child->owner_node);

		/* TODO: move to pool back */
	}

	/* TODO: chech do we need to init list here? */
	INIT_LIST_HEAD(&tsm->llc_resend_list);

}

static void stk_llc_resend_add(struct stl_transmiter *tsm, struct wls_pack *pack)
{
	list_add_tail(&pack->owner_node, &tsm->llc_resend_list);
}

static bool stk_llc_resend_get(struct stl_transmiter *tsm, struct wls_pack **pack)
{
	if (!list_empty(&tsm->llc_resend_list)) {
		/* TODO: go we need to use double pointer here?? */
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

void stk_s_llc_worker_v2(struct stl_transmiter *tsm, enum llc_state_s *llc_state_s)
{
	struct wls_pack *pack;
	enum llc_state_s llc_state_s_prev;

respin:
	llc_state_s_prev = *llc_state_s;

	switch (tsm->llc_state_s) {
		case S_LLC_IDLE: {
			if (stk_llc_has_received_pack(tsm)) {
				pack = stk_llc_get_first_received(tsm);

				stk_llc_resend_clear(tsm);

				if (stk_get_pack_type(pack) == TYPE_SINGLE) {
					tsm->trans_id = stk_get_trans_id(pack);
					*llc_state_s = S_LLC_RECEIVED_ALL;

				} else if (stk_get_pack_type(pack) == TYPE_INTERNAL) {
					pr_assert(false, "%s:%d\n", __func__, __LINE__);

				} else {
					pr_assert(false, "%s:%d\n", __func__, __LINE__);

				}

				stk_llc_mark_as_received(tsm);
			}

			break;
		}
		case S_LLC_RECEIVED_ALL: {
			stk_llc_single_prepare(tsm, &(tsm->send_list_pool[0]), send_data, 16);

			tt_timer_start(&tsm->llc_timer, CONFIG_WL_STD_TIME_ON_AIR_MS * 3);
			stk_llc_pack_commit(tsm, &tsm->send_list_pool[0]);
			stk_llc_resend_add(tsm, &tsm->send_list_pool[0]);

			tsm->retries_num = 0;
			*llc_state_s = S_LLC_SEND_ALL;

			break;
		}
		case S_LLC_SEND_ALL: {
			if (stk_llc_has_received_pack(tsm)) {
				pack = stk_llc_get_first_received(tsm);

				if (stk_get_pack_type(pack) != TYPE_INTERNAL)
					pr_warn("%s: unknown pack\n", __func__);

				if (stk_get_trans_id(pack) != tsm->trans_id)
					pr_warn("%s: trans_id mismatch\n", __func__);

				stk_llc_mark_as_received(tsm);
				*llc_state_s = S_LLC_IDLE;

			} else if (tt_timer_is_timeouted(&tsm->llc_timer)) {
				/* timeouted, resend last packet */
				*llc_state_s = S_LLC_RESEND;
			}

			break;
		}
		case S_LLC_RESEND: {
			/* timeouted, resend last packet */
			pr_assert(stk_llc_resend_get(tsm, &pack), "empty resend list?\n");

			stk_llc_single_prepare(tsm, &(tsm->send_list_pool[0]), send_data, 16);

			tt_timer_start(&tsm->llc_timer, CONFIG_WL_STD_TIME_ON_AIR_MS * 3);
			stk_llc_pack_commit(tsm, &tsm->send_list_pool[0]);

			tsm->retries_num++;

			pr_info("[s] not recieve ack, resending response, retries %u\n", tsm->retries_num);

			*llc_state_s = S_LLC_SEND_ALL;

			break;
		}

		default:
			pr_assert(false, "%s:%d\n", __func__, __LINE__);
	}

	/* as current state changed - run FSM again as we can process another state */
	if (llc_state_s_prev != *llc_state_s)
		goto respin;
}

void stk_s_llc_worker_v3(struct stl_transmiter *tsm, enum llc_state_s *llc_state_s)
{
	struct wls_pack *pack;
	enum llc_state_s llc_state_s_prev;

respin:
	llc_state_s_prev = *llc_state_s;

	switch (tsm->llc_state_s) {
		case S_LLC_IDLE: {
			if (stk_llc_has_received_pack(tsm)) {
				pack = stk_llc_get_first_received(tsm);

				/* TODO: treat all TYPE_INTERNAL != ACK packets unconditionnaly */
				if (stk_get_trans_id(pack) == tsm->trans_id) {
					/* we need to resend smth */
					if (stk_get_pack_type(pack) == TYPE_SINGLE) {
						*llc_state_s = S_LLC_RESEND;
					} else {
						pr_assert(false, "%s:%d\n", __func__, __LINE__);
					}
				} else {
					/* new packet came */
					stk_llc_resend_clear(tsm);
					tsm->retries_num = 0;

					if (stk_get_pack_type(pack) == TYPE_SINGLE) {
						tsm->trans_id = stk_get_trans_id(pack);
						*llc_state_s = S_LLC_RECEIVED_ALL;
					} else {
						pr_assert(false, "%s:%d\n", __func__, __LINE__);
					}
				}

				stk_llc_mark_as_received(tsm);
			}

			break;
		}
		case S_LLC_RECEIVED_ALL: {
			stk_llc_single_prepare(tsm, &(tsm->send_list_pool[0]), send_data, 16);

			stk_llc_pack_commit(tsm, &tsm->send_list_pool[0]);
			stk_llc_resend_add(tsm, &tsm->send_list_pool[0]);

			*llc_state_s = S_LLC_SEND_ALL;

			break;
		}
		case S_LLC_SEND_ALL: {
			pr_trace("[s:%u] recieved & send, trans_id\n", tsm->trans_id);

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
			pr_assert(false, "%s:%d\n", __func__, __LINE__);
	}

	/* as current state changed - run FSM again as we can process another state */
	if (llc_state_s_prev != *llc_state_s)
		goto respin;
}

void stk_s_llc_worker(struct stl_transmiter *tsm, enum llc_state *llc_state)
{
	/*
	 * What we need to do here:
	 * 1. process new packet
	 *   -> new pack in chain
	 *   -> pack send by pd by timeout (we need to resend smth)
	 * 2. generate pack to send
	 * 3. ??
	 */

	struct wls_pack *pack;
	// temp dumb implementation:
	union stk_pack_header_l1 header_l1;
	uint8_t trans_id;

	if (!stk_llc_has_received_pack(tsm))
		return;

	pack = stk_llc_get_first_received(tsm);

	if (stk_get_pack_type(pack) != TYPE_SINGLE) {
		pr_warn("%s: unknown pack\n", __func__);
		return;
	}

	trans_id = stk_get_trans_id(pack);

	stk_llc_mark_as_received(tsm);

	header_l1.single_pack.type = TYPE_INTERNAL;
	header_l1.single_pack.id = trans_id;

	tsm->send_list_pool[0].data[STK_P_HEADER_L1] = header_l1.byte;
	stk_llc_pack_commit(tsm, &tsm->send_list_pool[0]);
}

void stk_s_comm_loop(struct stl_transmiter *tsm)
{
	enum mac_state l_mac_state;
	enum llc_state l_llc_state;

	stk_s_tsm_init(tsm);

	l_mac_state = tsm->mac_state;
	l_llc_state = tsm->llc_state;

	while (true) {
		stk_s_llc_worker(tsm, &l_llc_state);
		tsm->llc_state = l_llc_state;

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

	switch (tsm->llc_state_m) {
		case M_LLC_IDLE: {
			if (stk_m_has_pack_to_send(tsm)) {
				stk_llc_resend_clear(tsm);
				tsm->retries_num = 0;
				stk_m_prep_trans_id(tsm);

				stk_llc_single_prepare(tsm, &(tsm->send_list_pool[0]), send_data, 16);

				stk_llc_pack_commit(tsm, &tsm->send_list_pool[0]);
				stk_llc_resend_add(tsm, &tsm->send_list_pool[0]);

				tt_timer_start(&tsm->llc_timer, CONFIG_WL_STD_TIME_ON_AIR_MS * 3);

				*llc_state_m = M_LLC_SEND_ALL;
			}

			break;
		}
		case M_LLC_SEND_ALL: {
			if (stk_llc_has_received_pack(tsm)) {
				pack = stk_llc_get_first_received(tsm);

				pr_assert(stk_get_pack_type(pack) != TYPE_SINGLE,
					  "%s: unknown pack, type %u\n", __func__, stk_get_pack_type(pack));

				pr_assert(stk_get_trans_id(pack) != tsm->trans_id,
					  "%s: trans_id mismatch, got %u\n", __func__, stk_get_trans_id(pack));

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

			pr_trace("[m:%u] send & recieved ACK, got id %u\n", tsm->trans_id, tsm->test_trans_id);

			*llc_state_m = M_LLC_IDLE;

			break;
		}
		case M_LLC_RESEND: {
			/* resend last packet (or series) */
			pr_assert(stk_llc_resend_get(tsm, &pack), "%s empty resend list?\n", __func__);

			stk_llc_resend_get(tsm, &pack);
			stk_llc_pack_commit(tsm, pack);

			tsm->retries_num++;

			pr_info("[m:%u] resending response, due to timeout pack, retries %u\n", tsm->trans_id, tsm->retries_num);

			*llc_state_m = M_LLC_SEND_ALL;

			break;
		}

		default:
			pr_assert(false, "%s:%d\n", __func__, __LINE__);
	}

	/* as current state changed - run FSM again as we can process another state */
	if (llc_state_m_prev != *llc_state_m)
		goto respin;
}

static void stk_m_llc_worker(struct stl_transmiter *tsm, enum llc_state *llc_state)
{
	struct wls_pack *pack;
	// temp dumb implementation:

	switch (tsm->llc_state) {
		case LLC_IDLE: {
			if (stk_m_has_pack_to_send(tsm)) {
				stk_m_prep_trans_id(tsm);
				stk_llc_single_prepare(tsm, &(tsm->send_list_pool[0]), send_data, 16);

				tt_timer_start(&tsm->llc_timer, CONFIG_WL_STD_TIME_ON_AIR_MS * 3);
				stk_llc_pack_commit(tsm, &tsm->send_list_pool[0]);

				tsm->retries_num = 0;
				*llc_state = LLC_SEND_ALL;
			}
			break;
		}
		case LLC_RECEIVED_ALL: {
			pr_info("sended and recieved ACK, retries %u, tr_id %u\n", tsm->retries_num, tsm->test_trans_id);
			*llc_state = LLC_IDLE;

			break;
		}
		case LLC_SEND_ALL: {
			if (stk_llc_has_received_pack(tsm)) {
				pack = stk_llc_get_first_received(tsm);

				if (stk_get_pack_type(pack) != TYPE_INTERNAL)
					pr_warn("%s: unknown pack\n", __func__);

				if (stk_get_trans_id(pack) != tsm->trans_id)
					pr_warn("%s: trans_id mismatch\n", __func__);

				tsm->test_trans_id = stk_get_trans_id(pack);

				stk_llc_mark_as_received(tsm);
				*llc_state = LLC_RECEIVED_ALL;

			} else if (tt_timer_is_timeouted(&tsm->llc_timer)) {
				/* timeouted, resend last packet */
				stk_llc_single_prepare(tsm, &(tsm->send_list_pool[0]), send_data, 16);

				tt_timer_start(&tsm->llc_timer, CONFIG_WL_STD_TIME_ON_AIR_MS * 3);
				stk_llc_pack_commit(tsm, &tsm->send_list_pool[0]);

				tsm->retries_num++;

				pr_info("resending, retries %u\n", tsm->retries_num);
			}

			break;
		}

		default:
			pr_assert(false, "%s:%d\n", __func__, __LINE__);
	}
}

void stk_m_comm_loop(struct stl_transmiter *tsm)
{
	enum mac_state l_mac_state;
	enum llc_state l_llc_state;

	stk_m_tsm_init(tsm);

	l_mac_state = tsm->mac_state;
	l_llc_state = tsm->llc_state;

	while (true) {
		stk_m_llc_worker(tsm, &l_llc_state);
		tsm->llc_state = l_llc_state;

		stk_mac_worker(tsm, &l_mac_state);
		tsm->mac_state = l_mac_state;
	}
}
