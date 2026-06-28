# ============================================================
#  MirageFS Build Animation System
# ============================================================

# ---- ANSI colors ----
C_RESET   := \033[0m
C_BOLD    := \033[1m
C_DIM     := \033[2m

C_RED     := \033[31m
C_GREEN   := \033[32m
C_YELLOW  := \033[33m
C_CYAN    := \033[36m

C_BRED    := \033[91m
C_BGREEN  := \033[92m
C_BYELLOW := \033[93m
C_BBLUE   := \033[94m

C_BG_RED   := \033[41m
C_BG_GREEN := \033[42m

# ---- module color ----
ifeq ($(MODULE),common)
  M_COLOR := $(C_BBLUE)
else ifeq ($(MODULE),config)
  M_COLOR := $(C_WHITE)
else ifeq ($(MODULE),lsa)
  M_COLOR := $(C_CYAN)
else ifeq ($(MODULE),object)
  M_COLOR := $(C_BYELLOW)
else
  M_COLOR := $(C_WHITE)
endif

# ---- progress state files ----
PROGRESS_FILE := $(OUTPUT_DIR)/.build_count
TOTAL_FILE    := $(OUTPUT_DIR)/.build_total

# ============================================================
#  entrance — Pikachu thunder (define: static text, safe to call)
# ============================================================

define ANIM_ENTRANCE
	@printf "$(C_BYELLOW)\n"; \
	printf "                            ░░░░░░░░░░░░░░\n"; \
	printf "                         ░░░░░░░░░░░░░░░░░░░░░░\n"; \
	printf "                      ░░░░░░░░░▓▓▓▓▓▓▓░░░░░░░░░░░\n"; \
	printf "                    ░░░░░░░░▓▓▓▓▓▓▓▓▓▓▓░░░░░░░░░░\n"; \
	printf "                  ░░░░░░░░▓▓▓▓▓▒▒▒▓▓▓▓▓▓░░░░░░░░\n"; \
	printf "                 ░░░░░░░▓▓▓▓▓▒▒▒▒▒▒▓▓▓▓▓░░░░░░░░\n"; \
	printf "                ░░░░░░░▓▓▓▓▒▒▒░░░▒▒▒▓▓▓▓░░░░░░░░\n"; \
	printf "               ░░░░░░▓▓▓▓▓▒▒░░░░░░▒▒▓▓▓▓▓░░░░░░\n"; \
	printf "              ░░░░░▓▓▓▓▓▓▒▒░░▒▒░░░▒▒▓▓▓▓▓▓░░░░░\n"; \
	printf "              ░░░░▓▓▓▓▓▓▒▒░░▒▒▒░░░▒▒▓▓▓▓▓▓░░░░\n"; \
	printf "             ░░░░▓▓▓▓▓▓▒▒░░▒▒▒▒░░▒▒▓▓▓▓▓▓▓░░░░\n"; \
	printf "             ░░░░▓▓▓▓▓▓▒░░░▒▒▒▒░░▒▒▓▓▓▓▓▓▓░░░░\n"; \
	printf "             ░░░░▓▓▓▓▓▓▒░░░▒▒▒▒░░▒▒▓▓▓▓▓▓▓░░░░\n"; \
	printf "              ░░░░▓▓▓▓▓▓▒░░▒▒▒▒░░▒▓▓▓▓▓▓▓░░░░\n"; \
	printf "              ░░░░░▓▓▓▓▓▓▒▒▒▒▒▒▒▒▓▓▓▓▓▓░░░░░\n"; \
	printf "               ░░░░░░▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓░░░░░░\n"; \
	printf "                ░░░░░░░░▓▓▓▓▓▓▓▓▓░░░░░░░░\n"; \
	printf "                 ░░░░░░░░░░░░░░░░░░░░░░░\n"; \
	printf "                  ░░░░░░░░░░░░░░░░░░░\n"; \
	printf "                      ░░░░░░░░░░░░\n"; \
	printf "$(C_RESET)\n"; \
	printf "$(C_BOLD)$(C_BYELLOW)    ⚡  MirageFS Build — Pika-Pika!  ⚡$(C_RESET)\n"; \
	printf "\n"
endef

# ============================================================
#  module enter (define, used without call — just variable expansion)
# ============================================================

define ANIM_MODULE_ENTER
	@printf "$(C_BOLD)$(M_COLOR)  ▸ $(MODULE)$(C_RESET)\n"
endef

# ============================================================
#  success (safe to $(call): only printf, no shell $())
# ============================================================

define ANIM_SUCCESS
	printf "\n"; \
	printf "$(C_BYELLOW)\n"; \
	printf "    ⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡\n"; \
	printf "    ⚡                                          ⚡\n"; \
	printf "    ⚡   $(C_BOLD)BUILD SUCCESS — Pika-Pika !$(C_BYELLOW)      ⚡\n"; \
	printf "    ⚡                                          ⚡\n"; \
	printf "    ⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡⚡\n"; \
	printf "$(C_RESET)\n"; \
	printf "\n"; \
	printf "  $(C_BYELLOW)   ⬆   ⬆   ⬆   ⬆   ⬆$(C_RESET)\n"; \
	printf "  $(C_BYELLOW)  ╱░╲ ╱░╲ ╱░╲ ╱░╲ ╱░╲$(C_RESET)\n"; \
	printf "  $(C_BYELLOW) ╱░░░╲░░░╲░░░╲░░░╲░░░╲$(C_RESET)\n"; \
	printf "  $(C_BYELLOW)╱░░░░░╲░░░░░╲░░░░░╲░░░░░╲$(C_RESET)\n"; \
	printf "  $(C_BOLD)$(C_YELLOW) ▓▓▓▓▓ ▓▓▓▓▓ ▓▓▓▓▓ ▓▓▓▓▓ ▓▓▓▓▓$(C_RESET)\n"; \
	printf "  $(C_YELLOW) ▓▓▓▓▓ ▓▓▓▓▓ ▓▓▓▓▓ ▓▓▓▓▓ ▓▓▓▓▓$(C_RESET)\n"; \
	printf "  $(C_DIM)$(C_YELLOW) ▓▓▓▓▓ ▓▓▓▓▓ ▓▓▓▓▓ ▓▓▓▓▓ ▓▓▓▓▓$(C_RESET)\n"; \
	printf "\n"
endef

# ============================================================
#  failure (used inside shell {} block, so no @ prefix)
# ============================================================

define ANIM_FAILURE
	printf "\n"; \
	printf "$(C_BG_RED)$(C_BOLD)\n"; \
	printf "    ██████████████████████████████████████████\n"; \
	printf "    ██                                        ██\n"; \
	printf "    ██   $(C_BYELLOW)BUILD FAILED — Pika-pi... 😿$(C_BG_RED)        ██\n"; \
	printf "    ██                                        ██\n"; \
	printf "    ██████████████████████████████████████████\n"; \
	printf "$(C_RESET)\n"; \
	printf "\n"; \
	printf "  $(C_RED)    ░░░░░░░░░░░░$(C_RESET)\n"; \
	printf "  $(C_RED)   ░░░░░▓▓▓▓░░░░░$(C_RESET)\n"; \
	printf "  $(C_RED)  ░░░░▓▓▓▓▓▓░░░░$(C_RESET)\n"; \
	printf "  $(C_RED)  ░░░░▓▓▓▓▓▓░░░░$(C_RESET)\n"; \
	printf "  $(C_RED)  ░░░░▓▓▓▓▓▓░░░░$(C_RESET)\n"; \
	printf "  $(C_RED)  ░░░░░░░░░░░░░░$(C_RESET)\n"; \
	printf "  $(C_RED)   ░░▓░░░░░▓░░░$(C_RESET)\n"; \
	printf "  $(C_RED)   ░░░▓░░░▓░░░░$(C_RESET)\n"; \
	printf "  $(C_RED)    ░░░░▓░░░░░$(C_RESET)\n"; \
	printf "  $(C_RED)     ░░░░░░░░$(C_RESET)\n"; \
	printf "  $(C_RED)    ░░░░░░░░░░$(C_RESET)\n"; \
	printf "\n"
endef

# ============================================================
#  build start timestamp file
# ============================================================

BUILD_START_FILE := $(OUTPUT_DIR)/.build_start

# ============================================================
#  progress init — count total .c files (define, called directly)
# ============================================================

define PROGRESS_INIT
	$(eval TOTAL_FILES := $(shell \
		total=0; \
		for m in $(1); do \
			n=$$(find src/$$m -name '*.c' 2>/dev/null | wc -l); \
			total=$$((total + n)); \
		done; \
		echo $$total \
	))
	@mkdir -p $(OUTPUT_DIR)
	@echo "0" > $(PROGRESS_FILE)
	@echo "$(TOTAL_FILES)" > $(TOTAL_FILE)
endef
