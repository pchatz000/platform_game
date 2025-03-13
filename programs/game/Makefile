# paths
LIB = ../../lib
INCLUDE = ../../include
MODULES = ../../modules

# compiler
CC = gcc

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CFLAGS = -Wall -Werror -g3 -I$(INCLUDE)
LDFLAGS = -lm

# Αρχεία .o
#OBJS = game.o $(MODULES)/interface.o $(MODULES)/state.o $(LIB)/k08.a $(LIB)/libraylib.a
OBJS = game.o $(MODULES)/set_utils.o $(MODULES)/interface.o $(MODULES)/state.o $(LIB)/k08.a $(LIB)/libraylib.a

# Το εκτελέσιμο πρόγραμμα
EXEC = game

# Παράμετροι για δοκιμαστική εκτέλεση
ARGS =

# Παράμετροι της βιβλιοθήκης raylib
include $(LIB)/libraylib.mk


$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	./$(EXEC) $(ARGS)

# Για να φτιάξουμε τα k08.a/libraylib.a τρέχουμε το make στο lib directory.
$(LIB)/%.a:
	$(MAKE) -C $(LIB) $*.a