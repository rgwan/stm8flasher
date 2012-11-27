#ifndef __E_W_ROUTINES_H__
#define __E_W_ROUTINES_H__


#include <stdint.h>

#define E_W_ROUTINES_NUM 6
//#define E_W_ROUTINES_NUM 7 


typedef struct {
	int device_size;
	char bl_version;
	int *bytes;
	uint8_t *data;
} E_W_ROUTINES;

extern int e_w_routine_32k_1_0_size;
extern uint8_t e_w_routine_32k_1_0_data[];
extern int e_w_routine_32k_1_2_size;
extern uint8_t e_w_routine_32k_1_2_data[];
extern int e_w_routine_32k_1_3_size;
extern uint8_t e_w_routine_32k_1_3_data[];
extern int e_w_routine_128k_2_0_size;
extern uint8_t e_w_routine_128k_2_0_data[];
extern int e_w_routine_128k_2_1_size;
extern uint8_t e_w_routine_128k_2_1_data[];
extern int e_w_routine_128k_2_2_size;
extern uint8_t e_w_routine_128k_2_2_data[];
extern int e_w_routine_256k_1_0_size;
extern uint8_t e_w_routine_256k_1_0_data[];

extern E_W_ROUTINES e_w_routines[];


#endif // __E_W_ROUTINES_H__
