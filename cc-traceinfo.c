#include "cc-traceinfo.h"

#include <execinfo.h>
#include <unistd.h>

void dump_stack(void)
{
	int nptrs;
	#define SIZE 100
	void *buffer[100];
//	char **strings;
//	int j;

	nptrs = backtrace(buffer, SIZE);
	printf("backtrace() returned %d addresses\n", nptrs);

	backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO);

	/* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
	would produce similar output to the following: */

//	strings = backtrace_symbols(buffer, nptrs);
//	if (strings == NULL) {
//	perror("backtrace_symbols");
//	exit(EXIT_FAILURE);
//	}
//
//	for (j = 0; j < nptrs; j++)
//		printf("%s\n", strings[j]);
//
//	free(strings);
}
