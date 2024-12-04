//
// Created by Mac on 2024/12/5.
//

#include "string.h"
#include "stdio.h"
#include "math.h"
#include "tim.h"
#include "adc.h"
#include "stm32f4xx_hal.h"
#include "atk_md0350/atk_md0350.h"
#include "led/led.h"
#include "arm_math.h"

#include "wave_analysis.h"


static int current_wave_buff = 0;
static int16_t wave_buff0[1024];
static int16_t wave_buff1[1024];
static int16_t last_wave[1024];
static int16_t current_wave[1024];
static int16_t last_fft[1024];
static int16_t wave_fft[1024];
static int draw_len = 1024;

static arm_rfft_instance_q15 fft_instance;

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


static void draw_wave(int16_t* last_wave, int16_t* wave, int start, int end, int start_x, int start_y, uint16_t color)
{
    int width = 320, height = 160;

    int last_x = -1, last_y = 0;
    int last_x_clean = -1, last_y_clean = -1;
    for (int index = start; index < end; ++index) {
        int x = (index - start) * width / (end - start);
        int y = -wave[index] * height / 4095;

        int x_clean = x;
        int y_clean = -last_wave[index] * height / 4095;

        if (last_x == -1) {
            last_x = x;
            last_y = y;
            last_x_clean = x_clean;
            last_y_clean = y_clean;
            continue ;
        }
        atk_md0350_draw_line(start_x + last_x_clean, start_y + last_y_clean, start_x + x_clean, start_y + y_clean, ATK_MD0350_WHITE);
        atk_md0350_draw_line(start_x + last_x, start_y + last_y, start_x + x, start_y + y, color);
        last_x = x;
        last_y = y;
        last_x_clean = x_clean;
        last_y_clean = y_clean;
    }
}

void show_wave()
{
    /* 复制当前波形 */
    if (current_wave_buff) {
        memcpy(current_wave, wave_buff1, sizeof(uint16_t) * 1024);
    } else {
        memcpy(current_wave, wave_buff0, sizeof(uint16_t) * 1024);
    }
    draw_wave(last_wave, current_wave, 0, draw_len, 0, 170, ATK_MD0350_RED);
    memcpy(last_wave, current_wave, sizeof(uint16_t) * 1024);
}


static void draw_fft(int16_t* wave, int start, int end, int start_x, int start_y, uint16_t color)
{
    int width = 320, height = 160;

    int last_x = -1, last_y = 0;
    for (int index = start; index < end; ++index) {
        int x = (index - start) * width / (end - start);
        int y = -wave[index] * height / 2000;

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

void show_fft()
{
    draw_fft(last_fft, 0, 256, 0, 280, ATK_MD0350_WHITE);

    uint16_t fftSize = 1024;      //定义rfft的长度
    uint8_t ifftFlag = 0;         //表示fft变换为正变换,1则为逆变换
    arm_rfft_init_q15(&fft_instance, fftSize, ifftFlag, 1);
    arm_rfft_q15(&fft_instance, current_wave, wave_fft);
    for (int i = 0; i < 1024; ++i) wave_fft[i] = fabs(wave_fft[i]);

    draw_fft(wave_fft, 0, 256, 0, 280, ATK_MD0350_RED);

    memcpy(last_fft, wave_fft, sizeof(uint16_t) * 1024);

    int max_index = 0, max_value = 0;
    for (int i = 5; i < 256; ++i) {
        if (wave_fft[i] > max_value) {
            max_value = wave_fft[i];
            max_index = i;
        }
    }

    int freq = max_index * 5 * 0.9756;

    draw_len = 128;


    static char last_freq_echo[32] = {0};
    static char freq_echo[32];
    snprintf(freq_echo, sizeof(freq_echo), "freq:%dHz", freq);

    atk_md0350_show_string(10, 400, 320, 32, last_freq_echo, ATK_MD0350_LCD_FONT_32, ATK_MD0350_WHITE);
    atk_md0350_show_string(10, 400, 320, 32, freq_echo, ATK_MD0350_LCD_FONT_32, ATK_MD0350_RED);

    memcpy(last_freq_echo, freq_echo, 32);
}

