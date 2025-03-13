# paths
LIB = ../lib
INCLUDE = ../include
MODULES = ../modules

# compiler
CC = gcc

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CFLAGS = -Wall -Werror -g3 -I$(INCLUDE)
LDFLAGS = -lm

# Αρχεία .o
OBJS = state_test.o $(MODULES)/set_utils.o $(MODULES)/state.o $(LIB)/k08.a $(LIB)/libraylib.a
OBJS1 = utils_test.o $(MODULES)/set_utils.o $(LIB)/k08.a $(LIB)/libraylib.a

# Το εκτελέσιμο πρόγραμμα
EXEC = state_test

# Παράμετροι για δοκιμαστική εκτέλεση
ARGS =

# Παράμετροι της βιβλιοθήκης raylib
include $(LIB)/libraylib.mk


$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)

utils_test: $(OBJS1)
	$(CC) $(OBJS1) -o utils_test $(LDFLAGS)


clean:
	rm -f $(OBJS) $(EXEC)

run: $(EXEC)
	./$(EXEC) $(ARGS)

run1: utils_test
	./utils_test

valgrind: $(EXEC)
	valgrind --error-exitcode=1 --leak-check=full --show-leak-kinds=all ./$(EXEC) $(ARGS)

# Για να φτιάξουμε τα k08.a/libraylib.a τρέχουμε το make στο lib directory.
$(LIB)/%.a:
	$(MAKE) -C $(LIB) $*.a