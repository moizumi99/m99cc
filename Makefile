CFLAGS=-Wall -std=c11 -g -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

9cc: $(OBJS)
	gcc -o $@ $^

$(OBJS): 9cc.h $(SRCS)

test: 9cc
	./9cc -test
	./test.sh

clean:
	rm -f 9cc *.o *~ tmp*
