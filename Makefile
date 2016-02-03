CC=clang

ODIR=obj
_OBJ=main.o
OBJ=$(patsubst %,$(ODIR)/%,$(_OBJ))

SDIR=src

_DEPS=
DEPS=$(patsubst %,$(SDIR)/%,$(_DEPS))

LIBS=-lxml2

CFLAGS=-ggdb -I$(SDIR)

$(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

main:$(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

run:
	./main data/Fader_Jakob.xml
