#include <stdio.h>
#include <stdlib.h>

#include "ADTMap.h"
#include "ADTVector.h"
#include "ADTList.h"
#include "state.h"

#define OBJECT_NUM 4*PORTAL_NUM

// Οι ολοκληρωμένες πληροφορίες της κατάστασης του παιχνιδιού.
// Ο τύπος State είναι pointer σε αυτό το struct, αλλά το ίδιο το struct
// δεν είναι ορατό στον χρήστη.

struct state {
	Vector objects;			// περιέχει στοιχεία Object (Εμπόδια / Εχθροί / Πύλες)
	List portal_pairs;		// περιέχει PortalPair (ζευγάρια πυλών, είσοδος/έξοδος)
	Vector start_pos;		// κρατάει τα αρχικά positions των Objects (Αποθηκεύει objects)
							// Το παρακάτω map το χρησιμοποιώ για να μην κολλάνε οι enemies στα portals 
	Map last_col;			// για τον κάθε enemy, κρατώ το τελευταίο object που έκανε collide 
	struct state_info info;
};

// Ζευγάρια πυλών

typedef struct portal_pair {
	Object entrance;		// η πύλη entrance
	Object exit;			// οδηγεί στην exit
}* PortalPair;

static int* create_int(int a) {
	int* pointer = malloc(sizeof(int));
	*pointer = a;
	return pointer;
}

static int compare_ints(Pointer a, Pointer b) {
	return *(int*)a-*(int*)b;
}

