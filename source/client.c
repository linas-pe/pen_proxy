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
#include "common.h"

#include <pen_utils/pen_memory_pool.h>
#include <pen_socket/pen_listener.h>
#include <pen_socket/pen_proxy.h>
#include <pen_socket/pen_socket.h>

static void _on_event(pen_event_base_t *eb, uint16_t events);

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
    extern pen_connector_t pen_connector_new(pen_client_t);
    extern void pen_connector_close(pen_connector_t self);
    pen_client_t self = pen_memory_pool_get(g_self.pool_);

    if (PEN_UNLIKELY(self == NULL))
        return NULL;

    self->fd_ = fd;
    self->on_event_ = _on_event;
    self->user_ = pen_connector_new(self);

    if (PEN_UNLIKELY(self->user_ == NULL))
        goto error;

    if (PEN_UNLIKELY(!pen_event_add_w(ev, self))) {
        PEN_ERROR("[client] on_new_client add event failed !!!!");
        pen_connector_close(self->user_);
        goto error;
    }
    return self;
error:
    pen_memory_pool_put(g_self.pool_, self);
    return NULL;
}

bool
pen_client_init(pen_event_t ev)
{
    g_self.pool_ = PEN_MEMORY_POOL_INIT(g_client_pool_size, pen_event_base_t);
    if (PEN_UNLIKELY(g_self.pool_ == NULL))
        return false;

    g_self.ev_ = ev;
    g_self.listener_ = pen_listener_init(ev, g_local_host,
            g_local_port, 16, _on_new_client, NULL);
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

static inline void
_on_close(pen_client_t self)
{
    extern void pen_connector_close(pen_connector_t self);
    pen_connector_t conn = self->user_;

    pen_event_base_close(self);
    pen_memory_pool_put(g_self.pool_, self);

    if (conn != NULL)
        pen_connector_close(conn);
}

void
pen_client_proxy_success(pen_event_t ev, pen_client_t self)
{
    extern void pen_connector_close(pen_connector_t self);

    pen_connector_t conn = self->user_;
    self->user_ = NULL;
    conn->user_ = NULL;
    if (pen_event_proxy(self, g_self.ev_, conn, ev))
        return;
    pen_connector_close(conn);
    _on_close(self);
}

void
pen_client_proxy_failed(pen_client_t self)
{
    pen_event_base_close(self);
    pen_memory_pool_put(g_self.pool_, self);
}

static void
_on_event(pen_event_base_t *eb, uint16_t events)
{
    if (events == PEN_EVENT_CLOSE)
        return _on_close(eb);
}

