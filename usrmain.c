/* -------------------------------------------------------------------------
	Include
 ------------------------------------------------------------------------- */
#include "../ubiconfig.h"

// standard c library include
#include <stdio.h>
#include <sam4e.h>

// ubinos library include
#include "itf_ubinos/itf/bsp.h"
#include "itf_ubinos/itf/ubinos.h"
#include "itf_ubinos/itf/bsp_fpu.h"
#include "itf_ubinos/itf/bsp_intr.h"

// chipset driver include
#include "ioport.h"
#include "pio/pio.h"

// new estk driver include
#include "lib_new_estk_api/itf/new_estk_led.h"
#include "lib_new_estk_api/itf/new_estk_glcd.h"
#include "lib_switch/itf/lib_switch.h"
#include "lib_sensor/itf/lib_sensor.h"
#include "lib_motor_driver/itf/lib_motor_driver.h"

sem_pt	_g_sem;
sem_pt	_g_sem2;

int count = 0;
int count2 = 0;
int state = 0;
// custom library header file include
//#include "../../lib_default/itf/lib_default.h"

// user header file include

/* -------------------------------------------------------------------------
	Global variables
 ------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------
	Prototypes
 ------------------------------------------------------------------------- */
static void timer_isr(void);
static void print_lcd(void * arg);
static void switch_isr1(void);
static void switch_isr2(void);

/* -------------------------------------------------------------------------
	Function Definitions
 ------------------------------------------------------------------------- */
int usrmain(int argc, char * argv[]) {
	int r;
	unsigned int dummy;

	printf("\n\n\n\r");
	printf("================================================================================\n\r");
	printf("exe_ubinos_test (build time: %s %s)\n\r", __TIME__, __DATE__);
	printf("================================================================================\n\r");

	r = sem_create(&_g_sem);
	if( 0 != r){
		logme("fail at sem_create\r\n");
	}

	r = semb_create(&_g_sem2);
	if( 0 != r){
		logme("fail at sem2_create\r\n");
	}

	r = task_create(NULL, print_lcd, NULL, task_getmiddlepriority(), 256, "root");
	if (0 != r) {
		logme("fail at task_create\r\n");
	}

	PMC->PMC_PCER0 = 1 << ID_TC3;

	TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKDIS;
	TC1->TC_CHANNEL[0].TC_IDR = 0xFFFFFFFF;

	TC1->TC_CHANNEL[0].TC_CMR = (TC_CMR_TCCLKS_TIMER_CLOCK5 | TC_CMR_CPCTRG);
	TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN;
	TC1->TC_CHANNEL[0].TC_IER = TC_IER_CPCS;

	intr_connectisr(TC3_IRQn, timer_isr, 0x40, INTR_OPT__LEVEL);

	intr_enable(TC3_IRQn);

	TC1->TC_CHANNEL[0].TC_RC = 32768;

	TC1->TC_CHANNEL[0].TC_CCR = TC_CCR_SWTRG;

	ubik_comp_start();

	return 0;
}

static void timer_isr(void){
	unsigned int reg;

	sem_give(_g_sem);

	count++;

	reg = TC1->TC_CHANNEL[0].TC_SR;
	printf("HW_TIMER [TC:%d] \r\n",reg);
}

static void print_lcd(void*arg){
	switch_init(switch_isr2,switch_isr1);
	glcd_init();

	for(;;){
		sem_take(_g_sem);
		sem_take(_g_sem2);

		glcdGotoChar(0,0);
		glcd_printf("HW_TIMER : %3d",count);

		if(state == 1)
		{
		sem_give(_g_sem2);
		count2=count;
		}

		task_sleep(100);

	}
}

static void switch_isr1(void){
	if(state == 0){
		sem_give(_g_sem2);
		state=1;
		count=count2;
	}

	else if(state == 1)
	{
		sem_clear(_g_sem2);
		state=0;
	}
}

static void switch_isr2(void){
	count = -1;
}
