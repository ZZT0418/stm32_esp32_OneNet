#include "onenet_dm.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "led_ws2812.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "string.h"
#include "my_uart.h"
#include "esp_log.h"

extern SemaphoreHandle_t s_data_mutex;
extern float s_temperature;
extern float s_light;
extern int s_alarm_status;    // 报警状态：0 1 2 3
extern int s_motor_status;    // 电机状态：0-停止，1-运行

#define TAG "onenet_dm"



/*==========================物模型初始化函数========================
 * @brief:物模型初始化
 * @param:无
 * @return:无
 *================================================================*/
// void onenet_dm_init(void)
// {
//     ESP_LOGI(TAG,"物模型初始化");
// }   

/*==========================属性处理函数============================
 * @brief:处理onenet下行的物模型数据
 * @param:platform_data:平台下发的json数据
 * @return:无
 *================================================================*/
void onenet_property_handle(cJSON *platform_dm_data)
{
    //从JSON数据中读取“params”
    cJSON *params_json = cJSON_GetObjectItem(platform_dm_data,"params");
    if(params_json != NULL)
    {
        //遍历 "params" 下的所有子节点
        cJSON *name_js=params_json->child;
        //遍历链表
        while(name_js != NULL)
        {
            if(strcmp(name_js->string,"Alarm") == 0)  //报警状态控制
            {
                int alarm_status = cJSON_GetNumberValue(name_js);
                s_alarm_status = alarm_status;
                // 向STM32发送报警状态控制命令
                char cmd[50];
                sprintf(cmd, "CMD:ALARM,%d\r\n", alarm_status);
                ESP_LOGI(TAG, "发送命令到STM32: %s", cmd);  // 添加日志
                uart_tx(cmd);
            }
            if(strcmp(name_js->string,"Motor") == 0)  //电机状态控制
            {
                int motor_status = cJSON_IsTrue(name_js) ? 1 : 0;
                s_motor_status = motor_status;
                // 向STM32发送电机状态控制命令
                char cmd[50];
                sprintf(cmd, "CMD:MOTOR,%d\r\n", motor_status);
                uart_tx(cmd);
            }
            name_js = name_js->next;
        }
    }

}      




/*==========================属性上传函数============================
 * @brief:物模型属性上传
 * @param:无
 * @return:上传的属性数据
 *================================================================*/
cJSON *onenet_property_upload(void)
{
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) return NULL;
    
    cJSON_AddStringToObject(root, "id", "123456");
    cJSON_AddStringToObject(root, "version", "1.0");
    
    cJSON *params = cJSON_AddObjectToObject(root, "params");
    if (params == NULL) {
        cJSON_Delete(root);
        return NULL;
    }
    
    xSemaphoreTake(s_data_mutex, portMAX_DELAY);
    float temp = s_temperature;
    float light = s_light;
    int alarm = s_alarm_status;
    int motor = s_motor_status;
    xSemaphoreGive(s_data_mutex);
    
    // 温度属性
    cJSON *temp_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(temp_obj, "value", temp);
    cJSON_AddItemToObject(params, "CurrentTemperature", temp_obj);
    
    // 光照属性
    cJSON *light_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(light_obj, "value", light);
    cJSON_AddItemToObject(params, "LightLuxValue", light_obj);
    
    // 报警状态
    cJSON *alarm_obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(alarm_obj, "value", alarm);
    cJSON_AddItemToObject(params, "Alarm", alarm_obj);
    
    // 电机状态
    cJSON *motor_obj = cJSON_CreateObject();
    cJSON_AddBoolToObject(motor_obj, "value", motor > 0);
    cJSON_AddItemToObject(params, "Motor", motor_obj);
    
    ESP_LOGI(TAG, "准备上传数据: Temp=%.1f, Light=%.1f, Alarm=%d, Motor=%d", 
             temp, light, alarm, motor);
    
    return root;
}