static int compare_objects(Pointer a, Pointer b) {
	return a-b;
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

	state->objects = vector_create(0, free);		// Δημιουργία του vector
	state->start_pos = vector_create(0, free);		// Δημιουργία του vector που κρατάει τα αρχικά positions
	state->last_col = map_create(compare_objects, NULL, NULL); 

	for (int i = 0; i < OBJECT_NUM; i++) {
		// Δημιουργία του Object και προσθήκη στο vector
		Object obj = malloc(sizeof(*obj));
		vector_insert_last(state->objects, obj);

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
			
		// Φτιάχνω copy του αρχικού object για την αρχικοποίηση του state	

		Object copy = malloc(sizeof(*copy));
		*copy = *obj;
		vector_insert_last(state->start_pos, copy);
	}



	//Το map περιέχει κάθε στιγμή τα portals που δεν έχουν χρησιμοποιηθεί ως exits
	Map exits = map_create(compare_ints, free, NULL);	
	
	for (int i=0, portalcnt = 0; i<vector_size(state->objects); i++) {
		Object obj = (Object)vector_get_at(state->objects, i);

		if (obj->type == PORTAL) {
			map_insert(exits, create_int(portalcnt), obj);
			portalcnt++;
		}
	}

	state->portal_pairs = list_create(free);
	
	for (int i=0; i<vector_size(state->objects); i++) {
		Object obj = (Object)vector_get_at(state->objects, i);
		
		if (obj->type != PORTAL) continue;
		
		PortalPair pair = malloc(sizeof(*pair));
		pair->entrance = obj;

		int rand_portal = rand()%PORTAL_NUM;
		
		
		//Αυτό το loop βρίσκει την επόμενη διαθέσιμη πύλη για έξοδο
		for (int j=rand_portal; true; j=(j+1)%PORTAL_NUM) {
			Pointer element = map_find(exits, &j);
			if (element != NULL) {
				pair->exit = (Object)element;
				map_remove(exits, &j); 
				break;
			}
		}

		list_insert_next(state->portal_pairs, list_last(state->portal_pairs), pair);
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
	List obj_list = list_create(NULL);

	for (int i=0; i < vector_size(state->objects); i++) {
		Object cur = (Object)vector_get_at(state->objects, i);
		if (cur->rect.x >= x_from && cur->rect.x <= x_to) {
			list_insert_next(obj_list, list_last(obj_list), cur);
		}
	}
	return obj_list;
}

// Αρχικοποιεί το state μετα απο κάθε run.

void state_init(State state) {

	map_destroy(state->last_col);
	state->last_col = map_create(compare_objects, NULL, NULL);

	Object character = state->info.character;
	character->forward = true;
	character->jumping = false;
	character->rect.x = 0;
	character->rect.y = - character->rect.height;
	// Τα objects παίρνουν τις παλιές τους τιμές, δεν αλλάζουν όμως address
	for (int i=0; i<OBJECT_NUM; i++) {
		Object now = vector_get_at(state->objects, i);
		Object start = vector_get_at(state->start_pos, i);
		*now = *start;
	}
	return;
}

// Παίρνει ως παράμετρο ένα portal object και την κατεύθυνση που μπαίνει κάτι στο portal
// και επιστρέφει την έξοδο που οδηγεί το πρώτο.

Object find_pair(State state, Object start, bool forward) {
	// Την έξοδο την βρίσκω σειριακά, απλά τσεκάροντας αν έχω μπει σαν entrance ή σαν exit,
	// ανάλογα με την φορά του χαρακτήρα.

	Object end_normal = NULL;
	Object end_reverse = NULL;
	for (ListNode i = list_first(state->portal_pairs); 
		i!=LIST_EOF; 
		i = list_next(state->portal_pairs, i)) {
		PortalPair cur = (PortalPair)list_node_value(state->portal_pairs, i);
		if (start == cur->entrance) end_normal = cur->exit;
		if (start == cur->exit) end_reverse = cur->entrance;
	}
	if (forward) { // Αν κινείται προς τα δεξιά επιστρέφουμε την κανονική έξοδο.
		return end_normal;
	}
	else { // Διαφορετικά επιστρέφουμε την πύλη που την χρησιμοποιεί ως έξοδο, δηλ. την είσοδό της.
		return end_reverse;
	}
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
 
	if (state->info.paused && !(keys->n) ) return;
	
	Object character = state->info.character;

	//Collisions Χαρακτήρα
	for (int i=0; i<OBJECT_NUM; i++) {
		Object col_obj = (Object)vector_get_at(state->objects, i);
		
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
				if (((PortalPair)list_node_value(state->portal_pairs, list_last(state->portal_pairs)))->entrance->rect.x == 
					col_obj->rect.x) {
					state->info.wins++;
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

	portal_timer = (portal_timer) ? portal_timer-1 : 0;

	if (!state->info.playing) return ;

	//Collisions Εχθρών
	for (int i=0; i<OBJECT_NUM; i++) {
		Object obj = (Object)vector_get_at(state->objects, i);
		if (obj->type != ENEMY) continue;


		for (int j=0; j<OBJECT_NUM; j++) {
			Object col_obj = (Object)vector_get_at(state->objects, j);
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
						obj->rect.x = nxt_portal->rect.x;
						map_insert(state->last_col, obj, nxt_portal);
					}
					break;
				default:
					break;
			}
		} 
	}

	state->info.current_portal = 0;
	int portalcnt = 0;
	for (ListNode i=list_first(state->portal_pairs); i!=LIST_EOF; i=list_next(state->portal_pairs, i)) {
		Object portal = ((PortalPair)list_node_value(state->portal_pairs, i))->entrance;
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

	for (int i=0; i<OBJECT_NUM; i++) {
		Object obj = (Object)vector_get_at(state->objects, i);
		bool* dir = &(obj->forward);
		// Κίνηση 5 pixel στην ίδια κατεύθυνση για τους enemies.
		if (obj->type == ENEMY) obj->rect.x += (2*(int)(*dir)-1) * 5; 
	}

}

// Καταστρέφει την κατάσταση state ελευθερώνοντας τη δεσμευμένη μνήμη.

void state_destroy(State state) {
	map_destroy(state->last_col);
	list_destroy(state->portal_pairs);
	vector_destroy(state->start_pos);
	vector_destroy(state->objects);
	free(state->info.character);
	free(state);
}