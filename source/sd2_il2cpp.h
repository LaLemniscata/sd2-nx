/* sd2_il2cpp.h -- managed-side hook targets in CloverPit's libil2cpp.so.
 *
 * RVAs come from Il2CppDumper's dump.cs for CloverPit 1.4.8 and were each checked
 * against libil2cpp.so by disassembling the entry. GUARD_* is the first
 * instruction word at that RVA; every installer verifies it before writing, so a
 * game update that moves these leaves the engine stock rather than corrupted.
 *
 * WHY HOOK MANAGED CODE AT ALL, given sd2_input.c prefers nativeInjectEvent?
 * Because the dump settles a question that guesswork had got wrong. CloverPit
 * drives input through Rewired, and Rewired's Android path is
 * UnityInputJoystickSource + ThreadSafeUnityInput + AndroidUnityInputHelper --
 * i.e. it reads UnityEngine.Input, plus a JNI query to android.hardware.input
 * .InputManager for device metadata. So UnityEngine.Input IS the surface Rewired
 * consults, and hooking it is the badpiggies_nx approach after all, just aimed at
 * the joystick methods rather than the touch ones.
 *
 * The two paths are complementary, not alternatives:
 *   - these hooks make a controller EXIST and report state;
 *   - nativeInjectEvent (sd2_input.c) is still the right route for touch.
 *
 * A NICE SURPRISE: CloverPit already ships Nintendo button prompts. Its
 * global-metadata.dat contains JOYSTICK_ELEMENT_NAME_NINTENDO_A / _B / _X / _Y /
 * _ZL / _ZR / _L / _R / _PLUS / _MINUS / _LEFT_STICK_* / _HOME. If the joystick
 * we present is identified by Rewired as a Nintendo pad, the game draws correct
 * Switch prompts with no further work.
 */
#ifndef CLOVERPIT_IL2CPP_H
#define CLOVERPIT_IL2CPP_H

#include <stdint.h>
#include "so_util.h"

typedef struct {
  const char *name;
  uint32_t    rva;
  uint32_t    guard;
} il2cpp_target;

/* UnityEngine.Input, CloverPit 1.4.8. Verified by disassembly. */
#define RVA_Input_GetKey             0x2d71a98u
#define RVA_Input_GetKeyDown         0x2d71b10u
#define RVA_Input_GetKeyUp           0x2d71ad4u
#define RVA_Input_GetAxis            0x2d7135cu
#define RVA_Input_GetAxisRaw         0x2d714ccu
#define RVA_Input_GetButtonDown      0x2d7163cu
#define RVA_Backend_RequestAd        0x15d9ca4u
#define RVA_Application_get_internetReachability 0x2cfdf28u
#define RVA_Input_get_mouseScrollDelta 0x2d71bfcu
#define RVA_Input_GetMouseButton     0x2d71868u
#define RVA_Input_get_mousePosition  0x2d71b74u
#define RVA_Input_get_touchCount     0x2d7204cu

#define GUARD_Input_GetKey            0xf81e0ffeu  /* str x30,[sp,#-0x20]!     */
#define GUARD_Input_GetKeyDown        0xf81e0ffeu
#define GUARD_Input_GetKeyUp          0xf81e0ffeu
#define GUARD_Input_GetAxis           0x14000001u  /* b .+4 (il2cpp pad thunk) */
#define GUARD_Input_GetAxisRaw        0x14000001u
#define GUARD_Input_GetButtonDown     0x14000001u
#define GUARD_Backend_RequestAd       0xa9bc67feu
#define GUARD_Application_get_internetReachability 0xa9bf4ffeu
#define GUARD_Input_get_mouseScrollDelta 0xd10083ffu
#define GUARD_Input_GetMouseButton    0xf81e0ffeu
#define GUARD_Input_get_mousePosition 0xd10083ffu
#define GUARD_Input_get_touchCount    0xa9bf4ffeu

