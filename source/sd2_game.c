/* sd2_game.c -- calls into Soda Dungeon 2's managed code, resolved by name.
 *
 * Everything goes through the il2cpp runtime API, with no RVAs, starting from the
 * static Main.services (GameServices). A method that cannot be found is logged and
 * skipped, and a managed exception is caught by il2cpp_runtime_invoke and logged.
 *
 * MIT.
 */
#include <stddef.h>
#include <stdint.h>

#include "so_util.h"
#include "util.h"
#include "sd2_game.h"

typedef void *(*fn_domain_get)(void);
typedef void *(*fn_domain_assembly_open)(void *, const char *);
typedef void *(*fn_assembly_get_image)(void *);
typedef void *(*fn_class_from_name)(void *, const char *, const char *);
typedef void *(*fn_class_get_field)(void *, const char *);
typedef void  (*fn_field_static_get)(void *, void *);
typedef void  (*fn_field_get)(void *, void *, void *);
typedef void *(*fn_class_get_method)(void *, const char *, int);
typedef void *(*fn_runtime_invoke)(void *, void *, void **, void **);
typedef void *(*fn_object_get_class)(void *);
typedef void *(*fn_virtual_method)(void *, void *);
typedef void *(*fn_thread_current)(void);
typedef void *(*fn_thread_attach)(void *);

static fn_domain_get           p_domain_get;
static fn_domain_assembly_open p_assembly_open;
static fn_assembly_get_image   p_get_image;
static fn_class_from_name      p_class_from_name;
static fn_class_get_field      p_get_field;
static fn_field_static_get     p_static_get;
static fn_field_get            p_field_get;
static fn_class_get_method     p_get_method;
static fn_runtime_invoke       p_invoke;
static fn_object_get_class     p_object_class;
static fn_virtual_method       p_virtual;
static fn_thread_current       p_thread_current;
static fn_thread_attach        p_thread_attach;

static int   g_bound;
static void *g_main_class;
static void *g_services_field;

int sd2_game_bind(so_module *m) {
  p_domain_get      = (fn_domain_get)so_try_find_addr_rx(m, "il2cpp_domain_get");
  p_assembly_open   = (fn_domain_assembly_open)so_try_find_addr_rx(m, "il2cpp_domain_assembly_open");
  p_get_image       = (fn_assembly_get_image)so_try_find_addr_rx(m, "il2cpp_assembly_get_image");
  p_class_from_name = (fn_class_from_name)so_try_find_addr_rx(m, "il2cpp_class_from_name");
  p_get_field       = (fn_class_get_field)so_try_find_addr_rx(m, "il2cpp_class_get_field_from_name");
  p_static_get      = (fn_field_static_get)so_try_find_addr_rx(m, "il2cpp_field_static_get_value");
  p_field_get       = (fn_field_get)so_try_find_addr_rx(m, "il2cpp_field_get_value");
  p_get_method      = (fn_class_get_method)so_try_find_addr_rx(m, "il2cpp_class_get_method_from_name");
  p_invoke          = (fn_runtime_invoke)so_try_find_addr_rx(m, "il2cpp_runtime_invoke");
  p_object_class    = (fn_object_get_class)so_try_find_addr_rx(m, "il2cpp_object_get_class");
  p_virtual         = (fn_virtual_method)so_try_find_addr_rx(m, "il2cpp_object_get_virtual_method");
  p_thread_current  = (fn_thread_current)so_try_find_addr_rx(m, "il2cpp_thread_current");
  p_thread_attach   = (fn_thread_attach)so_try_find_addr_rx(m, "il2cpp_thread_attach");

  g_bound = p_domain_get && p_assembly_open && p_get_image && p_class_from_name &&
            p_get_field && p_static_get && p_field_get && p_get_method && p_invoke &&
            p_object_class && p_virtual && p_thread_current && p_thread_attach;
  debugPrintf("[game] runtime API %s\n", g_bound ? "bound" : "MISSING");
  return g_bound ? 0 : -1;
}

