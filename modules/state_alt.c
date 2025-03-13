#include <stdio.h>
#include <stdlib.h>

#include "ADTSet.h"
#include "ADTMap.h"
#include "ADTVector.h"
#include "ADTList.h"
#include "state.h"
#include "set_utils.h"

#define OBJECT_NUM 4*PORTAL_NUM

// Οι ολοκληρωμένες πληροφορίες της κατάστασης του παιχνιδιού.
// Ο τύπος State είναι pointer σε αυτό το struct, αλλά το ίδιο το struct
// δεν είναι ορατό στον χρήστη.

struct state {
	Set objects;			// περιέχει στοιχεία Object (Εμπόδια / Εχθροί / Πύλες)
	Map portal_pairs;		// περιέχει την έξοδο του portal όταν το object μπαίνει με φορά προς τα δεξιά
	Map reverse_pairs;		// περιέχει την έξοδο του portal όταν το object μπαίνει με φορά προς τα αριστερά
	Vector start_pos;		// κρατάει τα αρχικά positions των Enemies (float)
							// Το παρακάτω map το χρησιμοποιώ για να μην κολλάνε οι enemies στα portals 
	Map last_col;			// για τον κάθε enemy, κρατώ το τελευταίο object που έκανε collide 

	struct state_info info;
};

// Ο παρακάτω τύπος χρησιμοποιείται σε μια λίστα, που κρατάει τις αλλαγές που πρεπει να γίνουν
// στο set και τις κάνει μετά την διάσχιση του.

typedef struct enemy_tmp {
	Object obj;
	float new_x;
}* Enemy_Tmp;

static int* create_int(int a) {
	int* pointer = malloc(sizeof(int));
	*pointer = a;
	return pointer;
}

static float* create_float(float a) {
	float* pointer = malloc(sizeof(float));
	*pointer = a;
	return pointer;
}

static int compare_ints(Pointer a, Pointer b) {
	return *(int*)a-*(int*)b;
}

static int compare_pointers(Pointer a, Pointer b) {
	return a-b;
}


// Σύγκριση πρώτα κατά x και μετα κατα pointer value

static int compare_objects(Pointer a, Pointer b) {
	if (((Object)a)->rect.x == ((Object)b)->rect.x) 
		return a-b;
	return ((Object)a)->rect.x - ((Object)b)->rect.x;
}

// Δημιουργεί και επιστρέφει την αρχική κατάσταση του παιχνιδιού

