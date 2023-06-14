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

#include "write_buffer.h"

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

static bool
_on_new_client(pen_event_t ev, pen_socket_t fd,
        void *user PEN_UNUSED, struct sockaddr_in *addr PEN_UNUSED)
{
    pen_event_base_t *eb;
    pen_client_t *self = pen_memory_pool_get(g_self.pool_);

    if (PEN_UNLIKELY(self == NULL))
        return false;

    eb = &self->eb_;
    eb->fd_ = fd;
    eb->on_event_ = _on_event;

    if (PEN_UNLIKELY(!pen_connector_new(&self->connector_)))
        goto error;

    if (PEN_UNLIKELY(!pen_event_add_w(ev, eb)))
        goto error;

    return true;
error:
    pen_memory_pool_put(g_self.pool_, self);
    return false;
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
pen_client_proxy_success(pen_connector_t *connector)
{
    pen_client_t *self = PEN_ENTRY(connector, pen_client_t, connector_);
    pen_assert(pen_event_mod_rw(g_self.ev_, &self->eb_),
            "on client proxy failed.");
}

void
pen_client_proxy_closed(pen_connector_t *connector)
{
    pen_client_t *self = PEN_ENTRY(connector, pen_client_t, connector_);
    _on_close(&self->eb_);
}

/////////////////////////// Handle client event ///////////////////////////////

static void
_on_read(pen_event_base_t *eb)
{
    extern char pen_read_buf[];
    pen_client_t *self = (pen_client_t *)eb;
    ssize_t ret, wret;

again:
    ret = read(eb->fd_, pen_read_buf, PEN_GLOBAL_BUF_REAL);
    if (ret < 0) {
        if (errno == EAGAIN) {
            errno = 0;
            return;
        }
        _on_close(eb);
        return;
    }

    if (ret == 0) {
        PEN_WARN("[client] read 0 size!");
        _on_close(eb);
        return;
    }

    wret = write(self->connector_.eb_->fd_, pen_read_buf, ret);
    if (wret < 0) {
        if (errno != EAGAIN) {
            _on_close(eb);
            return;
        }
        wret = 0;
    }

    if (wret < ret) {
        pen_assert(pen_event_mod_w(g_self.ev_, eb), "on_read error.");
        pen_assert(pen_write_buffer_append(self->connector_.eb_,
                   pen_read_buf + wret, ret - wret), "on_read error.");
        return;
    }
    if (ret == PEN_GLOBAL_BUF_REAL)
        goto again;
}

static void
_on_close(pen_event_base_t *eb)
{
    pen_client_t *self = (pen_client_t *) eb;

    close(eb->fd_);
    pen_connector_delete(&self->connector_);
    pen_memory_pool_put(g_self.pool_, self);
}

static bool
_on_write(pen_event_base_t *eb)
{
    extern void pen_connector_reopen(pen_connector_t *self);
    pen_client_t *self = (pen_client_t *) eb;

    if (eb->wbuf_ == NULL)
        return true;
    if (!pen_send_write_buffer(eb))
        return false;
    if (eb->wbuf_ == NULL)
        pen_connector_reopen(&self->connector_);
    return true;
}

static void
_on_event(pen_event_base_t *eb, uint16_t events)
{
    if (events == PEN_EVENT_CLOSE)
        return _on_close(eb);

    if ((events & PEN_EVENT_WRITE) && !_on_write(eb))
        return;

    if (events & PEN_EVENT_READ)
        _on_read(eb);
}

