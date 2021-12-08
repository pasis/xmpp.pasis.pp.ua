/* gcc `pkg-config --cflags --libs libstrophe` -o bot bot.c */

#include <strophe.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Простой генератор идентификаторов, возвращающий 32-битный счетчик в виде
 * 16-ричного числа.
 */
const char *generate_id(void)
{
	static char str[9];
	static uint32_t counter = 0;

	snprintf(str, sizeof(str), "%x", counter++);
	return str;
}

void bot_message_store(const char *text, const char *from)
{
	/* TODO */
}

void bot_disconnect(xmpp_conn_t *conn)
{
	xmpp_disconnect(conn);
}

/*
 * Обработчик сообщений. Мы сохраняем сообщения из групповых чатов, а личные
 * сообщения рассматриваются как команды боту.
 */
int message_handler(xmpp_conn_t *conn, xmpp_stanza_t *stanza, void *userdata)
{
        xmpp_ctx_t *ctx = xmpp_conn_get_context(conn);
	xmpp_stanza_t *reply;
	const char *type;
	char *text;
	bool is_quit = false;

	text = xmpp_message_get_body(stanza);
	if (text == NULL) {
		/* Игнорируем пустые сообщения. */
		return 1;
	}

	/* Атрибут "type" является необязательным и может быть NULL. */
	if (type && strcmp(type, "groupchat") == 0) {
		/* Если сообщение из группового чата, сохраняем его в файл. */
		bot_message_store(text, xmpp_stanza_get_from(stanza));
	} else {
		/* На данный момент мы поддерживаем одну команду "quit". */
		if (strcmp(text, "quit") == 0)
			is_quit = true;
	}

	xmpp_free(ctx, text);

	if (is_quit) {
		/* Мы получили команду к выходу.*/
		bot_disconnect(conn);
	}
	return 1;
}

void muc_join(xmpp_conn_t *conn,
	      const char *muc_jid,
	      const char *nick,
	      const char *password)
{
	xmpp_ctx_t *ctx = xmpp_conn_get_context(conn);
	xmpp_stanza_t *pres;
	xmpp_stanza_t *x;
	xmpp_stanza_t *pass;
	const char *id;
	const char *from;
	char *fulljid;
	size_t len;

	from = xmpp_conn_get_bound_jid(conn)
	    ?: xmpp_conn_get_jid(conn);

	len = strlen(muc_jid) + strlen(nick) + 2U;
	fulljid = malloc(len);
	snprintf(fulljid, len, "%s/%s", muc_jid, nick);

	id = generate_id();

	pres = xmpp_presence_new(ctx);
	xmpp_stanza_set_id(pres, id);
	xmpp_stanza_set_from(pres, from);
	xmpp_stanza_set_to(pres, fulljid);
	x = xmpp_stanza_new(ctx);
	xmpp_stanza_set_name(x, "x");
	xmpp_stanza_set_ns(x, "http://jabber.org/protocol/muc");
	xmpp_stanza_add_child(presence, x);
	xmpp_stanza_release(x);
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
		/* Регистрируем обработчик сообщений. */
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
