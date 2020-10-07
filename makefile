PROJECT = SCOPY
CFLAGS += -Wall
JUNK = *.o *~

.PHONY: run

all: scopy.o
	gcc -o scopy scopy.c $(FLAGS)

clean:
	$(RM) $(TARGET) $(JUNK)
