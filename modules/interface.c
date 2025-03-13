#include <math.h>

#include "interface.h"
#include "ADTQueue.h"
#include "ADTVector.h"
#include "ADTSet.h"
#include "ADTStack.h"

#define GROUND_HEIGHT 0.85f*SCREEN_HEIGHT
#define CHAR_GAME_X	0.25f*SCREEN_WIDTH
#define CHAR_GAME_Y GROUND_HEIGHT


typedef struct Particle {
	float x;
	float y;
	float vx;
	float ax;
	float age;
	float lifespan;
}* Particle;

static int particle_compare(Pointer a, Pointer b) {
	return a-b;
}

Particle particle_create(int xmin, int xmax) {
	Particle foo = malloc(sizeof(*foo));
	foo->x = (rand()+xmin)%(xmax+1);
	foo->y = GROUND_HEIGHT;
	foo->vx = rand()%11-5;
	foo->ax = 0.3f;
	foo->age = 0;
	foo->lifespan = GetRandomValue(45, 60);
	return foo;
}

void system_update(Set system) {
	Stack removes = stack_create(NULL);

	for (SetNode i = set_first(system); i!=SET_EOF; i=set_next(system, i)) {
		Particle foo = (Particle)set_node_value(system, i);
		if (foo->age > foo->lifespan-3) {
			stack_insert_top(removes, foo);
			continue;
		}
		if (foo->vx < -5) foo->ax = 0.2f;
		if (foo->vx > 5) foo->ax = -0.2f;
		foo->vx += foo->ax;
		foo->age += 1.0f;
		foo->y -= 2.0f;
	}

	while (stack_size(removes)) {
		Pointer tmp = stack_top(removes);
		stack_remove_top(removes);
		set_remove(system, set_find(system, tmp));
	}
}

void draw_system(Set system, float x) {
	for (SetNode i=set_first(system); i!=SET_EOF; i=set_next(system, i)) {
		Particle foo = set_node_value(system, i);
		Color tmp = MAROON, tmp1 = ORANGE;
		tmp.a = 230-230*foo->age/foo->lifespan;
		tmp1.a = 230-230*foo->age/foo->lifespan;
		float radius = 6-5*foo->age/foo->lifespan;
		DrawCircle(foo->x+foo->vx+x, foo->y-6, radius+1, tmp);
		DrawCircle(foo->x+foo->vx+x, foo->y-6, radius, tmp1);
	}
}


Vector systems;

Music music;
int frametimer;
int music_paused = 0;

void MyDrawText(const char *text, int posX, int posY, int fontSize) {
	DrawText(text, posX, posY, fontSize, ORANGE);
	DrawText(text, posX-fontSize/20, posY-fontSize/20, fontSize, YELLOW);
}

// Αρχικοποιεί το interface του παιχνιδιού

void interface_init() {
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "game");
	SetWindowPosition(0,0);
	SetTargetFPS(60);

    InitAudioDevice();
	music = LoadMusicStream("../../resources/Kirby dream land theme song.mp3");
	PlayMusicStream(music);
    systems = vector_create(0, NULL);
	for (int i=0; i<5; i++) 
		vector_insert_last(systems, set_create(particle_compare, NULL));
}

// Κλείνει το interface του παιχνιδιού
void interface_close() {
	UnloadMusicStream(music);
	CloseAudioDevice();
	CloseWindow();

}

// Μετατρέπει συντεταγμένες παιχνιδιού σε οθόνης. (gtr = game to raylib)
Rectangle gtr(Object obj, Object character) {
	return (Rectangle){ 
		.x = obj->rect.x - character->rect.x + CHAR_GAME_X,
		.y = obj->rect.y + GROUND_HEIGHT,
		.width = obj->rect.width,
		.height = obj->rect.height};
}

void print_rectangle(Rectangle foo) {
	printf("------\nx: %f\ny: %f\nwidth: %f\nheight: %f\n", foo.x, foo.y, foo.width, foo.height);
}

