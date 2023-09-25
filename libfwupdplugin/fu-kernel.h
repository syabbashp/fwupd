/*
 * Copyright (C) 2017 Richard Hughes <richard@hughsie.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#pragma once

#include <gio/gio.h>

gboolean
fu_kernel_locked_down(void);
gboolean
fu_kernel_check_version(const gchar *minimum_kernel, GError **error);
gchar *
fu_kernel_get_firmware_search_path(GError **error);
gboolean
fu_kernel_set_firmware_search_path(const gchar *path, GError **error);
gboolean
fu_kernel_reset_firmware_search_path(GError **error);
GHashTable *
fu_kernel_get_config(GError **error);
GHashTable *
fu_kernel_parse_config(const gchar *buf, gsize bufsz, GError **error);
GHashTable *
fu_kernel_get_cmdline(GError **error);
GHashTable *
fu_kernel_parse_cmdline(const gchar *buf, gsize bufsz);
gboolean
fu_kernel_check_cmdline_mutable(GError **error);
gboolean
fu_kernel_add_cmdline_arg(const gchar *arg, GError **error);
gboolean
fu_kernel_remove_cmdline_arg(const gchar *arg, GError **error);
