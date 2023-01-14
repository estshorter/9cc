CFLAGS=-std=gnu17 -g -static -Wall -Wextra
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

9cc: $(OBJS)
	$(CC) -o 9cc $(OBJS) $(LDFLAGS)

$(OBJS): 9cc.h

test: 9cc
	./test.sh

clean: 
	rm -f 9cc *.o *~ tmp*

.PHONY: test clean
