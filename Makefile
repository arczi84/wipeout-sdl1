CC ?= gcc
EMCC ?= emcc
UNAME_S := AmigaOS
#UNAME_S := Msys
UNAME_O := $(shell uname -o)
#RENDERER ?= SOFTWARE
RENDERER = GL_LEGACY
USE_GLX ?= false
# DEBUG=true adds -pg, which only works against the instrumented -prof/-debug
# MiniGL libraries selected below. Release builds link the plain ones.
DEBUG ?= false
AMIGA_SHARED_MINIGL ?= false
MGL_V12_SDK ?= /mnt/d/dev/Pistorm3D/Pistorm3D_v12
SDL_MGL_V12 ?= /mnt/d/dev/Pistorm3D/tests/sdl-stormmesa-window/libSDL-mgl-v12.a
USER_CFLAGS ?= -D__MORPHOS__ -DNO_INTRO -w
SDL_VER	=

L_FLAGS ?= -lm
C_FLAGS ?= -std=gnu99 -Wall -Wno-unused-variable -Isrc $(USER_CFLAGS)

ifeq ($(DEBUG), true)
	C_FLAGS := $(C_FLAGS) -g -pg
else
	C_FLAGS := $(C_FLAGS) -O3
endif


# Rendeder ---------------------------------------------------------------------

ifeq ($(RENDERER), GL)
	RENDERER_SRC = src/render_gl.c
	C_FLAGS := $(C_FLAGS) -DRENDERER_GL
else ifeq ($(RENDERER), GL_LEGACY)
	RENDERER_SRC = src/render_gl_legacy.c
	C_FLAGS := $(C_FLAGS) -DRENDERER_GL_LEGACY
else ifeq ($(RENDERER), SOFTWARE)
	RENDERER_SRC = src/render_software.c
	C_FLAGS := $(C_FLAGS) -DRENDERER_SOFTWARE
else
$(error Unknown RENDERER)
endif

ifeq ($(GL_VERSION), GLES2)
	C_FLAGS := $(C_FLAGS) -DUSE_GLES2
endif


# MorphOS ------------------------------------------------------------------------

ifeq ($(UNAME_S), MorphOS)
	C_FLAGS := $(C_FLAGS)  -noixemul
	C_FLAGS := $(C_FLAGS) -I/usr/local/include
	L_FLAGS := $(L_FLAGS) -L/usr/local/lib
	L_FLAGS_SDL = -noixemul -lSDL -lGL -lm -lc
	CC = ppc-morphos-gcc-9
# MorphOS ------------------------------------------------------------------------

else ifeq ($(UNAME_S), AmigaOS)
	ifeq ($(AMIGA_SHARED_MINIGL), true)
	# Keep libnix/libm after SDL and the MiniGL dispatch archive.  Putting -lm
	# before them leaves the generated __LIB_LIST__ empty, so DOSBase is never
	# opened and the executable crashes in libnix before main().
	L_FLAGS :=
	C_FLAGS += -m68060 -mhard-float -fbbb=+ -ffast-math \
		-fomit-frame-pointer -fno-strict-aliasing -noixemul \
		-DNO_INLINE_VARARGS -DNO_INLINE_STDARG -DGL_GLEXT_LEGACY \
		-DAMIGA_SHARED_MINIGL -I$(MGL_V12_SDK)/include \
		-I/mnt/d/dev/Pistorm3D/libSDL12-mgl/include \
		-I/mnt/d/amiga-gcc2/include -Isrc -Isrc/wipeout/ -Isrc/libs
	L_FLAGS_SDL = -m68060 -mhard-float -fbbb=+ -noixemul \
		-Wl,--start-group $(SDL_MGL_V12) \
		$(MGL_V12_SDK)/libminigl_dispatch.a -Wl,--end-group -lm -ldebug
	TARGET_NATIVE = wipegame-mgl_v12
	else
	C_FLAGS += -O2 -m68040 -mhard-float -fbbb=-
#-fomit-frame-pointer -finline-functions -ffast-math -ftree-vectorize -fbbb=+ -mhard-float
	C_FLAGS += -I/mnt/d/amiga-gcc2/include -Isrc -Isrc/wipeout/ -Isrc/libs
ifeq ($(DEBUG), true)
	L_FLAGS_SDL =  -L/mnt/d/amiga-gcc2/lib/ -lSDL-mgl-prof -lmgl-new-debug -lm -noixemul -Wl,--allow-multiple-definition
