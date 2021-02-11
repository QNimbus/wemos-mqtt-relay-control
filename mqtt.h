// MQTT Client ID
#define MQTT_CLIENT_ID "wemos_001"

// MQTT Topics
#define MQTT_BASE "node/"MQTT_CLIENT_ID
#define MQTT_SYS MQTT_BASE"/sys"
#define WILL_TOPIC MQTT_SYS"/status"
#define SSID_TOPIC MQTT_SYS"/ssid"
#define IPV4_TOPIC MQTT_SYS"/ip"
#define TEMPERATURE_TOPIC MQTT_SYS"/temp"
#define UPTIME_TOPIC MQTT_SYS"/uptime"
#define RELAY_LED_TOPIC MQTT_BASE"/relay_led"
#define RELAY_PWR_TOPIC MQTT_BASE"/relay_pwr"
