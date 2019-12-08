# stopwatch

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

sem_pt	_g_sem; //_g_sem 세마포 포인터 선언
sem_pt	_g_sem2; // _g_sem2 세마포 포인터 선언

int count = 0;  //timer_isr에서 사용할 count 선언
int count2 = 0;  //스톱워치 시작/중지기능에서 사용할 count2 선언
int state = 0;  //스톱워치 시작/중지를 제어할 state 선언
// custom library header file include
//#include "../../lib_default/itf/lib_default.h"

// user header file include

/* -------------------------------------------------------------------------
	Global variables
 ------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------
	Prototypes
 ------------------------------------------------------------------------- */
static void timer_isr(void);  //1초를 세는 인터럽트 선언
static void print_lcd(void * arg);
static void switch_isr1(void);  //시작/정지를 하는 인터럽트 선언
static void switch_isr2(void);  //스톱워치 리셋 인터럽트 선언

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

	r = sem_create(&_g_sem);  //카운팅 세마포 생성
	if( 0 != r){
		logme("fail at sem_create\r\n");
	}

	r = semb_create(&_g_sem2);  //바이너리 세마포 생성
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

	sem_give(_g_sem);  //_g_sem unlock

	count++;  //count 증가

	reg = TC1->TC_CHANNEL[0].TC_SR;
	printf("HW_TIMER [TC:%d] \r\n",reg);
}

static void print_lcd(void*arg){
	switch_init(switch_isr2,switch_isr1);  //각 스위치에 인터럽트 초기화
	glcd_init();  //glcd초기화

	for(;;){
		sem_take(_g_sem);  //_g_sem lock
		sem_take(_g_sem2);  //_g_sem2 lock

		glcdGotoChar(0,0);
		glcd_printf("HW_TIMER : %3d",count);  //count값 출력

		if(state == 1)  //
		{
		sem_give(_g_sem2);  //_g_sem2 unlock
		count2=count;  //count2에 현재 count값 저장
		}

		task_sleep(100);  //0.1초 sleep

	}
}

static void switch_isr1(void){
	if(state == 0){  //현재 state가 0이면
		sem_give(_g_sem2);  //_g_sem2 unlock
		state=1;  //state 1로 변경
		count=count2;  //count값을 count2로 변경
	}

	else if(state == 1)  //현재 state가 1이면
	{
		sem_clear(_g_sem2);  //_g_sem2 세마포값 0설정, lock
		state=0;  //state 0으로 변경
	}
}

static void switch_isr2(void){
	count = -1;  //count값 –1로 변경
}
