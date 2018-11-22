/*
 * Copyright (c) 2018 Igalia S.L.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef LIBAPPBRIDGE_H
#define LIBAPPBRIDGE_H

#include <string>

#include <libwindowmanager.h>
#include <libhomescreen.hpp>
#include <ilm/ilm_control.h>

class AppBridgeDelegate {
  public:
    virtual ~AppBridgeDelegate() {}
    virtual void OnActive() {}
    virtual void OnInactive() {}
    virtual void OnVisible() {}
    virtual void OnInvisible() {}
    virtual void OnSyncDraw() {}
    virtual void OnFlushDraw() {}
    virtual void OnTabShortcut() {}
    virtual void OnScreenMessage(const char* message) {}
    virtual void OnSurfaceCreated(int id, pid_t surface_pid) {}
    virtual void OnSurfaceDestroyed(int id, pid_t surface_pid) {}
    virtual void OnRequestedSurfaceID(int id, pid_t* surface_pid_output) {}
};

class AppBridge
{
  public:
    explicit AppBridge(int port, const std::string& token,
                       const std::string& id, const std::string& role,
                       AppBridgeDelegate* delegate);
    void SetName(const std::string& name) { m_name = name; }
    virtual ~AppBridge();

    void SetupSurface(int id);
    void OnIviControlUpdated(ilmObjectType object, t_ilm_uint id,
                             t_ilm_bool created);
    static void IviControlCallback(ilmObjectType object,
                                   t_ilm_uint id,
                                   t_ilm_bool created,
                                   void *user_data);
  private:
    std::string m_role;
    std::string m_path;

    std::string m_id;
    std::string m_name;

    int m_port;
    std::string m_token;

    LibWindowmanager *m_wm;
    LibHomeScreen *m_hs;

    bool m_pending_create = false;
    AppBridgeDelegate* m_delegate;

    int InitWindowManager(void);
    int InitHomeScreen(void);
};

#endif  // LIBAPPBRIDGE_H
