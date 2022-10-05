release:
	gcc -o analyzer src/*.c -Wall -Werror

debug:
	gcc -o analyzer -DDEBUG -g -ggdb src/*.c -Wall -Werror
