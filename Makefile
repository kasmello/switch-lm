#   make                       build switch-lm.nro
#   make run [SWITCH_IP=…]     build, send to hbmenu's netloader (press Y on the Switch), show its output
#   make mac                   build a desktop version with Homebrew SDL2 (no Switch needed)
#   make mac-run               build and run the desktop version
#   make setup                 install devkitPro + tools, enable git hooks
#   make doctor                check the dev environment without changing anything
#   make clean                 remove Switch and desktop build outputs
#
# setup, doctor, mac, mac-run and clean work without devkitPro installed.

#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

# Default to the standard install location so a fresh shell without DEVKITPRO still builds.
DEVKITPRO ?= /opt/devkitpro
export DEVKITPRO

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# ROMFS is the directory containing data to be added to RomFS, relative to the Makefile (Optional)
#
# NO_ICON: if set to anything, do not use icon.
# NO_NACP: if set to anything, no .nacp file is generated.
# APP_TITLE is the name of the app stored in the .nacp file (Optional)
# APP_AUTHOR is the author of the app stored in the .nacp file (Optional)
# APP_VERSION is the version of the app stored in the .nacp file (Optional)
# APP_TITLEID is the titleID of the app stored in the .nacp file (Optional)
# ICON is the filename of the icon (.jpg), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.jpg
#     - icon.jpg
#     - <libnx folder>/default_icon.jpg
#
# CONFIG_JSON is the filename of the NPDM config file (.json), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.json
#     - config.json
#   If a JSON file is provided or autodetected, an ExeFS PFS0 (.nsp) is built instead
#   of a homebrew executable (.nro). This is intended to be used for sysmodules.
#   NACP building is skipped as well.
#---------------------------------------------------------------------------------
TARGET		:=	switch-lm
BUILD		:=	build
SOURCES		:=	source third_party/imgui
DATA		:=	data
INCLUDES	:=	include third_party/imgui third_party/imgui/backends
APP_TITLE	:=	SwitchLM
APP_AUTHOR	:=	Karmel
ROMFS	:=	romfs
# GPU compute shaders (*.glsl), compiled by uam to $(ROMFS)/shaders/*.dksh (Switch build only)
SHADERS	:=	shaders

.DEFAULT_GOAL := all
DKP_RULES := $(wildcard $(DEVKITPRO)/libnx/switch_rules)

#---------------------------------------------------------------------------------
# Helper targets (top-level pass only; none of these need devkitPro)
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

NO_DKP_GOALS := setup doctor mac mac-run clean
NXLINK := $(DEVKITPRO)/tools/bin/nxlink

# Desktop build: same sources, host compiler, Homebrew SDL2.
# Skip devkitPro's Switch sdl2-config (portlibs/switch/bin), which its build rules put first on PATH.
MAC_OUT     := build-mac
MAC_BIN     := $(MAC_OUT)/$(TARGET)
MAC_SRC     := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp))
MAC_HDR     := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.hpp))
SDL2_CONFIG := $(firstword $(filter-out $(DEVKITPRO)/%,$(shell which -a sdl2-config 2>/dev/null)))
MAC_SDL     := $(if $(SDL2_CONFIG),-I$(shell $(SDL2_CONFIG) --prefix)/include $(shell $(SDL2_CONFIG) --cflags --libs))
# Use the macOS SDK's C++ headers explicitly. Some Command Line Tools installs leave an incomplete
# usr/include/c++/v1 behind that clang searches first ("'cstddef' file not found").
MAC_SDK     := $(if $(filter Darwin,$(shell uname -s)),$(shell xcrun --show-sdk-path 2>/dev/null))
MAC_STDCXX  := $(if $(wildcard $(MAC_SDK)/usr/include/c++/v1/cstddef),-nostdinc++ -isystem $(MAC_SDK)/usr/include/c++/v1)

.PHONY: setup doctor run mac mac-run clean

setup:
	@scripts/setup.sh

doctor:
	@scripts/setup.sh --check

run: all
	@$(NXLINK) -s $(if $(SWITCH_IP),-a $(SWITCH_IP)) $(TARGET).nro

mac: $(MAC_BIN)

$(MAC_BIN): $(MAC_SRC) $(MAC_HDR)
	@test -n "$(SDL2_CONFIG)" || { echo "sdl2-config not found: run 'brew install sdl2 sdl2_ttf' (or make setup)"; exit 1; }
	@mkdir -p $(MAC_OUT)
	@echo "building $@"
	@c++ -std=gnu++20 -g -O0 -Wall $(MAC_STDCXX) $(foreach dir,$(INCLUDES),-I$(dir)) $(MAC_SRC) -o $@ $(MAC_SDL)

