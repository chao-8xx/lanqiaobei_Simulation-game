#ifndef _fun_h
#define _fun_h

#include "stm32g4xx.h"                  // Device header
#include "headfile.h"

void led_show(uint8_t led ,uint8_t mode );
void lcd_show();
void key_scan();
void led_pro();
void check_alarm(void) ;
void update_rate(void) ;
void adc_pro();
void rate_pro();


#endif 
