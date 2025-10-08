.POSIX:

################################# The Prelude ##################################

.PHONY: release
.PHONY: debug
.PHONY: clang-release
.PHONY: msvc-release
.PHONY: multi-compiler help
.PHONY: test
.PHONY: clang-test
.PHONY: msvc-test
.PHONY: coverage

######################## Default Test Targets ########################

test-vec:
	@echo "Hold on. Build in progress... (output supressed until test results)"
	@$(MAKE) _test BUILD_TYPE=TEST DS=vector > /dev/null
	cat $(PATH_RESULTS)test_vector.txt | python $(COLORIZE_UNITY_SCRIPT)
	@$(MAKE) coverage BUILD_TYPE=TEST DS=vector > /dev/null

test-com:
	@echo "Hold on. Build in progress... (output supressed until test results)"
	@$(MAKE) _test BUILD_TYPE=TEST DS=conficol_shared > /dev/null
	cat $(PATH_RESULTS)test_conficol_shared.txt | python $(COLORIZE_UNITY_SCRIPT)
	@$(MAKE) coverage BUILD_TYPE=TEST DS=conficol_shared > /dev/null

test-all:
	@echo "Hold on. Build in progress... (output supressed until test results)"
	@$(MAKE) --always-make test-vec > /dev/null
	cat $(PATH_RESULTS)test_vector.txt | python $(COLORIZE_UNITY_SCRIPT)
	cat $(PATH_RESULTS)test_conficol_shared.txt | python $(COLORIZE_UNITY_SCRIPT)
	@$(MAKE) coverage BUILD_TYPE=TEST > /dev/null

test-com-verbose:
	@$(MAKE) _test BUILD_TYPE=TEST DS=conficol_shared
	@$(MAKE) coverage BUILD_TYPE=TEST DS=conficol_shared

test-vec-verbose:
	@$(MAKE) _test BUILD_TYPE=TEST DS=vector
	@$(MAKE) coverage BUILD_TYPE=TEST DS=vector

test-all-verbose:
	@$(MAKE) --always-make test-vec
	@$(MAKE) coverage BUILD_TYPE=TEST

######################## Clang Test Targets ########################

clang-test-vec:
	@echo "Hold on. Clang build in progress... (output suppressed until test results)"
	@$(MAKE) clang-_test BUILD_TYPE=TEST DS=vector > /dev/null
	cat $(PATH_RESULTS)clang_test_vector.txt | python $(COLORIZE_UNITY_SCRIPT)

clang-test-com:
	@echo "Hold on. Clang build in progress... (output suppressed until test results)"
	@$(MAKE) clang-_test BUILD_TYPE=TEST DS=conficol_shared > /dev/null
	cat $(PATH_RESULTS)clang_test_conficol_shared.txt | python $(COLORIZE_UNITY_SCRIPT)

clang-test-all:
	@echo "Hold on. Clang build in progress... (output suppressed until test results)"
	@$(MAKE) --always-make clang-test-vec > /dev/null
	cat $(PATH_RESULTS)clang_test_vector.txt | python $(COLORIZE_UNITY_SCRIPT)
	cat $(PATH_RESULTS)clang_test_conficol_shared.txt | python $(COLORIZE_UNITY_SCRIPT)

clang-test-com-verbose:
	@$(MAKE) clang-_test BUILD_TYPE=TEST DS=conficol_shared

clang-test-vec-verbose:
	@$(MAKE) clang-_test BUILD_TYPE=TEST DS=vector

clang-test-all-verbose:
	@$(MAKE) --always-make clang-test-vec

######################### MSVC Test Targets #########################

msvc-test-vec:
	@echo "Hold on. MSVC build in progress... (output suppressed until test results)"
	@$(MAKE) msvc-_test BUILD_TYPE=TEST DS=vector > /dev/null
	cat $(PATH_RESULTS)msvc_test_vector.txt | python $(COLORIZE_UNITY_SCRIPT)

msvc-test-com:
	@echo "Hold on. MSVC build in progress... (output suppressed until test results)"
	@$(MAKE) msvc-_test BUILD_TYPE=TEST DS=conficol_shared > /dev/null
	cat $(PATH_RESULTS)msvc_test_conficol_shared.txt | python $(COLORIZE_UNITY_SCRIPT)

msvc-test-all:
	@echo "Hold on. MSVC build in progress... (output suppressed until test results)"
	@$(MAKE) --always-make msvc-test-vec > /dev/null
	cat $(PATH_RESULTS)msvc_test_vector.txt | python $(COLORIZE_UNITY_SCRIPT)
	cat $(PATH_RESULTS)msvc_test_conficol_shared.txt | python $(COLORIZE_UNITY_SCRIPT)

msvc-test-com-verbose:
	@$(MAKE) msvc-_test BUILD_TYPE=TEST DS=conficol_shared

msvc-test-vec-verbose:
	@$(MAKE) msvc-_test BUILD_TYPE=TEST DS=vector

msvc-test-all-verbose:
	@$(MAKE) --always-make msvc-test-vec

######################### Other Targets #########################

release:
	@$(MAKE) lib BUILD_TYPE=RELEASE DS=ALL

libvector: release-vec
release-vec:
	@$(MAKE) lib BUILD_TYPE=RELEASE DS=vector

debug:
	@$(MAKE) lib BUILD_TYPE=DEBUG DS=ALL

debug-vec:
	@$(MAKE) lib BUILD_TYPE=DEBUG DS=vector

# Clang targets
clang-libvector: clang-release-vec
clang-release-vec:
	@$(MAKE) clang-lib BUILD_TYPE=RELEASE DS=vector

