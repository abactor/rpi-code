#include "rs.h"
#include "rs_app.h"
#include <math.h>
#include <stdio.h>
#include "string.h"
#include <stdlib.h>
#include <time.h>
//#include <unistd.h>




void init_rs_buff(void){
int i;
	for (i=0;i<NN;i++){
		rs_buff[i]=0;
		//printf("This is the %ith element of rs_buff: %u\n",i,rs_buff[i]);
	}

}

void load_rs_test_data(void){
int i;

	//printf("Loading data bytes\n");
	for (i=0;i<RS_NUM_ACTUAL_DATA_BYTES;i++){
		rs_buff[i]=i;
	//	printf("%i ",i,rs_buff[i]);
	}
	
	//printf("\n\n");

}


void corrupt_rs_data(int badbytes){
int i;
int temp_holder;
 
	//printf("Data Corrupted by %i bytes, from:\n",badbytes);
	for (i=0;i<badbytes;i++){
	//	printf("%i ",rs_buff[i]);
		rs_buff[i]=rs_buff[i]^0xFF;	//flip bits
	}
	
	/*
    printf("\n");
	printf("Data Corrupted by %i bytes, to:\n",badbytes);
	for (i=0;i<badbytes;i++){
		printf("%i ",rs_buff[i]);
	}

	printf("\n\n");
	*/
}


