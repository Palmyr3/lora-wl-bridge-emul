#include <stdio.h>

/* emulator [PC] implementation */
#define pr_fmt(fmt)		"stk: " fmt
#define pr_efmt(fmt)		"stk: ERR: " fmt
#define pr_wfmt(fmt)		"stk: WARN: " fmt
#define pr_asstfmt(fmt)		"stk: ASST: " fmt
#define pr_trace(fmt, ...)	fprintf(stdout, pr_fmt(fmt), ##__VA_ARGS__)
#define pr_info(fmt, ...)	fprintf(stdout, pr_fmt(fmt), ##__VA_ARGS__)
#define pr_assert(cond, fmt, ...)	({if (!(cond)) fprintf(stderr, pr_asstfmt(fmt), ##__VA_ARGS__);})
#define pr_err(fmt, ...)	fprintf(stderr, pr_efmt(fmt), ##__VA_ARGS__)
#define pr_warn(fmt, ...)	fprintf(stderr, pr_wfmt(fmt), ##__VA_ARGS__)
