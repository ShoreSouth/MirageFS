# ============================================================
#  编译工具
# ============================================================

CC := gcc
AR := ar

# ============================================================
#  编译选项
# ============================================================

CFLAGS := -Wall          \
           -Wextra       \
           -g            \
           -O2           \
           -fPIC         \
           -D_GNU_SOURCE

# ============================================================
#  自动推导工程根目录
# ============================================================

ROOT_DIR := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))..)

# ============================================================
#  输出目录
# ============================================================

OUTPUT_DIR := $(ROOT_DIR)/output

OBJ_DIR := $(OUTPUT_DIR)/obj
LIB_DIR := $(OUTPUT_DIR)/lib
BIN_DIR := $(OUTPUT_DIR)/bin

# ============================================================
#  include 路径
# ============================================================

INCLUDES := -I$(ROOT_DIR)/src

# ============================================================
#  动画系统
# ============================================================

include $(ROOT_DIR)/build/animation.mk

# ============================================================
#  通用规则
# ============================================================

OBJ := $(SRC:.c=.o)

# module-level targets (only when MODULE is set, i.e., not root)
ifdef MODULE

all: _module_header $(LIB)

_module_header:
	$(ANIM_MODULE_ENTER)

clean:
	@rm -f $(OBJ) $(LIB)

print:
	@echo "ROOT_DIR=$(ROOT_DIR)"
	@echo "MODULE=$(MODULE)"
	@echo "SRC=$(SRC)"
	@echo "OBJ=$(OBJ)"

endif

$(LIB): $(OBJ)
	@mkdir -p $(LIB_DIR)
	@$(AR) rcs $@ $^

# 静默模式（无进度条时）与动画模式（有进度条时）自动切换
%.o: %.c
	@if [ -f $(PROGRESS_FILE) ]; then \
		printf "    $(C_DIM)CC$(C_RESET)   %-40s" "$$(basename $<)"; \
		$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ 2>$@.err; \
		if [ $$? -eq 0 ]; then \
			rm -f $@.err; \
			printf " $(C_GREEN)✓$(C_RESET)\n"; \
			cnt=$$(cat $(PROGRESS_FILE) 2>/dev/null || echo 0); \
			echo $$((cnt + 1)) > $(PROGRESS_FILE); \
		else \
			printf " $(C_RED)✗$(C_RESET)\n"; \
			cat $@.err; \
			rm -f $@.err; \
			exit 1; \
		fi; \
		current=$$(cat $(PROGRESS_FILE) 2>/dev/null || echo 0); \
		total=$$(cat $(TOTAL_FILE) 2>/dev/null || echo 1); \
		pct=$$(( current * 100 / total )); \
		fill=$$(( pct / 4 )); empty=$$(( 25 - fill )); \
		bar=""; i=0; while [ $$i -lt $$fill ]; do bar="$${bar}▓"; i=$$((i+1)); done; \
		i=0; while [ $$i -lt $$empty ]; do bar="$${bar}░"; i=$$((i+1)); done; \
		printf "\r  $(C_BYELLOW)[$(C_RESET)%s$(C_BYELLOW)]$(C_RESET) %3d%%  (%d/%d)  %s\r" \
			"$$bar" $$pct $$current $$total "$$(basename $$(pwd))"; \
	else \
		printf "    $(C_DIM)CC$(C_RESET)   %-40s\n" "$<"; \
		$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@; \
	fi

# module-level clean and print are provided by each module Makefile
