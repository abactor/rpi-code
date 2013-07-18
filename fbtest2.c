/*
 * fbtest2.c
 *
 * http://raspberrycompote.blogspot.ie/2013/01/low-level-graphics-on-raspberry-pi-part.html
 *
 * Original work by J-P Rosti (a.k.a -rst- and 'Raspberry Compote')
 *
 * Licensed under the Creative Commons Attribution 3.0 Unported License
 * (http://creativecommons.org/licenses/by/3.0/deed.en_US)
 *
 * Distributed in the hope that this will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <math.h>
#include <time.h>
int main(int argc, char* argv[])
{
  int fbfd = 0;
  struct fb_var_screeninfo vinfo;
  struct fb_fix_screeninfo finfo;
  long int screensize = 0;
  char *fbp = 0;
  // Open the file for reading and writing
  fbfd = open("/dev/fb0", O_RDWR);
  if (!fbfd) {
    printf("Error: cannot open framebuffer device.\n");
    return(1);
  }
  printf("The framebuffer device was opened successfully.\n");

  // Get fixed screen information
  if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
    printf("Error reading fixed information.\n");
  }

  // Get variable screen information
  if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
    printf("Error reading variable information.\n");
  }
  printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, 
         vinfo.bits_per_pixel );

	
	
  // map fb to user mem 
  screensize = finfo.smem_len;
  fbp = (char*)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
  if ((int)fbp == -1) {
    printf("Failed to mmap.\n");
  }
  else {
    // draw...
    // just fill upper half of the screen with something
    //memset(fbp, 0xfa, screensize/2);
    // and lower half of the screen with something else
    //memset(fbp + screensize/2, 0x18, screensize/2);
	unsigned int i,j;
	unsigned int pix_offset;
	float a=-2.24;
	float b=-0.65;
	float c=0.43;
	float d=-2.43;
	float x=-0.72;
	float y=-0.64;
	
	j=0;
	srand ( time(NULL) );
	a=((float)rand()/RAND_MAX)-((float)rand()/RAND_MAX);
		b=((float)rand()/RAND_MAX)-((float)rand()/RAND_MAX);
		c=((float)rand()/RAND_MAX)-((float)rand()/RAND_MAX);
		d=((float)rand()/RAND_MAX)-((float)rand()/RAND_MAX);
	for (i=0;i<0xFFFFFFF;i++){
		x=sin(a*y)-cos(b*x);
		y=sin(c*x)+cos(d*y);
		//fbp[i]=sprite
		//xres=1680
		//yres=1050
		
		int xint=(int)(x*vinfo.xres/2)+(vinfo.xres/2);
		int yint=(int)(y*vinfo.yres/2)+(vinfo.yres/2);
		//printf("x: %f and y %f\n",x,y);
		//printf("xint: %i and yint %i\n",xint,yint);
		xint=xint%vinfo.xres;
		yint=yint%vinfo.yres;
		pix_offset = xint *3 + yint * finfo.line_length;

	  // now this is about the same as fbp[pix_offset] = value
	  //*((char*)(fbp + pix_offset)) = 16 * x / vinfo.xres;
	  fbp[pix_offset]=j;
	  fbp[pix_offset+1]=(j>>8);
	  fbp[pix_offset+2]=(j>>16);
		if (i&0xFFFF){
			a=((float)rand()/RAND_MAX)-((float)rand()/RAND_MAX);
			b=((float)rand()/RAND_MAX)-((float)rand()/RAND_MAX);
			c=((float)rand()/RAND_MAX)-((float)rand()/RAND_MAX);
			d=((float)rand()/RAND_MAX)-((float)rand()/RAND_MAX);
			j++;
		}
		
	  
	}
  }

  // cleanup
  munmap(fbp, screensize);
  close(fbfd);
  return 0;
}