/* UnityEngine.KeyCode values for joystick buttons.
 *   JoystickButton0..19  = 330..349  (any joystick)
 *   Joystick1Button0..19 = 350..369  (joystick 1 specifically)
 * global-metadata.dat confirms Joystick1Button0..19 are present, which is what
 * Rewired's Unity joystick source polls. Both ranges are answered. */
#define KEYCODE_JOYSTICK_ANY_BASE   330
#define KEYCODE_JOYSTICK1_BASE      350
#define KEYCODE_JOYSTICK_COUNT      20

/* Bind the il2cpp runtime exports the hooks need to allocate managed values.
 * Call after libil2cpp is loaded + finalized. Returns 0 if all required APIs
 * resolved. */
int cp_il2cpp_bind(so_module *il2cpp);

/* Verify guards and install the hooks. Returns how many were applied.
 * Requires cp_il2cpp_bind() to have succeeded. */
int cp_il2cpp_install_hooks(so_module *il2cpp);

/* Called once per frame by sd2_input.c with the current pad state.
 * `buttons` is a libnx HidNpadButton mask; sticks are -1.0 .. 1.0. */
void cp_pad_set(uint64_t buttons, float lx, float ly, float rx, float ry);

/* Latch this frame's buttons as "previous" so GetKeyDown/GetKeyUp can report
 * edges. Call once per frame AFTER the engine has had a chance to poll. */
void cp_il2cpp_frame_end(void);

/* True on the frame any button in `mask` goes down. */
int cp_pad_pressed(uint64_t mask);


/* ---- UnityEngine.Video.VideoPlayer ---------------------------------------
 * Only Panik.IntroScript uses VideoPlayer, and disassembling
 * IntroScript.<IntroCoroutine>d__32::MoveNext (0x29D1A30, 9904 bytes) shows the
 * COMPLETE set of VideoPlayer calls it makes:
 *
 *     Play x2   Stop x4   SetDirectAudioVolume x2   get_clip x1 -> get_length x1
 *
 * There is no Prepare, no prepareCompleted, no isPrepared, no isPlaying and no
 * loopPointReached anywhere in it. The intro is gated on a float timer compared
 * against developerVideoPlayer.clip.length, i.e. wall clock, and VideoClip
 * .length is serialized metadata (m_FrameCount / m_FrameRate) that answers
 * correctly with the media backend dead. THAT is why the game already walks
 * past the splash today, and why the hang risk recorded in milestone [V] does
 * not apply: nothing needs stubbing and nothing needs a completion signal.
 *
 * Play/Stop are REPLACED, not detoured. Replacing them also means libunity
 * never enters AndroidVideoMedia at all, so none of the AMedia stubs are
 * reached and "surface creation stalled" cannot happen.
 *
 * TWO-WORD GUARDS. The entry word of Play, Stop and get_length is the same
 * `str x30,[sp,#-0x20]!` prologue, so the first word alone cannot tell them
 * apart -- a shifted RVA would pass. The word at +0x10 is the per-method
 * initialised-class flag load, and it is distinct for each. Both are checked,
 * the same shape as GUARD_WaitVSync / GUARD_WaitVSync_ldr in sd2_patches.c.
 *
 * get_clip and get_length are NOT patched -- they are CALLED, from inside the
 * Play hook, to ask the engine how long the clip is so the right staged file is
 * chosen. Calling a stale address is worse than patching one, so they are
 * guard-checked before the first call and the whole identification step is
 * skipped (falling back to play order) if either fails. */


/* Install the splash-video hooks. Separate from cp_il2cpp_install_hooks so a
 * video failure can never take the input hooks down with it. Returns 1 if the
 * Play/Stop pair was applied. No-op (returns 0) when CP_VIDEO is 0. */
int cp_il2cpp_install_video_hooks(so_module *il2cpp);

#endif /* CLOVERPIT_IL2CPP_H */
