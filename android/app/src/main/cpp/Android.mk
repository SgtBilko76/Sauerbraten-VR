# Captured once, up front, into a plain (non-LOCAL_*) variable. $(call
# my-dir) resolves via $(lastword $(MAKEFILE_LIST)), which keeps growing
# across every include this file performs (it's not a call stack -- it
# never "pops" on return), so calling my-dir again anywhere after the
# third_party/ include below returns whatever the *last* nested makefile
# happened to be, not this directory. Every reference to this file's own
# directory after that point uses this saved variable instead of my-dir.
SAUER_CPP_DIR := $(call my-dir)
LOCAL_PATH := $(SAUER_CPP_DIR)

# ---------------------------------------------------------------------------
# Path to the (untouched) desktop Sauerbraten source tree, sibling to
# android/ at the repo root -- see src/Makefile for the upstream client
# build this is transcribed from.
# ---------------------------------------------------------------------------
SAUER_SRC_REL := ../../../../../src

# ---------------------------------------------------------------------------
# Vendored SDL2/SDL2_image/SDL2_mixer (see third_party/README.md for what's
# vendored and why). Defines SDL2_static/SDL2_image_static/SDL2_mixer_static
# among other modules; included first so later modules can reference them
# via LOCAL_STATIC_LIBRARIES.
# ---------------------------------------------------------------------------
include $(SAUER_CPP_DIR)/third_party/Android.mk

# third_party/Android.mk's all-subdir-makefiles include leaves LOCAL_PATH
# pointing at whichever subdirectory (e.g. SDL2_mixer/) was processed last
# -- restore it from the saved variable (not my-dir, see above) before
# resuming our own CLEAR_VARS blocks below.
LOCAL_PATH := $(SAUER_CPP_DIR)

# ---------------------------------------------------------------------------
# libenet.a -- Sauerbraten's vendored ENet networking library. Not needed
# for anything to actually connect yet (Phase 2 has no networking), but
# shared/cube.h -- included by every engine/fpsgame source file -- pulls in
# enet.h unconditionally, and the client links against enet symbols, so it
# has to build alongside the engine regardless.
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := enet
LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SAUER_SRC_REL)/enet/include
LOCAL_CFLAGS := -DHAS_SOCKLEN_T=1

LOCAL_SRC_FILES := \
    $(SAUER_SRC_REL)/enet/callbacks.c \
    $(SAUER_SRC_REL)/enet/compress.c \
    $(SAUER_SRC_REL)/enet/host.c \
    $(SAUER_SRC_REL)/enet/list.c \
    $(SAUER_SRC_REL)/enet/packet.c \
    $(SAUER_SRC_REL)/enet/peer.c \
    $(SAUER_SRC_REL)/enet/protocol.c \
    $(SAUER_SRC_REL)/enet/unix.c

include $(BUILD_STATIC_LIBRARY)

# ---------------------------------------------------------------------------
# libsauerengine.a -- Phase 2: the Sauerbraten client engine (shared/,
# engine/, fpsgame/), transcribed 1:1 from src/Makefile's CLIENT_OBJS list.
# Built as a static library so compile-only iteration doesn't require SDL2/
# SDL2_image/SDL2_mixer to already be linkable; now that they are (see
# above), libsauerquest.so below whole-archives this in as a genuine
# compile+link correctness check, ahead of Phase 4/5's real OpenXR-loop
# wiring.
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := sauerengine

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/$(SAUER_SRC_REL)/shared \
    $(LOCAL_PATH)/$(SAUER_SRC_REL)/engine \
    $(LOCAL_PATH)/$(SAUER_SRC_REL)/fpsgame \
    $(LOCAL_PATH)/$(SAUER_SRC_REL)/enet/include \
    $(LOCAL_PATH)/third_party/SDL2/include \
    $(LOCAL_PATH)/third_party/SDL2_image/include \
    $(LOCAL_PATH)/third_party/SDL2_mixer/include

# -fsigned-char: ARM defaults to unsigned char, x86 (what this codebase was
# always built on before) defaults to signed -- the desktop Makefile forces
# this explicitly for the same reason, and Sauerbraten's code (e.g. raw
# geometry/octree byte math) relies on it.
LOCAL_CPPFLAGS := -fsigned-char -fno-exceptions -fno-rtti -Wall \
    -Wno-unused-variable -Wno-unused-function -Wno-unused-but-set-variable \
    -Wno-reorder

