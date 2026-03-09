#include <driver/gpio.h>
#include <driver/uart.h>
#include <esp_log.h>
#include <cstring>

#include "iot/thing.h"
#include "mqtt_client.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_netif.h"

#define TAG "fan_mqtt"
#define MQTT_URI "mqtt://broker.emqx.io:1883"
#define WIFI_CONNECTED_BIT BIT0
static EventGroupHandle_t wifi_event_group = NULL;

namespace iot {

class FanMqtt : public Thing {
 private:
  esp_mqtt_client_handle_t client_ = nullptr;
  bool mqtt_connected_ = false;
  bool fan_on_ = false;
  const char* topic_ = "syj_connect_esp01s_fan_topic";

  // 发送MQTT消息
  void SendMqttMessage(const std::string& message) {
    if (client_ == nullptr) {
      ESP_LOGE(TAG, "MQTT client is not initialized!");
      return;
    }
    if (!mqtt_connected_) {
      ESP_LOGE(TAG, "MQTT is not connected! Cannot send message.");
      return;
    }
    int msg_id = esp_mqtt_client_publish(
        client_, topic_, message.c_str(), message.length(), 1, 0);
    ESP_LOGI(TAG, "Message sent, msg_id:%d, topic:%s, msg:%s", msg_id, topic_, message.c_str());
  }

  // WiFi事件处理
  static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                      int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
      esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
      ESP_LOGI(TAG, "Wi-Fi disconnected, reconnecting...");
      esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
      xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
  }

  // MQTT事件处理
  static void mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    FanMqtt* self = static_cast<FanMqtt*>(handler_args);
    esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);
    switch (event_id) {
      case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT connected");
        self->mqtt_connected_ = true;
        esp_mqtt_client_subscribe(self->client_, self->topic_, 1);
        break;
      case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT disconnected");
        self->mqtt_connected_ = false;
        break;
      case MQTT_EVENT_DATA: {
        ESP_LOGI(TAG, "Received data: %.*s", event->data_len, event->data);
        std::string payload(event->data, event->data + event->data_len);
        if (payload == "1") {
          self->fan_on_ = true;
          ESP_LOGI(TAG, "[网络状态] 风扇已打开");
        } else if (payload == "0") {
          self->fan_on_ = false;
          ESP_LOGI(TAG, "[网络状态] 风扇已关闭");
        }
        break;
      }
      case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT event error");
        self->mqtt_connected_ = false;
        break;
      default:
        break;
    }
  }

  static void mqtt_task(void* pvParameters) {
    FanMqtt* self = static_cast<FanMqtt*>(pvParameters);
    self->RunMqttTask();
  }

  void RunMqttTask() {
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi connected, starting MQTT...");
    mqtt_app_start();
    ESP_LOGI(TAG, "MQTT task completed");
    vTaskDelete(NULL);
  }
  //MQTT 配置初始化方式改为结构体全零初始化+分步赋值，这样不会再有任何顺序或 designator 报错，兼容所有 ESP-IDF 版本和编译器。
  void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = MQTT_URI;
    mqtt_cfg.credentials.username = "";
    mqtt_cfg.credentials.authentication.password = "";
    mqtt_cfg.credentials.client_id = "esp32_fan";
    mqtt_cfg.session.last_will.topic = topic_;
    mqtt_cfg.session.last_will.msg = "0";
    mqtt_cfg.session.last_will.qos = 1;
    mqtt_cfg.session.last_will.retain = 0;
    mqtt_cfg.session.disable_clean_session = 0;
    mqtt_cfg.session.keepalive = 30;
    mqtt_cfg.network.timeout_ms = 10000;
    client_ = esp_mqtt_client_init(&mqtt_cfg);
    if (client_ == nullptr) {
      ESP_LOGE(TAG, "Failed to initialize MQTT client");
      return;
    }
    esp_mqtt_client_register_event(
          client_, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID),
          mqtt_event_handler, this);
    esp_mqtt_client_start(client_);
  }

 public:
  FanMqtt()
      : Thing("FanMqtt", "通过MQTT远程获取和控制风扇的状态") {
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
    xTaskCreate(mqtt_task, "mqtt_task", 4096, this, 5, NULL);
    properties_.AddBooleanProperty("state", "风扇状态：true-开，false-关", [this]() -> bool {
      return fan_on_;
    });
    methods_.AddMethod("SetFanState", "设置风扇状态", ParameterList({
      Parameter("state", "风扇状态：0-关，1-开", kValueTypeNumber, true)
    }), [this](const ParameterList& parameters) {
      int state = parameters["state"].number();
      ESP_LOGI(TAG, "收到设置命令: %d", state);
      fan_on_ = (state == 1);
      SendMqttMessage(state == 1 ? "1" : "0");
    });
    methods_.AddMethod("turn_on", "开风扇", ParameterList(), [this](const ParameterList&) {
      ESP_LOGI(TAG, "收到开风扇命令");
      fan_on_ = true;
      SendMqttMessage("1");
    });
    methods_.AddMethod("turn_off", "关风扇", ParameterList(), [this](const ParameterList&) {
      ESP_LOGI(TAG, "收到关风扇命令");
      fan_on_ = false;
      SendMqttMessage("0");
    });
  }

  ~FanMqtt() {
    if (client_ != nullptr) {
      esp_mqtt_client_stop(client_);
      esp_mqtt_client_destroy(client_);
      client_ = nullptr;
    }
  }
};

}  // namespace iot

DECLARE_THING(FanMqtt);