 # N64 DIAGNOSTIC ROM (Phase 8 wireframe navigation), not playable game. Requires a full installed libdragon SDK.
# Example: make -C n64 N64_INST=/n64_toolchain
all: ck64_diag.z64
.PHONY: all clean
BUILD_DIR := build
include $(N64_INST)/include/n64.mk

OBJ_DIR := $(BUILD_DIR)
PORTABLE_SRC := ../portable/src
PORTABLE_INCLUDE := ../portable/include
CFLAGS += -I$(PORTABLE_INCLUDE)

OBJS := $(OBJ_DIR)/main.o \
  $(OBJ_DIR)/ck64_clock.o \
  $(OBJ_DIR)/ck64_runtime.o \
  $(OBJ_DIR)/ck64_gameflow.o \
  $(OBJ_DIR)/ck64_gameflow_save.o \
  $(OBJ_DIR)/ck64_music.o \
  $(OBJ_DIR)/ck64_save.o \
  $(OBJ_DIR)/ck64_world3d.o \
  $(OBJ_DIR)/ck64_perf.o \
  $(OBJ_DIR)/ck64_playercore.o \
  $(OBJ_DIR)/ck64_player_motion.o \
  $(OBJ_DIR)/ck64_player_run.o \
  $(OBJ_DIR)/ck64_player_state0.o \
  $(OBJ_DIR)/wireframe.o

ck64_diag.z64: N64_ROM_TITLE = "CK64 Port Diagnostic"
# Save media is intentionally disabled: only RAM scratch records exist.
ck64_diag.z64: N64_ROM_SAVETYPE = none
$(BUILD_DIR)/ck64_diag.elf: $(OBJS)

$(OBJ_DIR)/ck64_%.o: $(PORTABLE_SRC)/ck64_%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR) ck64_diag.z64

-include $(wildcard $(BUILD_DIR)/*.d)
