all: apollo
apollo:
	g++ -std=c++17 -W -O3 -march=native -o apollo *.c *.cpp -I ./glad/include/ -I TODO/include/ -lglfw -lassimp -lGL
.PHONY:
	clean all
clean:
	rm apollo