# MSVC targets  
msvc-libvector: msvc-release-vec
msvc-release-vec:
	@$(MAKE) msvc-lib BUILD_TYPE=RELEASE DS=vector

# Multi-compiler target to build with all three compilers
multi-compiler:
	@echo "Building with GCC..."
	@$(MAKE) libvector
	@echo "Building with Clang..."
	@$(MAKE) clang-libvector  
	@echo "Building with MSVC..."
	@$(MAKE) msvc-libvector
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[32;1mAll three compilers completed!\033[0m"
	@echo "Libraries built:"
	@echo "  - GCC:   $(PATH_BUILD)libvector.$(STATIC_LIB_EXTENSION)"
	@echo "  - Clang: $(PATH_BUILD)libclang_libvector.$(STATIC_LIB_EXTENSION)"
	@echo "  - MSVC:  $(PATH_BUILD)libmsvc_libvector.$(STATIC_LIB_EXTENSION)"
	@echo "----------------------------------------"

# Help target
help:
	@echo "Available targets:"
	@echo "  libvector         - Build static library with GCC (default)"
	@echo "  clang-libvector   - Build static library with Clang"
	@echo "  msvc-libvector    - Build static library with MSVC"
	@echo "  multi-compiler    - Build with all three compilers"
	@echo ""
	@echo "Test targets:"
	@echo "  test-vec          - Run vector unit tests with GCC"
	@echo "  test-com          - Run conficol_shared unit tests with GCC"
	@echo "  test-all          - Run all unit tests with GCC"
	@echo "  clang-test-vec    - Run vector unit tests with Clang"
	@echo "  clang-test-com    - Run conficol_shared unit tests with Clang"
	@echo "  clang-test-all    - Run all unit tests with Clang"
	@echo "  msvc-test-vec     - Run vector unit tests with MSVC"
	@echo "  msvc-test-com     - Run conficol_shared unit tests with MSVC"
	@echo "  msvc-test-all     - Run all unit tests with MSVC"
	@echo ""
	@echo "Other targets:"
	@echo "  clean             - Clean all build artifacts"
	@echo "  help              - Show this help message"
	@echo ""
	@echo "Compiler-specific features:"
	@echo "  GCC:   Full warning set + GCC analyzer + sanitizers + coverage"
	@echo "  Clang: Clang-specific warnings + static analyzer + thread safety"
	@echo "  MSVC:  /W4 /analyze + MSVC-specific static analysis"

######################### Environment Setup #########################

CLEANUP = rm -f
MKDIR = mkdir -p
TARGET_EXTENSION=exe
# Set the OS-specific tool cmds / executable extensions
ifeq ($(OS),Windows_NT)

  TARGET_EXTENSION = exe
  STATIC_LIB_EXTENSION = lib

  ifeq ($(shell uname -s),) # not in a bash-like shell
    CLEANUP = del /F /Q
    MKDIR = mkdir
  else # in a bash-like shell, like msys
    CLEANUP = rm -f
    MKDIR = mkdir -p
  endif

else

  TARGET_EXTENSION = out
  STATIC_LIB_EXTENSION = a
  CLEANUP = rm -f
  MKDIR = mkdir -p

endif

BUILD_TYPE ?= RELEASE
DS ?= ALL

# Relevant paths
PATH_UNITY        = Unity/src/
PATH_SRC          = src/
PATH_INC          = inc/
PATH_CFG          = cfg/
PATH_DEP          = submodules/
PATH_TEST_FILES   = test/
PATH_BUILD        = build/
REL_DIR           = rel/
DBG_DIR           = dbg/
OBJ_DIR           = objs/
ifeq ($(BUILD_TYPE), RELEASE)
	PATH_OBJ_FILES = $(PATH_BUILD)$(REL_DIR)$(OBJ_DIR)
else
	PATH_OBJ_FILES = $(PATH_BUILD)$(DBG_DIR)$(OBJ_DIR)
endif
PATH_RESULTS      = $(PATH_BUILD)results/
PATH_PROFILE      = $(PATH_BUILD)profile/
PATH_BENCHMARK	   = benchmark/
PATH_SCRIPTS      = scripts/
BUILD_DIRS        = $(PATH_BUILD) $(PATH_OBJ_FILES)

COLORIZE_CPPCHECK_SCRIPT = $(PATH_SCRIPTS)colorize_cppcheck.py
COLORIZE_UNITY_SCRIPT = $(PATH_SCRIPTS)colorize_unity.py

# Lists of files

UNITY_SRC_FILES = $(wildcard $(PATH_UNITY)*.c)
UNITY_HDR_FILES = $(wildcard $(PATH_UNITY)*.h)
UNITY_OBJ_FILES = $(patsubst %.c, $(PATH_OBJ_FILES)%.o, $(notdir $(UNITY_SRC_FILES)))
UNITY_LIB = unity

COLLECTION_LIB_NAME = conficol

SHARED_SRC_FILES = $(PATH_SRC)conficol_shared.c
SHARED_HDR_FILES = $(PATH_INC)conficol_shared.h
SRC_FILES += $(SHARED_SRC_FILES)
HDR_FILES += $(SHARED_HDR_FILES)
ifeq ($(DS), ALL)
  SRC_FILES += $(wildcard $(PATH_SRC)*.c)
  HDR_FILES += $(wildcard $(PATH_INC)*.h) $(wildcard $(PATH_CFG)$(DS)_cfg.h) $(PATH_DEP)
  SRC_TEST_FILES = $(wildcard $(PATH_TEST_FILES)*.c)
  LIB_FILE = $(PATH_BUILD)lib$(COLLECTION_LIB_NAME).$(STATIC_LIB_EXTENSION)
else
  SRC_FILES += $(PATH_SRC)$(DS).c
  HDR_FILES += $(PATH_INC)$(DS).h $(wildcard $(PATH_CFG)$(DS)_cfg.h) $(PATH_DEP)
  SRC_TEST_FILES = $(PATH_TEST_FILES)test_$(DS).c
  LIB_FILE = $(PATH_BUILD)lib$(DS).$(STATIC_LIB_EXTENSION)
endif
TEST_EXECUTABLES = $(patsubst %.c, $(PATH_BUILD)%.$(TARGET_EXTENSION), $(notdir $(SRC_TEST_FILES)))
LIB_LIST_FILE = $(patsubst %.$(STATIC_LIB_EXTENSION), $(PATH_BUILD)%.lst, $(notdir $(LIB_FILE)))
TEST_LIST_FILE = $(patsubst %.$(TARGET_EXTENSION), $(PATH_BUILD)%.lst, $(notdir $(TEST_EXECUTABLES)))
TEST_OBJ_FILES = $(patsubst %.c, $(PATH_OBJ_FILES)%.o, $(notdir $(SRC_TEST_FILES)))
RESULTS = $(patsubst %.c, $(PATH_RESULTS)%.txt, $(notdir $(SRC_TEST_FILES)))

# List of all gcov coverage files I'm expecting
GCOV_FILES = $(SRC_FILES:.c=.c.gcov)

ifeq ($(BUILD_TYPE), TEST)
  BUILD_DIRS += $(PATH_RESULTS)
else ifeq ($(BUILD_TYPE), PROFILE)
  BUILD_DIRS += $(PATH_PROFILE)
endif

# List of all object files we're expecting for the data structures
OBJ_FILES = $(patsubst %.c,$(PATH_OBJ_FILES)%.o, $(notdir $(SRC_FILES)))

# Compiler setup
CROSS	= 
CC = $(CROSS)gcc

COMPILER_WARNING_FLAGS = \
    -Wall -Wextra -Wpedantic -pedantic-errors \
    -Wconversion -Wdouble-promotion -Wnull-dereference \
    -Wwrite-strings -Wformat=2 -Wformat-overflow=2 \
    -Wformat-signedness -Wuseless-cast -Wstrict-prototypes \
    -Wcast-align=strict -Wcast-qual \
    -Wimplicit-fallthrough=3 -Wswitch-default \
    -Wswitch-enum -Wfloat-equal -Wuse-after-free=2 \
    -Wdeprecated-declarations -Wmissing-prototypes -Wparentheses \
    -Wreturn-type -Wlogical-op -Wstrict-aliasing \
    -Wuninitialized -Wmaybe-uninitialized -Wshadow \
    -Wduplicated-cond \
    -Walloc-zero -Walloc-size

# Includes some -Wno-... flags for warnings that I'd normally want for my lib
# src but **not** for my test file, which intentionally has all sorts of
# naughty shenanigans going on
COMPILER_WARNINGS_TEST_BUILD = \
    -Wall -Wextra -Wpedantic -pedantic-errors \
    -Wconversion -Wdouble-promotion -Wnull-dereference \
    -Wwrite-strings -Wformat=2 -Wformat-overflow=2 \
    -Wformat-signedness \
    -Wcast-align=strict -Wcast-qual \
    -Wimplicit-fallthrough=3 -Wswitch-default \
    -Wswitch-enum -Wfloat-equal -Wuse-after-free=2 \
    -Wdeprecated-declarations -Wmissing-prototypes -Wparentheses \
    -Wreturn-type -Wlogical-op -Wstrict-aliasing \
    -Wuninitialized -Wmaybe-uninitialized -Wshadow \
    -Walloc-zero -Walloc-size \
    -Wno-analyzer-use-of-uninitialized-value -Wno-uninitialized \
    -Wno-maybe-uninitialized

# Consider -Wmismatched-dealloc
COMPILER_SANITIZERS = \
    -fsanitize=undefined -fsanitize-trap \
    -fsanitize=enum  -fsanitize=bool -fsanitize=bounds

LIB_OPTIMIZATION_FLAGS ?=
ifdef LTO
LIB_OPTIMIZATION_FLAGS = -ffunction-sections -fdata-sections # Facilitates link-time optimization (LDO)
endif
COMPILER_OPTIMIZATION_LEVEL_DEBUG = -Og -g3
COMPILER_OPTIMIZATION_LEVEL_SPEED = -O3 $(LIB_OPTIMIZATION_FLAGS)
COMPILER_OPTIMIZATION_LEVEL_SPACE = -Os $(LIB_OPTIMIZATION_FLAGS)
COMPILER_STANDARD = -std=c23
INCLUDE_PATHS = -I. -I$(PATH_INC) -I$(PATH_UNITY) -I$(PATH_CFG) -I$(PATH_DEP)
COMMON_DEFINES =
DIAGNOSTIC_FLAGS = -fdiagnostics-color
COMPILER_STATIC_ANALYZER = -fanalyzer

ifeq ($(BUILD_TYPE), TEST)
COMMON_DEFINES += -DMAX_VEC_LEN=UINT32_MAX
endif

# Compile up the compiler flags
CFLAGS = $(INCLUDE_PATHS) $(COMMON_DEFINES) \
         $(DIAGNOSTIC_FLAGS) $(COMPILER_WARNING_FLAGS) $(COMPILER_STATIC_ANALYZER) \
         $(COMPILER_STANDARD) $(COMPILER_OPTIMIZATION_LEVEL_SPEED)

CFLAGS_TEST = \
         -DTEST $(COMMON_DEFINES) \
         $(INCLUDE_PATHS) \
         $(DIAGNOSTIC_FLAGS) $(COMPILER_WARNINGS_TEST_BUILD) \
         $(COMPILER_STATIC_ANALYZER) $(COMPILER_STANDARD) \
         $(COMPILER_SANITIZERS) $(COMPILER_OPTIMIZATION_LEVEL_DEBUG)

ifeq ($(BUILD_TYPE), RELEASE)
CFLAGS += -DNDEBUG $(COMPILER_OPTIMIZATION_LEVEL_SPEED)

else ifeq ($(BUILD_TYPE), BENCHMARK)
CFLAGS += -DNDEBUG $(COMPILER_OPTIMIZATION_LEVEL_SPEED)

else ifeq ($(BUILD_TYPE), PROFILE)
CFLAGS += -DNDEBUG $(COMPILER_OPTIMIZATION_LEVEL_DEBUG) -pg
LDFLAGS += -pg

else ifeq ($(BUILD_TYPE), TEST)
CFLAGS += $(COMPILER_OPTIMIZATION_LEVEL_DEBUG) \
          -fcondition-coverage -fprofile-arcs -ftest-coverage
LDFLAGS += -lgcov --coverage

else
CFLAGS += $(COMPILER_SANITIZERS) $(COMPILER_OPTIMIZATION_LEVEL_DEBUG)
endif

# Compile up linker flags
LDFLAGS += $(DIAGNOSTIC_FLAGS)

############################# Clang Configuration ##############################

CLANG = clang
CLANG_WARNING_FLAGS = \
    -Wall -Wextra -Wpedantic -pedantic-errors \
    -Wconversion -Wdouble-promotion -Wnull-dereference \
    -Wwrite-strings -Wformat=2 -Wformat-type-confusion \
    -Wformat-security -Wcast-align -Wimplicit-fallthrough \
    -Wswitch-default -Wswitch-enum -Wfloat-equal \
    -Wdeprecated-declarations -Wmissing-prototypes -Wparentheses \
    -Wreturn-type -Wstrict-aliasing -Wuninitialized \
    -Wshadow \
    -Wnewline-eof -Wcomma -Wloop-analysis -Wlogical-op-parentheses \
    -Wbitwise-op-parentheses -Wshift-op-parentheses \
    -Wtautological-compare -Wunreachable-code-aggressive \
    -Wthread-safety -Wthread-safety-negative

CLANG_STATIC_ANALYZER = --analyze -Xanalyzer -analyzer-output=text
CLANG_SANITIZERS = \
    -fsanitize=undefined -fsanitize=bounds -fsanitize-address \
    -fsanitize=enum -fsanitize=bool

CLANG_CFLAGS = $(INCLUDE_PATHS) $(COMMON_DEFINES) \
               -fcolor-diagnostics $(CLANG_WARNING_FLAGS) \
               $(COMPILER_STANDARD) $(COMPILER_OPTIMIZATION_LEVEL_SPEED)

ifeq ($(BUILD_TYPE), RELEASE)
CLANG_CFLAGS += -DNDEBUG $(COMPILER_OPTIMIZATION_LEVEL_SPEED)
else
CLANG_CFLAGS += $(CLANG_SANITIZERS) $(COMPILER_OPTIMIZATION_LEVEL_DEBUG)
endif

############################## MSVC Configuration ###############################

MSVC = cl.exe
MSVC_WARNING_FLAGS = \
    /W4 /WX /analyze /wd4996 /wd4127 \
    /Wall /wd4668 /wd4820 /wd4710 /wd4711 /wd5045 \
    /wd4464 /wd4255 /wd4061 /wd4062

MSVC_INCLUDE_PATHS = $(subst -I,/I,$(INCLUDE_PATHS))
MSVC_DEFINES = $(subst -D,/D,$(COMMON_DEFINES))

MSVC_CFLAGS = $(MSVC_INCLUDE_PATHS) $(MSVC_DEFINES) \
              $(MSVC_WARNING_FLAGS) /std:c17

ifeq ($(BUILD_TYPE), RELEASE)
MSVC_CFLAGS += /DNDEBUG /O2 /GL
else
MSVC_CFLAGS += /Od /Zi /RTC1
endif

############################ Test Configurations #############################

# Clang test configuration
CLANG_WARNING_FLAGS_TEST = \
    -Wall -Wextra -Wpedantic -pedantic-errors \
    -Wconversion -Wdouble-promotion -Wnull-dereference \
    -Wwrite-strings -Wformat=2 -Wformat-type-confusion \
    -Wformat-security -Wcast-align -Wimplicit-fallthrough \
    -Wswitch-default -Wswitch-enum -Wfloat-equal \
    -Wdeprecated-declarations -Wmissing-prototypes -Wparentheses \
    -Wreturn-type -Wstrict-aliasing -Wuninitialized \
    -Wshadow -Walloc-size-larger-than=1073741824 \
    -Wnewline-eof -Wcomma -Wloop-analysis -Wlogical-op-parentheses \
    -Wbitwise-op-parentheses -Wshift-op-parentheses \
    -Wtautological-compare -Wunreachable-code-aggressive \
    -Wno-analyzer-use-of-uninitialized-value -Wno-uninitialized \
    -Wno-maybe-uninitialized

CLANG_CFLAGS_TEST = \
    -DTEST $(COMMON_DEFINES) \
    $(INCLUDE_PATHS) \
    -fcolor-diagnostics $(CLANG_WARNING_FLAGS_TEST) \
    $(COMPILER_STANDARD) \
    $(CLANG_SANITIZERS) $(COMPILER_OPTIMIZATION_LEVEL_DEBUG)

# MSVC test configuration  
MSVC_WARNING_FLAGS_TEST = \
    /W4 /analyze /wd4996 /wd4127 \
    /wd4668 /wd4820 /wd4710 /wd4711 /wd5045 \
    /wd4464 /wd4255 /wd4061 /wd4062 /wd4214 \
    /wd4201 /wd4204 /wd4221

MSVC_CFLAGS_TEST = \
    /DTEST $(MSVC_DEFINES) \
    $(MSVC_INCLUDE_PATHS) \
    $(MSVC_WARNING_FLAGS_TEST) /std:c17 \
    /Od /Zi /RTC1

# gcov Flags
GCOV = gcov
GCOV_FLAGS = --conditions --function-summaries --branch-probabilities --branch-counts
ifeq ($(GCOV_CON), 1)
GCOV_FLAGS += --use-colors --stdout
endif
GCOV_CONSOLE_OUT_FILE = gcov_console_out.txt

# gcovr Flags
GCOVR_FLAGS = --html-details $(PATH_RESULTS)coverage.html \
             --exclude-lines-by-pattern 'assert'

############################# The Rules & Recipes ##############################

######################### Lib Rules ########################
# Build the static library files
.PHONY: collection
collection: $(BUILD_DIRS) $(LIB_FILE) $(LIB_LIST_FILE)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[32mCollection Library \033[35m$(LIB_FILE) \033[32;1mbuilt\033[0m!"
	@echo "----------------------------------------"

.PHONY: lib
# Build the static library files
lib: $(BUILD_DIRS) $(LIB_FILE) $(LIB_LIST_FILE)
	@echo
	@echo "----------------------------------------"
	@echo -e "Library \033[35m$(LIB_FILE) \033[32;1mbuilt\033[0m!"
	@echo "----------------------------------------"

######################### Clang Lib Rules ########################

# Clang static library build
CLANG_LIB_FILE = $(PATH_BUILD)libclang_$(notdir $(LIB_FILE))
CLANG_LIB_LIST_FILE = $(patsubst %.$(STATIC_LIB_EXTENSION), $(PATH_BUILD)clang_%.lst, $(notdir $(LIB_FILE)))
CLANG_OBJ_FILES = $(patsubst %.c,$(PATH_OBJ_FILES)clang_%.o, $(notdir $(SRC_FILES)))

.PHONY: clang-lib
clang-lib: $(BUILD_DIRS) $(CLANG_LIB_FILE) $(CLANG_LIB_LIST_FILE)
	@echo
	@echo "----------------------------------------"
	@echo -e "Clang Library \033[35m$(CLANG_LIB_FILE) \033[32;1mbuilt\033[0m!"
	@echo "----------------------------------------"

$(CLANG_LIB_FILE): $(CLANG_OBJ_FILES) $(BUILD_DIRS)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[32mConstructing\033[0m the clang static library: $@..."
	@echo
	ar rcs $@ $(CLANG_OBJ_FILES)

# Clang object file compilation rules
$(PATH_OBJ_FILES)clang_%.o : $(PATH_SRC)%.c $(PATH_INC)%.h $(PATH_CFG)%_cfg.h
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling with Clang\033[0m the collection source file: $<..."
	@echo
	$(CLANG) -c $(CLANG_CFLAGS) $< -o $@
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[33mRunning Clang static analysis\033[0m on $<..."
	@echo
	-$(CLANG) $(CLANG_STATIC_ANALYZER) $(CLANG_CFLAGS) $< 2>&1 | tee $(PATH_BUILD)clang_analysis.log

$(PATH_OBJ_FILES)clang_%.o : $(PATH_SRC)%.c $(PATH_INC)%.h
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling with Clang\033[0m the collection source file: $<..."
	@echo
	$(CLANG) -c $(CLANG_CFLAGS) $< -o $@
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[33mRunning Clang static analysis\033[0m on $<..."
	@echo
	-$(CLANG) $(CLANG_STATIC_ANALYZER) $(CLANG_CFLAGS) $< 2>&1 | tee $(PATH_BUILD)clang_analysis.log

$(PATH_OBJ_FILES)clang_%.o : $(PATH_SRC)%.c
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling with Clang\033[0m the collection source file: $<..."
	@echo
	$(CLANG) -c $(CLANG_CFLAGS) $< -o $@
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[33mRunning Clang static analysis\033[0m on $<..."
	@echo
	-$(CLANG) $(CLANG_STATIC_ANALYZER) $(CLANG_CFLAGS) $< 2>&1 | tee $(PATH_BUILD)clang_analysis.log

$(CLANG_LIB_LIST_FILE): $(CLANG_LIB_FILE)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[33mDisassembly\033[0m of $< into $@..."
	@echo
	objdump -D $< > $@

########################## MSVC Lib Rules ##########################

# MSVC static library build
MSVC_LIB_FILE = $(PATH_BUILD)libmsvc_$(notdir $(LIB_FILE))
MSVC_LIB_LIST_FILE = $(patsubst %.$(STATIC_LIB_EXTENSION), $(PATH_BUILD)msvc_%.lst, $(notdir $(LIB_FILE)))
MSVC_OBJ_FILES = $(patsubst %.c,$(PATH_OBJ_FILES)msvc_%.obj, $(notdir $(SRC_FILES)))

.PHONY: msvc-lib
msvc-lib: $(BUILD_DIRS) $(MSVC_LIB_FILE) $(MSVC_LIB_LIST_FILE)
	@echo
	@echo "----------------------------------------"
	@echo -e "MSVC Library \033[35m$(MSVC_LIB_FILE) \033[32;1mbuilt\033[0m!"
	@echo "----------------------------------------"

$(MSVC_LIB_FILE): $(MSVC_OBJ_FILES) $(BUILD_DIRS)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[32mConstructing\033[0m the MSVC static library: $@..."
	@echo
	lib.exe /OUT:$@ $(MSVC_OBJ_FILES)

# MSVC object file compilation rules  
$(PATH_OBJ_FILES)msvc_%.obj : $(PATH_SRC)%.c $(PATH_INC)%.h $(PATH_CFG)%_cfg.h
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling with MSVC\033[0m the collection source file: $<..."
	@echo
	$(MSVC) /c $(MSVC_CFLAGS) /Fo$@ $< 2>&1 | tee $(PATH_BUILD)msvc_compile.log

$(PATH_OBJ_FILES)msvc_%.obj : $(PATH_SRC)%.c $(PATH_INC)%.h
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling with MSVC\033[0m the collection source file: $<..."
	@echo
	$(MSVC) /c $(MSVC_CFLAGS) /Fo$@ $< 2>&1 | tee $(PATH_BUILD)msvc_compile.log

$(PATH_OBJ_FILES)msvc_%.obj : $(PATH_SRC)%.c
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling with MSVC\033[0m the collection source file: $<..."
	@echo
	$(MSVC) /c $(MSVC_CFLAGS) /Fo$@ $< 2>&1 | tee $(PATH_BUILD)msvc_compile.log

$(MSVC_LIB_LIST_FILE): $(MSVC_LIB_FILE)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[33mDisassembly\033[0m of $< into $@..."
	@echo
	-dumpbin.exe /DISASM $< > $@ 2>/dev/null || echo "dumpbin not available for disassembly"

$(LIB_FILE): $(OBJ_FILES) $(BUILD_DIRS) 
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[32mConstructing\033[0m the static library: $@..."
	@echo
	ar rcs $@ $(OBJ_FILES)

######################## Test Rules ########################
_test: $(BUILD_DIRS) $(TEST_EXECUTABLES) $(LIB_FILE) $(TEST_LIST_FILE) $(RESULTS)
	@echo
	@echo -e "\033[36mAll tests completed!\033[0m"
	@echo

coverage: $(GCOV_FILES)
	@echo
	@echo -e "\033[36mCoverage data gathered.\033[0m"
	@echo

# Write the test results to a result .txt file
$(PATH_RESULTS)%.txt: $(PATH_BUILD)%.$(TARGET_EXTENSION) $(COLORIZE_UNITY_SCRIPT)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[35mExecuting\033[0m $<..."
	@echo
	-./$< 2>&1 | tee $@ | python $(COLORIZE_UNITY_SCRIPT)

$(PATH_BUILD)%.$(TARGET_EXTENSION): $(TEST_OBJ_FILES) $(LIB_FILE)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[32mLinking\033[0m $(TEST_OBJ_FILES), $(UNITY_LIB), and the collection static lib $(LIB_FILE) into an executable..."
	@echo
	$(CC) $(LDFLAGS) -o $@ $(TEST_OBJ_FILES) -l$(UNITY_LIB) -L$(PATH_BUILD) -l$(basename $(notdir $(LIB_FILE)))

$(PATH_OBJ_FILES)%.o: $(PATH_TEST_FILES)%.c $(COLORIZE_CPPCHECK_SCRIPT)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling\033[0m the test file: $<..."
	@echo
	$(CC) -c $(CFLAGS_TEST) $< -o $@
	@echo

$(PATH_OBJ_FILES)%.o: $(PATH_UNITY)%.c $(PATH_UNITY)%.h
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling\033[0m the unity file: $<..."
	@echo
	$(CC) -c $(CFLAGS_TEST) $< -o $@
	@echo

.PHONY: unity_static_analysis
unity_static_analysis: $(PATH_UNITY)unity.c $(COLORIZE_CPPCHECK_SCRIPT)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[33mRunning static analysis\033[0m on $<..."
	@echo
	cppcheck --template='{severity}: {file}:{line}: {message}' $< 2>&1 | tee $(PATH_BUILD)cppcheck.log | python $(COLORIZE_CPPCHECK_SCRIPT)

###################### Clang Test Rules ######################

# Clang test variables
CLANG_TEST_EXECUTABLES = $(patsubst %.c, $(PATH_BUILD)clang_%.$(TARGET_EXTENSION), $(notdir $(SRC_TEST_FILES)))
CLANG_TEST_LIST_FILE = $(patsubst %.$(TARGET_EXTENSION), $(PATH_BUILD)clang_%.lst, $(notdir $(CLANG_TEST_EXECUTABLES)))
CLANG_TEST_OBJ_FILES = $(patsubst %.c, $(PATH_OBJ_FILES)clang_%.o, $(notdir $(SRC_TEST_FILES)))
CLANG_UNITY_OBJ_FILES = $(patsubst %.c, $(PATH_OBJ_FILES)clang_%.o, $(notdir $(UNITY_SRC_FILES)))
CLANG_RESULTS = $(patsubst %.c, $(PATH_RESULTS)clang_%.txt, $(notdir $(SRC_TEST_FILES)))

clang-_test: $(BUILD_DIRS) $(CLANG_TEST_EXECUTABLES) $(CLANG_LIB_FILE) $(CLANG_TEST_LIST_FILE) $(CLANG_RESULTS)
	@echo
	@echo -e "\033[36mAll Clang tests completed!\033[0m"
	@echo

# Write the clang test results to a result .txt file
$(PATH_RESULTS)clang_%.txt: $(PATH_BUILD)clang_%.$(TARGET_EXTENSION) $(COLORIZE_UNITY_SCRIPT)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[35mExecuting Clang\033[0m $<..."
	@echo
	-./$< 2>&1 | tee $@ | python $(COLORIZE_UNITY_SCRIPT)

$(PATH_BUILD)clang_%.$(TARGET_EXTENSION): $(CLANG_TEST_OBJ_FILES) $(CLANG_UNITY_OBJ_FILES) $(CLANG_LIB_FILE)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[32mLinking with Clang\033[0m $(CLANG_TEST_OBJ_FILES), $(UNITY_LIB), and the collection static lib $(CLANG_LIB_FILE) into an executable..."
	@echo
	$(CLANG) -o $@ $(CLANG_TEST_OBJ_FILES) $(CLANG_UNITY_OBJ_FILES) $(CLANG_LIB_FILE)

$(PATH_OBJ_FILES)clang_test_%.o: $(PATH_TEST_FILES)test_%.c
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling with Clang\033[0m the test file: $<..."
	@echo
	$(CLANG) -c $(CLANG_CFLAGS_TEST) $< -o $@
	@echo

$(PATH_OBJ_FILES)clang_unity.o: $(PATH_UNITY)unity.c $(PATH_UNITY)unity.h
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling with Clang\033[0m the unity file: $<..."
	@echo
	$(CLANG) -c $(CLANG_CFLAGS_TEST) $< -o $@
	@echo

$(PATH_OBJ_FILES)clang_unity_memory.o: $(PATH_UNITY)unity_memory.c $(PATH_UNITY)unity_memory.h
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling with Clang\033[0m the unity memory file: $<..."
	@echo
	$(CLANG) -c $(CLANG_CFLAGS_TEST) $< -o $@
	@echo

####################### MSVC Test Rules #######################

# MSVC test variables
MSVC_TEST_EXECUTABLES = $(patsubst %.c, $(PATH_BUILD)msvc_%.exe, $(notdir $(SRC_TEST_FILES)))
MSVC_TEST_LIST_FILE = $(patsubst %.exe, $(PATH_BUILD)msvc_%.lst, $(notdir $(MSVC_TEST_EXECUTABLES)))
MSVC_TEST_OBJ_FILES = $(patsubst %.c, $(PATH_OBJ_FILES)msvc_%.obj, $(notdir $(SRC_TEST_FILES)))
MSVC_UNITY_OBJ_FILES = $(patsubst %.c, $(PATH_OBJ_FILES)msvc_%.obj, $(notdir $(UNITY_SRC_FILES)))
MSVC_RESULTS = $(patsubst %.c, $(PATH_RESULTS)msvc_%.txt, $(notdir $(SRC_TEST_FILES)))

msvc-_test: $(BUILD_DIRS) $(MSVC_TEST_EXECUTABLES) $(MSVC_LIB_FILE) $(MSVC_TEST_LIST_FILE) $(MSVC_RESULTS)
	@echo
	@echo -e "\033[36mAll MSVC tests completed!\033[0m"
	@echo

# Write the MSVC test results to a result .txt file
$(PATH_RESULTS)msvc_%.txt: $(PATH_BUILD)msvc_%.exe $(COLORIZE_UNITY_SCRIPT)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[35mExecuting MSVC\033[0m $<..."
	@echo
	-./$< 2>&1 | tee $@ | python $(COLORIZE_UNITY_SCRIPT)

$(PATH_BUILD)msvc_%.exe: $(MSVC_TEST_OBJ_FILES) $(MSVC_UNITY_OBJ_FILES) $(MSVC_LIB_FILE)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[32mLinking with MSVC\033[0m $(MSVC_TEST_OBJ_FILES), Unity, and the collection static lib $(MSVC_LIB_FILE) into an executable..."
	@echo
	link.exe /OUT:$@ $(MSVC_TEST_OBJ_FILES) $(MSVC_UNITY_OBJ_FILES) $(MSVC_LIB_FILE) 2>&1 | tee $(PATH_BUILD)msvc_link.log

$(PATH_OBJ_FILES)msvc_test_%.obj: $(PATH_TEST_FILES)test_%.c
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling with MSVC\033[0m the test file: $<..."
	@echo
	$(MSVC) /c $(MSVC_CFLAGS_TEST) /Fo$@ $< 2>&1 | tee $(PATH_BUILD)msvc_test_compile.log

$(PATH_OBJ_FILES)msvc_unity.obj: $(PATH_UNITY)unity.c $(PATH_UNITY)unity.h
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling with MSVC\033[0m the unity file: $<..."
	@echo
	$(MSVC) /c $(MSVC_CFLAGS_TEST) /Fo$@ $(PATH_UNITY)unity.c 2>&1 | tee $(PATH_BUILD)msvc_unity_compile.log

$(PATH_OBJ_FILES)msvc_unity_memory.obj: $(PATH_UNITY)unity_memory.c $(PATH_UNITY)unity_memory.h
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling with MSVC\033[0m the unity memory file: $<..."
	@echo
	$(MSVC) /c $(MSVC_CFLAGS_TEST) /Fo$@ $(PATH_UNITY)unity_memory.c 2>&1 | tee $(PATH_BUILD)msvc_unity_memory_compile.log

######################### Generic ##########################

# Compile the collection source file into an object file
$(PATH_OBJ_FILES)%.o : $(PATH_SRC)%.c $(PATH_INC)%.h $(PATH_CFG)%_cfg.h $(COLORIZE_CPPCHECK_SCRIPT)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling\033[0m the collection source file: $<..."
	@echo
	$(CC) -c $(CFLAGS) $< -o $@
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[33mRunning static analysis\033[0m on $<..."
	@echo
	cppcheck --template='{severity}: {file}:{line}: {message}' $< 2>&1 | tee $(PATH_BUILD)cppcheck.log | python $(COLORIZE_CPPCHECK_SCRIPT)

$(PATH_OBJ_FILES)%.o : $(PATH_SRC)%.c $(PATH_INC)%.h
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling\033[0m the collection source file: $<..."
	@echo
	$(CC) -c $(CFLAGS) $< -o $@
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[33mRunning static analysis\033[0m on $<..."
	@echo
	cppcheck --template='{severity}: {file}:{line}: {message}' $< 2>&1 | tee $(PATH_BUILD)cppcheck.log | python $(COLORIZE_CPPCHECK_SCRIPT)

$(PATH_OBJ_FILES)%.o : $(PATH_SRC)%.c
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mCompiling\033[0m the collection source file: $<..."
	@echo
	$(CC) -c $(CFLAGS) $< -o $@
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[33mRunning static analysis\033[0m on $<..."
	@echo
	cppcheck --template='{severity}: {file}:{line}: {message}' $< 2>&1 | tee $(PATH_BUILD)cppcheck.log | python $(COLORIZE_CPPCHECK_SCRIPT)

$(LIB_LIST_FILE): $(LIB_FILE)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[33mDisassembly\033[0m of $< into $@..."
	@echo
	objdump -D $< > $@

# Produces an object dump that includes the disassembly of the executable
$(PATH_BUILD)%.lst: $(PATH_BUILD)%.$(TARGET_EXTENSION)
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[33mDisassembly\033[0m of $< into $@..."
	@echo
	objdump -D $< > $@

# NOTE:
# gcov seems very picky about how the directory to look for .gcno and .gcda
# files is specified. The string for the directory must utilize forward slashes
# '/', not back slashes '\', and must not end with a forward slash. Otherwise,
# gcov exists with a cryptic
# 		<obj_dir>/.gcno:cannot open notes file
# kind of error. Hence, I use $(<path>:%/=%) /w PATH_OBJ_FILES.
#
# Also, I've redirected gcov's output because I want to prioritize viewing the
# unit test results. Coverage results are meant to be inspected manually rather
# than fed back immediately to the developer.

$(PATH_SRC)%.c.gcov: $(PATH_SRC)%.c
	@echo
	@echo "----------------------------------------"
	@echo -e "\033[36mAnalyzing coverage\033[0m for $<..."
	$(GCOV) $(GCOV_FLAGS) --object-directory $(PATH_OBJ_FILES:%/=%) $< > $(PATH_RESULTS)$(GCOV_CONSOLE_OUT_FILE)
	mv *.gcov $(PATH_RESULTS)
	gcovr $(GCOVR_FLAGS)
	@echo


# Make the directories if they don't already exist
$(PATH_RESULTS):
	$(MKDIR) $@

$(PATH_OBJ_FILES):
	$(MKDIR) $@

$(PATH_BUILD):
	$(MKDIR) $@

$(PATH_PROFILE):
	$(MKDIR) $@

# Clean rule to remove generated files
.PHONY: clean
clean:
	@echo
	$(CLEANUP) $(PATH_BUILD)$(REL_DIR)$(OBJ_DIR)*.o
	$(CLEANUP) $(PATH_BUILD)$(DBG_DIR)$(OBJ_DIR)*.o
	$(CLEANUP) $(PATH_BUILD)$(REL_DIR)$(OBJ_DIR)*.obj
	$(CLEANUP) $(PATH_BUILD)$(DBG_DIR)$(OBJ_DIR)*.obj
	$(CLEANUP) $(PATH_BUILD)$(REL_DIR)$(OBJ_DIR)*.gcda
	$(CLEANUP) $(PATH_BUILD)$(DBG_DIR)$(OBJ_DIR)*.gcno
	$(CLEANUP) $(PATH_BUILD)*.$(TARGET_EXTENSION)
	$(CLEANUP) $(PATH_BUILD)*.exe
	$(CLEANUP) $(PATH_BUILD)clang_*
	$(CLEANUP) $(PATH_BUILD)msvc_*
	$(CLEANUP) $(PATH_RESULTS)*.txt
	$(CLEANUP) $(PATH_BUILD)*.lst
	$(CLEANUP) $(PATH_BUILD)*.log
	$(CLEANUP) $(PATH_BUILD)*.$(STATIC_LIB_EXTENSION)
	$(CLEANUP) $(PATH_BUILD)libclang_*.$(STATIC_LIB_EXTENSION)
	$(CLEANUP) $(PATH_BUILD)libmsvc_*.$(STATIC_LIB_EXTENSION)
	$(CLEANUP) $(PATH_RESULTS)*.gcov
	$(CLEANUP) $(PATH_RESULTS)*.html
	$(CLEANUP) $(PATH_RESULTS)*.css
	$(CLEANUP) *.gcov
	@echo

.PRECIOUS: $(PATH_BUILD)%.$(TARGET_EXTENSION)
.PRECIOUS: $(PATH_BUILD)Test%.o
.PRECIOUS: $(PATH_RESULTS)%.txt
.PRECIOUS: $(PATH_RESULTS)%.lst
.PRECIOUS: *.gcov
