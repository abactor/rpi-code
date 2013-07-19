#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

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

int main(int argc, char* argv[]){
	char file_name[128]="itest.bmp";
	uint32_t xpos;
	uint32_t ypos;
	uint32_t pnum;
	int ret;
	uint8_t iterate=0;
	bmp img;
	FILE *fp;
	fp = fopen(file_name, "rb");
	
	if(fp == NULL) {
		printf("CANNOT OPEN FILE.  PROGRAM EXITING");
		return 1;
	}
	else {
		ret=fread(&img, 1, 54, fp);
		printf("%#06x Should be 0x4D4d\n",img.sig);
		printf("%u wide, %u high\n",img.img_width,img.img_height);
		printf("%u hres, %u vres, %hu bit depth\n",img.h_res,img.v_res,img.bit_depth);
		printf("data offest is: %u\n",img.data_file_offset); 
		pnum=img.img_width*img.img_height*img.bit_depth/8;
		printf("%u bytes long\n",pnum);
		ret=fread(&img.data[0][0][0],1,pnum,fp);
		printf("%u bytes read from %s\n",ret,(char*)&file_name);

		for(xpos=2;xpos<img.img_width;xpos++){
			for (ypos=2;ypos<img.img_height;ypos++){
				if ((img.data[xpos][ypos][0]>0)&&(img.data[xpos][ypos][2]>0)&&(img.data[xpos][ypos][2]>0)){
					/*
					printf("pixel at %u,%u has colours (RGB) %hu %hu %hu\n", xpos, ypos,
						img.data[xpos][ypos][0],
						img.data[xpos][ypos][1],
						img.data[xpos][ypos][2]);	
					*/	
					//printf("%x, %x, %x\n",&img.data[xpos][ypos][0],&img.data[xpos][ypos][1],&img.data[xpos][ypos][2]);
						
				}
			}
			
			
		}
		
	
	}

	fclose(fp);
	
	fp = fopen("outfile.bmp", "wb");
	
	if(fp == NULL) {
		printf("CANNOT OPEN FILE.  PROGRAM EXITING");
		return 1;
	}
	else {
		ret=fwrite(&img, 8, 54, fp);

		//ret=fread(&img.data[0][0][0],8,img.img_width*img.img_height*img.bit_depth/8,fp);
		
		pnum=0;
		
		for(xpos=0;xpos<img.img_width;xpos++){
			for (ypos=0;ypos<img.img_height;ypos++){
				img.data[xpos][ypos][0]=iterate%33;
				img.data[xpos][ypos][1]=iterate%133;
				img.data[xpos][ypos][2]=iterate%15;
				if (iterate<254)
					iterate=(iterate+1)%254;
				
				pnum++;
			}
			
			
		}
		printf("pnum is %u\n",pnum);
//		fwrite(&img.data[0][0][0],8,img.img_width*img.img_height*img.bit_depth/8,fp);
		fwrite(&img.data[0][0][0],1,img.img_width*img.img_height*img.bit_depth/8,fp);
		
	
	}

	fclose(fp);

}
