all:
	gcc -O3 -std=c11 -Wall lodepng.c dragon.c -o dragon.exe

clean:
	rm dragon.exe
