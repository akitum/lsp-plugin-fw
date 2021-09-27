/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 27 сент. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_3D_VIEWER3D_H_
#define LSP_PLUG_IN_PLUG_FW_CTL_3D_VIEWER3D_H_

#ifndef LSP_PLUG_IN_PLUG_FW_CTL_IMPL_
    #error "Use #include <lsp-plug.in/plug-fw/ctl.h>"
#endif /* LSP_PLUG_IN_PLUG_FW_CTL_IMPL_ */

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/tk/tk.h>

namespace lsp
{
    namespace ctl
    {
        /**
         * ComboBox controller
         */
        class Viewer3D: public Widget
        {
            public:
                static const ctl_class_t metadata;

            protected:
                static status_t     slot_draw3d(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_resize(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_mouse_down(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_mouse_up(tk::Widget *sender, void *ptr, void *data);
                static status_t     slot_mouse_move(tk::Widget *sender, void *ptr, void *data);

            protected:
                lltl::darray<vertex3d_t>    vVertices;  // Vertices of the scene

            protected:
                status_t            render(ws::IR3DBackend *r3d);
                void                commit_view(ws::IR3DBackend *r3d);

            public:
                explicit Viewer3D(ui::IWrapper *wrapper, tk::Area3D *widget);
                virtual ~Viewer3D();

                virtual status_t    init();

            public:
                virtual void        set(ui::UIContext *ctx, const char *name, const char *value);
                virtual void        end(ui::UIContext *ctx);
        };

    } /* namespace ctl */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_CTL_3D_VIEWER3D_H_ */
