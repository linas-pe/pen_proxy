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

#include <pen_socket/pen_connect_pool.h>
#include <pen_socket/pen_socket.h>

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

    args.host_ = g_remote_host;
    args.port_ = g_remote_port;
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

pen_connector_t
pen_connector_new(pen_client_t client)
{
    return pen_connect_pool_get(g_self.pool_, client);
}

void
pen_connector_close(pen_connector_t self)
{
    pen_connect_pool_close(self);
}

///////////////////////////// Handler connector events ////////////////////////

static void
_on_connected(void *user)
{
    extern void pen_client_proxy_success(pen_event_t ev, pen_client_t self);
    pen_client_proxy_success(g_self.ev_, (pen_client_t)user);
}

static void
_on_read(void *user PEN_UNUSED)
{
    pen_assert2(false);
}

static void
_on_close(void *user)
{
    extern void pen_client_proxy_failed(pen_client_t self);
    if (user != NULL)
        pen_client_proxy_failed(user);
}

static bool
_on_write(void *user PEN_UNUSED)
{
    pen_assert2(false);
    return true;
}