else
	L_FLAGS_SDL =  -L/mnt/d/amiga-gcc2/lib/ -lSDL-mgl -lmgl-new -lm -noixemul -Wl,--allow-multiple-definition
endif
	endif
	CC = m68k-amigaos-gcc
# AmigaOS ------------------------------------------------------------------------  -  // new-debug| prof works > -cosmos
else ifeq ($(UNAME_S), AmigaOS-gcc4)
	#PATH=/dev/msys64/usr/local/amiga
	C_FLAGS += $(C_FLAGS) -m68040 -O0 -msoft-float -noixemul -g -gstabs # -funroll-loops#
	C_FLAGS += $(C_FLAGS) -I/dev/msys64/usr/local/amiga/m68k-amigaos/include/ -Isrc -Isrc/wipeout/ -Isrc/libs
	ifeq ($(RENDERER), GL_LEGACY)
		C_FLAGS += -DRENDERER_GL
		L_FLAGS_SDL = -L/dev/msys64/usr/local/amiga/m68k-amigaos/lib -lSDL-mgl  -noixemul  -lmgl-new-debug  -ldebug -Wl,--allow-multiple-definition
		TARGET_NATIVE = wipegame-gcc4-mgl
	else
		L_FLAGS_SDL = -L/dev/msys64/usr/local/amiga/m68k-amigaos/lib -lSDL.ix -lgl_dummy  -lm040 -ldebug
		TARGET_NATIVE = wipegame-gcc4
	endif
	CC = /e/usr/local/amiga/bin/m68k-amigaos-gcc-4.exe --sysroot=e:/usr/local/amiga/bin
# AmigaOS ------------------------------------------------------------------------
else ifeq ($(UNAME_S), AmigaOS-gcc6)
	#PATH=/dev/msys64/usr/local/amiga
	C_FLAGS := $(C_FLAGS)  -O3 -fomit-frame-pointer -m68040 -noixemul
	C_FLAGS += $(C_FLAGS) -I/dev/msys64/usr/local/amiga/m68k-amigaos/include/ -Isrc -Isrc/wipeout/ -Isrc/libs
	ifeq ($(RENDERER), GL_LEGACY)
		C_FLAGS += -DRENDERER_GL
		L_FLAGS_SDL = -L/dev/msys64/usr/local/amiga/m68k-amigaos/lib -lSDLgcc6 -noixemul -lgl -lnix-gcc6 -ldebug
		TARGET_NATIVE = wipegame-gcc4-gl
	elsels
		L_FLAGS_SDL = -L/dev/msys64/usr/local/amiga/m68k-amigaos/lib -lSDL.ix -lgl_dummy  -lm040 -ldebug
		TARGET_NATIVE = wipegame-gcc4
	endif
	CC = /e/usr/local/amiga6/bin/m68k-amigaos-gcc.exe --sysroot=/e/usr/local/amiga6/bin
# AmigaOS ------------------------------------------------------------------------
else ifeq ($(UNAME_S), AmigaOS-wsl-gcc4)
# QuarTex 49fps ( -O2 -m68040  -noixemul )
	C_FLAGS := $(C_FLAGS)  -O0 -m68040  -noixemul
	C_FLAGS += $(C_FLAGS) -I/dev/msys64/usr/local/amiga/m68k-amigaos/include/ -Isrc -Isrc/wipeout/ -Isrc/libs
	ifeq ($(RENDERER), GL_LEGACY)
		C_FLAGS += -DRENDERER_GL
		L_FLAGS_SDL = -L/dev/msys64/usr/local/amiga/m68k-amigaos/lib -lSDL-mgl -lmgl-cosmos  -Wl,--allow-multiple-definition -lm2.2 -ldebug
		L_FLAGS_SDL += -lnix222 -noixemul
		TARGET_NATIVE = wipegame-gcc4-mgl
	else
		L_FLAGS_SDL = -L/dev/msys64/usr/local/amiga/m68k-amigaos/lib -lSDL -lm040 -ldebug -noixemul
		TARGET_NATIVE = wipegame-gcc4
	endif
	CC = /mnt/e/usr/local/amiga/bin/m68k-amigaos-gcc-4.exe --sysroot=e:/usr/local/amiga/bin
