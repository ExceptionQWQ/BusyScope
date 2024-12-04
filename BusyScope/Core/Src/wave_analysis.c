//
// Created by Mac on 2024/12/5.
//

#include "string.h"
#include "math.h"
#include "tim.h"
#include "adc.h"
#include "stm32f4xx_hal.h"
#include "atk_md0350/atk_md0350.h"
#include "led/led.h"

#include "wave_analysis.h"


static int current_wave_buff = 0;
static uint16_t wave_buff0[1024];
static uint16_t wave_buff1[1024];
static uint16_t last_wave[1024];


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    static int index = 0;

    if (hadc->Instance == ADC1) {
        uint16_t value = HAL_ADC_GetValue(&hadc1);
 
        if (current_wave_buff) {
            wave_buff0[index++] = value;
        } else {
            wave_buff1[index++] = value;
        }

        if (index >= 1024) {
            index = 0;
            current_wave_buff = !current_wave_buff;
        }
    }
}


void wave_analysis_start()
{
    __HAL_TIM_SET_PRESCALER(&htim2, 84 - 1);
    __HAL_TIM_SET_AUTORELOAD(&htim2, 100 - 1);
    HAL_ADC_Start_IT(&hadc1);
    HAL_TIM_Base_Start(&htim2);
}

void wave_analysis_stop()
{
    HAL_TIM_Base_Stop(&htim2);
    HAL_ADC_Stop_IT(&hadc1);
}


static void draw_wave(uint16_t* wave, int start, int end, uint16_t color)
{
    int start_x = 0, start_y = 300, width = 320, height = 160;

    int last_x = -1, last_y = 0;
    for (int index = start; index < end; ++index) {
        int x = (index - start) * width / (end - start);
        int y = -wave[index] * height / 4095;

        if (last_x == -1) {
            last_x = x;
            last_y = y;
            continue ;
        }
        atk_md0350_draw_line(start_x + last_x, start_y + last_y, start_x + x, start_y + y, color);
        last_x = x;
        last_y = y;
    }
}

void show_wave()
{
    draw_wave(last_wave, 0, 1024, ATK_MD0350_WHITE); /* 清除上一次绘制结果 */
    /* 复制当前波形 */
    if (current_wave_buff) {
        memcpy(last_wave, wave_buff1, sizeof(uint16_t) * 1024);
    } else {
        memcpy(last_wave, wave_buff0, sizeof(uint16_t) * 1024);
    }
    draw_wave(last_wave, 0, 1024, ATK_MD0350_RED);
}