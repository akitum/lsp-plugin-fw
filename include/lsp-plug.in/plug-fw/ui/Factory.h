/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 24 нояб. 2020 г.
 *
 * lsp-plugin-fw is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-plugin-fw is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-plugin-fw. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LSP_PLUG_IN_PLUG_FW_UI_FACTORY_H_
#define LSP_PLUG_IN_PLUG_FW_UI_FACTORY_H_

#ifndef LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_
    #error "Use #include <lsp-plug.in/plug-fw/ui.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_UI_IMPL_H_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/meta/types.h>
#include <lsp-plug.in/plug-fw/plug.h>
#include <lsp-plug.in/common/types.h>

namespace lsp
{
    namespace ui
    {
        /**
         * Factory function
         * @param meta
         * @return
         */
        typedef Module * (* factory_func_t)(const meta::plugin_t *meta);

        /**
         * Factory for UI module
         */
        class LSP_SYMBOL_HIDDEN Factory
        {
            private:
                Factory & operator = (const Factory &);

            private:
                static Factory         *pRoot;
                Factory                *pNext;
                factory_func_t          pFunc;
                const meta::plugin_t  **vList;
                size_t                  nItems;

            public:
                explicit Factory();
                explicit Factory(factory_func_t func, const meta::plugin_t **list, size_t items);
                explicit Factory(const meta::plugin_t **list, size_t items);
                virtual ~Factory();

            public:
                /**
                 * Get root registered factory
                 * @return root registered factory
                 */
                static inline Factory          *root()      { return pRoot;             }

                /**
                 * Get next registered factory
                 * @return next registered factory
                 */
                inline Factory                 *next()      { return pNext;             }

            public:
                /**
                 * Enumerate the metadata for plugins produced by the factory
                 * @param index index of plugin starting with 0 and growing with 1
                 * @return plugin metadata or NULL if there is no more data in enumeration
                 */
                virtual const meta::plugin_t   *enumerate(size_t index) const;

                /**
                 * Create plugin UI module
                 * @param meta plugin metadata returned from enumerate() method
                 * @return pointer to created plugin UI or NULL on error.
                 * Plugin UI module should be destroyed by the operator delete call
                 */
                virtual Module                 *create(const meta::plugin_t *meta) const;
        };
    }
}

#endif /* LSP_PLUG_IN_PLUG_FW_UI_FACTORY_H_ */