State state_create() {
	// Δημιουργία του state
	State state = malloc(sizeof(*state));

	// Γενικές πληροφορίες
	state->info.current_portal = 0;			// Δεν έχουμε περάσει καμία πύλη
	state->info.wins = 0;					// Δεν έχουμε νίκες ακόμα
	state->info.playing = true;				// Το παιχνίδι ξεκινάει αμέσως
	state->info.paused = false;				// Χωρίς να είναι paused.

	// Πληροφορίες για το χαρακτήρα.
	Object character = state->info.character = malloc(sizeof(*character));
	character->type = CHARACTER;
	character->forward = true;
	character->jumping = false;

    // Ο χαρακτήρας (όπως και όλα τα αντικείμενα) έχουν συντεταγμένες x,y σε ένα
    // καρτεσιανό επίπεδο.
	// - Στο άξονα x το 0 είναι η αρχή στης πίστας και οι συντεταγμένες
	//   μεγαλώνουν προς τα δεξιά.
	// - Στον άξονα y το 0 είναι το "δάπεδο" της πίστας, και οι
	//   συντεταγμένες μεγαλώνουν προς τα _κάτω_.
	// Πέρα από τις συντεταγμένες, αποθηκεύουμε και τις διαστάσεις width,height
	// κάθε αντικειμένου. Τα x,y,width,height ορίζουν ένα παραλληλόγραμμο, οπότε
	// μπορούν να αποθηκευτούν όλα μαζί στο obj->rect τύπου Rectangle (ορίζεται
	// στο include/raylib.h).
	// 
	// Προσοχή: τα x,y αναφέρονται στην πάνω-αριστερά γωνία του Rectangle, και
	// τα y μεγαλώνουν προς τα κάτω, οπότε πχ ο χαρακτήρας που έχει height=38,
	// αν θέλουμε να "κάθεται" πάνω στο δάπεδο, θα πρέπει να έχει y=-38.

	character->rect.width = 70;
	character->rect.height = 38;
	character->rect.x = 0;
	character->rect.y = - character->rect.height;

	// Δημιουργία των objects (πύλες / εμπόδια / εχθροί) και προσθήκη στο vector
	// state->objects. Η πίστα περιέχει συνολικά 4*PORTAL_NUM αντικείμενα, από
	// τα οποία τα PORTAL_NUM είναι πύλες, και τα υπόλοια εμπόδια και εχθροί.

	state->objects = set_create(compare_objects, NULL);		
	state->start_pos = vector_create(0, free);
	state->last_col = map_create(compare_pointers, NULL, NULL);

	for (int i = 0; i < OBJECT_NUM; i++) {
		// Δημιουργία του Object και προσθήκη στο vector
		Object obj = malloc(sizeof(*obj));

		// Κάθε 4 αντικείμενα υπάρχει μια πύλη. Τα υπόλοιπα αντικείμενα
		// επιλέγονται τυχαία.

		if(i % 4 == 3) {							// Το 4ο, 8ο, 12ο κλπ αντικείμενο
			obj->type = PORTAL;						// είναι πύλη.
			obj->rect.width = 100;
			obj->rect.height = 5;

		} else if(rand() % 2 == 0) {				// Για τα υπόλοιπα, με πιθανότητα 50%
			obj->type = OBSTACLE;					// επιλέγουμε εμπόδιο.
			obj->rect.width = 10;
			obj->rect.height = 80;

		} else {
			obj->type = ENEMY;						// Και τα υπόλοιπα είναι εχθροί.
			obj->rect.width = 30;
			obj->rect.height = 30;
			obj->forward = false;					// Οι εχθροί αρχικά κινούνται προς τα αριστερά.
		}

		// Τα αντικείμενα είναι ομοιόμορφα τοποθετημένα σε απόσταση SPACING
		// μεταξύ τους, και "κάθονται" πάνω στο δάπεδο.

		obj->rect.x = (i+1) * SPACING;
		obj->rect.y = - obj->rect.height;
		set_insert(state->objects, obj);

		// Αν το object είναι enemy κρατώ την αρχική θέση στο vector
		if (obj->type == ENEMY) vector_insert_last(state->start_pos, create_float(obj->rect.x)); 

	}



	// Το map περιέχει κάθε στιγμή τα portals που δεν έχουν χρησιμοποιηθεί ως exits
	// H αντιστοιχία είναι από int(αριθμός portal) σε object
	Map exits = map_create(compare_ints, free, NULL);	
	int portalcnt = 0;
	for (SetNode i=set_first(state->objects); i!=SET_EOF; i=set_next(state->objects, i)) {
		Object obj = (Object)set_node_value(state->objects, i);

		if (obj->type == PORTAL) {
			map_insert(exits, create_int(portalcnt), obj);
			portalcnt++;
		}
	}
	
	// Δημιουργώ 2 maps ένα με κανονικά portal pairs και ένα με αντίστροφα
	state->portal_pairs = map_create(compare_objects, NULL, NULL);
	state->reverse_pairs = map_create(compare_objects, NULL, NULL);

	for (SetNode i=set_first(state->objects); i!=SET_EOF; i=set_next(state->objects, i)) {
		Object obj = (Object)set_node_value(state->objects, i);

		
		if (obj->type != PORTAL) continue;
		
		int rand_portal = rand()%PORTAL_NUM;

		Object exit;
		
		//Αυτό το loop βρίσκει την επόμενη διαθέσιμη πύλη για έξοδο
		for (int j=rand_portal; true; j=(j+1)%PORTAL_NUM) {
			Pointer element = map_find(exits, &j);
			if (element != NULL) {
				exit = (Object)element;
				map_remove(exits, &j); 
				break;
			}
		}
		map_insert(state->portal_pairs, obj, exit);
		map_insert(state->reverse_pairs, exit, obj);
	}

	map_destroy(exits);

	return state;
}

