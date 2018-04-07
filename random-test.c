#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define CONFIG_WL_PACK_CONST_LEN		16


uint32_t rand_pack_bit(unsigned int *seedp)
{
	/* Return bit from 0 to CONFIG_WL_PACK_CONST_LEN * 8 */
	return ((rand_r(seedp) / (RAND_MAX / (CONFIG_WL_PACK_CONST_LEN * 8))));
}

bool b_rand(unsigned int *seedp, int true_score)
{
	return ((rand_r(seedp) / (RAND_MAX / 100)) < true_score);
}

int main(void)
{
	int true_pst = 2;
	unsigned int seedp = 1;
	int true_c = 0, false_c = 0;

	printf("RAND_MAX:%u\n", RAND_MAX);

	printf("%d\n", rand_r(&seedp));
	printf("%d\n", rand_r(&seedp));

	seedp = 1;
	printf("%d\n", rand_r(&seedp));
	printf("%d\n", rand_r(&seedp));

	printf("binary random, true %d%%\n", true_pst);
	seedp = 1;

	for (int i = 0; i < 100; i++)
		if ((rand_r(&seedp) / (RAND_MAX / 100)) < true_pst) {
			printf("1");
			true_c++;
		} else {
			printf("0");
			false_c++;
		}

	printf("\ntrue %d, false %d, true %d%%\n", true_c, false_c, (true_c * 100) / (true_c + false_c));

	printf("binary 2 random, true %d%%\n", true_pst);

	seedp = 1;
	true_c = 0;
	false_c = 0;

	for (int i = 0; i < 1000; i++)
		if (b_rand(&seedp, true_pst)) {
			printf("1");
			true_c++;
		} else {
			printf("0");
			false_c++;
		}

	printf("\ntrue %d, false %d, true %d%%\n", true_c, false_c, (true_c * 100) / (true_c + false_c));


	printf("rand_pack_bit random [0..%d]\n", CONFIG_WL_PACK_CONST_LEN * 8 - 1);
	for (int i = 0; i < 10000; i++)
		if (rand_pack_bit(&seedp) >= CONFIG_WL_PACK_CONST_LEN * 8)
			printf("FAIL\n");

	printf("%d\n", rand_pack_bit(&seedp));
}
