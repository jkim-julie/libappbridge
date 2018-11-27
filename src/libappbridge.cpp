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
#include <cassert>
#include <cstdio>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "libappbridge.h"

#define AGL_FATAL(fmt, ...) fatal("[AppBridge] ERROR: " fmt "\n", ##__VA_ARGS__)
#define AGL_WARN(fmt, ...) warn("[AppBridge] WARNING: " fmt "\n", ##__VA_ARGS__)
#define AGL_DEBUG(fmt, ...) debug("[AppBridge] DEBUG: " fmt "\n", ##__VA_ARGS__)
#define AGL_TRACE(file,line) debug("[AppBridge] %s:%d\n", file,line);

namespace {
constexpr const char kAreaNormalFull[] = "normal.full";

void fatal(const char* format, ...)
{
  va_list va_args;
  va_start(va_args, format);
  vfprintf(stderr, format, va_args);
  va_end(va_args);

  exit(EXIT_FAILURE);
}

void warn(const char* format, ...)
{
  va_list va_args;
  va_start(va_args, format);
  vfprintf(stderr, format, va_args);
  va_end(va_args);
}

void debug(const char* format, ...)
{
  va_list va_args;
  va_start(va_args, format);
  vfprintf(stderr, format, va_args);
  va_end(va_args);
}

int InitIlmControl(notificationFunc callback, void *user_data)
{
  if (ilm_init() != ILM_SUCCESS) {
    AGL_FATAL("Failed with ilm_init.");
    return -1;
  }
  if (ilm_registerNotification(callback, user_data) != ILM_SUCCESS) {
    AGL_FATAL("Failed with ilm_registerNotification.");
    return -1;
  }
  return 0;
}

int DestroyIlmControl()
{
  if (ilm_unregisterNotification() != ILM_SUCCESS)
    AGL_FATAL("Failed with ilm_unregisterNotification.");

  if (ilm_destroy() != ILM_SUCCESS) {
    AGL_FATAL("Failed with ilm_destroy.");
    return -1;
  }
  return 0;
}

} // namespace

void AppBridge::OnIviControlUpdated (ilmObjectType object, t_ilm_uint id,
                                    t_ilm_bool created)
{
  if (object == ILM_SURFACE) {
    struct ilmSurfaceProperties surf_props;

    ilm_getPropertiesOfSurface(id, &surf_props);
    pid_t surf_pid = surf_props.creatorPid;

    if (!created) {
      AGL_DEBUG("ILM_SURFACE (id=%d, pid=%d) destroyed.", (int)id, (int)surf_pid);
      m_delegate->OnSurfaceDestroyed(id, surf_pid);
      return;
    }

    AGL_DEBUG("ILM_SURFACE (id=%d, pid=%d) is created.", (int)id, (int)surf_pid);

    m_delegate->OnSurfaceCreated(id, surf_pid);
    pid_t found_surface_id;
    m_delegate->OnRequestedSurfaceID(id, &found_surface_id);
    AGL_DEBUG("ILM_SURFACE OnRequestedSurfaceID returns (found_surface_id:%d)", (int)found_surface_id);
    if (surf_pid == found_surface_id)
      SetupSurface(id);
    return;
  }

  if (object == ILM_LAYER) {
    if (created)
      AGL_DEBUG("ILM_LAYER: %d created.", (int)id);
    else
      AGL_DEBUG("ILM_LAYER: %d destroyed.", (int)id);
    return;
  }
}

void AppBridge::IviControlCallback (ilmObjectType object, t_ilm_uint id,
                                           t_ilm_bool created, void *user_data)
{
  AppBridge *runextra = static_cast<AppBridge*>(user_data);
  runextra->OnIviControlUpdated(object, id, created);
}

