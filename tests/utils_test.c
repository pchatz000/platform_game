#include "acutest.h"
#include "set_utils.h"

static int* create_int(int a) {
	int* pointer = malloc(sizeof(int));
	*pointer = a;
	return pointer;
}

static int compare_ints(Pointer a, Pointer b) {
	return *(int*)a-*(int*)b;
}


void test_set_utils() {

	Set foo = set_create(compare_ints, free);
	for(int i=0; i<10; i++) 
		if (i!=7)
			set_insert(foo, create_int(i));

	int x = 7;

	TEST_ASSERT((*(int*)set_find_eq_or_greater(foo, &x)) == 8);
	TEST_ASSERT((*(int*)set_find_eq_or_smaller(foo, &x)) == 6);

	x = 6;
	TEST_ASSERT((*(int*)set_find_eq_or_greater(foo, &x)) == 6);

	set_destroy(foo);
}

TEST_LIST = {
	{ "test set utils", test_set_utils },
	{ NULL, NULL } // τερματίζουμε τη λίστα με NULL
};