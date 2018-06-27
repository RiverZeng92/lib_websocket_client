#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"

#include "websocket_client.h"

EventGroupHandle_t tcp_event_group;
#define WIFI_CONNECTED_BIT BIT0

esp_err_t event_handler(void *ctx, system_event_t *event) {

	const char* TAG = "event_task";

	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		esp_wifi_connect();
		xEventGroupClearBits(tcp_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_CONNECTED:
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI(TAG, "got ip:%s\n",
				ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip))
		;
		xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_AP_STACONNECTED:
		ESP_LOGI(TAG, "station:"MACSTR" join,AID=%d\n",
				MAC2STR(event->event_info.sta_connected.mac),
				event->event_info.sta_connected.aid)
		;
		xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_AP_STADISCONNECTED:
		ESP_LOGI(TAG, "station:"MACSTR"leave,AID=%d\n",
				MAC2STR(event->event_info.sta_disconnected.mac),
				event->event_info.sta_disconnected.aid)
		;
		xEventGroupClearBits(tcp_event_group, WIFI_CONNECTED_BIT);
		break;
	default:
		break;
	}

	return ESP_OK;
}

void rec_callback1(web_socket_data_package* data) {
	const char* TAG = "client_task";
	ESP_LOGI(TAG, "rec1: %s", (char* )(data->data));
}

void rec_callback2(web_socket_data_package* data) {
	const char* TAG = "client_task";
	ESP_LOGI(TAG, "rec2: %s", (char* )(data->data));
}

static void client_task(void *pvParameters) {

	const char* TAG = "client_task";
	ESP_LOGI(TAG, "task tcp_conn.");

	/*wating for connecting to AP*/
	xEventGroupWaitBits(tcp_event_group, WIFI_CONNECTED_BIT, false, true,
	portMAX_DELAY);

	struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(8080),
			.sin_addr.s_addr = inet_addr("192.168.75.153") };

	web_socket_info info = { .server_addr = addr, .path = "/" };

	web_socket_ctx ctx;
	web_socket_ctx ctx2;

	xTaskCreate(&web_socket_send_task, "send_task", 4096, NULL, 14, NULL);
	xTaskCreate(&web_socket_recv_task, "recv_task", 4096, NULL, 13, NULL);

	esp_err_t result = connect_websocket(&info, &ctx);
	web_socket_recv_handler handler;
	web_socket_recv_handler handler2;

	if (result == ESP_OK) {
		handler.socket_id = ctx.socket_id;
		handler.recv_callback = &rec_callback1;
		add_recv_task(&handler);
	}

	esp_err_t result2 = connect_websocket(&info, &ctx2);
	if (result2 == ESP_OK) {
		handler2.socket_id = ctx2.socket_id;
		handler2.recv_callback = &rec_callback2;
		add_recv_task(&handler2);
	}

	while (1) {
		char* a = "hello fsgasdsgdasfsgd2";

		send_string_data(&ctx, a, strlen(a));
		send_string_data(&ctx2, a, strlen(a));
		vTaskDelay(600 / portTICK_RATE_MS);

	}

}

void app_main(void) {

	tcp_event_group = xEventGroupCreate();
	nvs_flash_init();
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;

	xTaskCreate(&client_task, "client_task", 4096, NULL, 15, NULL);

	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	wifi_config_t sta_config = { .sta = { .ssid = "BattleSystem", .password =
			"B123456789", .bssid_set = false } };
	ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));
	ESP_ERROR_CHECK(esp_wifi_start());
	//ESP_ERROR_CHECK(esp_wifi_connect());

	//xEventGroupClearBits(tcp_event_group, WIFI_CONNECTED_BIT);

	//gpio_set_direction(GPIO_NUM_4, GPIO_MODE_OUTPUT);
	//int level = 0;

	while (true) {
		//gpio_set_level(GPIO_NUM_4, level);
		//level = !level;
		vTaskDelay(300 / portTICK_PERIOD_MS);
	}
}

