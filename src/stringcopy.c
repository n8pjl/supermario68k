#include "stringcopy.h"
#include <stdint.h>
#include <string.h>

// StringCopy copies the string at dest to src, returning the length of the
// string, including the null termination character This was made to replace
// numerous calls to strcpy and strlen, this to save memory. Proved to save
// memory in all these cases, but also in most cases it replaces a single
// strcpy()

int16_t StringCopy(char *dest, char *src)
{
	strcpy(dest, src);

	return strlen(src) + 1;
}
