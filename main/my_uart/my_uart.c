#include "my_uart.h"
#include "esp_log.h"

#define UART_TX_PIN         GPIO_NUM_1        
#define UART_RX_PIN         GPIO_NUM_2
#define UART_PORT_NUM       UART_NUM_1           
#define UART_BAUD_RATE      115200  
#define UART_BUF_SIZE       128                 // 收/发缓冲区大小
#define UART_QUEUE_LEN      10                  // 中断接收队列长度   
//static QueueHandle_t        uart_queue;         // UART队列句柄（中断接收用）

#define TAG "my_uart"

void uart_rx_task(void *pvParameters);

// 共享的传感器数据（最新值）
float s_temperature = 0.0;
float s_light = 0.0;
int s_alarm_status = 0;    // 报警状态：0-未报警，1-报警
int s_motor_status = 0;    // 电机状态：0-停止，1-运行

// 保护共享数据的互斥锁
SemaphoreHandle_t s_data_mutex = NULL;

// UART 接收任务句柄（如果需要可以外部控制）
static TaskHandle_t s_uart_rx_task_handle = NULL;

 void uart_init(void)
 {
    //声明UART结构体并配置
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits= UART_STOP_BITS_1,
        .source_clk = UART_SCLK_APB,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    //应用参数配置到指定的UART
    uart_param_config(UART_PORT_NUM,&uart_config);
    //引脚映射
    uart_set_pin(   UART_PORT_NUM, 
                    UART_TX_PIN,
                    UART_RX_PIN,
                    UART_PIN_NO_CHANGE,    //RTS引脚：无，设为默认
                    UART_PIN_NO_CHANGE     // CTS引脚：无，设为默认
                );
    //启动UART驱动
    uart_driver_install(UART_PORT_NUM,
                        UART_BUF_SIZE*2,
                        UART_BUF_SIZE*2,
                        UART_QUEUE_LEN,
                        NULL,
                        0);

     // 创建互斥锁
    s_data_mutex = xSemaphoreCreateMutex();
    if (s_data_mutex == NULL) {
        ESP_LOGE(TAG, "互斥锁创建失败");
        return;
    }

    // 创建接收任务
    BaseType_t ret = xTaskCreate(uart_rx_task, "uart_rx", 4096, NULL,
                                 configMAX_PRIORITIES - 1, &s_uart_rx_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "ESP32串口接收任务创建失败");
    } else {
        ESP_LOGI(TAG, "ESP32串口接收任务创建成功");
    }
    ESP_LOGI(TAG, "my_uart init success");
 }


 //UART接收任务，来读取从stm32来的 UART 数据
void uart_rx_task(void *pvParameters)
{
    uint8_t *data = malloc(UART_BUF_SIZE);
    if (!data) {
        ESP_LOGE(TAG, "Failed to allocate receive buffer");
        vTaskDelete(NULL);
    }

    while (1) {
        // 读取 UART 数据，最多等待 100ms
        int len = uart_read_bytes(UART_PORT_NUM, data, UART_BUF_SIZE - 1, pdMS_TO_TICKS(100));
        if (len > 0) {
            data[len] = '\0';   // 确保字符串结束
            // STM32 发送的数据以 "\r\n" 结尾，我们按行处理
            char *line = (char*)data;
            char *newline = strchr(line, '\n');
            if (newline) *newline = '\0';   // 去除换行符

            float temp, lux;
            int alarm, motor;
            // 解析格式：TEMP=xx.x,LUX=xxx.x,ALARM=x,MOTOR=x
            if (sscanf(line, "TEMP=%f,LUX=%f,ALARM=%d,MOTOR=%d", &temp, &lux, &alarm, &motor) == 4) {
                // 更新共享数据（加锁）
                if (s_data_mutex) {
                    xSemaphoreTake(s_data_mutex, portMAX_DELAY);
                    s_temperature = temp;
                    s_light = lux;
                    s_alarm_status = alarm;
                    s_motor_status = motor;
                    xSemaphoreGive(s_data_mutex);
                    ESP_LOGD(TAG, "Updated: Temp=%.1f, Lux=%.1f, Alarm=%d, Motor=%d", temp, lux, alarm, motor);
                } else {
                    ESP_LOGE(TAG, "Data mutex not initialized");
                }
            } else {
                ESP_LOGW(TAG, "Parse failed: %s", line);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    free(data);
}

// UART发送函数，向STM32发送控制命令
void uart_tx(const char *data)
{
    if (data != NULL) {
        int len = strlen(data);
        uart_write_bytes(UART_PORT_NUM, data, len);
        ESP_LOGD(TAG, "Sent: %s", data);
    }
}