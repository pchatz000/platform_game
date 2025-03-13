#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "state.h"
#include "raylib.h"

#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 540

// Αρχικοποιεί το interface του παιχνιδιού
void interface_init();

// Κλείνει το interface του παιχνιδιού
void interface_close();

// Σχεδιάζει ένα frame με την τωρινή κατάσταση του παιχνδιού
void interface_draw_frame(State state);