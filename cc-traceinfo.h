#include <stdio.h>

void dump_stack(void);

//#define STK_TRACE_USE

/* emulator [PC] implementation */
#define pr_fmt(fmt)		"stk: " fmt
#define pr_efmt(fmt)		"stk: ERR: " fmt
#define pr_wfmt(fmt)		"stk: WARN: " fmt
#define pr_asstfmt(fmt)		"stk: ASST: %s:%d " fmt
#ifdef STK_TRACE_USE
	#define pr_trace(fmt, ...)	fprintf(stdout, pr_fmt(fmt), ##__VA_ARGS__)
#else
	#define pr_trace(fmt, ...)
#endif /* STK_TRACE_USE */
#define pr_info(fmt, ...)	fprintf(stdout, pr_fmt(fmt), ##__VA_ARGS__)
#define pr_assert(cond, fmt, ...)	({if (!(cond)) {fprintf(stderr, pr_asstfmt(fmt), __func__, __LINE__, ##__VA_ARGS__); dump_stack();}})
#define pr_err(fmt, ...)	fprintf(stderr, pr_efmt(fmt), ##__VA_ARGS__)
#define pr_warn(fmt, ...)	fprintf(stderr, pr_wfmt(fmt), ##__VA_ARGS__)
