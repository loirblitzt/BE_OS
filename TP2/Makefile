CC=`which gcc`
LD=$(CC)
RM=`which rm` -f


SRC=$(wildcard *.c)
OBJ=$(SRC:.c=.o)

CFLAGS=-Wall -Wextra -g -std=c99 -D_POSIX_SOURCE -I./include/ 
LFLAGS=

TARGET=crond

all : $(TARGET)


$(TARGET) : $(OBJ)
	$(LD) $(LFLAGS) $^ -o $@ 

%.o : %.c
	$(CC) $(CFLAGS) $< -c

clean : 
	$(RM) $(BIN) $(OBJ) *~ semantic.cache

exe : 
	./$(BIN)
