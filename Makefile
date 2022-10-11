release:
	gcc -o analyzer src/*.c -Wall -Werror

debug:
	gcc -o analyzer -DDEBUG -g -ggdb src/*.c -Wall -Werror

preprocess:
	gcc -S -save-temps=obj -DDEBUG src/*.c -Wall -Werror

zip:
	zip -r staticAnalysis.zip .
