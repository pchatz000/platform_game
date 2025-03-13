#include <stdlib.h>
#include "set_utils.h"
#include "common_types.h"

Pointer set_find_eq_or_greater(Set set, Pointer value) {	
	if (set_find(set, value) != NULL) 
		return set_find(set, value);
		
	set_insert(set, value);

	SetNode x = set_next(set, set_find_node(set, value));

	DestroyFunc destroy = set_set_destroy_value(set, NULL);
	set_remove(set, value);
	set_set_destroy_value(set, destroy);
	
	
	if (x == SET_EOF) return NULL;
	return set_node_value(set, x);
}


Pointer set_find_eq_or_smaller(Set set, Pointer value) {
	if (set_find(set, value) != NULL) 
		return set_find(set, value);
		
	set_insert(set, value);

	SetNode x = set_previous(set, set_find_node(set, value));

	DestroyFunc destroy = set_set_destroy_value(set, NULL);
	set_remove(set, value);
	set_set_destroy_value(set, destroy);
	
	
	if (x == SET_BOF) return NULL;
	return set_node_value(set, x);
}