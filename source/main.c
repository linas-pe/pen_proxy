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

#include <signal.h>

#include <pen_utils/pen_options.h>
#include <pen_utils/pen_profile.h>
#include <pen_socket/pen_signal.h>
#include <pen_socket/pen_event.h>
#include <pen_socket/pen_dns.h>
#include <pen_socket/pen_proxy.h>

#include "client.h"
#include "connector.h"

static const char *_profile = NULL;
static bool running = true;

static inline void
_init_options(int argc, char *argv[])
{
#define _s(a,b,d) {#a, &b, sizeof(b), PEN_OPTION_STRING, d},
    pen_option_t opts[] = {
        _s(--config, _profile, "Profile name(default NULL)")
    };
    PEN_OPTIONS_INIT(argc, argv, opts);
#undef _s
}

PEN_ALWAYS_INLINE
static inline bool
_init_profile(void)
{
    extern const char *g_local_host;
    extern unsigned short g_local_port;
    extern const char *g_remote_host;
    extern unsigned short g_remote_port;
    extern uint16_t g_client_pool_size;

#define _s(a,b,d) {#a, &b, sizeof(b), PEN_OPTION_STRING, d},
#define _i(a,b,d) {#a, &b, sizeof(b), PEN_OPTION_UINT16, d},
#define _b(a,b,d) {#a, &b, sizeof(b), PEN_OPTION_BOOL, d},
    if (_profile == NULL)
        return false;

    pen_option_t opts[] = {
        _s(local_host, g_local_host, "local bind host(default NULL)")
        _i(local_port, g_local_port, "local bind port(default 8123)")
        _s(remote_host, g_remote_host, "remote bind host(default 127.0.0.1)")
        _i(remote_port, g_remote_port, "remote bind port(default 1234)")
        _i(client_pool, g_client_pool_size, "client pool size(default 8)")
        _s(log_info, __pen_log_filename, "log info file name(default NULL)")
        _s(log_err, __pen_err_filename, "log error file name(default NULL)")
    };
    return pen_profile_init(_profile, opts, sizeof(opts) / sizeof(opts[0]));
#undef _b
#undef _i
#undef _s
}

static void
_on_signal(int id PEN_UNUSED)
{
    fflush(NULL);
    running = false;
}

int
main(int argc, char *argv[])
{
    pen_event_t evs[2];

    _init_options(argc, argv);
    if (!_init_profile())
        return 1;

    pen_socket_init_once();

    evs[0] = pen_event_init(3);
    pen_assert(evs[0] != NULL, "event init failed.");
    pen_assert(pen_signal_init(evs[0]), "signal init failed.");
    pen_assert(pen_signal(SIGTERM, _on_signal), "SIGTERM init failed.");
    pen_assert(pen_signal(SIGINT, _on_signal), "SIGINT init failed.");

    pen_assert2(pen_event_proxy_init());
    pen_assert(pen_dns_init(evs[0]), "dns init failed.");
    pen_assert(pen_client_init(evs[0]), "client init failed");

    evs[1] = pen_connector_init();
    pen_assert(evs[1] != NULL, "connector init failed");

    pen_event_start_all(&running, evs, 2);

    pen_connector_destroy();
    pen_client_destroy();
    pen_signal_destroy();
    pen_dns_destroy();
    pen_event_destroy(evs[0]);
    pen_log_destroy();
    return 0;
}

