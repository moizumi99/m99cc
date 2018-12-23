CFLAGS=-Wall -std=c11 -g -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast
SRCS=main.c codegen.c parse.c token.c util.c dtype.c token_test.c parse_test.c util_test.c dtype_test.c
OBJS=$(SRCS:.c=.o)

m99cc: $(OBJS)
	gcc -o $@ $^

$(OBJS): m99cc.h $(SRCS)

.PHONY: test
test: m99cc test.sh
	./m99cc -test
	./m99cc -test_token
	./m99cc -test_parse > /dev/null
	./m99cc -test_dtype
	./test.sh

.PHONY: clean
clean:
	rm -f m99cc *.o *~ tmp*
