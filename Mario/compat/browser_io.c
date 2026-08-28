#include "browser_io.h"

#include <stdlib.h>

void free_menu_strings(char **arr, size_t arr_len)
{
	for (size_t i = 0; i < arr_len; i++) {
		if (arr[i])
			free(arr[i]);
	}
}