// Επιστρέφει τις βασικές πληροφορίες του παιχνιδιού στην κατάσταση state

StateInfo state_info(State state) {
	return &(state->info);
}

// Επιστρέφει μια λίστα με όλα τα αντικείμενα του παιχνιδιού στην κατάσταση state,
// των οποίων η συντεταγμένη x είναι ανάμεσα στο x_from και x_to.

List state_objects(State state, float x_from, float x_to) {
	// Φτιάχνω ένα object πριν απο τα objects που θέλω να επιστρέψω.
	List obj_list = list_create(NULL);
	Object point_a = malloc(sizeof(*point_a));
 	point_a->rect.x = x_from-1; // Το -1 το κάνω για να πάρω και κάποιο ακριανό object στην λίστα
 	// (επίσης αν δεν υπάρχει δημιουργεί segfault)

 	Set S = state->objects;

 	// Χρησιμοποιώ την set utils για να βρω το πρώτο στοιχείο της λίστας μου.
	SetNode start = set_find_node(S, set_find_eq_or_greater(S, point_a));

	free(point_a);

	for (SetNode i=start; i!=SET_EOF && i!=NULL; i = set_next(S, i)) {
		Object obj = set_node_value(S, i);
		if (obj->rect.x > x_to) break;
		list_insert_next(obj_list, list_last(obj_list), obj);
	}
	return obj_list;
}

// Αρχικοποιεί το state μετα απο κάθε run.

void state_init(State state) {
	Object character = state->info.character;
	character->forward = true;
	character->jumping = false;
	character->rect.x = 0;
	character->rect.y = - character->rect.height;

	// Για τους εχθρούς φτιάχνω μια λίστα που περιέχει τα objects τους, 
	// τους αφαιρώ απο το set και τους ξαναπροσθέτω με μια καινούρια θέση που παίρνω
	// απο το start_pos vector. Έτσι στην κάθε run στην ίδια εκτέλεση του παιχνιδιού θα έχουν την ίδια θέση.
 

	List enemy_list = list_create(NULL);
	for (SetNode i = set_first(state->objects); i!=SET_EOF; i = set_next(state->objects, i)) {
		Object obj = set_node_value(state->objects, i);
		if (obj->type == ENEMY)  
			list_insert_next(enemy_list, list_last(enemy_list), obj);
	}

	int vector_pos = 0;
	for (ListNode i = list_first(enemy_list); i!=LIST_EOF; i = list_next(enemy_list, i)) {
		Object obj = list_node_value(enemy_list, i);
		set_remove(state->objects, obj);
		obj->rect.x = *(float*)vector_get_at(state->start_pos, vector_pos);
		obj->forward = false;
		set_insert(state->objects, obj);
		vector_pos++;
	}

	list_destroy(enemy_list);

	return;
}

// Παίρνει ως παράμετρο ένα portal object και την κατεύθυνση που μπαίνει κάτι στο portal
// και επιστρέφει την έξοδο που οδηγεί το πρώτο.

Object find_pair(State state, Object start, bool forward) {
	if (forward) return map_find(state->portal_pairs, start);
	else return map_find(state->reverse_pairs, start);
}

// Επιστρέφει μία λίστα με τα γειτονικά objects του object που παίρνει ως παράμετρο.
// Αυτά πιθανώς συγκρούονται με το δοθέν

