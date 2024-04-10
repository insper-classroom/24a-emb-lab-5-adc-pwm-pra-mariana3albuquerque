/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

const int ADC0_X = 26; //x
const int ADC0_CHANNEL = 0;
const int ADC1_Y = 27;
const int ADC1_CHANNEL = 1;

QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;
} adc_t;

int adjust_scale(int adc_value) {
    // Mapear de 0-4095 para -255 a +255
    int adjusted_value = ((adc_value - 2048) * 255) / 2048;

    // Aplicar a zona morta
    if (adjusted_value > -150 && adjusted_value < 150) {
        return 0;
    } else {
        return adjusted_value;
    }
}

void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF ;

    uart_putc_raw(uart0, data.axis); 
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb); 
    uart_putc_raw(uart0, -1); 
}

void x_task(void *p) {
    adc_t data;
    adc_init();
    adc_gpio_init(ADC0_X);

    int v[5];
    //int mediaX;

    while (1) {
        adc_select_input(ADC0_CHANNEL);
        v[0]=v[1];
        v[1]=v[2];
        v[2]=v[3];
        v[3]=v[4];
        v[4]=adc_read();
        int mediaX = ((v[4]+v[3]+v[2]+v[1]+v[0])/5);

        data.axis = 0;
        data.val = adjust_scale(mediaX);

        xQueueSend(xQueueAdc, &data, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p) {
    adc_t data;
    adc_init();
    adc_gpio_init(ADC1_Y);

    int v[5];
    //int mediaY;

    while (1) {
        adc_select_input(ADC1_CHANNEL);
        v[0]=v[1];
        v[1]=v[2];
        v[2]=v[3];
        v[3]=v[4];
        v[4]=adc_read();
        int mediaY = ((v[4]+v[3]+v[2]+v[1]+v[0])/5);

        data.axis = 1;
        data.val = adjust_scale(mediaY);
        
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void uart_task(void *p) {
    adc_t data;

    while (1) {
        xQueueReceive(xQueueAdc, &data, portMAX_DELAY);
        // printf("%d\n", data);
        if (data.val != 0) write_package(data);
    }
}

int main() {
    stdio_init_all();

    xQueueAdc = xQueueCreate(32, sizeof(adc_t));

    xTaskCreate(x_task, "x_task", 4096, NULL, 1, NULL);
    xTaskCreate(y_task, "y_task", 4096, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}