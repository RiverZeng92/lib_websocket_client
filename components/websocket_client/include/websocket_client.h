
/*****************
 * author:River Zeng
 *****************/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WEBSOCKET_CLIENT_H
#define WEBSOCKET_CLIENT_H

#include "lwip/err.h"
#include "lwip/sys.h"


#include <sys/socket.h>



//TCP服务器状态
struct tcp_client_state
{
 u8_t state;
};

// CRLF characters to terminate lines/handshakes in headers.
#define CRLF "\r\n"

// Amount of time (in ms) a user may be connected before getting disconnected
// for timing out (i.e. not sending any data to the server).
#define TIMEOUT_IN_MS 10000

// ACTION_SPACE is how many actions are allowed in a program. Defaults to
// 5 unless overwritten by user.
#ifndef CALLBACK_FUNCTIONS
#define CALLBACK_FUNCTIONS 1
#endif

// Don't allow the client to send big frames of data. This will flood the Arduinos
// memory and might even crash it.
#ifndef MAX_FRAME_LENGTH
#define MAX_FRAME_LENGTH 256
#endif

#define SIZE(array) (sizeof(array) / sizeof(*array))

// WebSocket protocol constants
// First byte
#define WS_FIN            0x80
#define WS_OPCODE_TEXT    0x01
#define WS_OPCODE_BINARY  0x02
#define WS_OPCODE_CLOSE   0x08
#define WS_OPCODE_PING    0x09
#define WS_OPCODE_PONG    0x0a
// Second byte
#define WS_MASK           0x80
#define WS_SIZE16         126
#define WS_SIZE64         127


# define WS_SEND_MAX_LENGTH  0xffff

/********
 * 发送队列的深度
 */

#define SEND_DATA_QUEUE_DEPTH  25

/********
 * 接受任务队列深度
 */
#define RECV_DATA_QUEUE_DEPTH  4

/**
 * 用於连接websocket的类型
 */
typedef struct {
	struct sockaddr_in server_addr;
	char* path;
} web_socket_info;


/**
 * websocket的连接状态
 */
typedef enum{
	DISCONNECT=0,
	CONNECTING,
	CONNECTED
} ws_conn_state;



struct web_socket_ctx;

/**
 * websocket发送数据
 */
typedef struct {
	struct web_socket_ctx *ctx;
	int socket_id;
	void* data;
	uint8_t is_binary;
	uint8_t is_ping;
	uint16_t len;
} web_socket_data_package;

/**
 * websocket的上下文
 */
typedef struct web_socket_ctx {
	int socket_id;
	ws_conn_state state;
	void (*recv_callback)(web_socket_data_package*  data);
} web_socket_ctx;


/**
 * socket接收队列任务最大值
 */
#define WEBSOCKET_MAX_NUM 10

/**
 * websocket接收数据
 */
//typedef struct {
//	web_socket_ctx *ctx;
//	int socket_id;
//
//} web_socket_recv_handler;


void web_socket_send_task(void *param);

esp_err_t connect_websocket(web_socket_info *info, web_socket_ctx *ctx);


void web_socket_recv_task(void* params);

esp_err_t send_string_data(web_socket_ctx* ctx, char* data, uint16_t total_length);

esp_err_t send_binary_data(web_socket_ctx* ctx, uint8_t* data, uint16_t total_length);

esp_err_t add_recv_task(web_socket_ctx* ctx);

esp_err_t send_ping_data(web_socket_ctx* ctx);



#endif

#ifdef __cplusplus
}
#endif
