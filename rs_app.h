/* ========================================
 *
 RS_encoding
 application code
 
 *
 * ========================================
*/



#define RS_BYTES_SENT 25 
//MM and KK are defined in rs.h
//MM is 8
//KK is 245
//NN is 255

#define RS_NUM_PARITY_BYTES (NN-KK)					//255-245=10
//#define RS_NUM_ZERO_PAD NN-RS_BYTES_SENT				//255-25=230
//alternate define:
#define RS_NUM_ACTUAL_DATA_BYTES (RS_BYTES_SENT-RS_NUM_PARITY_BYTES)	//25-10=15
#define RS_NUM_ZERO_PAD (KK-RS_NUM_ACTUAL_DATA_BYTES)			//245-15=230



dtype rs_buff[NN];


void init_rs_buff(void);
void load_rs_test_data(void);
void corrupt_rs_data(int badbytes);
//void print_rs_data(void);
void test_rs(int num_bad);