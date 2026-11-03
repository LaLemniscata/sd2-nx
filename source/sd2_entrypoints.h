/* sd2_entrypoints.h -- UnityPlayer native methods recovered from
 * libunity.so's RegisterNatives tables (Soda Dungeon 2, Unity 6000.0.73f1, arm64).
 *
 * Derived mechanically by tools/extract_entrypoints.py: every R_AARCH64_RELATIVE
 * relocation is walked in 24-byte strides looking for the JNINativeMethod shape
 * { const char *name; const char *sig; void *fn; }. No guessing, no hand-typed
 * offsets -- if the game updates, re-run the tool.
 *
 * libunity.so links at vaddr 0, so these ARE RVAs:
 *     runtime_addr = unity_mod.load_virtbase + OFF_xxx
 *
 * PREFER resolving by NAME from the RegisterNatives capture in jni_fake.c and
 * using these only as a cross-check -- that is what the reference ports do and
 * it survives a game update. */
#ifndef CLOVERPIT_ENTRYPOINTS_H
#define CLOVERPIT_ENTRYPOINTS_H

#include <stdint.h>
#include "so_util.h"

#define OFF_JNI_OnLoad                        0x733dd0 /* exported; resolve by name */


/* ---- drive-critical lifecycle ---------------------------------- */
#define OFF_initJni                           0x00732c20 /* (Landroid/content/Context;I)V */
#define OFF_nativeRecreateGfxState            0x00732eec /* (ILandroid/view/Surface;)V */
#define OFF_nativeSendSurfaceChangedEvent     0x00732f54 /* ()V */
#define OFF_nativeUnityPlayerSetRunning       0x00733d24 /* (Z)V */
#define OFF_nativeRender                      0x007331c0 /* ()Z */
#define OFF_nativeResume                      0x00732dd8 /* ()V */
#define OFF_nativePause                       0x00732d74 /* ()Z */
#define OFF_nativeFocusChanged                0x00732e88 /* (Z)V */
#define OFF_nativeInjectEvent                 0x00733220 /* (Landroid/view/InputEvent;)Z */
#define OFF_nativeDone                        0x00732c94 /* ()Z */
#define OFF_nativeApplicationUnload           0x00732e38 /* ()V */
/* MISSING FROM THIS BUILD: nativeLowMemory */
#define OFF_nativeOrientationChanged          0x00733cc4 /* (II)V */
#define OFF_nativeConfigurationChanged        0x00733130 /* (Landroid/content/res/Configuration;)V */
#define OFF_nativeOnApplyWindowInsets         0x00732fa8 /* (Landroid/view/WindowInsets;)V */
#define OFF_nativeViewPaddingChanged          0x00814298 /* (IIIII)V */
#define OFF_nativeMemoryUsageChanged          0x00732d20 /* (I)V */

/* ---- native audio pump (FMOD output -> you supply the ByteBuffer)  */
#define OFF_fmodGetInfo                       0x00eb8728 /* (I)I */
#define OFF_fmodProcess                       0x00eb87f0 /* (Ljava/nio/ByteBuffer;)I */
#define OFF_fmodProcessMicData                0x00eb887c /* (Ljava/nio/ByteBuffer;I)I */

/* ---- secondary ------------------------------------------------- */
#define OFF_nativeUnitySendMessage            0x00733828 /* (Ljava/lang/String;Ljava/lang/String;[B)V */
#define OFF_nativeMuteMasterAudio             0x00733a54 /* (Z)V */
#define OFF_nativeGetNoWindowMode             0x00733d78 /* ()Z */
#define OFF_nativeIsAutorotationOn            0x007339f4 /* ()Z */
#define OFF_nativeSetLaunchURL                0x00733ab8 /* (Ljava/lang/String;)V */
#define OFF_nativeHidePreservedContent        0x00733c74 /* ()V */
#define OFF_permissionResponseToNative        0x00733bf0 /* (JZ)V */
#define OFF_onAudioVolumeChanged              0x0072f7ec /* (I)V */

/* ---- soft keyboard / IME --------------------------------------- */
#define OFF_nativeGetSoftInputType            0x0073eef4 /* ()I */
#define OFF_nativeReportKeyboardConfigChanged 0x00733788 /* ()V */
#define OFF_nativeSetInputArea                0x007334d8 /* (IIII)V */
#define OFF_nativeSetInputSelection           0x00733668 /* (II)V */
#define OFF_nativeSetInputString              0x007335c0 /* (Ljava/lang/String;)V */
#define OFF_nativeSetKeyboardIsVisible        0x00733560 /* (Z)V */
#define OFF_nativeSoftInputCanceled           0x007336d8 /* ()V */
#define OFF_nativeSoftInputClosed             0x007337d0 /* ()V */
#define OFF_nativeSoftInputLostFocus          0x00733730 /* ()V */

/* ---- signatures: ret (*)(JNIEnv*, jobject thiz, args...) ---------------- */
typedef void     (*fn_initJni_u6)(void*, void*, void*, int32_t); /* 4-ARG in Unity 6 */
typedef void     (*fn_gfxstate)(void*, void*, int32_t, void*);
typedef void     (*fn_v)(void*, void*);
typedef uint8_t  (*fn_z)(void*, void*);
typedef void     (*fn_vz)(void*, void*, int32_t);
typedef uint8_t  (*fn_inject)(void*, void*, void*, int32_t);
typedef void     (*fn_orient)(void*, void*, int32_t, int32_t);
typedef void     (*fn_vobj)(void*, void*, void*);
typedef void     (*fn_pad5)(void*, void*, int32_t, int32_t, int32_t, int32_t, int32_t);
typedef int32_t  (*fn_fmodGetInfo)(void*, void*, int32_t);
typedef int32_t  (*fn_fmodProcess)(void*, void*, void* /*java.nio.ByteBuffer*/);

#define UNITY_RESOLVE(mod, off) ((void *)((uintptr_t)(mod).load_virtbase + (off)))

/* ===========================================================================
 * UNITY 6 DRIVE SEQUENCE -- differs from the 2020.3/2022.3 reference ports:
 *
 *   initJni(env, thiz, ctx, 0);              // <-- 4 ARGS. 3-arg call = crash.
 *   nativeRecreateGfxState(env, thiz, 0, surface);
 *   nativeSendSurfaceChangedEvent(env, thiz);
 *   nativeUnityPlayerSetRunning(env, thiz, 1);   // <-- U6 ONLY, before render
 *   nativeResume(env, thiz);
 *   nativeFocusChanged(env, thiz, 1);
 *   for (;;) {
 *       pump_input();                         // -> nativeInjectEvent
 *       if (!nativeRender(env, thiz)) break;
 *   }
 *   nativeApplicationUnload(env, thiz);
 *   nativeDone(env, thiz);
 *
 * AUDIO: this build has NO AndroidAudio::GetAndroidAudioOutputType force-patch
 * target the way 2020.3 did. Unity 6 registers fmodGetInfo/fmodProcess as
 * RegisterNatives callbacks for the Java AudioTrack thread. The port drives
 * them DIRECTLY: run your own SDL2 audio thread, wrap your PCM staging buffer
 * in a fake java.nio.ByteBuffer (jni_fake.c NewDirectByteBuffer), and call
 * fmodProcess(env, thiz, buf) each callback to pull mixed PCM out of the
 * engine. fmodGetInfo(env, thiz, i) returns rate/channels/format -- query it
 * once at startup rather than hardcoding 24000/256.
 * =========================================================================== */

#endif /* CLOVERPIT_ENTRYPOINTS_H */
