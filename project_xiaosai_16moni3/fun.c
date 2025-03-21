#include "headfile.h"

uint8_t view=0;	//全局切屏变量

typedef enum { high, low } type;
type selected_type = high ;	//上下限报警选择

typedef enum { edit_off, edit_on } EditMode;
EditMode edit_mode = edit_off;		//修改上下限报警模式

uint16_t value =60;	//上下限报警值

uint16_t temp_high = 100;
uint16_t temp_low= 60;	//上下限临时存储

uint16_t high_rate =100;
uint16_t low_rate = 60; //上下限最终储值
uint16_t alarm_count = 0;//报警次数

//led封装函数
void led_show(uint8_t led ,uint8_t mode )
{
	HAL_GPIO_WritePin (GPIOD ,GPIO_PIN_2 ,GPIO_PIN_SET );
	if(mode )
		HAL_GPIO_WritePin (GPIOC ,GPIO_PIN_8 <<(led -1),GPIO_PIN_RESET );
	else 
		HAL_GPIO_WritePin (GPIOC ,GPIO_PIN_8 <<(led -1),GPIO_PIN_SET );
	HAL_GPIO_WritePin (GPIOD ,GPIO_PIN_2 ,GPIO_PIN_RESET );
}



//输入捕获（PB4）
double capture =0;
uint16_t  fre =0;

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
        if(htim->Instance==TIM3)
	{
		if(htim->Channel==HAL_TIM_ACTIVE_CHANNEL_1)//中断源选择直接输入的通道
		{
		capture =HAL_TIM_ReadCapturedValue(htim,TIM_CHANNEL_1);
		__HAL_TIM_SetCounter(htim,0);  //计时值清零
		fre =(80000000/80)/capture ;
		}
	}
}

//心率与频率的换算
uint16_t rate =30;
void rate_pro()
{
if(fre  <= 1000)
{
	rate=30;
}
else if (fre>1000 &&fre <2000 )
{
	rate = 0.17 *fre -140;
}
else 
{
	rate = 200;
}

}

// adc R37:
double  getADC(void)
{
    uint16_t adc;
    HAL_ADC_Start(&hadc2);
    adc = HAL_ADC_GetValue(&hadc2);
    return adc *3.3/4096;
}

//ADC与上下限报警及相关的最大最小报警值的选取
void adc_pro()
{
	 value = 60 + (getADC () - 1.0) * 45;   // 1-3V对应60-150
	 if (edit_mode == edit_on ) 
	{
        if (selected_type == high ) temp_high  = value;
        else temp_low  = value;
    }
	
}

//最大最小心率的更新
uint16_t max_rate = 0;     	 // 最大心率
uint16_t min_rate = 0xFFFF; 	// 最小心率（初始设为极大值）
void update_rate(void) 
{
    // 更新最大值
    if (rate  > max_rate) {
        max_rate = rate;
    }
    // 更新最小值（需排除初始值0xFFFF）
    if (rate < min_rate && rate != 0) {
        min_rate = rate;
    }
}

//报警系统函数
void check_alarm(void) 
{
    static uint8_t last_alarm_state = 0; // 修改为静态变量
    uint8_t current_state = 0;

    if (rate > high_rate) 
        current_state = 1;
    else if (rate < low_rate) 
        current_state = 2;

    // 仅当状态从正常→报警，或报警类型切换时增加次数
    if (current_state != 0 && last_alarm_state != current_state) 
    {
        alarm_count++;
        HAL_TIM_Base_Start_IT(&htim2); // 启动LD6计时
    }
    last_alarm_state = current_state; // 更新状态
}


// TIM2中断回调函数（独立定义，不在check_alarm内部）
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        static uint32_t counter = 0;
        counter++;
        if (counter >= 500) { // 5秒后关闭LD6
            led_show(6, 0);   // 仅操作LD6
            HAL_TIM_Base_Stop_IT(&htim2);
            counter = 0;
        } else {
            led_show(6, 1);    // 仅操作LD6
        }
    }
}

//key 结构体封装
uint8_t B1_state;
uint8_t B1_last_state =1;			//定义第0次按键未按下
uint8_t B2_state;
uint8_t B2_last_state =1;
uint8_t B3_state;
uint8_t B3_last_state =1;
uint8_t B4_state;
uint8_t B4_last_state =1;

void key_scan()
{
	B1_state = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_0);
	B2_state = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_1);
	B3_state = HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_2);
	B4_state = HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);
	if(B1_state == 0 && B1_last_state  == 1)		//按下B1按键
	{
		view ++;
		view %=3;
		LCD_Clear (Black );
		
	}
	if(B2_state == 0 && B2_last_state  == 1  && view == 2)		//按下B2按键
	{
		selected_type =! selected_type ;
	}
	if(B3_state == 0 && B3_last_state  == 1  && view == 2)		//按下B3按键
	{
		edit_mode =! edit_mode ;
		if(edit_mode == edit_on )
		{
			high_rate = temp_high ;
			low_rate = temp_low ;
		}
	}

	if(B4_state == 0 && B4_last_state  == 1  && view == 0)		//按下B4按键
	{
		alarm_count =0;
		LCD_Clear (Black );
	}

	B1_last_state = B1_state ;
	B2_last_state = B2_state ;
	B3_last_state = B3_state ;
	B4_last_state = B4_state ;

}

//led显示函数
void led_pro()
{
	if(view == 0)	led_show (1,1);
		else led_show (1,0);
	if(view == 1)	led_show (2,1);
			else led_show (2,0);
	if(view == 2)	led_show (3,1);
			else led_show (3,0);
	if(edit_mode == edit_on && selected_type == high && view ==2 )
		led_show (4,1);
			else led_show (4,0);
	if(edit_mode == edit_on && selected_type == low  && view ==2  )
		led_show (5,1);
			else led_show (5,0);
}

//lcd 函数
void lcd_show()
{
	
	char text[20];
	if(view == 0)
	{
		sprintf (text, "       HEART  ");
		LCD_DisplayStringLine (Line1 ,(uint8_t*)text );
		sprintf (text, "       Rate:%d  ",rate  );
		LCD_DisplayStringLine (Line3 ,(uint8_t*)text );
		sprintf (text, "       Con:%d",alarm_count  );
		LCD_DisplayStringLine (Line4 ,(uint8_t*)text );
	}
	if(view == 1)
	{
		sprintf (text, "       RECORD  ");
		LCD_DisplayStringLine (Line1 ,(uint8_t*)text );
		sprintf (text, "       Max:%d  ",max_rate);
		LCD_DisplayStringLine (Line3 ,(uint8_t*)text );
		sprintf (text, "       Min:%d  ",min_rate);
		LCD_DisplayStringLine (Line4 ,(uint8_t*)text );
	}
	if(view == 2)
	{
		sprintf (text, "        PARA  ");
		LCD_DisplayStringLine (Line1 ,(uint8_t*)text );
		sprintf (text, "      High:%d  ",high_rate);
		LCD_DisplayStringLine (Line3 ,(uint8_t*)text );
		sprintf (text, "       Low:%d  ",low_rate);
		LCD_DisplayStringLine (Line4 ,(uint8_t*)text );

	}
}

