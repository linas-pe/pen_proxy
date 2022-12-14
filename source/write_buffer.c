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
#include "write_buffer.h"

#include <pen_memory_pool.h>
#include <pen_log.h>

struct _pen_write_buffer {
    pen_write_buffer_t buf_;
    char placeholder_[PEN_GLOBAL_BUF_REAL];
};

char pen_read_buf[PEN_GLOBAL_BUF_REAL];

static struct {
    pen_memory_pool_t pool_;
} g_self;

bool
pen_write_buffer_init(void)
{
    g_self.pool_ = PEN_MEMORY_POOL_INIT(32, struct _pen_write_buffer);
    return g_self.pool_ != NULL;
}

void
pen_write_buffer_destroy(void)
{
    pen_memory_pool_destroy(g_self.pool_);
}

bool
pen_write_buffer_append(pen_event_base_t *eb, char *buf, size_t len)
{
    pen_write_buffer_t *wb = pen_memory_pool_get(g_self.pool_);
    if (PEN_UNLIKELY(wb == NULL))
        return false;

    memcpy(wb->data_, buf, len);
    wb->len_ = len;
    eb->wbuf_ = wb;
    return true;
}

bool
pen_send_write_buffer(pen_event_base_t *eb)
{
    ssize_t ret;
    size_t len;
    pen_write_buffer_t *wb = eb->wbuf_;

    len = wb->len_ - wb->offset_;
    ret = write(eb->fd_, wb->data_ + wb->offset_, len);

    if (PEN_UNLIKELY(ret < 0)) {
        if (PEN_UNLIKELY(errno != EAGAIN))
            return false;
        errno = 0;
        return true;
    }

    wb->offset_ += ret;
    if (PEN_LIKELY(ret == (ssize_t)len)) {
        pen_memory_pool_put(g_self.pool_, wb);
        eb->wbuf_ = NULL;
    }
    return true;
}

