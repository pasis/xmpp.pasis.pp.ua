/* gcc `pkg-config --cflags --libs libstrophe` -o bot bot.c */

#include <strophe.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Простой генератор идентификаторов, возвращающий 32-битный счетчик в виде
 * 16-ричного числа.
 */
const char *generate_id()
{
	static char str[9];
	static uint32_t counter = 0;

	snprintf(str, sizeof(str), "%x", counter++);
	return str;
}

int message_handler(xmpp_conn_t *conn, xmpp_stanza_t *stanza, void *userdata)
{
        xmpp_ctx_t *ctx = xmpp_conn_get_context(conn);
	xmpp_stanza_t *reply;
	char *text;

	text = xmpp_message_get_body(stanza);
	if (text == NULL) {
		/* Игнорируем пустые сообщения. */
		return 1;
	}

	if (strcmp(text, "quit") == 0) {
		/* Мы получили команду к выходу. */
		xmpp_disconnect(conn);
		xmpp_free(ctx, text);
		return 0;
	}

	/* Создаем новое сообщение в ответ на оригинальную строфу. */
	reply = xmpp_message_new(ctx, "chat", xmpp_stanza_get_from(stanza),
				 generate_id());
	/* Устанавливаем текст внутри элемента <body/>. */
	xmpp_message_set_body(reply, text);
	xmpp_free(ctx, text);

	xmpp_send(conn, reply);
	xmpp_stanza_release(reply);

	return 1;
}

void conn_handler(xmpp_conn_t *conn,
		  xmpp_conn_event_t status,
		  int error,
		  xmpp_stream_error_t *stream_error,
		  void *userdata)
{
	xmpp_ctx_t *ctx = xmpp_conn_get_context(conn);
	xmpp_stanza_t *pres;

	/* Мы не обрабатываем эти аргументы в примере. */
	(void)error;
	(void)stream_error;

	if (status == XMPP_CONN_CONNECT) {
		/* Регистрируем обраточик сообщений. */
		xmpp_handler_add(conn, message_handler, NULL,
				 "message", NULL, NULL);

		/* Отправляем начальный <presence/>, чтобы
		   отображаться как "в сети" у контактов. */
		pres = xmpp_presence_new(ctx);
		xmpp_send(conn, pres);
		xmpp_stanza_release(pres);
	} else {
		/* Предполагаем, что это событие
		   XMPP_CONN_DISCONNECT. */
		xmpp_stop(ctx);
	}
}

void bot_run()
{
	/* libstrophe контекст. */
	xmpp_ctx_t *ctx;
	/* Объект XMPP соединения. */
	xmpp_conn_t *conn;
	/* Код возврата для libstrophe API. */
	int rc;

	/* Создание нового контекста. */
	ctx = xmpp_ctx_new(NULL, NULL);
	/* Создание нового "пустого" объекта XMPP соединения. */
	conn = xmpp_conn_new(ctx);
	/* Установка JID для соединения. */
	xmpp_conn_set_jid(conn, "user@xmpp.org");
	/* Установка пароля для соединения. */
	xmpp_conn_set_pass(conn, "password");
	/* Объявление о намерении установить XMPP соединение. */
	rc = xmpp_connect_client(conn, NULL, 0, conn_handler, NULL);
	if (rc == XMPP_EOK) {
		/* Запуск блокирующего цикла событий. */
		xmpp_run(ctx);
	}

	/* Освобождение ранее созданных объектов. */
	xmpp_conn_release(conn);
	xmpp_ctx_free(ctx);
}

int main()
{
	xmpp_initialize();

	bot_run();

	xmpp_shutdown();
	return 0;
}
