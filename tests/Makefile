include ../build/build.mk

TEST_OUTPUT_DIR := $(OUTPUT_DIR)/tests
TEST_OBJ_DIR := $(TEST_OUTPUT_DIR)/obj
TEST_BIN_DIR := $(TEST_OUTPUT_DIR)/bin
COVERAGE_DIR := $(OUTPUT_DIR)/coverage

TEST_CFLAGS := $(CFLAGS) $(INCLUDES) -I$(ROOT_DIR)/tests -std=c17
TEST_LDFLAGS ?=
COVERAGE_MIN ?= 0

TEST_FRAMEWORK_OBJ := $(TEST_OBJ_DIR)/framework/test_framework.o
CONFIG_TEST_OBJ := $(TEST_OBJ_DIR)/config/test_config.o
CONFIG_TEST_BIN := $(TEST_BIN_DIR)/config_test
COMMON_TEST_OBJ := $(TEST_OBJ_DIR)/common/test_common.o
COMMON_TEST_BIN := $(TEST_BIN_DIR)/common_test
LSA_TEST_OBJ := $(TEST_OBJ_DIR)/lsa/test_lsa.o
LSA_TEST_BIN := $(TEST_BIN_DIR)/lsa_test
OBJECT_TEST_OBJ := $(TEST_OBJ_DIR)/object/test_object.o
OBJECT_TEST_BIN := $(TEST_BIN_DIR)/object_test
FSC_TEST_OBJ := $(TEST_OBJ_DIR)/fsc/test_fsc.o
FSC_TEST_BIN := $(TEST_BIN_DIR)/fsc_test
FOPS_TEST_OBJ := $(TEST_OBJ_DIR)/fops/test_fops.o
FOPS_TEST_BIN := $(TEST_BIN_DIR)/fops_test
NAMEI_TEST_OBJ := $(TEST_OBJ_DIR)/namei/test_namei.o
NAMEI_TEST_BIN := $(TEST_BIN_DIR)/namei_test
RUNTIME_TEST_OBJ := $(TEST_OBJ_DIR)/runtime/test_runtime.o
RUNTIME_TEST_BIN := $(TEST_BIN_DIR)/runtime_test
MSH_TEST_OBJ := $(TEST_OBJ_DIR)/msh/test_msh.o
MSH_TEST_BIN := $(TEST_BIN_DIR)/msh_test

TEST_BINS := $(CONFIG_TEST_BIN)
TEST_BINS += $(COMMON_TEST_BIN)
TEST_BINS += $(LSA_TEST_BIN)
TEST_BINS += $(OBJECT_TEST_BIN)
TEST_BINS += $(FSC_TEST_BIN)
TEST_BINS += $(FOPS_TEST_BIN)
TEST_BINS += $(NAMEI_TEST_BIN)
TEST_BINS += $(RUNTIME_TEST_BIN)
TEST_BINS += $(MSH_TEST_BIN)

ifeq ($(MODULE),config)
SELECTED_TEST_BINS := $(CONFIG_TEST_BIN)
else ifeq ($(MODULE),common)
SELECTED_TEST_BINS := $(COMMON_TEST_BIN)
else ifeq ($(MODULE),lsa)
SELECTED_TEST_BINS := $(LSA_TEST_BIN)
else ifeq ($(MODULE),object)
SELECTED_TEST_BINS := $(OBJECT_TEST_BIN)
else ifeq ($(MODULE),fsc)
SELECTED_TEST_BINS := $(FSC_TEST_BIN)
else ifeq ($(MODULE),fops)
SELECTED_TEST_BINS := $(FOPS_TEST_BIN)
else ifeq ($(MODULE),namei)
SELECTED_TEST_BINS := $(NAMEI_TEST_BIN)
else ifeq ($(MODULE),runtime)
SELECTED_TEST_BINS := $(RUNTIME_TEST_BIN)
else ifeq ($(MODULE),msh)
SELECTED_TEST_BINS := $(MSH_TEST_BIN)
else ifeq ($(MODULE),)
SELECTED_TEST_BINS := $(TEST_BINS)
else
$(error unknown test module: $(MODULE))
endif

test: build-app run-selected

test-no-build: $(SELECTED_TEST_BINS) run-selected

run-selected: $(SELECTED_TEST_BINS)
	@set -e; \
	for bin in $(SELECTED_TEST_BINS); do \
		printf "\n==> %s\n" "$$bin"; \
		"$$bin"; \
	done

