# Top-level convenience targets. The real build lives in switch-lm/Makefile.
#
#   make setup                 install devkitPro + tools, enable git hooks
#   make doctor                check the dev environment without changing anything
#   make / make build          build switch-lm/switch-lm.nro
#   make run [SWITCH_IP=…]     send the .nro to hbmenu's netloader (press Y on the Switch) and show its output
#   make clean

export DEVKITPRO ?= /opt/devkitpro
APP   := switch-lm
NRO   := $(APP)/$(APP).nro
NXLINK := $(DEVKITPRO)/tools/bin/nxlink

.PHONY: all build setup doctor run clean

all: build

build:
	@$(MAKE) --no-print-directory -C $(APP)

setup:
	@scripts/setup.sh

doctor:
	@scripts/setup.sh --check

run: build
	@$(NXLINK) -s $(if $(SWITCH_IP),-a $(SWITCH_IP)) $(NRO)

clean:
	@$(MAKE) --no-print-directory -C $(APP) clean