/* Main.services, or NULL until the game has created it. */
static void *services(void) {
  if (!g_bound)
    return NULL;
  void *domain = p_domain_get();
  if (!domain)
    return NULL;
  if (!p_thread_current())
    p_thread_attach(domain);

  if (!g_main_class) {
    void *assembly = p_assembly_open(domain, "Assembly-CSharp.dll");
    void *image = assembly ? p_get_image(assembly) : NULL;
    g_main_class = image ? p_class_from_name(image, "", "Main") : NULL;
    g_services_field = g_main_class ? p_get_field(g_main_class, "services") : NULL;
    if (!g_services_field) {
      debugPrintf("[game] Main.services not found\n");
      g_main_class = NULL;
      return NULL;
    }
  }
  void *out = NULL;
  p_static_get(g_services_field, &out);
  return out;
}

static void *service(const char *name) {
  void *svc = services();
  if (!svc)
    return NULL;
  void *field = p_get_field(p_object_class(svc), name);
  if (!field) {
    debugPrintf("[game] GameServices.%s not found\n", name);
    return NULL;
  }
  void *out = NULL;
  p_field_get(svc, field, &out);
  return out;
}

/* Returns the invoke result. *ok is 0 when the method is missing or throws. */
static void *call(void *obj, const char *name, int argc, void **args, int *ok) {
  *ok = 0;
  if (!obj)
    return NULL;
  void *method = p_get_method(p_object_class(obj), name, argc);
  if (!method) {
    debugPrintf("[game] method %s not found\n", name);
    return NULL;
  }
  method = p_virtual(obj, method);
  void *exc = NULL;
  void *ret = p_invoke(method, obj, args, &exc);
  if (exc) {
    debugPrintf("[game] %s threw a managed exception\n", name);
    return NULL;
  }
  *ok = 1;
  return ret;
}

/* A boxed bool holds its value right after the 16-byte object header. */
static int unbox_bool(void *boxed) {
  return boxed ? *(uint8_t *)((uintptr_t)boxed + 0x10) : 0;
}

void sd2_game_close_top_menu(void) {
  int ok;
  void *top = call(service("uiSystem"), "GetTopMenu", 0, NULL, &ok);
  if (!ok || !top)
    return;
  call(top, "ForceCloseIfPossible", 0, NULL, &ok);
  debugPrintf("[game] B: ForceCloseIfPossible %s\n", ok ? "called" : "failed");
}

void sd2_game_save(void) {
  int ok;
  void *mgr = service("saveManager");
  void *set = call(mgr, "CurrentSaveFileIsSet", 0, NULL, &ok);
  if (!ok || !unbox_bool(set)) {
    debugPrintf("[game] save skipped: no current save file\n");
    return;
  }
  uint8_t use_backup = 0;
  void *args[1] = { &use_backup };
  call(mgr, "SaveGameData", 1, args, &ok);
  debugPrintf("[game] save on exit %s\n", ok ? "done" : "failed");
}

void sd2_game_cursor_show(int show) {
  int ok;
  call(service("cursorController"), show ? "ShowCursorGraphic" : "HideCursorGraphic", 0, NULL, &ok);
  debugPrintf("[game] cursor %s %s\n", show ? "show" : "hide", ok ? "ok" : "failed");
}

/* AdPlacement 0 is the interstitial after an adventure, which takes the game's own
 * timeout path. Every other placement is rewarded, and takes the failure path that
 * tells the waiting menu no reward was earned. */
static void *g_ad_backend;
static int   g_ad_placement;

/* Some menus call RequestAd before subscribing to the result, so the answer waits
 * for the next frame or they would miss it and wait forever. */
void sd2_game_ad_unavailable(void *backend, int placement) {
  g_ad_backend = backend;
  g_ad_placement = placement;
}

void sd2_game_tick(void) {
  if (!g_ad_backend)
    return;
  void *backend = g_ad_backend;
  int placement = g_ad_placement;
  g_ad_backend = NULL;
  int ok;
  call(backend, placement == 0 ? "InterstitialTimedOut" : "BroadcastRewardedAdFailure",
       0, NULL, &ok);
  debugPrintf("[game] ad placement %d unavailable (%s)\n", placement, ok ? "ok" : "failed");
}
