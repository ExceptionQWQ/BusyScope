//
// Created by Mac on 2024/12/4.
//


#include "math.h"
#include "tim.h"
#include "dac.h"
#include "stm32f4xx_hal.h"

#include "wave_generator.h"

#define PI 3.1415926

static int timer_freq = 100 * 1000;
static double time_x = 0;
static int sample_points = 0; /* 每周期采样点个数 */
static int wave_table_size = 256;
static double wave_table[4096]; /* 预先生成0-2pi之内的点，中断内直接查表获取sin函数值 */

static void convert_value_to_adc(double value)
{
    int adc_value = (int)((value + 1.0f) / 2.0f * 4095);
    HAL_DAC_SetValue(&hdac, DAC1_CHANNEL_1, DAC_ALIGN_12B_R,  adc_value);
}


static void wave_tick()
{
    static int index = 0;
    ++index;
    if (index >= wave_table_size) index = 0;
    convert_value_to_adc(wave_table[index]);
}

static void generator_sin_table(int table_size)
{
    for (int i = 0; i < table_size; ++i) {
        double x = 2 * PI * i / table_size;
        wave_table[i] = sin(x);
    }
}

static void generator_square_table(int table_size, int duty)
{
    for (int i = 0; i < table_size; ++i) {
        if (i <= duty * table_size / 100) wave_table[i] = 1;
        else wave_table[i] = -1;
    }
}

void wave_generator_sin(int freq)
{
    HAL_TIM_Base_Stop_IT(&htim1);
    HAL_DAC_Stop(&hdac, DAC1_CHANNEL_1);

    __HAL_TIM_SET_PRESCALER(&htim1, 84 - 1);
    __HAL_TIM_SET_AUTORELOAD(&htim1, 10 - 1);

    wave_table_size = timer_freq / freq;
    if (wave_table_size < 25) { /* 小于25Hz的信号不生成 */
        return ;
    }

    generator_sin_table(wave_table_size);

    set_tim1_callback(wave_tick);
    HAL_DAC_Start(&hdac, DAC1_CHANNEL_1);
    HAL_TIM_Base_Start_IT(&htim1);
}

void wave_generator_square(int freq, int duty)
{
    HAL_TIM_Base_Stop_IT(&htim1);
    HAL_DAC_Stop(&hdac, DAC1_CHANNEL_1);

    __HAL_TIM_SET_PRESCALER(&htim1, 84 - 1);
    __HAL_TIM_SET_AUTORELOAD(&htim1, 10 - 1);

    wave_table_size = timer_freq / freq;
    if (wave_table_size < 25) { /* 小于25Hz的信号不生成 */
        return ;
    }

    generator_square_table(wave_table_size, duty);

    set_tim1_callback(wave_tick);
    HAL_DAC_Start(&hdac, DAC1_CHANNEL_1);
    HAL_TIM_Base_Start_IT(&htim1);
}