LOCAL_SRC_FILES := \
    $(SAUER_SRC_REL)/engine/glescompat.cpp \
    $(SAUER_SRC_REL)/shared/crypto.cpp \
    $(SAUER_SRC_REL)/shared/geom.cpp \
    $(SAUER_SRC_REL)/shared/glemu.cpp \
    $(SAUER_SRC_REL)/shared/stream.cpp \
    $(SAUER_SRC_REL)/shared/tools.cpp \
    $(SAUER_SRC_REL)/shared/zip.cpp \
    $(SAUER_SRC_REL)/engine/3dgui.cpp \
    $(SAUER_SRC_REL)/engine/bih.cpp \
    $(SAUER_SRC_REL)/engine/blend.cpp \
    $(SAUER_SRC_REL)/engine/blob.cpp \
    $(SAUER_SRC_REL)/engine/client.cpp \
    $(SAUER_SRC_REL)/engine/command.cpp \
    $(SAUER_SRC_REL)/engine/console.cpp \
    $(SAUER_SRC_REL)/engine/cubeloader.cpp \
    $(SAUER_SRC_REL)/engine/decal.cpp \
    $(SAUER_SRC_REL)/engine/dynlight.cpp \
    $(SAUER_SRC_REL)/engine/glare.cpp \
    $(SAUER_SRC_REL)/engine/grass.cpp \
    $(SAUER_SRC_REL)/engine/lightmap.cpp \
    $(SAUER_SRC_REL)/engine/main.cpp \
    $(SAUER_SRC_REL)/engine/material.cpp \
    $(SAUER_SRC_REL)/engine/menus.cpp \
    $(SAUER_SRC_REL)/engine/movie.cpp \
    $(SAUER_SRC_REL)/engine/normal.cpp \
    $(SAUER_SRC_REL)/engine/octa.cpp \
    $(SAUER_SRC_REL)/engine/octaedit.cpp \
    $(SAUER_SRC_REL)/engine/octarender.cpp \
    $(SAUER_SRC_REL)/engine/physics.cpp \
    $(SAUER_SRC_REL)/engine/pvs.cpp \
    $(SAUER_SRC_REL)/engine/rendergl.cpp \
    $(SAUER_SRC_REL)/engine/rendermodel.cpp \
    $(SAUER_SRC_REL)/engine/renderparticles.cpp \
    $(SAUER_SRC_REL)/engine/rendersky.cpp \
    $(SAUER_SRC_REL)/engine/rendertext.cpp \
    $(SAUER_SRC_REL)/engine/renderva.cpp \
    $(SAUER_SRC_REL)/engine/server.cpp \
    $(SAUER_SRC_REL)/engine/serverbrowser.cpp \
    $(SAUER_SRC_REL)/engine/shader.cpp \
    $(SAUER_SRC_REL)/engine/shadowmap.cpp \
    $(SAUER_SRC_REL)/engine/sound.cpp \
    $(SAUER_SRC_REL)/engine/texture.cpp \
    $(SAUER_SRC_REL)/engine/water.cpp \
    $(SAUER_SRC_REL)/engine/world.cpp \
    $(SAUER_SRC_REL)/engine/worldio.cpp \
    $(SAUER_SRC_REL)/fpsgame/ai.cpp \
    $(SAUER_SRC_REL)/fpsgame/client.cpp \
    $(SAUER_SRC_REL)/fpsgame/entities.cpp \
    $(SAUER_SRC_REL)/fpsgame/fps.cpp \
    $(SAUER_SRC_REL)/fpsgame/monster.cpp \
    $(SAUER_SRC_REL)/fpsgame/movable.cpp \
    $(SAUER_SRC_REL)/fpsgame/render.cpp \
    $(SAUER_SRC_REL)/fpsgame/scoreboard.cpp \
    $(SAUER_SRC_REL)/fpsgame/server.cpp \
    $(SAUER_SRC_REL)/fpsgame/waypoint.cpp \
    $(SAUER_SRC_REL)/fpsgame/weapon.cpp

include $(BUILD_STATIC_LIBRARY)

# ---------------------------------------------------------------------------
# libsauerquest.so -- Phase 1 (OpenXR bootstrap, still the only thing
# actually driving the frame loop) + Phase 2 (whole-archives the Sauerbraten
# engine in purely to prove it links against real SDL2/SDL2_image/
# SDL2_mixer/enet on-device; nothing calls into it yet -- that's Phase 4/5).
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := sauerquest
LOCAL_LDLIBS := -llog -landroid -lGLESv3 -lEGL -ldl -lz -lOpenSLES

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/openxr_sdk/include \
    $(LOCAL_PATH)/openxr_sdk/src/common \
    $(LOCAL_PATH)/vr_glue \
    $(LOCAL_PATH)/$(SAUER_SRC_REL)/engine

LOCAL_SRC_FILES := \
    vr_glue/TBXR_Common.c \
    vr_glue/OpenXrInput.c \
    vr_glue/sauerquest_vr_bootstrap.c \
    vr_glue/VrPerf.c \
    vr_glue/VrTelemetry.c

LOCAL_CFLAGS := -Wall -Wno-unused-variable -Wno-unused-function

# Phase 4: sauerquest_vr_bootstrap.c now calls into the engine
# (android_sauer_main/tick/drawframe/endframe, via androidbridge.h) every
# frame, so sauerengine/enet's object files are genuinely referenced --
# WHOLE_STATIC_LIBRARIES is kept anyway since it's still correct (and
# harmless) now that real references exist too.
LOCAL_WHOLE_STATIC_LIBRARIES := sauerengine enet
LOCAL_STATIC_LIBRARIES := SDL2_image_static SDL2_mixer_static SDL2_static
# SDL2_image_static/SDL2_mixer_static inherit LOCAL_SHARED_LIBRARIES :=
# SDL2 from their sibling shared-module block in the vendored Android.mk
# (there's no CLEAR_VARS between the "SDL2_image"/"SDL2_mixer" shared
# modules and their "_static" variants -- this is upstream SDL2_image/
# SDL2_mixer's own intended design: SDL2 core stays a shared library even
# when the codec libraries are statically linked into the app). Declaring
# it here too makes both the linker AND Gradle's native-lib packaging
# step aware libSDL2.so needs to end up in the APK's jniLibs.
LOCAL_SHARED_LIBRARIES := SDL2

include $(BUILD_SHARED_LIBRARY)
