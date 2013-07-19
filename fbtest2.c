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
#include <stdint.h>
#include <sys/types.h>

typedef struct bmp{
#define ROWS	1680
#define COLUMNS 1050
#define COLOURS 3
	uint16_t 	sig;			//signature must be 0x4D4D
	uint32_t 	file_size_bytes;	
	uint16_t	rsvd1;
	uint16_t	rsvd2;
	uint32_t	data_file_offset;
	uint32_t	header_size;		//must be 40
	uint32_t	img_width;		//in pixels
	uint32_t	img_height;
	uint16_t	img_planes;		//must be 1
	uint16_t	bit_depth;
	uint32_t	compression;		//0=none, 1=rle-8, 2=rle-4
	uint32_t	img_size_bytes;		//image size with padding
	uint32_t	h_res;			//horizontal resolution
	uint32_t	v_res;
	uint32_t	num_colours;
	uint32_t	num_important_chars;
	
	uint8_t 	data[ROWS][COLUMNS][COLOURS];		
	

} __attribute__ ((__packed__)) bmp;


load_bmp(char * file_name, bmp * img){
	int ret;
	uint32_t xpos;
	uint32_t ypos;
	uint32_t pnum;
	uint32_t width, height;
	FILE *fp;
	fp = fopen(file_name, "rb");
	bmp temp_img;
	
	if(fp == NULL) {
		printf("CANNOT OPEN FILE.  PROGRAM EXITING");
		return 1;
	}
	else {
		ret=fread(img, 1, 54, fp);
		printf("%#06x Should be 0x4D4d\n",img->sig);
		printf("%u wide, %u high\n",img->img_width,img->img_height);
		width=img->img_width;
		height=img->img_height;
		printf("%u hres, %u vres, %hu bit depth\n",img->h_res,img->v_res,img->bit_depth);
		printf("data offest is: %u\n",img->data_file_offset); 
		pnum=img->img_width*img->img_height*img->bit_depth/8;
		printf("%u bytes long\n",pnum);
		ret=fread(&img->data[0][0][0],1,pnum,fp);
		printf("%u bytes read from %s\n",ret,(char*)&file_name);
/*		temp_img=*img;
		
		for(xpos=0;xpos<10;xpos++){
			for (ypos=0;ypos<10;ypos++){
				temp_img.data[width-xpos-1][height-ypos-1][0]=img->data[xpos][ypos][0];
				temp_img.data[width-xpos-1][height-ypos-1][1]=img->data[xpos][ypos][1];
				temp_img.data[width-xpos-1][height-ypos-1][2]=img->data[xpos][ypos][2];
			}			
		}
		*img=temp_img;
		
*/
	}

	fclose(fp);
}

int main(int argc, char* argv[])
{
	  int fbfd = 0;
	  struct fb_var_screeninfo vinfo;
	  struct fb_fix_screeninfo finfo;
	  long int screensize = 0;
	  char *fbp = 0;
	  bmp my_image;
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

	
		//fbp[i]=sprite
		//xres=1680
		//yres=1050
		load_bmp("test.bmp",&my_image);
		memcpy(fbp,&my_image.data[0][0][0],ROWS*COLUMNS*COLOURS);
		//int xint=(int)(x*vinfo.xres/2)+(vinfo.xres/2);
		//int yint=(int)(y*vinfo.yres/2)+(vinfo.yres/2);
		//printf("x: %f and y %f\n",x,y);
		//printf("xint: %i and yint %i\n",xint,yint);
		//xint=xint%vinfo.xres;
		//yint=yint%vinfo.yres;
		//pix_offset = xint *3 + yint * finfo.line_length;

	  // now this is about the same as fbp[pix_offset] = value
	  //*((char*)(fbp + pix_offset)) = 16 * x / vinfo.xres;
	  /*fbp[pix_offset]=j;
	  fbp[pix_offset+1]=(j>>8);
	  fbp[pix_offset+2]=(j>>16);
		*/	
		
	  
	}
  

  // cleanup
  munmap(fbp, screensize);
  close(fbfd);
  return 0;
}
