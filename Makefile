all:
	gcc -O3 -Wall lodepng.c dragon.c -o dragon.exe

clean:
	rm dragon.exe
