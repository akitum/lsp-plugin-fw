/*
 * Copyright (C) 2021 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2021 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-plugin-fw
 * Created on: 12 дек. 2021 г.
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

#ifndef LSP_PLUG_IN_PLUG_FW_WRAP_VST2_IMPL_UI_WRAPPER_H_
#define LSP_PLUG_IN_PLUG_FW_WRAP_VST2_IMPL_UI_WRAPPER_H_

#include <lsp-plug.in/plug-fw/version.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/ui_wrapper.h>
#include <lsp-plug.in/plug-fw/wrap/vst2/ui_ports.h>

namespace lsp
{
    namespace vst2
    {
        UIWrapper::UIWrapper(ui::Module *ui, resource::ILoader *loader, vst2::Wrapper *wrapper):
            IWrapper(ui, loader)
        {
            pWrapper        = wrapper;
            sRect.top       = 0;
            sRect.left      = 0;
            sRect.bottom    = 0;
            sRect.right     = 0;
        }

        UIWrapper::~UIWrapper()
        {
        }

        vst2::UIPort *UIWrapper::create_port(const meta::port_t *port, const char *postfix)
        {
            // Find the matching port for the backend
            vst2::UIPort *vup = NULL;
            vst2::Port *vp    = pWrapper->find_by_id(port->id);
            if (vp == NULL)
                return vup;

            switch (port->role)
            {
                case meta::R_MESH:
                    vup = new vst2::UIMeshPort(port, vp);
                    break;

                case meta::R_STREAM:
                    vup = new vst2::UIStreamPort(port, vp);
                    break;

                case meta::R_FBUFFER:
                    vup = new vst2::UIFrameBufferPort(port, vp);
                    break;

                case meta::R_OSC:
                    if (meta::is_out_port(port))
                        vup     = new vst2::UIOscPortIn(port, vp);
                    else
                        vup     = new vst2::UIOscPortOut(port, vp);
                    break;

                case meta::R_PATH:
                    vup = new vst2::UIPathPort(port, vp);
                    break;

                case meta::R_CONTROL:
                case meta::R_METER:
                case meta::R_BYPASS:
                    // VST specifies only INPUT parameters, output should be read in different way
                    if (meta::is_out_port(port))
                        vup     = new vst2::UIMeterPort(port, vp);
                    else
                        vup     = new vst2::UIParameterPort(port, static_cast<vst2::ParameterPort *>(vp));
                    break;

                case meta::R_PORT_SET:
                {
                    char postfix_buf[MAX_PARAM_ID_BYTES], param_name[MAX_PARAM_ID_BYTES];
                    UIPortGroup     *upg     = new vst2::UIPortGroup(static_cast<vst2::PortGroup *>(vp));

                    for (size_t row=0; row<upg->rows(); ++row)
                    {
                        // Generate postfix
                        snprintf(postfix_buf, sizeof(postfix_buf)-1, "%s_%d", (postfix != NULL) ? postfix : "", int(row));

                        // Obtain the related port for backend
                        for (const meta::port_t *p=port->members; p->id != NULL; ++p)
                        {
                            // Initialize port name
                            strncpy(param_name, p->id, sizeof(param_name));
                            strncat(param_name, postfix_buf, sizeof(param_name));
                            param_name[sizeof(param_name) - 1] = '\0';

                            // Obtain backend port and create UI port for it
                            vp    = pWrapper->find_by_id(port->id);
                            if (vp != NULL)
                                create_port(vp->metadata(), postfix_buf);
                        }
                    }

                    vup     = upg;
                    break;
                }

                default:
                    break;
            }

            // Add port to the list of UI ports
            if (vup != NULL)
            {
                vUIPorts.add(vup);
                vPorts.add(vup);
            }

            return vup;
        }

        status_t UIWrapper::init(void *root_widget)
        {
            status_t res = STATUS_OK;

            // Obtain UI metadata
            const meta::plugin_t *meta = pUI->metadata();
            if (pUI->metadata() == NULL)
                return STATUS_BAD_STATE;

            // Create list of ports and sort it in ascending order by the identifier
            for (const meta::port_t *port = meta->ports ; port->id != NULL; ++port)
                create_port(port, NULL);

            // Initialize parent
            if ((res = IWrapper::init()) != STATUS_OK)
                return res;

            // Initialize display settings
            tk::display_settings_t settings;
            resource::Environment env;

            settings.resources      = pLoader;
            settings.environment    = &env;

            LSP_STATUS_ASSERT(env.set(LSP_TK_ENV_DICT_PATH, LSP_BUILTIN_PREFIX "i18n"));
            LSP_STATUS_ASSERT(env.set(LSP_TK_ENV_LANG, "en_US"));
            LSP_STATUS_ASSERT(env.set(LSP_TK_ENV_CONFIG, "lsp-plugins"));

            // Create the display
            pDisplay = new tk::Display(&settings);
            if (pDisplay == NULL)
                return STATUS_NO_MEM;
            if ((res = pDisplay->init(0, NULL)) != STATUS_OK)
                return res;

            // Load visual schema
            if ((res = init_visual_schema()) != STATUS_OK)
                return res;

            // Initialize the UI
            if ((res = pUI->init(this, pDisplay)) != STATUS_OK)
                return res;

            // Build the UI
            if (meta->ui_resource != NULL)
            {
                if ((res = build_ui(meta->ui_resource, root_widget)) != STATUS_OK)
                {
                    lsp_error("Error building UI for resource %s: code=%d", meta->ui_resource, int(res));
                    return res;
                }
            }

            // Bind resize slot
            tk::Window *wnd  = window();
            if (wnd != NULL)
                wnd->slots()->bind(tk::SLOT_RESIZE, slot_ui_resize, this);

            // Call the post-initialization routine
            if (res == STATUS_OK)
                res = pUI->post_init();

            return res;
        }

        void UIWrapper::destroy()
        {
            // Unbind all UI ports
            for (size_t i=0, n=vUIPorts.size(); i < n; ++i)
            {
                ui::IPort *port = vUIPorts.uget(i);
                port->unbind_all();
                delete port;
            }
            vUIPorts.flush();

            // Call parent instance
            IWrapper::destroy();
        }

        core::KVTStorage *UIWrapper::kvt_lock()
        {
            return pWrapper->kvt_lock();
        }

        core::KVTStorage *UIWrapper::kvt_trylock()
        {
            return pWrapper->kvt_trylock();
        }

        bool UIWrapper::kvt_release()
        {
            return pWrapper->kvt_release();
        }

        void UIWrapper::dump_state_request()
        {
            pWrapper->query_display_draw();
        }

        void UIWrapper::main_iteration()
        {
            transfer_dsp_to_ui();
            IWrapper::main_iteration();
        }

        const meta::package_t *UIWrapper::package() const
        {
            return pWrapper->package();
        }

        void UIWrapper::transfer_dsp_to_ui()
        {
            // Try to sync position
            pUI->position_updated(pWrapper->position());
            pUI->sync_meta_ports();

            // DSP -> UI communication
            for (size_t i=0, nports=vUIPorts.size(); i < nports; ++i)
            {
                // Get UI port
                vst2::UIPort *vup   = vUIPorts.uget(i);
                do {
                    if (vup->sync())
                        vup->notify_all();
                } while (vup->sync_again());
            } // for port_id

            // Perform KVT synchronization
            core::KVTStorage *kvt = pWrapper->kvt_lock();
            if (kvt != NULL)
            {
                // Synchronize DSP -> UI transfer
                size_t sync;
                const char *kvt_name;
                const core::kvt_param_t *kvt_value;

                do
                {
                    sync = 0;

                    core::KVTIterator *it = kvt->enum_tx_pending();
                    while (it->next() == STATUS_OK)
                    {
                        kvt_name = it->name();
                        if (kvt_name == NULL)
                            break;
                        status_t res = it->get(&kvt_value);
                        if (res != STATUS_OK)
                            break;
                        if ((res = it->commit(core::KVT_TX)) != STATUS_OK)
                            break;

                        kvt_dump_parameter("TX kvt param (DSP->UI): %s = ", kvt_value, kvt_name);
                        kvt_notify_write(kvt, kvt_name, kvt_value);
                        ++sync;
                    }
                } while (sync > 0);

                // Synchronize UI -> DSP transfer
                #ifdef LSP_DEBUG
                {
                    core::KVTIterator *it = kvt->enum_rx_pending();
                    while (it->next() == STATUS_OK)
                    {
                        kvt_name = it->name();
                        if (kvt_name == NULL)
                            break;
                        status_t res = it->get(&kvt_value);
                        if (res != STATUS_OK)
                            break;
                        if ((res = it->commit(core::KVT_RX)) != STATUS_OK)
                            break;

                        kvt_dump_parameter("RX kvt param (UI->DSP): %s = ", kvt_value, kvt_name);
                    }
                }
                #else
                    kvt-> commit_all(core::KVT_RX);    // Just clear all RX queue for non-debug version
                #endif

                // Call garbage collection and release KVT storage
                kvt->gc();
                kvt_release();
            }
        }

        bool UIWrapper::show_ui(void *root_widget)
        {
            // Force all parameters to be re-shipped to the UI
            for (size_t i=0; i<vUIPorts.size(); ++i)
            {
                vst2::UIPort  *vp   = vUIPorts.uget(i);
                if (vp != NULL)
                    vp->notify_all();
            }

            core::KVTStorage *kvt = kvt_lock();
            if (kvt != NULL)
            {
                kvt->touch_all(core::KVT_TO_UI);
                kvt_release();
            }
            transfer_dsp_to_ui();

            // Show the UI window
            // TODO
//            ws::size_limit_t sr;
//            tk::Window *wnd     = window();
//            wnd->get_padded_size_limits(&sr);
//            wnd->size_request(&sr);
//
//            sRect.top           = 0;
//            sRect.left          = 0;
//            sRect.right         = sr.nMinWidth;
//            sRect.bottom        = sr.nMinHeight;
//
//            ws::rectangle_t r;
//            r.nLeft             = 0;
//            r.nTop              = 0;
//            r.nWidth            = sr.nMinWidth;
//            r.nHeight           = sr.nMinHeight;
//            resize_ui(&r);
//
//            wnd->show();

            return true;
        }

        void UIWrapper::hide_ui()
        {
            tk::Window *wnd     = window();
            if (wnd != NULL)
                wnd->hide();
        }

        void UIWrapper::resize_ui(const ws::rectangle_t *r)
        {
            tk::Window *wnd     = window();
            if (wnd == NULL)
                return;

            sRect.top           = 0;
            sRect.left          = 0;
            sRect.right         = r->nWidth;
            sRect.bottom        = r->nHeight;

            ws::rectangle_t rr;
            wnd->get_rectangle(&rr);
            lsp_trace("Get geometry: width=%d, height=%d", int(rr.nWidth), int(rr.nHeight));

            if ((rr.nWidth <= 0) || (rr.nHeight <= 0))
            {
                ws::size_limit_t sr;
                wnd->get_padded_size_limits(&sr);
                lsp_trace("Size request: width=%d, height=%d", int(sr.nMinWidth), int(sr.nMinHeight));
                rr.nWidth   = sr.nMinWidth;
                rr.nHeight  = sr.nMinHeight;
            }

            lsp_trace("audioMasterSizeWindow width=%d, height=%d", int(rr.nWidth), int(rr.nHeight));
            if (((sRect.right - sRect.left) != rr.nWidth) ||
                  ((sRect.bottom - sRect.top) != rr.nHeight))
                pWrapper->pMaster(pWrapper->pEffect, audioMasterSizeWindow, rr.nWidth, rr.nHeight, 0, 0);
        }

        ERect *UIWrapper::ui_rect()
        {
            lsp_trace("left=%d, top=%d, right=%d, bottom=%d",
                    int(sRect.left), int(sRect.top), int(sRect.right), int(sRect.bottom)
                );
            return &sRect;
        }

        status_t UIWrapper::slot_ui_resize(tk::Widget *sender, void *ptr, void *data)
        {
            UIWrapper *wrapper = static_cast<UIWrapper *>(ptr);
            ws::rectangle_t *rect = static_cast<ws::rectangle_t *>(data);
            wrapper->resize_ui(rect);
            return STATUS_OK;
        }
    } /* namespace vst2 */
} /* namespace lsp */



#endif /* LSP_PLUG_IN_PLUG_FW_WRAP_VST2_IMPL_UI_WRAPPER_H_ */
