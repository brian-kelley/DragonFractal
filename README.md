# DragonFractal
Simple, fast Dragon Fractal generator

-Pass the number of iterations to dragon.exe (default is 10).

-Program outputs dragonN.png, where N is the number of iterations.

-Output image is just big enough to hold the fractal, with a 1 pixel border.

-Pass --dense to make line segments 1 pixel long instead of 2

-Pass --blend 0xRRGGBB 0xRRGGBB to blend from the 1st color to the 2nd along the path

-Pass --partial k to only draw k of the path (0.5 for half the path, etc)

(v1.0): 1-14-17: initial implementation done

(v1.1): 1-15-17: added blend, dense and partial options
