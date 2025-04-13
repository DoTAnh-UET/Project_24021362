CFLAGS = -g -Wall -Wextra -std=c++11  
CLIBS = -I src/include -L src/lib -lmingw32 -lSDL2main -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lSDL2
OBJECTS = main.o  

all: test  

test: $(OBJECTS)  
	g++ $(CFLAGS) -o test $(OBJECTS) $(CLIBS)  

main.o: main.cpp  
	g++ $(CFLAGS) -c main.cpp $(CLIBS)  

run: test 
	./test

.PHONY: clean  
clean:  
	del -f *.o *.exe  