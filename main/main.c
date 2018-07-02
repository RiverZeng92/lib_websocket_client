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

void rec_callback3(web_socket_data_package* data) {
	const char* TAG = "client_task";
	ESP_LOGI(TAG, "rec3: %s", (char* )(data->data));
}

static void client_task(void *pvParameters) {

	const char* TAG = "client_task";
	ESP_LOGI(TAG, "task tcp_conn.");

	/*wating for connecting to AP*/
	xEventGroupWaitBits(tcp_event_group, WIFI_CONNECTED_BIT, false, true,
	portMAX_DELAY);

	xTaskCreate(&web_socket_send_task, "send_task", 4096, NULL, 14, NULL);
	xTaskCreate(&web_socket_recv_task, "recv_task", 4096, NULL, 13, NULL);

	struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(8080),
			.sin_addr.s_addr = inet_addr("192.168.75.187") };

	web_socket_info info = { .server_addr = addr, .path = "/" };

	web_socket_ctx ctx;
	web_socket_ctx ctx2;
	web_socket_ctx ctx3;

	esp_err_t result = connect_websocket(&info, &ctx);

	if (result == ESP_OK) {
		ctx.recv_callback = &rec_callback1;
		add_recv_task(&ctx);
	}

	esp_err_t result2 = connect_websocket(&info, &ctx2);
	if (result2 == ESP_OK) {
		ctx2.recv_callback = &rec_callback2;
		add_recv_task(&ctx2);
	}

	esp_err_t result3 = connect_websocket(&info, &ctx3);
	if (result3 == ESP_OK) {
		ctx3.recv_callback = &rec_callback3;
		add_recv_task(&ctx3);
	}

	uint8_t a4[5] = {0xff, 0xff, 0xaa, 0xaa, 0xaa};
	while (1) {

		char* a1 = "hello fsgasdsgdasfsgd1";

		char* a2 = "hello fsgasdsgdasfsgd2";

		char* a3 = "hello fsgasdsgdasfsgd3";



		send_string_data(&ctx, a1, strlen(a1));
		send_string_data(&ctx2, a2, strlen(a2));
		//send_string_data(&ctx3, a3, strlen(a3));

		send_binary_data(&ctx3, a4, 5);

		if (ctx.state != CONNECTED) {

			ESP_LOGI(TAG, "close1:---------------------------- ");
		}

		if (ctx2.state != CONNECTED) {
			ESP_LOGI(TAG, "close2:---------------------------- ");

		}

		if (ctx3.state != CONNECTED) {
			ESP_LOGI(TAG, "close3:---------------------------- ");

		}

		vTaskDelay(10 / portTICK_RATE_MS);

		send_ping_data(&ctx);
		send_ping_data(&ctx2);
		send_ping_data(&ctx3);
		vTaskDelay(300/ portTICK_RATE_MS);

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

