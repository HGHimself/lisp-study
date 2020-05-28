IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=obj
LDIR =../lib

LIBS=-lm -ledit

_DEPS = include.h mpc.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = main.o mpc.o read.o eval.o lenv.o lval.o builtins.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

lispy: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~
#
# lispy: main.o mpc.o read.o eval.o lenv.o lval.o builtins.o
# 	ld -o lispy -lm -ledit main.o mpc.o read.o eval.o lenv.o lval.o builtins.o -macosx_version_min 10.13 -lSystem
# main.o: main.c
# 	gcc -c -o main.o main.c
# mpc.o: mpc.c
# 	gcc -c -o mpc.o mpc.c
# read.o: read.c
# 	gcc -c -o read.o read.c
# eval.o: eval.c
# 	gcc -c -o eval.o eval.c
# lenv.o: lenv.c
# 	gcc -c -o lenv.o lenv.c
# lval.o: lval.c
# 	gcc -c -o lval.o lval.c
# builtins.o: builtins.c
# 	gcc -c -o builtins.o builtins.c
