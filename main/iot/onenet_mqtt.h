#ifndef _ONENET_MQTT_H_
#define _ONENET_MQTT_H_ 
#include "esp_err.h"

#define ONENET_PRODUCT_ID   "c00Dk7G0zV"                                        //onenet产品ID
#define ONETNET_ACCESS_KEY  "cYufD3cjFR0vA5nJ6wUhyUEnb8XNSWEFcy0Zj9FBh20="      //onenet产品密钥
#define ONENET_DEVICE_NAME  "esp32s3_gateway"                                       //onenet设备名称

esp_err_t onenet_mqqt_start(void);
esp_err_t onenet_mqtt_subscribe(void);

#endif