List check_col_list(State state, Object obj) {
	Set set = state->objects;
	List list = list_create(NULL);
	// Αν το δοθέν είναι χαρακτήρας τον προσθέτω στο set.
	if (obj->type == CHARACTER) 
		set_insert(state->objects, obj);

	SetNode find_point = set_find_node(state->objects, obj);
	
	// Βρίσκω τα διπλανά objects.
	SetNode bef = set_previous(state->objects, find_point);
	SetNode after = set_next(state->objects, find_point);
	
	// Και αν δεν είναι NULL τα προσθέτω στη λίστα
	if (bef != SET_BOF) list_insert_next(list, list_last(list), set_node_value(set, bef));
	if (after != SET_EOF) list_insert_next(list, list_last(list), set_node_value(set, after));

	if (obj->type == CHARACTER) 
		set_remove(state->objects, obj);


	return list;
}

// Ενημερώνει την κατάσταση state του παιχνιδιού μετά την πάροδο 1 frame.
// Το keys περιέχει τα πλήκτρα τα οποία ήταν πατημένα κατά το frame αυτό.

void state_update(State state, KeyState keys) {
	// Όταν ο χαρακτήρας μπαίνει σε portal θέτω ενα timer κάποιων frames,
	// στα οποίο δεν μπορεί να ξαναμπεί. Έτσι λύνω το πρόβλημα του να μπαινο-βγαίνει από ένα portal. 

	static int portal_timer;

	if (!state->info.playing) { 
		if (keys->enter) {
			state_init(state);
			state->info.playing = true;
		}
		return;
	}
	
	if (keys->p) state->info.paused ^= 1; //toggle
 	
 	// Αν είναι paused το game συνεχίζουμε στην συνάρτηση μόνο αν είναι το n πατημένο
	if (state->info.paused && !(keys->n)) return;
	
	Object character = state->info.character;
	// Παίρνουμε τα πιθανά objects που κάνουν collide.
	List to_check = check_col_list(state, character);

	//Collisions Χαρακτήρα
	for (ListNode i=list_first(to_check); i!=LIST_EOF; i=list_next(to_check, i)) {
		Object col_obj = (Object)list_node_value(to_check, i);
		if (!CheckCollisionRecs(character->rect, col_obj->rect)) continue;
		
		switch (col_obj->type) {
			case OBSTACLE:
				state->info.playing = false;
				break;
			case ENEMY:
				state->info.playing = false;
				break;
			case PORTAL:
				if (portal_timer != 0) continue;
				character->jumping = true;
				Object nxt_portal = find_pair(state, col_obj, character->forward);
				if (SPACING*PORTAL_NUM*4 == col_obj->rect.x) {
					state->info.wins++;
					list_destroy(to_check);
					state_init(state);
					return;
				}
				character->rect.x = nxt_portal->rect.x;
				portal_timer = 30;

				break;
			default:
				break;
		}
	}

	list_destroy(to_check);

	portal_timer = (portal_timer) ? portal_timer-1 : 0;

	if (!state->info.playing) return ;


	//Collisions Εχθρών
	// Η λίστα αυτή περιέχει στοιχεία τύπου Enemy_Tmp και σε αυτη κρατάω τις αλλαγές που γίνονται στο set.
	List enemy_list = list_create(free);


	for (SetNode i=set_first(state->objects); i!=SET_EOF; i=set_next(state->objects, i)) {
		Object obj = (Object)set_node_value(state->objects, i);
		if (obj->type != ENEMY) continue;
		to_check = check_col_list(state, obj);

		for (ListNode j=list_first(to_check); j!=LIST_EOF; j=list_next(to_check, j)) {
			Object col_obj = (Object)list_node_value(to_check, j);
			if (col_obj->type == ENEMY) continue;

			if (!CheckCollisionRecs(obj->rect, col_obj->rect)) continue;
			switch (col_obj->type) {
				case OBSTACLE:
					obj->forward^=1; // Αλλαγή κατεύθυνσης
					map_insert(state->last_col, obj, col_obj);
					break;
				case PORTAL:
					if (map_find(state->last_col, obj) != col_obj) {
						Object nxt_portal = find_pair(state, col_obj, obj->forward);
						Enemy_Tmp pair = malloc(sizeof(*pair));
						pair->obj = obj;
						pair->new_x = nxt_portal->rect.x;
						list_insert_next(enemy_list, list_last(enemy_list), pair);
						map_insert(state->last_col, obj, nxt_portal);
					}
					break;
				default:
					break;
			}
		}
		list_destroy(to_check); 
	}
	// Φτιάχνω ότι αλλαγές έγιναν στο παραπάνω loops.
	// Αυτές είναι τα enemies που μπήκανε σε portal
	for (ListNode i = list_first(enemy_list); i != LIST_EOF; i = list_next(enemy_list, i)) {
		Object obj = ((Enemy_Tmp)list_node_value(enemy_list, i))->obj;
		float new_x =  ((Enemy_Tmp)list_node_value(enemy_list, i))->new_x;
		set_remove(state->objects, obj);
		obj->rect.x = new_x;
		set_insert(state->objects, obj);

	}

	list_destroy(enemy_list);

	state->info.current_portal = 0;
	int portalcnt = 0;
	for (MapNode i=map_first(state->portal_pairs); i!=MAP_EOF; i=map_next(state->portal_pairs, i)) {
		Object portal = map_node_key(state->portal_pairs, i);
		if (portal->rect.x > character->rect.x) break;
		portalcnt++;
		state->info.current_portal = portalcnt;

	}

	float pixel_move_x = 0;

	if (keys->right) { 									// Είναι πατημένο το right
		if (character->forward) pixel_move_x = 12; 		// και πηγαίνει δεξιά
		else character->forward = true;					// και πηγαίνει αριστερά
	}
	else if (keys->left) {								// Είναι πατημένο το left
		if (!(character->forward)) pixel_move_x = 12;	// και πηγαίνει αριστερά
		else character->forward = false;				// και πηγαίνει δεξιά
	}
	else pixel_move_x = 7;								// Δεν είναι πατημένο κανένα άρα συνεχίζουμε με μικρό step

	character->rect.x += (2*(int)(character->forward)-1) * pixel_move_x;
	
	// Θέτεται σε κατάσταση άλματος μόνο όταν βρίσκεται στο έδαφος.
	if (keys->up && character->rect.y == -38) { 
		character->jumping = true;
	}
	
	float pixel_move_y = 0;
	
	// Εάν ο χαρακτήρας έχει ξεπεράσει το όριο άλματος.
	if (character->rect.y <= -250) character->jumping = false; 
	
	// Εάν ο χαρακτήρας δεν βρίσκεται στο έδαφος θα κινηθεί 15px προς κάποια κατεύθυνση.
	if (character->rect.y != -character->rect.height || character->jumping) pixel_move_y = 15;
	
	character->rect.y -= (2*(int)(character->jumping)-1) * pixel_move_y;

	// Κίνηση των enemies

	// Δημιουργώ λίστα με τους enemies, τους αφαιρώ, τους αλλάζω και τους ξαναπροσθέτω.
	
	enemy_list = list_create(NULL);
	for (SetNode i = set_first(state->objects); i!=SET_EOF; i = set_next(state->objects, i)) {
		Object obj = set_node_value(state->objects, i);
		if (obj->type == ENEMY)  
			list_insert_next(enemy_list, list_last(enemy_list), obj);
	}

	for (ListNode i = list_first(enemy_list); i!=LIST_EOF; i = list_next(enemy_list, i)) {
		Object obj = list_node_value(enemy_list, i);
		bool* dir = &(obj->forward);
		set_remove(state->objects, obj);
		obj->rect.x += (2*(int)(*dir)-1) * 5;
		set_insert(state->objects, obj);
	}
	list_destroy(enemy_list);
}

// Καταστρέφει την κατάσταση state ελευθερώνοντας τη δεσμευμένη μνήμη.

void state_destroy(State state) {
	vector_destroy(state->start_pos);
	map_destroy(state->last_col);
	map_destroy(state->portal_pairs);
	map_destroy(state->reverse_pairs);

	set_set_destroy_value(state->objects, free); // Για να γίνουν free τα objects
	set_destroy(state->objects);

	free(state->info.character);
	free(state);
}