/* gcc `pkg-config --cflags --libs libstrophe` -o bot bot.c */

#include <strophe.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Simple ID generator which returns 32-bit counter in hex representation. */
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
		/* Ignore empty messages. */
		return 1;
	}

	if (strcmp(text, "quit") == 0) {
		/* We recevied command to quit. */
		xmpp_disconnect(conn);
		xmpp_free(ctx, text);
		return 0;
	}

	/* Create new message in response to the original stanza. */
	reply = xmpp_message_new(ctx, "chat", xmpp_stanza_get_from(stanza),
				 generate_id());
	/* Set text inside <body/> element. */
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

	/* We don't handle these arguments in the example. */
	(void)error;
	(void)stream_error;

	if (status == XMPP_CONN_CONNECT) {
		/* Register incoming message event handler. */
		xmpp_handler_add(conn, message_handler, NULL,
				 "message", NULL, NULL);

		/* Send initial <presence/> to become "online"
		   for the contacts. */
		pres = xmpp_presence_new(ctx);
		xmpp_send(conn, pres);
		xmpp_stanza_release(pres);
	} else {
		/* Assume this is XMPP_CONN_DISCONNECT event. */
		xmpp_stop(ctx);
	}
}

void bot_run()
{
	/* libstrophe context. */
	xmpp_ctx_t *ctx;
	/* XMPP connection object. */
	xmpp_conn_t *conn;
	/* Return code for libstrophe API. */
	int rc;

	/* Create new context. */
	ctx = xmpp_ctx_new(NULL, NULL);
	/* Create new "empty" XMPP connection object. */
	conn = xmpp_conn_new(ctx);
	/* Set JID for the connection. */
	xmpp_conn_set_jid(conn, "user@xmpp.org");
	/* Set password for the connection. */
	xmpp_conn_set_pass(conn, "password");
	/* Start XMPP connection establishment. */
	rc = xmpp_connect_client(conn, NULL, 0, conn_handler, NULL);
	if (rc == XMPP_EOK) {
		/* Start blocking event loop. */
		xmpp_run(ctx);
	}

	/* Release previously created objects. */
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
