CFLAGS=-Wall -std=c11 -g -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

m99cc: $(OBJS)
	gcc -o $@ $^

$(OBJS): m99cc.h $(SRCS)

test: m99cc
	./m99cc -test
	./test.sh

clean:
	rm -f m99cc *.o *~ tmp*
