#include "../include/test.h"

#include "../vendor/libfun/include/hashmap.h"

#include <stdio.h>


int main()
{
	struct fhashmap map;

	fhashmap_xinit(&map, sizeof(int));
	fhashmap_xinsert(&map, "test", &(int) { add(2, 2) });

	printf("2 + 2 = %d, and value stored in the hashmap is %d\n",
	       add(2, 2), *(int *) fhashmap_get(&map, "test"));
}
