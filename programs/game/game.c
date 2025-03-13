#include <stdio.h>
#include <stdlib.h>

#include "interface.h"
#include "raylib.h"
#include "state.h"

int main(void) {
	srand(0);
	interface_init();
	State state = state_create();
	while(!WindowShouldClose()) {
		interface_draw_frame(state);
		struct key_state keys = 
		 {	IsKeyDown(KEY_UP),
		 	IsKeyDown(KEY_LEFT),
		 	IsKeyDown(KEY_RIGHT),
		 	IsKeyPressed(KEY_ENTER),
		 	IsKeyPressed(KEY_N),
		 	IsKeyPressed(KEY_P)
		 };
		
		state_update(state, &keys);
	}
	state_destroy(state);
	interface_close();

	return 0;
}