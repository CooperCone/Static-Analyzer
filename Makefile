CC=gcc
CPP=cpp
DBG_FLAGS=-DDEBUG -g -ggdb
CFLAGS=-Wall -Werror

release:
	$(CC) -o analyzer src/*.c $(CFLAGS)

debug:
	$(CC) -o analyzer $(DBG_FLAGS) src/*.c $(CFLAGS)

preprocess:
	gcc -S -save-temps=obj -DDEBUG src/*.c -Wall -Werror

zip:
	zip -r staticAnalysis.zip .

clean:
	rm analyzer
