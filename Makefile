all: main

clean:
	rm -rf *.log *.o main

ipc.o: ipc.c context.h ipc.h pipes.h
	clang -c -std=c99 -pedantic -Werror -Wall ipc.c -o ipc.o

main.o: main.c common.h context.h ipc.h pa2345.h pipes.h
	clang -c -std=c99 -pedantic -Werror -Wall main.c -o main.o

main: ipc.o main.o pipes.o queue.o
	clang ipc.o main.o pipes.o queue.o -o main -Llib64 -lruntime

pipes.o: pipes.c ipc.h pipes.h
	clang -c -std=c99 -pedantic -Werror -Wall pipes.c -o pipes.o

queue.o: queue.c ipc.h queue.h
	clang -c -std=c99 -pedantic -Werror -Wall queue.c -o queue.o