float bounce = -5, add = 0.3;

// Σχεδιάζει ένα frame με την τωρινή κατάσταση του παιχνδιού
void interface_draw_frame(State state) {
	if (IsKeyPressed(KEY_M)) {
		if (music_paused) ResumeMusicStream(music);
		else PauseMusicStream(music);
		music_paused ^= 1; 
	}

	if (bounce <= -5) add = 0.2;
	if (bounce >= 5) add = -0.2;
	bounce+=add;

	StateInfo info = state_info(state);
	Object character = info->character; 

	UpdateMusicStream(music);

	BeginDrawing();

	ClearBackground(RAYWHITE);

	for (int i=0; i<5; i++) { // Δημιουργώ 4 systems απο particles, ώστε να μην είναι τα ίδια σε κάθε portal
		system_update((Set)vector_get_at(systems, i));
		if (!(rand()%5)) {
			set_insert((Set)vector_get_at(systems, i), particle_create(0, 100));
		}
	}

	// Ουρανός
	DrawRectangleGradientV(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, MAROON, BLUE);

	// Έδαφος
	DrawRectangleGradientV(0, GROUND_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT-GROUND_HEIGHT, GREEN, (Color){22, 47, 19, 255});
	
	// Ήλιος
	DrawCircleGradient(SCREEN_WIDTH, -15, SCREEN_WIDTH*0.2f, ORANGE, GOLD);

	// Χαρακτήρας
	Color c1=ORANGE, c2=RED, c3=RED, c4=GOLD;

	if (!character->forward) {
		c1 = GOLD;
		c4 = ORANGE;
	}

	DrawRectangleGradientEx(gtr(character, character), c1, c2, c3, c4);
	



	DrawFPS(850, 10);
	char portal_text[100], wins_text[100];
	sprintf(wins_text, "wins: %d", info->wins);

	MyDrawText(wins_text, 12, 12, 45);

	List objs = state_objects(state, character->rect.x-CHAR_GAME_X, character->rect.x + SCREEN_WIDTH);
	if (list_size(objs) != 0)
		for (ListNode i = list_first(objs); i != LIST_EOF; i = list_next(objs, i)) {
			Object obj = list_node_value(objs, i);
			Color obj_color1, obj_color2;
			if (obj->type == ENEMY) {
				obj_color1 = DARKBROWN;
				obj_color2 = BLACK;
				DrawRectangleGradientEx(gtr(obj, character), obj_color1, obj_color2, obj_color2, obj_color1);
			}
			else if (obj->type == OBSTACLE) {
				obj_color1 = GREEN;
				obj_color2 = LIME;
				DrawRectangleGradientEx(gtr(obj, character), obj_color1, obj_color2, obj_color2, obj_color1);
			}
			else {
				sprintf(portal_text, "%d", (int)obj->rect.x/(4*SPACING));
				obj_color1 = MAROON;
				obj_color2 = YELLOW;
				float print_pos = gtr(obj, character).x + obj->rect.width/2 - MeasureText(portal_text, 50)/2;
				draw_system((Set)vector_get_at(systems, ((int)obj->rect.x/(4*SPACING))%5), gtr(obj, character).x);
				DrawRectangleGradientEx(gtr(obj, character), obj_color1, obj_color2, obj_color2, obj_color1);
				MyDrawText(portal_text, print_pos, GROUND_HEIGHT-70+bounce, 50);
			}

		}
	list_destroy(objs);


	if (!info->playing)
		MyDrawText("[game over]", (SCREEN_WIDTH/2)-MeasureText("[game over]", 100)/2, SCREEN_HEIGHT/2-50+bounce, 100);
	else if (info->paused) 
		MyDrawText("[paused]", (SCREEN_WIDTH/2)-MeasureText("[paused]", 70)/2, SCREEN_HEIGHT/2-35+bounce, 70);
	EndDrawing();
}