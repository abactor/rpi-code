/*
 * fbtest2.c
 *
*Abactor added bitmap (.bmp) file loading and parsing, and blitting to screen
*The bitmap parser is specific to 24-bit colours (one byte for each of B-G-R)
*To ensure that your screen is setup properly for this example, run "fbset -g 1680 1050 1680 1050 24" from the terminal before running ./fbtest2
*"fbset -depth 24" will change only the color bit-depth
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
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/ioctl.h>

typedef struct bmp{
#define COLUMNS	1680	//these value can be redifined, or changed at runtime based on file paramters
#define ROWS 1050	//use c99 or malloc to allow for dynamic memory allocation
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
	uint32_t	v_res;			//vertical resolution
	uint32_t	num_colours;
	uint32_t	num_important_chars;
	
	uint8_t 	data[ROWS][COLUMNS][COLOURS];		
	

} __attribute__ ((__packed__)) bmp;

bmp temp_img;	//this is a local/temporary variable to load_bmp(), yet causes a segfault due to stack size issues on large files if instantiated within load_bmp()


int load_bmp(char * file_name, bmp * img){
	#define DATA_OFFSET 54					//.bmp files have a static offset

	int ret;
	uint32_t xpos;
	uint32_t ypos;
	uint32_t pnum;
	FILE *fp;
	fp = fopen(file_name, "rb");				//open file_name.bmp as binary readable
	
	if(fp == NULL) {
		printf("CANNOT OPEN FILE.  PROGRAM EXITING");
		return 1;
	}
	else {
		ret=fread(img, 1, DATA_OFFSET, fp);		//read 54 bytes into bmp struct, pointed to by img
		printf("%#06x Should be 0x4D4d\n",img->sig);	//sanity check on bmp data
		printf("%u wide, %u high\n",img->img_width,img->img_height);
		printf("%u hres, %u vres, %hu bit depth\n",img->h_res,img->v_res,img->bit_depth);
		printf("data offest is: %u\n",img->data_file_offset);	//data offset should coincide with DATA_OFFSET 
		pnum=img->img_width*img->img_height*img->bit_depth/8;	//double-checking number of bytes in file
		printf("%u bytes long\n",pnum);				//should match number of bytes read
		ret=fread(&img->data[0][0][0],1,pnum,fp);
		printf("%u bytes read from %s\n",ret,(char*)file_name);
		temp_img=*img;					//store a temporary copy of the struct

		for (ypos=0;ypos<COLUMNS;ypos++){		//need to re-arrange struct.data[][][]
			for(xpos=0;xpos<ROWS;xpos++){
				//bmp files are BGR and frame buffer is RGB so swap the Red and Blue colour indices
				//bmp files are scanned from bottom left to the right and up, so vertically swap array 
				//this will likely be quicker to do a memcpy every row at a time instead of pixel-by-pixel copying

				img->data[xpos][ypos][0]=temp_img.data[ROWS-xpos-1][ypos][2];
				img->data[xpos][ypos][1]=temp_img.data[ROWS-xpos-1][ypos][1];
				img->data[xpos][ypos][2]=temp_img.data[ROWS-xpos-1][ypos][0];
			}
		}
	}

	fclose(fp);
	return 0;
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

		int ret=load_bmp("test.bmp",&my_image);					//open file and load into "bmp" struct
		if (ret==0){
			memcpy(fbp,&my_image.data[0][0][0],ROWS*COLUMNS*COLOURS);		//blit array to the screen
		}
	}
  

  // cleanup
  munmap(fbp, screensize);
  close(fbfd);
  return 0;
}
