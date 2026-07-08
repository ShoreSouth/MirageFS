# ============================================================
#  引入统一构建配置
# ============================================================

include build/build.mk

# ============================================================
#  模块列表
# ============================================================

MODULES := common
MODULES += config
MODULES += lsa
MODULES += object
MODULES += fsc
MODULES += fops

# ============================================================
#  最终程序
# ============================================================

TARGET := $(BIN_DIR)/miragefs

# ============================================================
#  APP
# ============================================================

APP_SRC := src/app/main.c
APP_OBJ := $(OBJ_DIR)/main.o

# ============================================================
#  依赖库
# ============================================================

LIBS  := $(LIB_DIR)/libconfig.a
LIBS  += $(LIB_DIR)/libfops.a
LIBS  += $(LIB_DIR)/libfsc.a
LIBS  += $(LIB_DIR)/libobject.a
LIBS  += $(LIB_DIR)/liblsa.a
LIBS  += $(LIB_DIR)/libcommon.a

LINK_LIBS := -Wl,--start-group $(LIBS) -Wl,--end-group

# ============================================================
#  默认目标 — 全流程编排（shell inline，避免 $(call) 转义问题）
# ============================================================

all:
	@$(ANIM_ENTRANCE)
	@mkdir -p $(OUTPUT_DIR); date +%s > $(BUILD_START_FILE)
	@cores=$$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4); \
	printf "$(C_DIM)  modules:$(C_RESET) $(C_BOLD)$(C_GREEN)%s$(C_RESET)$(C_DIM)  |  jobs:$(C_RESET) %s\n\n" \
		"$(MODULES)" "$$cores"
	@$(call PROGRESS_INIT,$(MODULES))
	@$(MAKE) -s _build 2>&1 && { \
		printf "\n"; \
		rm -f $(PROGRESS_FILE) $(TOTAL_FILE); \
		$(ANIM_SUCCESS); \
		if [ -f $(BUILD_START_FILE) ]; then \
			start=$$(cat $(BUILD_START_FILE)); \
			now=$$(date +%s); \
			elapsed=$$((now - start)); \
			if [ $$elapsed -lt 60 ]; then \
				printf "  $(C_DIM)build time: $${elapsed}s$(C_RESET)\n"; \
			else \
				mins=$$((elapsed / 60)); secs=$$((elapsed % 60)); \
				printf "  $(C_DIM)build time: $${mins}m $${secs}s$(C_RESET)\n"; \
			fi; \
			rm -f $(BUILD_START_FILE); \
		fi; \
	} || { \
		printf "\n"; \
		rm -f $(PROGRESS_FILE) $(TOTAL_FILE); \
		$(ANIM_FAILURE); \
		if [ -f $(BUILD_START_FILE) ]; then \
			start=$$(cat $(BUILD_START_FILE)); \
			now=$$(date +%s); \
			elapsed=$$((now - start)); \
			printf "  $(C_DIM)build time: $${elapsed}s (failed)$(C_RESET)\n"; \
			rm -f $(BUILD_START_FILE); \
		fi; \
		exit 1; \
	}

# ============================================================
#  inner build
# ============================================================

_build: prepare modules app link

prepare:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(LIB_DIR)
	@mkdir -p $(BIN_DIR)

modules:
	@for m in $(MODULES); do \
		$(MAKE) -s -C src/$$m; \
	done

app: $(APP_OBJ)

$(APP_OBJ): $(APP_SRC)
	@printf "    $(C_DIM)CC$(C_RESET)   %-40s" "main.c"
	@$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@ && printf " $(C_GREEN)✓$(C_RESET)\n"

link: $(TARGET)

$(TARGET): $(APP_OBJ)
	@printf "    $(C_MAGENTA)LD$(C_RESET)   %-40s" "miragefs"
	@$(CC) $(CFLAGS) -o $@ $(APP_OBJ) $(LINK_LIBS) && printf " $(C_GREEN)✓$(C_RESET)\n"

# ============================================================
#  清理
# ============================================================

clean:
	@for m in $(MODULES); do \
		$(MAKE) -s -C src/$$m clean; \
	done
	@rm -rf output
	@find . -type d -name __pycache__ -prune -exec rm -rf {} +
	@printf "  $(C_GREEN)✓$(C_RESET) clean done\n"

# ============================================================
#  调试辅助
# ============================================================

print:
	@echo "ROOT_DIR=$(ROOT_DIR)"
	@echo "OUTPUT_DIR=$(OUTPUT_DIR)"
	@echo "OBJ_DIR=$(OBJ_DIR)"
	@echo "LIB_DIR=$(LIB_DIR)"
	@echo "BIN_DIR=$(BIN_DIR)"

.PHONY: all _build prepare modules app link clean print