# AmigaOS ------------------------------------------------------------------------
else ifeq ($(UNAME_S), AmigaOS-wsl-gcc4-ix)
# QuarTex fps ( -O2 -m68040 )
	C_FLAGS := $(C_FLAGS)  -O2 -m68040
	C_FLAGS += $(C_FLAGS) -I/dev/msys64/usr/local/amiga/m68k-amigaos/include/ -Isrc -Isrc/wipeout/ -Isrc/libs
	ifeq ($(RENDERER), GL_LEGACY)
		C_FLAGS += -DRENDERER_GL
		L_FLAGS_SDL = -L/dev/msys64/usr/local/amiga/m68k-amigaos/lib -lSDL-mgl.ix -lmgl-ix  -Wl,--allow-multiple-definition -ldebug
		TARGET_NATIVE = wipegame-gcc4-mgl.ix
	else
		L_FLAGS_SDL = -L/dev/msys64/usr/local/amiga/m68k-amigaos/lib -lSDL -lm040 -ldebug -noixemul
		TARGET_NATIVE = wipegame-gcc4
	endif
	CC = /mnt/e/usr/local/amiga/bin/m68k-amigaos-gcc-4.exe --sysroot=e:/usr/local/amiga/bin
# AmigaOS ------------------------------------------------------------------------
else ifeq ($(UNAME_S), AmigaOS-wsl-gcc6)
	#PATH=/dev/msys64/usr/local/amiga
	C_FLAGS := $(C_FLAGS)  -O3 -m68040
	C_FLAGS += $(C_FLAGS) -I/dev/msys64/usr/local/amiga/m68k-amigaos/include/ -Isrc -Isrc/wipeout/ -Isrc/libs
	ifeq ($(RENDERER), GL_LEGACY)
		C_FLAGS += -DRENDERER_GL
		L_FLAGS_SDL = -L/dev/msys64/usr/local/amiga/m68k-amigaos/lib -lSDL_mgl -lmgl -lm
		TARGET_NATIVE = wipegame-gcc6-gl
	else
		L_FLAGS_SDL = -L/dev/msys64/usr/local/amiga/m68k-amigaos/lib -lSDL -lm040 -ldebug -noixemul
		TARGET_NATIVE = wipegame-gcc4
	endif
	CC = m68k-amigaos-gcc --sysroot=/opt/amiga/bin
# AmigaOS ------------------------------------------------------------------------

else ifeq ($(UNAME_S), Darwin)
	C_FLAGS := $(C_FLAGS) -x objective-c -I/opt/homebrew/include -D_THREAD_SAFE -w
	L_FLAGS := $(L_FLAGS) -L/opt/homebrew/lib -framework Foundation

	ifeq ($(RENDERER), GL)
		L_FLAGS := $(L_FLAGS) -lGLEW -GLU -framework OpenGL
	endif

	L_FLAGS_SDL = -lSDL2
	L_FLAGS_SOKOL = -framework Cocoa -framework QuartzCore -framework AudioToolbox


# Linux ------------------------------------------------------------------------

else ifeq ($(UNAME_S), Linux)
	ifeq ($(RENDERER), GL)
		L_FLAGS := $(L_FLAGS) -lGLEW

		# Prefer modern GLVND instead of legacy X11-only GLX
		ifeq ($(USE_GLX), true)
			L_FLAGS := $(L_FLAGS) -lGL
		else
			L_FLAGS := $(L_FLAGS) -lOpenGL
		endif
	endif

	L_FLAGS_SDL = -lSDL2 -lGL
	L_FLAGS_SOKOL = -lX11 -lXcursor -pthread -lXi -ldl -lasound


# Windows MSYS ------------------------------------------------------------------
else ifeq ($(UNAME_O), Msys)
	ifeq ($(GL_LEGACY), RENDERER)
		L_FLAGS := $(L_FLAGS) -lopengl32
	else ifeq ($(GL), RENDERER)
		L_FLAGS := $(L_FLAGS) -lopengl32 -lglew32
	endif

	C_FLAGS := $(C_FLAGS) -DSDL_MAIN_HANDLED -D__MSYS__ -Isrc -Isrc/wipeout/ -Isrc/libs
	L_FLAGS_SDL = -lSDL$(SDL_VER) $(L_FLAGS)  -lopengl32 -lglew32
	L_FLAGS_SOKOL = --pthread -ldl -lasound
	CC = /d/dev/msys64/mingw64/bin/gcc

