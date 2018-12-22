CFLAGS=-Wall -std=c11 -g -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
SRCS=main.c codegen.c parse.c token.c util.c dtype.c token_test.c parse_test.c util_test.c dtype_test.c
OBJS=$(SRCS:.c=.o)

m99cc: $(OBJS)
	gcc -o $@ $^

$(OBJS): m99cc.h $(SRCS)

test: m99cc
	./m99cc -test
	./m99cc -test_token
	./m99cc -test_parse
	./m99cc -test_dtype
	./test.sh

clean:
	rm -f m99cc *.o *~ tmp*
