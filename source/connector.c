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
#include "connector.h"

#include <pen_connect_pool.h>
#include <pen_socket.h>

#include "client.h"
#include "write_buffer.h"

static void _on_read(void *self);
static void _on_close(void *self);
static bool _on_write(void *self);
static void _on_connected(void *self);

const char *g_remote_host = "127.0.0.1";
unsigned short g_remote_port = 1234;

static struct {
    pen_event_t ev_;
    pen_connect_pool_t pool_;
} g_self;

pen_event_t
pen_connector_init(void)
{
    pen_event_t ev;
    static pen_connect_pool_args_t args = {
        .on_connected_ = _on_connected,
        .on_read_ = _on_read,
        .on_write_ = _on_write,
        .on_close_ = _on_close,
    };

    ev = pen_event_init(16);
    if (PEN_UNLIKELY(ev == NULL))
        return NULL;

    args.host_ = g_remote_host,
    args.port_ = g_remote_port,
    g_self.ev_ = args.ev_ = ev;
    g_self.pool_ = pen_connect_pool_init(&args);
    if (PEN_UNLIKELY(g_self.pool_ == NULL)) {
        pen_event_destroy(ev);
        return NULL;
    }
    return g_self.ev_;
}

void
pen_connector_destroy(void)
{
    pen_connect_pool_destroy(g_self.pool_);
    pen_event_destroy(g_self.ev_);
}

bool
pen_connector_new(pen_connector_t *self)
{
    pen_event_base_t *eb;

    eb = pen_connect_pool_get(g_self.pool_, self);
    if (PEN_UNLIKELY(eb == NULL))
        return false;
    self->eb_ = eb;

    return true;
}

void
pen_connector_delete(pen_connector_t *self)
{
    if (self->eb_ == NULL)
        return;
    pen_connect_pool_put(self->eb_);
    self->eb_ = NULL;
}

void
pen_connector_reopen(pen_connector_t *self)
{
    pen_assert(pen_event_mod_rw(g_self.ev_, self->eb_),
            "connector reopen failed.");
}

static inline pen_event_base_t *
_client_eb(pen_connector_t *self)
{
    pen_client_t *client = PEN_ENTRY(self, pen_client_t, connector_);
    return &client->eb_;
}

///////////////////////////// Handler connector events ////////////////////////

static void
_on_connected(void *user)
{
    extern void pen_client_proxy_success(pen_connector_t *self);
    pen_client_proxy_success((pen_connector_t*)user);
}

static void
_on_read(void *user)
{
    extern char pen_read_buf[];
    pen_connector_t *self = (pen_connector_t *)user;
    pen_event_base_t *eb = self->eb_;
    ssize_t ret, wret;

    if (eb == NULL)
        return;
again:
    ret = read(eb->fd_, pen_read_buf, PEN_GLOBAL_BUF_REAL);
    if (ret < 0) {
        if (errno == EAGAIN) {
            errno = 0;
            return;
        }
        _on_close(user);
        return;
    }

    if (ret == 0) {
        PEN_WARN("[client] read 0 size!");
        _on_close(user);
        return;
    }

    wret = write(_client_eb(self)->fd_, pen_read_buf, ret);
    if (wret < 0) {
        if (errno != EAGAIN) {
            _on_close(user);
            return;
        }
        wret = 0;
    }

    if (wret < ret) {
        pen_assert(pen_event_mod_w(g_self.ev_, eb), "on read failed.");
        pen_assert(pen_write_buffer_append(_client_eb(self),
                   pen_read_buf + wret, ret - wret), "on read failed.");
        return;
    }
    if (ret == PEN_GLOBAL_BUF_REAL)
        goto again;
}

static void
_on_close(void *user)
{
    extern void pen_client_proxy_closed(pen_connector_t *self);
    pen_connector_t *self = user;

    self->eb_ = NULL;
    pen_client_proxy_closed(self);
}

static bool
_on_write(void *user)
{
    extern void pen_client_proxy_success(pen_connector_t *self);
    pen_connector_t *self = (pen_connector_t *)user;
    pen_event_base_t *eb = self->eb_;

    if (eb == NULL)
        return true;
    if (eb->wbuf_ == NULL)
        return true;
    if (!pen_send_write_buffer(eb))
        return false;
    if (eb->wbuf_ == NULL)
        pen_client_proxy_success(self);

    return true;
}

