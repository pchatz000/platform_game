//////////////////////////////////////////////////////////////////
//
// Test για το state.h module
//
//////////////////////////////////////////////////////////////////

#include "acutest.h"			// Απλή βιβλιοθήκη για unit testing

#include "state.h"

void test_state_create() {
	State state = state_create();
	TEST_ASSERT(state != NULL);

	StateInfo info = state_info(state);
	TEST_ASSERT(info != NULL);

	TEST_ASSERT(info->character->type == CHARACTER); 
	TEST_ASSERT(info->character->rect.x == 0);
	TEST_ASSERT(info->character->forward == true);
	TEST_ASSERT(info->character->jumping == false);
	TEST_ASSERT(info->current_portal == 0);
	TEST_ASSERT(info->wins == 0);
	TEST_ASSERT(info->playing);
	TEST_ASSERT(!info->paused);

	List obj_list = state_objects(state, 0.0f, 4*PORTAL_NUM*SPACING);
	//Έλεγχος για τον αριθμό όλων των αντικειμένων.
	TEST_ASSERT(list_size(obj_list) == 4*PORTAL_NUM);
	//Έλεγχος για το αν είναι ευθυγραμμισμένα και σε απόσταση SPACING μεταξύ τους. 
	int i = 1;
	for (ListNode x = list_first(obj_list); x != LIST_EOF; x = list_next(obj_list, x)) {
		Object obj = (Object)list_node_value(obj_list, x);
		TEST_ASSERT(i * SPACING == obj->rect.x);
		i++;
	}
	list_destroy(obj_list);

	obj_list = state_objects(state, 2.0f*SPACING, 7.0f*SPACING);
	//Αντικείμενα στις θέσεις 2*spacing, 3*spacing κλπ.
	TEST_ASSERT(list_size(obj_list) == 6); 
	list_destroy(obj_list);

	obj_list = state_objects(state, 2.0f*SPACING, 2.0f*SPACING);
	TEST_ASSERT(list_size(obj_list) == 1); 
	list_destroy(obj_list);
	state_destroy(state);
}

void test_state_update() {
	State state = state_create();
	TEST_ASSERT(state != NULL && state_info(state) != NULL);
	// Πληροφορίες για τα πλήκτρα (αρχικά κανένα δεν είναι πατημένο)
	struct key_state keys = { false, false, false, false, false, false };
	
	// Χωρίς κανένα πλήκτρο, ο χαρακτήρας μετακινείται 7 pixels μπροστά
	Rectangle old_rect = state_info(state)->character->rect;
	state_update(state, &keys);
	Rectangle new_rect = state_info(state)->character->rect;

	
	TEST_ASSERT( new_rect.x == old_rect.x + 7 && new_rect.y == old_rect.y );

	// Με πατημένο το δεξί βέλος, ο χαρακτήρας μετακινείται 12 pixes μπροστά
	keys.right = true;
	old_rect = state_info(state)->character->rect;
	state_update(state, &keys);
	new_rect = state_info(state)->character->rect;

	TEST_ASSERT( new_rect.x == old_rect.x + 12 && new_rect.y == old_rect.y );


	// Up και Left -> αλλαγή κατεύθυνσης και άλμα κατά 15px
	keys.right = false; keys.left = true; keys.up = true;
	old_rect = state_info(state)->character->rect;
	state_update(state, &keys);
	new_rect = state_info(state)->character->rect;

	TEST_ASSERT(state_info(state)->character->forward == false);
	TEST_ASSERT(new_rect.x == old_rect.x && new_rect.y == old_rect.y - 15);
	
	state_update(state, &keys);
	new_rect = state_info(state)->character->rect;
	TEST_ASSERT(new_rect.x == old_rect.x-12 && new_rect.y == old_rect.y - 30);
	state_destroy(state);

}


// Λίστα με όλα τα tests προς εκτέλεση
TEST_LIST = {
	{ "test_state_create", test_state_create },
	{ "test_state_update", test_state_update },

	{ NULL, NULL } // τερματίζουμε τη λίστα με NULL
};