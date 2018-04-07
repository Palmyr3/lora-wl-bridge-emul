#ifndef WL_ENV_CORE_H
#define WL_ENV_CORE_H

#include <stdint.h>
#include <stdbool.h>

#define CONFIG_WL_STD_TIME_ON_AIR_MS		100//1100ULL /* milliseconds */
#define CONFIG_WL_PACK_CONST_LEN		19

int	wl_emul_env_init(void);
void	wl_emul_env_destroy(void);
uint32_t wl_get_err_count(void);
uint8_t	wl_sendPacketTimeout(uint8_t id, uint8_t *payload, uint32_t timeout);
bool	wl_availableData(uint8_t id, uint32_t timeout);
uint8_t	wl_receivePacketTimeout(uint8_t id, uint32_t timeout, uint8_t *payload);

#endif /* WL_ENV_CORE_H */
