#ifndef __MY_UART_H_
#define __MY_UART_H_
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

 void uart_init(void);
void uart_tx(const char *data);

// 共享的传感器数据（最新值）
extern float s_temperature;
extern float s_light;
extern int s_alarm_status;    // 报警状态：0-未报警，1-报警
extern int s_motor_status;    // 电机状态：0-停止，1-运行

// 保护共享数据的互斥锁
extern SemaphoreHandle_t s_data_mutex;

#endif