# Windows NON-MSYS ---------------------------------------------------------------
else ifeq ($(OS), Windows_NT)
	$(error TODO: FLAGS for windows have not been set up. Please modify this makefile and send a PR!)

else
$(error Unknown environment)
endif



# Source files -----------------------------------------------------------------

TARGET_NATIVE ?= wipegame
BUILD_DIR = build/obj/native
BUILD_DIR_WASM = build/obj/wasm

WASM_RELEASE_DIR ?= build/wasm
TARGET_WASM ?= $(WASM_RELEASE_DIR)/wipeout.js
TARGET_WASM_MINIMAL ?= $(WASM_RELEASE_DIR)/wipeout-minimal.js

COMMON_SRC = \
	src/wipeout/race.c \
	src/wipeout/camera.c \
	src/wipeout/object.c \
	src/wipeout/droid.c \
	src/wipeout/ui.c \
	src/wipeout/hud.c \
	src/wipeout/image.c \
	src/wipeout/game.c \
	src/wipeout/menu.c \
	src/wipeout/main_menu.c \
	src/wipeout/ingame_menus.c \
	src/wipeout/title.c \
	src/wipeout/intro.c \
	src/wipeout/scene.c \
	src/wipeout/ship.c \
	src/wipeout/ship_ai.c \
	src/wipeout/ship_player.c \
	src/wipeout/track.c \
	src/wipeout/weapon.c \
	src/wipeout/particle.c \
	src/wipeout/sfx.c \
	src/utils.c \
	src/types.c \
	src/system.c \
	src/mem.c \
	src/input.c \
	$(RENDERER_SRC)


# Targets native ---------------------------------------------------------------

COMMON_OBJ = $(patsubst %.c, $(BUILD_DIR)/%.o, $(COMMON_SRC))
COMMON_DEPS = $(patsubst %.c, $(BUILD_DIR)/%.d, $(COMMON_SRC))

sdl: $(BUILD_DIR)/src/platform_sdl$(SDL_VER).o
sdl: $(COMMON_OBJ)
	$(CC) $^ -o $(TARGET_NATIVE) $(L_FLAGS) $(L_FLAGS_SDL)

sokol: $(BUILD_DIR)/src/platform_sokol.o
sokol: $(COMMON_OBJ)
	$(CC) $^ -o $(TARGET_NATIVE) $(L_FLAGS) $(L_FLAGS_SOKOL)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) -MMD -MP -c $< -o $@

-include $(COMMON_DEPS)


# Targets wasm -----------------------------------------------------------------

COMMON_OBJ_WASM = $(patsubst %.c, $(BUILD_DIR_WASM)/%.o, $(COMMON_SRC))
COMMON_DEPS_WASM = $(patsubst %.c, $(BUILD_DIR_WASM)/%.d, $(COMMON_SRC))

wasm: wasm_full wasm_minimal
	cp src/wasm-index.html $(WASM_RELEASE_DIR)/game.html


wasm_full: $(BUILD_DIR_WASM)/src/platform_sokol.o
wasm_full: $(COMMON_OBJ_WASM)
	mkdir -p $(WASM_RELEASE_DIR)
	$(EMCC) $^ -o $(TARGET_WASM) -lGLEW -lGL \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s ENVIRONMENT=web \
		--preload-file wipeout

wasm_minimal: $(BUILD_DIR_WASM)/src/platform_sokol.o
wasm_minimal: $(COMMON_OBJ_WASM)
	mkdir -p $(WASM_RELEASE_DIR)
	$(EMCC) $^ -o $(TARGET_WASM_MINIMAL) -lGLEW -lGL \
		-s ALLOW_MEMORY_GROWTH=1 \
		-s ENVIRONMENT=web \
		--preload-file wipeout \
		--exclude-file wipeout/music \
		--exclude-file wipeout/intro.mpeg

$(BUILD_DIR_WASM):
	mkdir -p $(BUILD_DIR_WASM)

$(BUILD_DIR_WASM)/%.o: %.c
	mkdir -p $(dir $@)
	$(EMCC) $(C_FLAGS) -MMD -MP -c $< -o $@

-include $(COMMON_DEPS_WASM)




.PHONY: clean
clean:
	$(RM) -rf $(BUILD_DIR) $(BUILD_DIR_WASM) $(WASM_RELEASE_DIR)
