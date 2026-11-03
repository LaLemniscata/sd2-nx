/* sd2_time.h -- UnityEngine.Time driven from a wall clock.
 *
 * The engine's TimeManager::Update is never called on this port (proved by the
 * render-loop heartbeat), so every managed Time.* value stays frozen. These
 * hooks answer the managed getters from our own monotonic clock instead.
 * RVAs are from Il2CppDumper output for CloverPit 1.4.8 and each is guarded.
 */
#ifndef CLOVERPIT_TIME_H
#define CLOVERPIT_TIME_H

#include <stdint.h>
#include "so_util.h"

/* UnityEngine.Time accessors, CloverPit 1.4.8. All share the same prologue
 * guard (stp x30, x19, [sp, #-0x10]! == 0xa9bf4ffe). */
#define RVA_Time_get_time                   0x2d34ba0u
#define RVA_Time_get_deltaTime              0x2d34c48u
#define RVA_Time_get_unscaledTime           0x2d34c70u
#define RVA_Time_get_unscaledDeltaTime      0x2d34c98u
#define RVA_Time_get_smoothDeltaTime        0x2d34cc0u
#define RVA_Time_get_timeScale              0x2d34ce8u
#define RVA_Time_get_frameCount             0x2d34d48u
#define RVA_Time_get_realtimeSinceStartup   0x2d33ba8u

/* Install the hooks. Each is verified against its guard word independently, so a
 * moved getter is skipped rather than mispatched. Returns how many landed. */
int cp_time_install(so_module *il2cpp);

/* Sample deltaTime. Call ONCE per frame from the render loop. */
void cp_time_tick(void);

int     cp_time_installed(void);
float   cp_time_now(void);
float   cp_time_delta(void);
int32_t cp_time_frames(void);

#endif /* CLOVERPIT_TIME_H */
