// ButtonLed.c: starter file for CECS447 Project 1 Part 1
// Runs on TM4C123, 
// Dr. Min He
// September 10, 2022
// Port B bits 5-0 outputs to the 6-bit DAC
// Port D bits 3-0 inputs from piano keys: CDEF:doe ray mi fa,negative logic connections.
// Port F is onboard LaunchPad switches and LED
// SysTick ISR: PF3 is used to implement heartbeat

#include "tm4c123gh6pm.h"
#include <stdint.h>
#include "ButtonLed.h"
#include "Sound.h"

// Constants
#define SW1 0x10  // bit position for onboard switch 1(left switch)
#define SW2 0x01  // bit position for onboard switch 2(right switch)
#define NVIC_EN0_PORTF 0x40000000  // enable PORTF edge interrupt
#define NVIC_EN0_PORTD 0x00000008  // enable PORTD edge interrupt

// Golbals
volatile uint8_t curr_mode=PIANO;  // 0: piano mode, 1: auto-play mode
//---------------------Switch_Init---------------------
// initialize onboard switch and LED interface
// Input: none
// Output: none 
void ButtonLed_Init(void){ volatile unsigned long  delay;
  SYSCTL_RCGC2_R |= 0x00000020;     // 1) activate clock for Port F
  delay = SYSCTL_RCGC2_R;           // allow time for clock to start
  GPIO_PORTF_LOCK_R = GPIO_LOCK_KEY;   // 2) unlock GPIO Port F
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
  GPIO_PORTF_AMSEL_R &= ~0x1F;        // 3) disable analog on PF
  GPIO_PORTF_PCTL_R &= ~0x000FFFFF;   // 4) PCTL GPIO on PF4-0
  GPIO_PORTF_DIR_R |= 0x0E;          // 5) PF4,PF0 in, PF3-1 out
  GPIO_PORTF_DIR_R &= ~0x11;          // 5) PF4,PF0 in, PF3-1 out
  GPIO_PORTF_AFSEL_R &= ~0x1F;        // 6) disable alt funct on PF7-0
  GPIO_PORTF_PUR_R |= 0x11;          // enable pull-up on PF0 and PF4
  GPIO_PORTF_DEN_R |= 0x1F;          // 7) enable digital I/O on PF4-0
	GPIO_PORTF_IS_R &= ~0x11;     				//  PF4,PF0 is edge-sensitive
  GPIO_PORTF_IBE_R &= ~0x11;    				//  PF4,PF0 is not both edges
  GPIO_PORTF_IEV_R &= ~0x11;    				//  PF4,PF0 falling edge event
  GPIO_PORTF_ICR_R = 0x11;      				//  Clear flags 4,0
  GPIO_PORTF_IM_R |= 0x11;      				//  Arm interrupt on PF4,PF0
  NVIC_PRI7_R = (NVIC_PRI7_R&0xFF1FFFFF)|0x00A00000;	// priority 5
  NVIC_EN0_R |= NVIC_EN0_PORTF;      									//  Enable interrupt 30 in NVIC
}

//---------------------PianoKeys_Init---------------------
// initialize onboard Piano keys interface: PORT D 0 - 3 are used to generate 
// tones: CDEF:doe ray mi fa
// No need to unlock. Only PD7 needs to be unlocked.
// Input: none
// Output: none 
void PianoKeys_Init(void){ volatile unsigned long  delay;
  SYSCTL_RCGC2_R |= SYSCTL_RCGC2_GPIOD;     // 1) activate clock for Port D
  delay = SYSCTL_RCGC2_R;           // allow time for clock to start
  GPIO_PORTD_CR_R = 0x0F;           // allow changes to PD0-3
  GPIO_PORTD_AMSEL_R &= ~0x0F;        // 3) disable analog on PD
  GPIO_PORTD_PCTL_R &= ~0x0000FFFF;   // 4) PCTL GPIO on PF4-0
  GPIO_PORTD_DIR_R &= ~0x0F;          // 5) PD in
  GPIO_PORTD_AFSEL_R &= ~0x0F;        // 6) disable alt funct on PD
  GPIO_PORTD_PUR_R |= 0x0F;          // enable pull-up on PD0-3
  GPIO_PORTD_DEN_R |= 0x0F;          // 7) enable digital I/O on PD
	
	GPIO_PORTD_IS_R &= ~0x0F;     				//  PF4,PF0 is edge-sensitive
  GPIO_PORTD_IBE_R &= ~0x0F;    				//  PF4,PF0 is not both edges
  GPIO_PORTD_IEV_R &= ~0x0F;    				//  PF4,PF0 falling edge event
  GPIO_PORTD_ICR_R = 0x0F;      				//  Clear flags 4,0
  GPIO_PORTD_IM_R |= 0x0F;      				//  Arm interrupt on PF4,PF0
	
  NVIC_PRI0_R = (NVIC_PRI0_R&0x1FFFFFFF)|0xA0000000;	
  NVIC_EN0_R |= NVIC_EN0_PORTD;      									//  Enable interrupt 30 in NVIC
}


uint8_t get_current_mode(void)
{
	return curr_mode;
}


void GPIOPortF_Handler(void){
	Delay();
  if(GPIO_PORTF_RIS_R&0x01){  // SW2 pressed
		if(get_current_mode() == PIANO){
			next_octave();
		}
		else{
			next_song();
		}
    GPIO_PORTF_ICR_R = 0x01;  // acknowledge flag0
  }
  if(GPIO_PORTF_RIS_R&0x10){  // SW1 pressed
		curr_mode = AUTO_PLAY;
    GPIO_PORTF_ICR_R = 0x10;  // acknowledge flag4
  }
}

uint8_t noteC1 = 0, noteD1 = 0, noteE1 = 0, noteF1 = 0;
void GPIOPortD_Handler(void){
	
	Delay();
	
	if(get_current_mode() == PIANO){
		if(GPIO_PORTD_RIS_R&0x01){	
			noteC1 ^=0x01;
			GPIO_PORTD_ICR_R = 0x01;	
		}
		if(GPIO_PORTD_RIS_R&0x02){	
			noteD1 ^=0x01;
			GPIO_PORTD_ICR_R = 0x02;	
		}
		if(GPIO_PORTD_RIS_R&0x04){	
			noteE1 ^=0x01;
			GPIO_PORTD_ICR_R = 0x04;	
		}
		if(GPIO_PORTD_RIS_R&0x08){	
			noteF1 ^=0x01;
			GPIO_PORTD_ICR_R = 0x08;	
		}
	}
	else{
		GPIO_PORTD_ICR_R = 0x0F;	
	}
}


