<div style="text-align: right">Language: <a href="/en/libstrophe-first-xmpp-program/">English</a> | <a href="/ru/libstrophe-first-xmpp-program/">Русский</a></div>

# libstrophe: Your first XMPP program

When learning a programming language, it is common to write a "Hello world" as the first program. There is similar tradition for XMPP: an echo bot. Let's write such a program in C language with libstrophe library. And figure libstrophe basics out on the way.

Building process is presented to Unix like systems or Cygwin. However, the code works under Windows with Visual Studio as well.

libstrophe source code is distributed with simple examples, including basic.c and bot.c:

| Example      | Description                              |
| ------------ |:---------------------------------------- |
| [basic.c][1] | Example of XMPP connection establishment |
| [bot.c][2]   | Example of an echo bot                   |


As can you see, we have already got a complete echo bot. And here we will repeat the way with small steps.

[1]: https://github.com/strophe/libstrophe/blob/master/examples/basic.c
[2]: https://github.com/strophe/libstrophe/blob/master/examples/bot.c

## Preparation

Install libstrophe library:
 * Refer to the package manager of your system
 * For binary based Linux distributions, also install development package (libstrophe-dev or libstrophe-devel)
 * Alternatively, libstrophe can be built and installed from sources ([https://github.com/strophe/libstrophe])

For simplicity, we will use `Makefile` to build our bot:
```make
all:
	gcc `pkg-config --cflags --libs libstrophe` -o bot bot.c
```

As you probably noticed, we will save our code to file `bot.c`. To build the program it is enough to run `make`.

## Connection with an XMPP server

Before using libstrophe we need to initialize it. This step sets global objects up. After finishing using libstrophe a good idea will be to release the global objects. Usually, this is done at program start and before termination respectively:

```c
include <strophe.h>

int main()
{
	xmpp_initialize();

	/* Code that uses libstrophe API. */

	xmpp_shutdown();
	return 0;
}
```

The first step to work with XMPP is to establish XMPP connection with a server:

```c
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
```

This code block has one missed item: `conn_handler`. We will get back to it later. And now let's review the block in details.

libstrophe operates with a small set of fundamental objects. Among which are "context" and "connection". They have types `xmpp_ctx_t` and `xmpp_conn_t` respectively. Such objects are created with interface `..._new()` and released with interfaces `..._free()` or `..._release()`. Where `_release()` suffix points that the object works with the "reference counting" approach and the object is destroyed only after releasing all the references. Users usually don't use reference counting for connection objects, therefore, `xmpp_conn_release()` destroys the object immediately in this case.

Context is an isolation domain. For each context it is required to launch a dedicated event loop which serves non-overlapping sets of connections and event handlers. For most cases it is enough to create a single context for an XMPP program. Multiple contexts may be handy for advanced users to optimize a multi-threaded application with multiple connections.

Connection object is an object which represents an XMPP connection. Initially, such an object is created without configuration and is set with various attributes further. Examples of the attributes are JID, password and others.
Connection object has an internal state which can be `DISCONNECTED`, `CONNECTING` or `CONNECTED`. Newly created object is always in `DISCONNECTED` state until user calls `xmpp_connect_client()`. It is important that `xmpp_connect_client()` performs only initial step of XMPP connection establishment. In particular, it resolves domain name and initiates TCP connection establishment. Further establishment is done asynchronously in the event loop.

Don't forget to replace strings "user@xmpp.org" and "password" with your configuration.

As it is mentioned above, libstrophe handles majority of its functionality in the [event loop][3]. Therefore, it is important to run the event loop in time and not to block application for long. There are two ways to execute the event loop: `xmpp_run()` and `xmpp_run_once()`. The first function is a blocking endless loop which is interrupted on `xmpp_stop()`. The second function runs a single iteration of the event loop and returns. The second way is useful if the application has its own event loop or uses an external one, for example, GTK.

[3]: https://en.wikipedia.org/wiki/Event_loop

## Event loop

libstrophe is written in the event-driven paradigm. When an event happens libstrophe calls a respective user's event handler and the application reacts to the event within the handler. User can register handlers for the following categories of events: XMPP connection events, incoming XMPP stanzas, timers. Events without a registered handler are lost.

Connection event handler is mandatory and registered during an `xmpp_connect_client()` call. `conn_handler()` is the handler in the code above. Main events for such a handler are `XMPP_CONN_CONNECT` and `XMPP_CONN_DISCONNECT`. The first event is raised after a successful XMPP connection establishment and the second after connection termination or loss.
The handler accepts a connection object as an argument. In this way it is possible to distinguish between different connections in a single handler function. All the handlers also accept userdata which can be unique for each handler. In the example above, we pass NULL to `xmpp_connect_client()` as userdata, because we don't use it in the example.

This is a simple implementation of the connection event handler:

```c
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
```

A connection event handler accepts the following arguments:

| Argument     | Description                                         |
| ------------ |:--------------------------------------------------- |
| conn         | Connection where the event happened                 |
| status       | The event (XMPP_CONN_CONNECT, XMPP_CONN_DISCONNECT) |
| error        | Error value on a connection loss                    |
| stream_error | XMPP stream error if such an error is detected      |
| userdata     | User data which is passed to xmpp_connect_client()  |


Logic of the code block is the following:

 * On successful XMPP connection establishment we register a stanza event handler for "message" stanzas. After this `message_handler()` will be called for every incoming message.
 * Send stanza `<presence/>` to define our "online" status.
 * After successful connection termination we stop the event loop with `xmpp_stop()` call what forces `xmpp_run()` to quit.

Registering stanza event handler within the connection handler we don't lose any message even if a message arrives to the TCP socket before `xmpp_handler_add()` is executed. This is guaranteed by libstrophe: event handlers block the event loop execution until they return. XMPP stanzas are processed within the event loop sequentially.

`xmpp_send()` doesn't actually send the stanza immediately and rather adds it to the send queue. The stanza is physically sent from the event loop when the TCP socket allows.

User must free allocated stanza when they don't need it anymore. In our case this happens after calling the send function. As mentioned above, interface `..._release()` releases a reference to the object, but doesn't necessarily destroys the object. libstrophe functions take additional reference to stanzas when required. Therefore, user doesn't have to worry about stanza life cycle within the event loop and may release a stanza even if it is occupied by libstrophe internally.
In most cases it stanzas may be released after `xmpp_send()` call. Taking this as a rule will reduce chance of memory leaks.

## Working with stanzas

The last step for our bot is to handle incoming messages:

```c
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
```

Stanza event handler has the following interface:

| Argument | Description                                              |
| -------- |:-------------------------------------------------------- |
| conn     | Connection where the stanza arrived                      |
| stanza   | The stanza which is represented by xmpp_stanza_t object  |
| userdata | User data which is passed to xmpp_handler_add()          |


Stanza handlers return value 0 or 1. This can be considered as a boolean value which specifies whether to leave the handler active or remove it. For messages handler we usually want to leave it active. However, sometimes we expect a single specific stanza and don't need the handler after receiving the stanza.

Logic of the message handler is the following:
 * Ignore messages without `<body/>` element
 * On incoming "quit" message initiate XMPP connection termination
 * Create new stanza object which contains copy of the original text and the recipient is the author of the original message
 * Add the stanza to the send queue.
 * Return from the handler back to the event loop

`xmpp_message_get_body()` returns text from the `<body/>` element as a newly allocated string. User must free the string eventually to avoid memory leaks. It is important to keep tracking what interface returns allocated string and what interface returns pointer to an internal string. As a rule, returned `char*` string must be freed and `const char*` strings mustn't be freed. To be on the safe side, refer to the doxygen documentation on ambiguous situations. Also it is important to keep in mind that a `const char*` string is valid until the respective object is destroyed.

`xmpp_disconnect()`, similarly to `xmpp_send()`, executes most of its functionality within the event loop. In fact, this function adds `</stream:stream>` to the send queue and further the event loop raises `XMPP_CONN_DISCONNECT` on incoming `</stream:stream>`.

In the example above we introduced function `generate_id()`. Its goal is to generate unique identifier for `id` attribute. There are multiple ways to generate identifiers, for example, monotonic counter, random string, UUID, etc. Here is a simple implementation:

```c
const char *generate_id()
{
	static char str[9];
	static uint32_t counter = 0;

	snprintf(str, sizeof(str), "%x", counter++);
	return str;
}
```

Fundamental interface for working with stanzas is the following:
 * `xmpp_stanza_new()`
 * `xmpp_stanza_set_name()`
 * `xmpp_stanza_set_text()`
 * `xmpp_stanza_set_attribute()`
 * `xmpp_stanza_add_child()`
 * `xmpp_stanza_release()`

However, for frequently used stanza types libstrophe provides convenient wrappers on top of the above API. For example, `xmpp_message_set_body()` can be implemented using the basic functions:

```c
void xmpp_message_set_body(xmpp_stanza_t *msg, const char *text)
{
	xmpp_ctx_t *ctx = xmpp_stanza_get_context(msg);
	xmpp_stanza_t *body;
	xmpp_stanza_t *stanza_text;

	body = xmpp_stanza_new(ctx);
	stanza_text = xmpp_stanza_new(ctx);

	xmpp_stanza_set_name(body, "body");
	xmpp_stanza_set_text(stanza_text, text);

	xmpp_stanza_add_child(body, stanza_text);
	xmpp_stanza_release(stanza_text);
	xmpp_stanza_add_child(msg, body);
	xmpp_stanza_release(body);
}
```

As you can see, wrappers allow to write more compact code, however, they're not mandatory.

## Final bot source code

In conclusion, let's assemble all the parts together. The code is also available by the link: [bot.c][4].

```c
#include <strophe.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
	xmpp_ctx_t *ctx;
	xmpp_conn_t *conn;
	int rc;

	ctx = xmpp_ctx_new(NULL, NULL);
	conn = xmpp_conn_new(ctx);
	xmpp_conn_set_jid(conn, "user@xmpp.org");
	xmpp_conn_set_pass(conn, "password");
	rc = xmpp_connect_client(conn, NULL, 0, conn_handler, NULL);
	if (rc == XMPP_EOK) {
		xmpp_run(ctx);
	}

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
```

[4]: /en/libstrophe-first-xmpp-program/bot.c

## Troubleshooting

In case of problems or to study XMPP details, debug logs will be handy. libstrophe provides convenient mechanism to work with logs. The most easiest way to enable logs is to use builtin log handler. The handler prints logs to stderr. This is achieved with a small modification to `xmpp_ctx_new()` call:

```c
void bot_run()
{
	/* ... */
	ctx = xmpp_ctx_new(NULL, xmpp_get_default_logger(XMPP_LEVEL_DEBUG));
	/* ... */
}
```