build-app:
	@env -u MODULE MAKEFLAGS= $(MAKE) -s -C $(ROOT_DIR) all

coverage:
	@$(MAKE) -s -C $(ROOT_DIR) clean
	@find $(ROOT_DIR)/src -name '*.gcda' -delete -o -name '*.gcno' -delete
	@mkdir -p $(COVERAGE_DIR)
	@$(MAKE) -s -C $(ROOT_DIR) CFLAGS="$(CFLAGS) -O0 --coverage" all
	@$(MAKE) -s TEST_CFLAGS="$(TEST_CFLAGS) -O0 --coverage" TEST_LDFLAGS="--coverage" test-no-build
	@lcov --capture --directory $(ROOT_DIR) --output-file $(COVERAGE_DIR)/coverage.info >/dev/null
	@lcov --remove $(COVERAGE_DIR)/coverage.info '/usr/*' '*/tests/*' '*/output/tests/*' --output-file $(COVERAGE_DIR)/coverage.filtered.info >/dev/null
	@genhtml $(COVERAGE_DIR)/coverage.filtered.info --output-directory $(COVERAGE_DIR)/html >/dev/null
	@lcov --summary $(COVERAGE_DIR)/coverage.filtered.info

coverage-check: coverage
	@line_rate=$$(lcov --summary $(COVERAGE_DIR)/coverage.filtered.info 2>/dev/null | awk '/lines/ { gsub("%", "", $$2); print $$2; exit }'); \
	awk -v actual="$$line_rate" -v min="$(COVERAGE_MIN)" 'BEGIN { \
		printf("coverage line-rate: %.2f%%, threshold: %.2f%%\n", actual, min); \
		exit(actual + 0.0 >= min + 0.0 ? 0 : 1); \
	}'

$(CONFIG_TEST_BIN): $(CONFIG_TEST_OBJ) $(TEST_FRAMEWORK_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(TEST_LDFLAGS) -o $@ $^ $(LINK_LIBS)

$(COMMON_TEST_BIN): $(COMMON_TEST_OBJ) $(TEST_FRAMEWORK_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(TEST_LDFLAGS) -o $@ $^ $(LINK_LIBS)

$(LSA_TEST_BIN): $(LSA_TEST_OBJ) $(TEST_FRAMEWORK_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(TEST_LDFLAGS) -o $@ $^ $(LINK_LIBS)

$(OBJECT_TEST_BIN): $(OBJECT_TEST_OBJ) $(TEST_FRAMEWORK_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(TEST_LDFLAGS) -o $@ $^ $(LINK_LIBS)

$(FSC_TEST_BIN): $(FSC_TEST_OBJ) $(TEST_FRAMEWORK_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(TEST_LDFLAGS) -o $@ $^ $(LINK_LIBS)

$(FOPS_TEST_BIN): $(FOPS_TEST_OBJ) $(TEST_FRAMEWORK_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(TEST_LDFLAGS) -o $@ $^ $(LINK_LIBS)

$(NAMEI_TEST_BIN): $(NAMEI_TEST_OBJ) $(TEST_FRAMEWORK_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(TEST_LDFLAGS) -o $@ $^ $(LINK_LIBS)

$(RUNTIME_TEST_BIN): $(RUNTIME_TEST_OBJ) $(TEST_FRAMEWORK_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(TEST_LDFLAGS) -o $@ $^ $(LINK_LIBS)

$(MSH_TEST_BIN): $(MSH_TEST_OBJ) $(TEST_FRAMEWORK_OBJ)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(TEST_LDFLAGS) -o $@ $^ $(LINK_LIBS)

$(TEST_OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@$(CC) $(TEST_CFLAGS) -c $< -o $@

clean:
	@rm -rf $(TEST_OUTPUT_DIR) $(COVERAGE_DIR)

.PHONY: test test-no-build run-selected build-app coverage coverage-check clean

LIBS := $(LIB_DIR)/libconfig.a
LIBS += $(LIB_DIR)/libnamei.a
LIBS += $(LIB_DIR)/libmsh.a
LIBS += $(LIB_DIR)/libruntime.a
LIBS += $(LIB_DIR)/libfops.a
LIBS += $(LIB_DIR)/libfsc.a
LIBS += $(LIB_DIR)/libobject.a
LIBS += $(LIB_DIR)/liblsa.a
LIBS += $(LIB_DIR)/libcommon.a

LINK_LIBS := -Wl,--start-group $(LIBS) -Wl,--end-group