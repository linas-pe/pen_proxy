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
#pragma once

#include <pen_types.h>
#include <pen_event.h>

PEN_EXTERN_C_START

typedef struct {
    pen_event_base_t *eb_;
    bool connected_;
} pen_connector_t;

pen_event_t pen_connector_init(void)
    PEN_WARN_UNUSED_RESULT
;

void pen_connector_destroy(void);

bool pen_connector_new(pen_connector_t *self)
    PEN_NONNULL(1)
    PEN_WARN_UNUSED_RESULT
;

void pen_connector_delete(pen_connector_t *self)
    PEN_NONNULL(1)
;

PEN_EXTERN_C_END