/*
void print_rs_data(){
int i;

    
	printf("Actual Data Bytes\n");
	for (i=0;i<RS_NUM_ACTUAL_DATA_BYTES;i++){
		printf("%u ",rs_buff[i]);
	}
	printf("\n");
	printf("Parity Bytes:\n");
	for (i=0;i<RS_NUM_PARITY_BYTES;i++){
		printf("%u ",rs_buff[i+KK]);
	}
	printf("\n\n");
    
}
*/
void test_rs(int num_bad){
int i,j;
int er_pos[RS_NUM_PARITY_BYTES];
int num_ers=0;


	
	if (num_bad<0 || num_bad>RS_NUM_ACTUAL_DATA_BYTES){
		num_bad=0;
		//printf("Corrupted bytes too high, defaulting to zero\n\n\n");
	}

//dtype temp_rs_buff[NN];

	i=RS_NUM_ACTUAL_DATA_BYTES;
	/*
    printf("NN:\t%i\n",NN);
	printf("KK:\t%i\n",KK);
	printf("MM:\t%i\n",MM);
	printf("RS_BYTES_SENT:\t%i\n",RS_BYTES_SENT);
	printf("RS_NUM_PARITY_BYTES:\t%i\n",RS_NUM_PARITY_BYTES);
	printf("RS_BYTES_SENT:\t%i\n",RS_BYTES_SENT);
	printf("NUM ACTUAL DATA:\t%i\n",RS_NUM_ACTUAL_DATA_BYTES);
	printf("NUM ACTUAL DATA calculated:\t%i\n",(RS_BYTES_SENT-RS_NUM_PARITY_BYTES));
	printf("RS_NUM_ZERO_PAD:\t%i\n\n",RS_NUM_ZERO_PAD);
    */
	init_rs_buff();
	init_rs();
	//print_rs_data();
	load_rs_test_data();
	
	struct timespec t;
	struct timespec t2;
	long t_diff;
	
	t_diff=0;
	for(j=0;j<1;j++){
		clock_gettime(CLOCK_REALTIME,&t);
		i=encode_rs(&rs_buff[0],&rs_buff[KK]);//encode buffer
		clock_gettime(CLOCK_REALTIME,&t2);
		t_diff+=t2.tv_nsec-t.tv_nsec;
	}
//	t_diff=t2.tv_sec-t.tv_sec;
	/*
    printf("Clock diff in seconds:\t%li microseconds\n",t_diff/1000);

	t_diff=0;
	for(j=0;j<10;j++){
		clock_gettime(CLOCK_REALTIME,&t);
		i=encode_rs(&rs_buff[0],&rs_buff[KK]);//encode buffer
		clock_gettime(CLOCK_REALTIME,&t2);
		t_diff+=t2.tv_nsec-t.tv_nsec;
	}
//	t_diff=t2.tv_sec-t.tv_sec;
	printf("Clock diff in seconds:\t%li microseconds\n",t_diff/1000);
	
	
	t_diff=0;
	for(j=0;j<100;j++){
		clock_gettime(CLOCK_REALTIME,&t);
		i=encode_rs(&rs_buff[0],&rs_buff[KK]);//encode buffer
		clock_gettime(CLOCK_REALTIME,&t2);
		t_diff+=t2.tv_nsec-t.tv_nsec;
	}
//	t_diff=t2.tv_sec-t.tv_sec;
	printf("Clock diff in seconds:\t%li microseconds\n",t_diff/1000);

	print_rs_data();
	corrupt_rs_data(num_bad);
	print_rs_data();

	memcpy(&temp_rs_buff[0],&rs_buff[0],RS_NUM_ACTUAL_DATA_BYTES);
	memcpy(&temp_rs_buff[KK],&rs_buff[KK],RS_NUM_PARITY_BYTES);
	//encoder always returns 0
	//printf("Encoder returned %i\n\n",i);
	
	
	t_diff=0;
	for(j=0;j<1;j++){
		
		memcpy(&rs_buff[0],&temp_rs_buff[0],RS_NUM_ACTUAL_DATA_BYTES);
		memcpy(&rs_buff[KK],&temp_rs_buff[KK],RS_NUM_PARITY_BYTES);
		clock_gettime(CLOCK_REALTIME,&t);
		i=eras_dec_rs(rs_buff,er_pos,num_ers);		//decode data
		clock_gettime(CLOCK_REALTIME,&t2);
		t_diff+=t2.tv_nsec-t.tv_nsec;
	}
//	t_diff=t2.tv_sec-t.tv_sec;
	printf("Clock diff in seconds:\t%li microseconds\n",t_diff/1000);
	//i=eras_dec_rs(rs_buff,er_pos,num_ers);
	
	t_diff=0;
	for(j=0;j<10;j++){
		
		memcpy(&rs_buff[0],&temp_rs_buff[0],RS_NUM_ACTUAL_DATA_BYTES);
		memcpy(&rs_buff[KK],&temp_rs_buff[KK],RS_NUM_PARITY_BYTES);
		clock_gettime(CLOCK_REALTIME,&t);
		i=eras_dec_rs(rs_buff,er_pos,num_ers);		//decode data
		clock_gettime(CLOCK_REALTIME,&t2);
		t_diff+=t2.tv_nsec-t.tv_nsec;
	}
//	t_diff=t2.tv_sec-t.tv_sec;
	printf("Clock diff in seconds:\t%li microseconds\n",t_diff/1000);
	//i=eras_dec_rs(rs_buff,er_pos,num_ers);
	
	t_diff=0;
	for(j=0;j<100;j++){
		
		memcpy(&rs_buff[0],&temp_rs_buff[0],RS_NUM_ACTUAL_DATA_BYTES);
		memcpy(&rs_buff[KK],&temp_rs_buff[KK],RS_NUM_PARITY_BYTES);
		clock_gettime(CLOCK_REALTIME,&t);
		i=eras_dec_rs(rs_buff,er_pos,num_ers);		//decode data
		clock_gettime(CLOCK_REALTIME,&t2);
		t_diff+=t2.tv_nsec-t.tv_nsec;
	}
//	t_diff=t2.tv_sec-t.tv_sec;
	printf("Clock diff in seconds:\t%li microseconds\n",t_diff/1000);
    
    */
    
    
	//i=eras_dec_rs(rs_buff,er_pos,num_ers);
	
	i=eras_dec_rs(rs_buff,er_pos,num_ers);		//decode data
	if (i==0){
		printf("Decoder found no corruption!\n\n");
	}
	else if (i==-1){
		printf("Decoder could not decode, too many errors, crummy.\nTry increasing the number of parity bits.\n\n");
	}
	else {
		printf("Decoder found %i errors\n\n",i);
	}
	//print_rs_data();
	
}
