FLAGS := -g -O3 -Wall -std=c17 -Iinclude

all: bin/ build/ bin/pi

bin/pi: build/pi.o
	${CC} ${FLAGS} $^ -o $@

build/pi.o: source/pi.c include/pi.h
	${CC} ${FLAGS} -c $< -o $@ -lpthread

%/:
	mkdir -p $@

clean:
	rm bin/* build/* pi1.txt pi2.txt shared_memory.tmp

distclean: clean
	rm -rf bin build
