CC=gcc
CFLAGS= -c -Wall -std=gnu99
LDLIBS= -lpthread -lm
SOURCES=main.c mergesort.c generator.c sortfunc.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=mergesort


all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXECUTABLE) $(LDLIBS)


#$@ = name of target, $< = name of prerequisite
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)
