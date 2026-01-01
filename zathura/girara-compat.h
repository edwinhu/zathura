/* SPDX-License-Identifier: Zlib */

#ifndef GIRARA_COMPAT_H
#define GIRARA_COMPAT_H

#include <girara/datastructures.h>
#include <glib.h>

/* Define g_autoptr cleanup functions for girara types */
G_DEFINE_AUTOPTR_CLEANUP_FUNC(girara_list_t, girara_list_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(girara_tree_node_t, girara_node_free)

#endif /* GIRARA_COMPAT_H */