int AppBridge::InitWindowManager(void)
{
  m_wm = new LibWindowmanager();
  if (m_wm->init(m_port, m_token.c_str())) {
    AGL_DEBUG("Failed to initialize LibWindowmanager");
    return -1;
  }

  std::function< void(json_object*) > h_active = [this](json_object* object) {
    AGL_DEBUG("Got Event_Active");
    if (this->m_delegate)
        this->m_delegate->OnActive();
  };

  std::function< void(json_object*) > h_inactive = [this](json_object* object) {
    AGL_DEBUG("Got Event_Inactive");
    if (this->m_delegate)
        this->m_delegate->OnInactive();
  };

  std::function< void(json_object*) > h_visible = [this](json_object* object) {
    AGL_DEBUG("Got Event_Visible");
    if (this->m_delegate)
        this->m_delegate->OnVisible();
  };

  std::function< void(json_object*) > h_invisible = [this](json_object* object) {
    AGL_DEBUG("Got Event_Invisible");
    if (this->m_delegate)
        this->m_delegate->OnInvisible();
  };

  std::function< void(json_object*) > h_syncdraw =
      [this](json_object* object) {
    AGL_DEBUG("Got Event_SyncDraw");
    this->m_wm->endDraw(this->m_role.c_str());

    if (this->m_delegate)
        this->m_delegate->OnSyncDraw();
  };

  std::function< void(json_object*) > h_flushdraw= [this](json_object* object) {
    AGL_DEBUG("Got Event_FlushDraw");
    if (this->m_delegate)
        this->m_delegate->OnFlushDraw();
  };

  m_wm->set_event_handler(LibWindowmanager::Event_Active, h_active);
  m_wm->set_event_handler(LibWindowmanager::Event_Inactive, h_inactive);
  m_wm->set_event_handler(LibWindowmanager::Event_Visible, h_visible);
  m_wm->set_event_handler(LibWindowmanager::Event_Invisible, h_invisible);
  m_wm->set_event_handler(LibWindowmanager::Event_SyncDraw, h_syncdraw);
  m_wm->set_event_handler(LibWindowmanager::Event_FlushDraw, h_flushdraw);

  return 0;
}

int AppBridge::InitHomeScreen(void)
{
  m_hs = new LibHomeScreen();
  if (m_hs->init(m_port, m_token.c_str())) {
    AGL_DEBUG("Failed to initialize LibHomeScreen");
    return -1;
  }

  std::function< void(json_object*) > handler = [this] (json_object* object) {
    AGL_DEBUG("Activesurface %s ", this->m_role.c_str());
    this->m_wm->activateWindow(this->m_role.c_str(), kAreaNormalFull);

    if (this->m_delegate)
      this->m_delegate->OnTabShortcut();
  };

  m_hs->set_event_handler(LibHomeScreen::Event_TapShortcut, handler);

  std::function< void(json_object*) > h_default= [this](json_object* object) {
    const char *j_str = json_object_to_json_string(object);
    AGL_DEBUG("Got event [%s]", j_str);
    if (this->m_delegate)
      this->m_delegate->OnScreenMessage(j_str);
  };
  m_hs->set_event_handler(LibHomeScreen::Event_OnScreenMessage, h_default);

  return 0;
}

AppBridge::AppBridge(int port, const std::string& token, const std::string& id,
                     const std::string& role, AppBridgeDelegate* delegate)
{
  assert(delegate);

  m_port = port;
  m_token = token;
  m_id = id;
  m_role = role;
  m_delegate = delegate;

  // Setup WindowManager API
  if (InitWindowManager())
    AGL_FATAL("Failed to setup WindowManager API");

  // Setup HomScreen API
  if (InitHomeScreen())
    AGL_FATAL("Failed to setup HomeScreen API");

  // Setup ilmController API
  if (InitIlmControl(IviControlCallback, this))
    AGL_FATAL("Failed to initialize ilm control.");
  m_pending_create = true;
}

AppBridge::~AppBridge()
{
  AGL_DEBUG("AppBridge dtor");
  if (DestroyIlmControl())
    AGL_FATAL("Failed to destroy ilm control.");
  delete m_wm;
  delete m_hs;
}

void AppBridge::SetupSurface (int id)
{
  AGL_DEBUG("requestSurfaceXDG(%s,%d)", m_role.c_str(), id);
  m_wm->requestSurfaceXDG(this->m_role.c_str(), id);

  if (m_pending_create) {
    // Recovering 1st time tap_shortcut is dropped because
    // the application has not been run yet (1st time launch)
    m_wm->activateWindow(this->m_role.c_str(), kAreaNormalFull);
  }
}
