#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "stdbool.h"
#include "lodepng.h"

typedef unsigned char Pixel;
typedef unsigned char Turn;

enum TURN
{
  LEFT,
  RIGHT
};

enum DIRECTION
{
  D_LEFT,
  D_UP,
  D_RIGHT,
  D_DOWN
};

Pixel* buf;
Turn* turnBuf;
unsigned BLACK = 0x000000;
unsigned WHITE = 0xFFFFFF;
unsigned blend1;
unsigned blend2;
bool dense;
int iters;
int maxTurns;
double partial;
int w;    //image width
int h;    //image height

int cx;   //center x (axis of rotation)
int cy;   //center y

unsigned lerp(unsigned c1, unsigned c2, double k)
{
  //rgba -> abgr, a omitted from a, b
  unsigned r = (c1 & 0xFF) * (1 - k) + (c2 & 0xFF) * k;
  unsigned g = ((c1 & 0xFF00) >> 8) * (1 - k) + ((c2 & 0xFF00) >> 8) * k;
  unsigned b = ((c1 & 0xFF0000) >> 16) * (1 - k) + ((c2 & 0xFF0000) >> 16) * k;
  return 0xFF000000 | (b << 16) | (g << 8) | r;
}

//precondition: turnBuf is populated
void createImage()
{
  //filename format: dragonN.png, where N is iteration count
  //calculate bounding box of resulting image by walking through the turns
  int x = 0;
  int y = 0;
  int minx = 0;
  int miny = 0;
  int maxx = 0;
  int maxy = 0;
  int dir = D_UP;
  for(int i = 0; i <= maxTurns; i++)
  {
    //move in current direction
    switch(dir)
    {
      case D_LEFT:
        x--;
        break;
      case D_UP:
        y--;
        break;
      case D_RIGHT:
        x++;
        break;
      case D_DOWN:
        y++;
        break;
      default:;
    }
    //update bounding box if outside
    minx = x < minx ? x : minx;
    miny = y < miny ? y : miny;
    maxx = x > maxx ? x : maxx;
    maxy = y > maxy ? y : maxy;
    //turn (except after last position update)
    if(i != maxTurns)
    {
      if(turnBuf[i] == LEFT)
      {
        dir = (dir + 1) % 4;
      }
      else
      {
        dir = (dir + 3) % 4;
      }
    }
  }
  //using the bounding box, compute final image size
  //note: in real image, each position update is 2 pixels
  //also add one pixel border on all 4 edges
  int iw = (maxx - minx) * (dense ? 1 : 2) + 3;
  int ih = (maxy - miny) * (dense ? 1 : 2) + 3;
  unsigned* pixels = malloc(iw * ih * sizeof(unsigned));
  //fill image with BLACK (most pixels will be in final product)
  for(int i = 0; i < iw * ih; i++)
    pixels[i] = (0xFF000000 | BLACK);
  //set starting point to stay in [0, iw) x [0, ih)
  x = (-minx) * (dense ? 1 : 2) + 1;
  y = (-miny) * (dense ? 1 : 2) + 1;
  dir = D_UP;
#define SET_PIXEL(x, y) {pixels[(x) + (y) * iw] = lerp(blend1, blend2, (double) i / maxTurns);} 
  for(int i = 0; i <= maxTurns * partial; i++)
  {
    //move in current direction, writing pixels on segment to WHITE
    SET_PIXEL(x, y);
    switch(dir)
    {
      case D_LEFT:
        SET_PIXEL(x - 1, y);
        x -= (dense ? 1 : 2);
        break;
      case D_UP:
        SET_PIXEL(x, y - 1);
        y -= (dense ? 1 : 2);
        break;
      case D_RIGHT:
        SET_PIXEL(x + 1, y);
        x += (dense ? 1 : 2);
        break;
      case D_DOWN:
        SET_PIXEL(x, y + 1);
        y += (dense ? 1 : 2);
        break;
      default:;
    }
    //turn (except after last position update)
    if(i != maxTurns)
    {
      if(turnBuf[i] == LEFT)
      {
        dir = (dir + 1) % 4;
      }
      else
      {
        dir = (dir + 3) % 4;
      }
    }
    else
    {
      SET_PIXEL(x, y);
    }
  }
  //get image filename
  char fname[64];
  sprintf(fname, "dragon%i.png", iters);
  lodepng_encode32_file(fname, (unsigned char*) pixels, iw, ih);
  free(pixels);
}

//precondition: turnBuf is allocated to exact size required
void getPath()
{
  int numTurns = 0;
  for(int i = 0; i < iters; i++)
  {
    //append RIGHT to the existing sequence
    turnBuf[numTurns] = RIGHT;
    //append inverse transpose of first numTurns turns
    int src = numTurns - 1;
    int dst = numTurns + 1;
    while(src >= 0)
    {
      turnBuf[dst] = (turnBuf[src] == LEFT) ? RIGHT : LEFT;
      src--;
      dst++;
    }
    //update numTurns
    numTurns = numTurns * 2 + 1;
  }
}

int main(int argc, const char** argv)
{
  //set default option values
  iters = 10;
  dense = false;
  blend1 = WHITE;
  blend2 = WHITE;
  partial = 1.0;
  int optStart = 0;
  if(argc >= 2)
  {
    int newIters;
    if(1 == sscanf(argv[1], "%i", &newIters) && newIters >= 0)
    {
      iters = newIters;
      optStart = 2;
    }
    else
    {
      puts("Defaulting to 10 iterations.");
      optStart = 1;
    }
  }
  for(int arg = optStart; arg < argc; arg++)
  {
    if(strcmp(argv[arg], "--dense") == 0)
    {
      dense = true;
    }
    else if(strcmp(argv[arg], "--blend") == 0)
    {
      if(argc >= arg + 2)
      {
        unsigned b1, b2;
        if(1 != sscanf(argv[arg + 1], "%x", &b1))
          puts("invalid first blend color, defaulting to white.");
        else
          blend1 = (b1 & 0xFF0000) >> 16 | (b1 & 0xFF00) | (b1 & 0xFF) << 16;
        if(1 != sscanf(argv[arg + 2], "%x", &b2))
          puts("invalid second blend color, defaulting to white.");
        else
          blend2 = (b2 & 0xFF0000) >> 16 | (b2 & 0xFF00) | (b2 & 0xFF) << 16;
      }
      else
        puts("Must follow --blend flag with two colors in the form 0xRRGGBB.");
      arg += 2;
    }
    else if(strcmp(argv[arg], "--partial") == 0)
    {
      if(arg == argc - 1)
      {
        puts("Must follow --partial option with a floating-point number between 0 and 1.");
        puts("Defaulting to 1.");
      }
      else
      {
        double p;
        if(1 != sscanf(argv[arg + 1], "%lf", &p) || p < 0 || p > 1)
          puts("Invalid partial path value, must be a floating-point number between 0 and 1.");
        else
          partial = p;
      }
    }
  }
  //use 1 byte per pixel, 1 = white, 0 = black
  //allocate initial buffer 
  w = 1000;
  h = 1000;
  maxTurns = (1 << iters) - 1;
  turnBuf = calloc(maxTurns, sizeof(Turn));
  getPath();
  createImage();
  free(turnBuf);
  //set one pixel at center of image (starting point)
  return 0;
}