mac-run: mac
	@./$(MAC_BIN)

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(MAC_OUT) $(ROMFS)/shaders $(TARGET).nro $(TARGET).nacp $(TARGET).elf \
		$(TARGET).nsp $(TARGET).nso $(TARGET).npdm

# Dear ImGui is a git submodule; a clone without --recurse-submodules has an empty folder.
ifeq ($(wildcard third_party/imgui/imgui.h),)
ifneq ($(filter-out setup doctor clean,$(or $(MAKECMDGOALS),all)),)
$(error third_party/imgui is empty. Run 'git submodule update --init' (or make setup))
endif
endif

# Without devkitPro, only the helper targets above can run.
ifeq ($(DKP_RULES),)
ifneq ($(filter-out $(NO_DKP_GOALS),$(or $(MAKECMDGOALS),all)),)
$(error devkitPro not found at $(DEVKITPRO). Run 'make setup' first)
endif
endif

endif

#---------------------------------------------------------------------------------
# Switch build (devkitPro libnx template)
#---------------------------------------------------------------------------------
ifneq ($(DKP_RULES),)

TOPDIR ?= $(CURDIR)
include $(DKP_RULES)

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

CFLAGS	:=	-g -Wall -O2 -ffunction-sections \
			$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__

# PKGCONF is devkitPro's Switch pkg-config. --cflags adds include/SDL2, which ImGui's SDL2
# backends need for <SDL.h>.
PKGCONF	:= $(DEVKITPRO)/portlibs/switch/bin/aarch64-none-elf-pkg-config
CFLAGS	+=	$(shell $(PKGCONF) --cflags sdl2 2>/dev/null)

# ImGui's default "open URL in shell" helper uses fork/execvp, which Horizon doesn't have.
CFLAGS	+=	-DIMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS

CXXFLAGS := $(CFLAGS) -std=gnu++20

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

# SDL2 + SDL2_ttf link flags come from the Switch pkg-config (they pull in EGL, freetype, etc.)
# deko3dd is deko3d's debug build: it validates API usage and logs mistakes. Switch to -ldeko3d
# (release) once performance matters.
LIBS	:= $(shell $(PKGCONF) --libs SDL2_ttf sdl2 2>/dev/null) -ldeko3dd -lnx

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(PORTLIBS) $(LIBNX)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(CONFIG_JSON)),)
	jsons := $(wildcard *.json)
	ifneq (,$(findstring $(TARGET).json,$(jsons)))
		export APP_JSON := $(TOPDIR)/$(TARGET).json
	else
		ifneq (,$(findstring config.json,$(jsons)))
			export APP_JSON := $(TOPDIR)/config.json
		endif
	endif
else
	export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)
endif

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.jpg)
	ifneq (,$(findstring $(TARGET).jpg,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).jpg
	else
		ifneq (,$(findstring icon.jpg,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.jpg
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
	export NROFLAGS += --icon=$(APP_ICON)
endif

ifeq ($(strip $(NO_NACP)),)
	export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

ifneq ($(APP_TITLEID),)
	export NACPFLAGS += --titleid=$(APP_TITLEID)
endif

ifneq ($(ROMFS),)
	export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif

SHADER_OUT	:=	$(ROMFS)/shaders
SHADER_DKSH	:=	$(patsubst $(SHADERS)/%.glsl,$(SHADER_OUT)/%.dksh,$(wildcard $(SHADERS)/*.glsl))
export ROMFS_DEPS	:=	$(foreach f,$(SHADER_DKSH),$(CURDIR)/$(f))

.PHONY: $(BUILD) all

#---------------------------------------------------------------------------------
all: $(SHADER_DKSH) $(BUILD)

$(SHADER_OUT):
	@mkdir -p $@

$(SHADER_OUT)/%.dksh: $(SHADERS)/%.glsl | $(SHADER_OUT)
	@echo {comp} $(notdir $<)
	@uam -s comp -o $@ $<

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
else
.PHONY:	all

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
ifeq ($(strip $(APP_JSON)),)

all	:	$(OUTPUT).nro

ifeq ($(strip $(NO_NACP)),)
$(OUTPUT).nro	:	$(OUTPUT).elf $(OUTPUT).nacp $(ROMFS_DEPS)
else
$(OUTPUT).nro	:	$(OUTPUT).elf $(ROMFS_DEPS)
endif

else

all	:	$(OUTPUT).nsp

$(OUTPUT).nsp	:	$(OUTPUT).nso $(OUTPUT).npdm

$(OUTPUT).nso	:	$(OUTPUT).elf

endif

$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------

endif
