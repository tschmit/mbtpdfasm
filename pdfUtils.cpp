#include <stdlib.h>
#include <string.h>
#include <climits>

const char *memstr(const char *haystack, int lhaystak, const char *needle) {
	const char *pcc = haystack;
	const char *pccn, *pcch;

	while (pcc - haystack < lhaystak - strlen(needle)) {
		pccn = needle;
		pcch = pcc;
		while (*pccn != 0 && *pccn == *pcch) {
			++pccn;
			++pcch;
		}
		if (*pccn == 0)
			return pcc;
		pcc++;
	}

	return NULL;
}



int pdfGetParmValueAsInt(const char *parmName, const char *buf, int lbuf) {
const char *pcc;

	pcc = memstr(buf, lbuf, parmName);
	if (pcc != NULL) {
		return atoi(pcc + strlen(parmName));
	}

	return INT_MIN;
}
