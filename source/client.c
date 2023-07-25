/*
 * Copyright (C) 2021  Linas <linas@justforfun.cn>
 * Author: linas <linas@justforfun.cn>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "client.h"

#include <pen_utils/pen_memory_pool.h>
#include <pen_socket/pen_listener.h>
#include <pen_socket/pen_proxy.h>
#include <pen_socket/pen_socket.h>

static void _on_event(pen_event_base_t *eb, uint16_t events);
static void _on_close(pen_event_base_t *eb);

const char *g_local_host = NULL;
unsigned short g_local_port = 1234;
uint16_t g_client_pool_size = 8;

static struct {
    pen_event_t ev_;
    pen_listener_t listener_;
    pen_memory_pool_t pool_;
} g_self;

static pen_event_base_t *
_on_new_client(pen_event_t ev, int fd,
        void *user PEN_UNUSED, struct sockaddr_in *addr PEN_UNUSED)
{
    pen_event_base_t *eb;
    pen_client_t *self = pen_memory_pool_get(g_self.pool_);

    if (PEN_UNLIKELY(self == NULL))
        return NULL;

    eb = &self->eb_;
    eb->fd_ = fd;
    eb->on_event_ = _on_event;

    if (PEN_UNLIKELY(!pen_connector_new(&self->connector_)))
        goto error;

    if (PEN_UNLIKELY(!pen_event_add_w(ev, eb)))
        goto error;

    return eb;
error:
    pen_memory_pool_put(g_self.pool_, self);
    return NULL;
}

bool
pen_client_init(pen_event_t ev)
{
    g_self.pool_ = PEN_MEMORY_POOL_INIT(g_client_pool_size, pen_client_t);
    if (PEN_UNLIKELY(g_self.pool_ == NULL))
        return false;

    g_self.ev_ = ev;
    g_self.listener_ = pen_listener_init(ev, g_local_host,
            g_local_port, 128, _on_new_client, NULL);
    if (PEN_LIKELY(g_self.listener_ != NULL))
        return true;

    pen_memory_pool_destroy(g_self.pool_);
    return false;
}

void
pen_client_destroy(void)
{
    pen_listener_destroy(g_self.listener_);
    pen_memory_pool_destroy(g_self.pool_);
}

void
pen_client_proxy_success(pen_event_t ev, pen_connector_t *connector)
{
    pen_client_t *self = PEN_ENTRY(connector, pen_client_t, connector_);
    if (pen_event_proxy(&self->eb_, g_self.ev_, connector->eb_, ev))
        return;
    pen_connector_close(connector);
    _on_close(&self->eb_);
}

/////////////////////////// Handle client event ///////////////////////////////

static void
_on_close(pen_event_base_t *eb)
{
    pen_client_t *self = (pen_client_t *) eb;

    pen_event_base_close(eb);
    pen_memory_pool_put(g_self.pool_, self);
}

static void
_on_event(pen_event_base_t *eb, uint16_t events)
{
    if (events == PEN_EVENT_CLOSE)
        return _on_close(eb);
}

