############################################################################
#
# Makefile for ytnova
#
############################################################################

# -------------------------------------------------------------------------
# Version Information
# -------------------------------------------------------------------------
VERSION     = 1.0.0-beta
VERSIONDATE = September 2026

# -------------------------------------------------------------------------
# Directory Configuration
# -------------------------------------------------------------------------
SRC_DIR     = src
INC_DIR     = include
OBJ_DIR     = obj
DOC_DIR     = docs
BUILD_DIR   = build
BIN_DIR     = .

# -------------------------------------------------------------------------
# Toolchain & Utilities
# -------------------------------------------------------------------------
CC          ?= cc
FUZZ_CC     ?= clang
MAKE_CMD    ?= $(MAKE)

# -------------------------------------------------------------------------
# Install Destinations
# -------------------------------------------------------------------------
PREFIX      ?= /usr/local
DESTDIR     ?=
BINDIR      = $(PREFIX)/bin
MANDIR      = $(PREFIX)/share/man
MAN1DIR     = $(MANDIR)/man1
DATADIR     = $(PREFIX)/share
YTNOVA_DATADIR = $(DATADIR)/ytnova

# For compatibility with old variable names
BINDEST     = $(DESTDIR)$(BINDIR)
MANDEST     = $(DESTDIR)$(MAN1DIR)
DATADEST    = $(DESTDIR)$(YTNOVA_DATADIR)

# -------------------------------------------------------------------------
# Compile Options
# -------------------------------------------------------------------------

# Optional extra compiler flags (for local experiments/debugging).
ADD_CFLAGS  =

# Default configuration for Linux (WSL/Ubuntu) with ncurses 6
# NOTE: This build now requires libarchive.
# On Debian/Ubuntu, install with: sudo apt-get install libarchive-dev
COLOR       = -DCOLOR_SUPPORT
CLOCK       = -DCLOCK_SUPPORT
READLINE    = -DREADLINE_SUPPORT

# Compiler Warnings (Scrupulous Mode)
# -Wall -Wextra: Enable most warnings
# -Wno-unused-parameter: Reduce noise from legacy callback signatures
# Note: -Werror=conflicting-types removed for compatibility with older compilers
WARNINGS    = -Wall -Wextra -Wno-unused-parameter

# Standard Flags
# External CPPFLAGS/CFLAGS/LDFLAGS/LDLIBS are packager-owned and are
# intentionally kept separate from project-required build flags.
# -I$(INC_DIR): Look for headers in the include/ directory
# -MMD -MP:     Auto-generate dependency files (.d) to track header changes
# -DVERSION, -DVERSIONDATE: Version info from Makefile variables
PROJECT_CPPFLAGS = -D_GNU_SOURCE -DHAVE_LIBARCHIVE -DWITH_UTF8 \
                   -DVERSION='"$(VERSION)"' -DVERSIONDATE='"$(VERSIONDATE)"' \
                   -DPACKAGED_LOCALE_DIR='"$(DATADIR)/locale"' \
                   -DPACKAGED_COMMANDS_PATH='"$(YTNOVA_DATADIR)/ytnova.commands"' \
                   -DPACKAGED_COMMAND_PRESET_DIR='"$(YTNOVA_DATADIR)/commands"' \
                   -DPACKAGED_APPLICATIONS_PATH='"$(YTNOVA_DATADIR)/ytnova.applications"' \
                   -DPACKAGED_THEME_PATH='"$(YTNOVA_DATADIR)/ytnova.themes"' \
                   $(COLOR) $(CLOCK) $(READLINE) \
                   -I$(INC_DIR) -MMD -MP
PROJECT_CFLAGS   = $(WARNINGS) $(ADD_CFLAGS)
PROJECT_LDFLAGS  =
PROJECT_LDLIBS   = -lncursesw -ltinfo -lreadline -larchive -lm
PROJECT_OPTFLAGS ?= -O2
THEME_CATALOG_SRC = etc/ytnova.themes
THEME_CATALOG_HDR = src/core/default_theme_catalog.h
THEME_CATALOG_SCRIPT = scripts/generate_theme_catalog.py
PROFILE_TEMPLATE_SRC = etc/ytnova.conf
PROFILE_TEMPLATE_HDR = src/core/default_profile_template.h
PROFILE_TEMPLATE_SCRIPT = scripts/generate_default_profile_template.py
COMMANDS_CATALOG_SRC = etc/ytnova.commands
COMMANDS_CATALOG_HDR = src/core/default_commands_catalog.h
COMMANDS_CATALOG_SCRIPT = scripts/generate_default_commands_catalog.py
APPLICATIONS_CATALOG_SRC = etc/ytnova.applications
APPLICATIONS_CATALOG_HDR = src/core/default_applications_catalog.h
APPLICATIONS_CATALOG_SCRIPT = scripts/generate_default_applications_catalog.py
COMMAND_PRESETS_SRC_DIR = etc/commands
COMMAND_PRESETS_HDR = src/core/default_command_presets_catalog.h
COMMAND_PRESETS_SCRIPT = scripts/generate_default_command_presets_catalog.py
HELP_F1_SOURCE = etc/help/f1.en.md
HELP_F1_LOCALE_SOURCES = $(wildcard etc/help/f1.*.md)
HELP_MAN_SOURCE = etc/help/man.en.md
HELP_MAN_MD = etc/ytnova.1.md
HELP_USAGE_MD = docs/USAGE.md
HELP_RUNTIME_HDR = src/core/generated_help_topics.h
HELP_GENERATOR_SCRIPT = scripts/generate_help_assets.py
GETTEXT_COMPILE_SCRIPT = scripts/compile_mo.py
GETTEXT_DOMAIN = ytnova
GETTEXT_POT = po/$(GETTEXT_DOMAIN).pot
GETTEXT_PO_FILES = $(wildcard po/*.po)
GETTEXT_MO_FILES = $(patsubst po/%.po,$(BUILD_DIR)/locale/%/LC_MESSAGES/$(GETTEXT_DOMAIN).mo,$(GETTEXT_PO_FILES))
CODE_QUALITY_HOTSPOT_SCRIPT = scripts/report_code_quality_hotspots.py

# Coverage build switch (for gcov/lcov-driven C coverage reports).
COVERAGE    ?= 0
ifeq ($(COVERAGE),1)
    PROJECT_CFLAGS  += --coverage
    PROJECT_LDFLAGS += --coverage
endif

# Sanitizer build switch (for dedicated ASan/UBSan QA runs).
SANITIZE    ?= 0
ifeq ($(SANITIZE),1)
    PROJECT_CFLAGS  += -fsanitize=address,undefined -fno-omit-frame-pointer -g -O1
    PROJECT_LDFLAGS += -fsanitize=address,undefined
endif

# -------------------------------------------------------------------------
# Build Mode Selection
# Run 'make DEBUG=1' for development (AddressSanitizer enabled)
# Run 'make' for release (Optimized, no runtime dependency on ASan)
# -------------------------------------------------------------------------
ifeq ($(DEBUG),1)
    # Debug Build: Enable ASan, Debug Symbols, disable optimization
    PROJECT_CFLAGS  += -fsanitize=address -g -O1 -fno-omit-frame-pointer
    PROJECT_LDFLAGS += -fsanitize=address
else
    # Release Build: Standard Optimization when no external CFLAGS were supplied.
    # Keep sanitizer builds at -O1 for better diagnostics and deterministic PTY timing.
    ifeq ($(SANITIZE),0)
        ifeq ($(origin CFLAGS),undefined)
            PROJECT_CFLAGS += $(PROJECT_OPTFLAGS)
        endif
    endif
endif

# -------------------------------------------------------------------------
# Files
# -------------------------------------------------------------------------
MAIN        = ytnova
MAIN_BIN    = $(BUILD_DIR)/$(MAIN)
MANSRC      = $(HELP_MAN_MD)
MANPAGE     = $(BUILD_DIR)/ytnova.1
MAN_TH      = .TH "YTNOVA" "1" "$(VERSIONDATE)" "ytnova $(VERSION)" "User Commands"

# Automatically find all .c files in src/ and subdirectories
SRCS        = $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*/*.c)
# Generate object filenames in obj/
OBJS        = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
# Dependency files generated by compiler
DEPS        = $(OBJS:.o=.d)
CLANG_TIDY_SRCS = $(shell find $(SRC_DIR) -name '*.c' -type f)

# -------------------------------------------------------------------------
# QA Defaults
# -------------------------------------------------------------------------
# Set QA_ON_BUILD=1 to run full QA (including pytest) after a normal build:
#   make QA_ON_BUILD=1
QA_ON_BUILD ?= 0
QA_LOG ?= qa-all.log
GCOV ?= gcov
LCOV ?= lcov
LCOV_INFO ?= coverage/lcov.info
LCOV_SUMMARY ?= coverage/summary.txt
FUZZ_BUILD_DIR ?= $(BUILD_DIR)/fuzz
FUZZ_ARTIFACT_DIR ?= $(FUZZ_BUILD_DIR)/artifacts
FUZZ_RUNS ?= 2000
FUZZ_SANITIZERS ?= fuzzer,address,undefined
SANITIZE_PYTEST_TIME_SCALE ?= 2.5
FUZZ_COMMON_SRC := tests/fuzz/fuzz_common.c
FUZZ_COMMON_HDR := tests/fuzz/fuzz_common.h
FUZZ_CFLAGS ?= -std=c99 -D_GNU_SOURCE -I$(INC_DIR) -Itests/fuzz \
	-g -O1 -fno-omit-frame-pointer -Wall -Wextra -Wno-unused-parameter \
	-fsanitize=$(FUZZ_SANITIZERS)
FUZZ_LDFLAGS ?= -fsanitize=$(FUZZ_SANITIZERS)
FUZZ_STRING_UTILS_BIN := $(FUZZ_BUILD_DIR)/fuzz_string_utils
FUZZ_PATH_UTILS_BIN := $(FUZZ_BUILD_DIR)/fuzz_path_utils
FUZZ_FILTER_CORE_BIN := $(FUZZ_BUILD_DIR)/fuzz_filter_core
FUZZ_BINS := $(FUZZ_STRING_UTILS_BIN) $(FUZZ_PATH_UTILS_BIN) $(FUZZ_FILTER_CORE_BIN)

# -------------------------------------------------------------------------
# Rules
# -------------------------------------------------------------------------

.PHONY: all clean clobber install uninstall docs changelog-draft hooks-install hooks-status \
	git-aliases-install git-aliases-status test \
		fuzz fuzz-smoke fuzz-string-utils fuzz-path-utils fuzz-filter-core qa-fuzz \
		test-v qa-clang qa-cppcheck qa-scan qa-valgrind qa-valgrind-interactive qa-valgrind-full \
		qa-pytest qa-fileops-integrity qa-split-panel-gates qa-pytest-coverage qa-sanitize qa-unsafe-apis qa-dead-history-comments qa-compatibility-shims qa-module-boundaries qa-clean-code qa-appstate-contract qa-ai-config qa-theme-catalog qa-profile-template qa-commands-catalog qa-applications-catalog qa-command-presets-catalog qa-help-assets qa-code-quality qa-all \
		qa-test-contract-resilience qa-tracker-id-leaks \
		ci-baseline mcp-doctor py-requirements \
		qa-all-log qa-deep theme-catalog profile-template commands-catalog applications-catalog command-presets-catalog \
		help-assets locale-catalogs update-gettext-pot \
		code-quality-hotspots

all: $(MAIN_BIN) $(MANPAGE) $(if $(filter 1,$(QA_ON_BUILD)),qa-all)

# Link the executable
$(MAIN_BIN): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(PROJECT_CFLAGS) $(LDFLAGS) $(PROJECT_LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(PROJECT_LDLIBS)

# Compile source files into object files
# Ensure the specific subdirectory exists in obj/ before compiling
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(PROJECT_CPPFLAGS) $(CFLAGS) $(PROJECT_CFLAGS) -c $< -o $@

# Create the build directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Generate tracked help projections and the build manpage from the canonical help source.
help-assets: | $(BUILD_DIR)
	$(PYTHON) $(HELP_GENERATOR_SCRIPT) --f1-source $(HELP_F1_SOURCE) \
		$(foreach src,$(filter-out $(HELP_F1_SOURCE),$(HELP_F1_LOCALE_SOURCES)),--f1-locale-source $(src) ) \
		--man-source $(HELP_MAN_SOURCE) \
		--man-md $(HELP_MAN_MD) --usage-md $(HELP_USAGE_MD) \
		--runtime-header $(HELP_RUNTIME_HDR) --man-roff $(MANPAGE) \
		--version "$(VERSION)" --versiondate "$(VERSIONDATE)" --write

docs: help-assets

# Generate the roff man page
$(MANPAGE): $(HELP_F1_SOURCE) $(HELP_MAN_SOURCE) $(HELP_GENERATOR_SCRIPT) | $(BUILD_DIR)
	$(PYTHON) $(HELP_GENERATOR_SCRIPT) --f1-source $(HELP_F1_SOURCE) \
		$(foreach src,$(filter-out $(HELP_F1_SOURCE),$(HELP_F1_LOCALE_SOURCES)),--f1-locale-source $(src) ) \
		--man-source $(HELP_MAN_SOURCE) \
		--man-roff $@ --version "$(VERSION)" --versiondate "$(VERSIONDATE)" --write

locale-catalogs: $(GETTEXT_MO_FILES)

$(BUILD_DIR)/locale/%/LC_MESSAGES/$(GETTEXT_DOMAIN).mo: po/%.po $(GETTEXT_COMPILE_SCRIPT) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(PYTHON) $(GETTEXT_COMPILE_SCRIPT) $< $@

update-gettext-pot:
	xgettext --from-code=UTF-8 --language=C \
		--keyword=_ --keyword=N_ --keyword=P_:1c,2 --keyword=NP_:1c,2 \
		--keyword=UI_ReadString:3 --keyword=UI_ReadStringWithPromptOptions:3 \
		--keyword=UI_ReadStringWithHelp:3 --keyword=UI_ShowHelpPopup:2 \
		--keyword=UI_ShowHelpPopupWithFooter:2 \
		--keyword=UI_ShowHelpPopupDismissAnyKey:2 \
		--keyword=UI_ShowStatusLineError:2 --keyword=UI_ShowStatusLineNotice:2 \
		--keyword=UI_Message:2 --keyword=UI_Notice:2 --keyword=UI_Warning:2 \
		--keyword=UI_Error:4 \
		--output=$(GETTEXT_POT) $(SRCS) include/ytnova_i18n.h

# Install binary, man page, and documentation
install: $(MAIN_BIN) $(MANPAGE) docs locale-catalogs
	@$(MAKE_CMD) install-shadow-check
	@echo "Installing ytnova $(VERSION) to $(PREFIX)..."
	install -d -m 755 $(BINDEST)
	install -m 755 $(MAIN_BIN) $(BINDEST)/$(MAIN)
	install -d -m 755 $(MANDEST)
	gzip -9c $(MANPAGE) > $(MANPAGE).gz
	install -m 644 $(MANPAGE).gz $(MANDEST)/$(MAIN).1.gz
	rm -f $(MANPAGE).gz
	install -d -m 755 $(DATADEST)
	install -d -m 755 $(DESTDIR)$(DATADIR)/locale
	install -m 644 etc/ytnova.commands $(DATADEST)/ytnova.commands
	install -m 644 etc/ytnova.applications $(DATADEST)/ytnova.applications
	install -d -m 755 $(DATADEST)/commands
	install -m 644 $(COMMAND_PRESETS_SRC_DIR)/*.conf $(DATADEST)/commands/
	install -m 644 etc/ytnova.themes $(DATADEST)/ytnova.themes
	@for mo in $(GETTEXT_MO_FILES); do \
		lang=$$(echo "$$mo" | sed -E 's#$(BUILD_DIR)/locale/([^/]+)/LC_MESSAGES/$(GETTEXT_DOMAIN)\\.mo#\\1#'); \
		install -d -m 755 "$(DESTDIR)$(DATADIR)/locale/$$lang/LC_MESSAGES"; \
		install -m 644 "$$mo" "$(DESTDIR)$(DATADIR)/locale/$$lang/LC_MESSAGES/$(GETTEXT_DOMAIN).mo"; \
	done
	@echo "Installation complete."
	@echo "Binary: $(BINDEST)/$(MAIN)"
	@echo "Manual: $(MANDEST)/$(MAIN).1.gz"
	@echo "Commands: $(DATADEST)/ytnova.commands"
	@echo "Applications: $(DATADEST)/ytnova.applications"
	@echo "Command presets: $(DATADEST)/commands/*.conf"
	@echo "Themes: $(DATADEST)/ytnova.themes"

.PHONY: install-shadow-check
install-shadow-check:
	@if [ "$(ALLOW_SHADOW_INSTALL)" = "1" ]; then \
		echo "Skipping ytnova shadow-install guard (ALLOW_SHADOW_INSTALL=1)."; \
		exit 0; \
	fi; \
	install_home="$$HOME"; \
	if [ -n "$$SUDO_USER" ]; then \
		install_home="$$(getent passwd "$$SUDO_USER" | cut -d: -f6)"; \
	fi; \
	shadow_found=0; \
	shadow_bin="$$install_home/.local/bin/$(MAIN)"; \
	for shadow_man in \
		"$$install_home/.local/share/man/man1/$(MAIN).1" \
		"$$install_home/.local/share/man/man1/$(MAIN).1.gz" \
		"$$install_home/.local/man/man1/$(MAIN).1" \
		"$$install_home/.local/man/man1/$(MAIN).1.gz"; do \
		if [ -e "$$shadow_man" ]; then \
			if [ $$shadow_found -eq 0 ]; then \
				echo "Refusing ytnova install: shadow user-local ytnova artifacts exist outside $(PREFIX)."; \
			fi; \
			echo "  stale man: $$shadow_man"; \
			shadow_found=1; \
		fi; \
	done; \
	if [ -e "$$shadow_bin" ]; then \
		if [ $$shadow_found -eq 0 ]; then \
			echo "Refusing ytnova install: shadow user-local ytnova artifacts exist outside $(PREFIX)."; \
		fi; \
		echo "  stale binary: $$shadow_bin"; \
		shadow_found=1; \
	fi; \
	if [ $$shadow_found -ne 0 ]; then \
		echo "Remove the stale user-local copies or rerun with ALLOW_SHADOW_INSTALL=1 if you really intend to keep them."; \
		exit 1; \
	fi

# Uninstall all installed files
uninstall:
	@echo "Uninstalling ytnova from $(PREFIX)..."
	rm -f $(BINDEST)/$(MAIN)
	rm -f $(MANDEST)/$(MAIN).1.gz
	rm -f $(DATADEST)/ytnova.commands
	rm -f $(DATADEST)/ytnova.applications
	rm -f $(DATADEST)/commands/*.conf
	-rmdir $(DATADEST)/commands 2>/dev/null || true
	rm -f $(DATADEST)/ytnova.themes
	-rmdir $(DATADEST) 2>/dev/null || true
	-rmdir $(MANDEST) 2>/dev/null || true
	-rmdir $(MANDIR) 2>/dev/null || true
	-rmdir $(BINDEST) 2>/dev/null || true
	-rmdir $(PREFIX)/share 2>/dev/null || true
	-rmdir $(PREFIX) 2>/dev/null || true
	@echo "Uninstall complete."

# Draft Changelog generator
# If no tags exist, uses the full history (HEAD).
changelog-draft:
	@echo "### Added"
	@RANGE=$$(git describe --tags --abbrev=0 2>/dev/null); \
	if [ -z "$$RANGE" ]; then RANGE="HEAD"; else RANGE="$$RANGE..HEAD"; fi; \
	git log $$RANGE --grep="^feat" --pretty=format:"- %s"
	@echo "\n\n### Fixed"
	@RANGE=$$(git describe --tags --abbrev=0 2>/dev/null); \
	if [ -z "$$RANGE" ]; then RANGE="HEAD"; else RANGE="$$RANGE..HEAD"; fi; \
	git log $$RANGE --grep="^fix" --pretty=format:"- %s"
	@echo "\n\n### Other"
	@RANGE=$$(git describe --tags --abbrev=0 2>/dev/null); \
	if [ -z "$$RANGE" ]; then RANGE="HEAD"; else RANGE="$$RANGE..HEAD"; fi; \
	git log $$RANGE --grep="^refactor\|^chore" --pretty=format:"- %s"
	@echo ""

hooks-install:
	git config core.hooksPath .githooks
	$(MAKE_CMD) git-aliases-install
	chmod +x .githooks/pre-push
	@echo "Git hooks installed from .githooks/ (core.hooksPath=.githooks)."
	@echo "Git aliases installed: push-fast, push-fast-up"

hooks-status:
	@echo "core.hooksPath=$$(git config --get core.hooksPath || echo .git/hooks)"
	$(MAKE_CMD) git-aliases-status

git-aliases-install:
	git config --local alias.push-fast '!f(){ YTNOVA_PRE_PUSH_FAST=1 git push "$$@"; }; f'
	git config --local alias.push-fast-up '!f(){ branch=$$(git rev-parse --abbrev-ref HEAD); YTNOVA_PRE_PUSH_FAST=1 git push -u origin "$$branch" "$$@"; }; f'

git-aliases-status:
	@echo "alias.push-fast=$$(git config --local --get alias.push-fast || echo '<not set>')"
	@echo "alias.push-fast-up=$$(git config --local --get alias.push-fast-up || echo '<not set>')"

# Clean build artifacts
clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR)
	rm -f core *~ *.orig *.bak
	find $(SRC_DIR) -name "*.o" -o -name "*.d" -delete

clobber: clean
	rm -f $(MAIN) # Just in case legacy binary exists

mrproper: clobber
	rm -rf $(OBJ_DIR) $(BUILD_DIR)
	find . -name "*.o" -o -name "*.d" -delete

# Include automatically generated dependencies
-include $(DEPS)

# Test Targets
PYTHON ?= $(shell if [ -x .venv/bin/python ]; then echo ".venv/bin/python"; elif command -v python3 >/dev/null 2>&1; then echo "python3"; else echo "python"; fi)
PYTEST ?= $(shell if [ -x .venv/bin/pytest ]; then echo ".venv/bin/pytest"; else echo "pytest"; fi)

test: $(MAIN_BIN)
	$(PYTEST)

test-v: $(MAIN_BIN)
	$(PYTEST) -v -s

# Fuzz Targets
$(FUZZ_BUILD_DIR):
	mkdir -p $(FUZZ_BUILD_DIR)

$(FUZZ_STRING_UTILS_BIN): tests/fuzz/fuzz_string_utils.c $(FUZZ_COMMON_SRC) $(FUZZ_COMMON_HDR) src/util/string_utils.c | $(FUZZ_BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_CFLAGS) -o $@ tests/fuzz/fuzz_string_utils.c $(FUZZ_COMMON_SRC) src/util/string_utils.c $(FUZZ_LDFLAGS)

$(FUZZ_PATH_UTILS_BIN): tests/fuzz/fuzz_path_utils.c $(FUZZ_COMMON_SRC) $(FUZZ_COMMON_HDR) src/util/path_utils.c | $(FUZZ_BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_CFLAGS) -o $@ tests/fuzz/fuzz_path_utils.c $(FUZZ_COMMON_SRC) src/util/path_utils.c $(FUZZ_LDFLAGS)

$(FUZZ_FILTER_CORE_BIN): tests/fuzz/fuzz_filter_core.c $(FUZZ_COMMON_SRC) $(FUZZ_COMMON_HDR) src/fs/filter_core.c | $(FUZZ_BUILD_DIR)
	$(FUZZ_CC) $(FUZZ_CFLAGS) -o $@ tests/fuzz/fuzz_filter_core.c $(FUZZ_COMMON_SRC) src/fs/filter_core.c $(FUZZ_LDFLAGS)

fuzz-string-utils: $(FUZZ_STRING_UTILS_BIN)

fuzz-path-utils: $(FUZZ_PATH_UTILS_BIN)

fuzz-filter-core: $(FUZZ_FILTER_CORE_BIN)

fuzz: $(FUZZ_BINS)

fuzz-smoke: fuzz
	@mkdir -p "$(FUZZ_ARTIFACT_DIR)"
	$(FUZZ_STRING_UTILS_BIN) -runs=$(FUZZ_RUNS) -artifact_prefix=$(FUZZ_ARTIFACT_DIR)/string-utils-
	$(FUZZ_PATH_UTILS_BIN) -runs=$(FUZZ_RUNS) -artifact_prefix=$(FUZZ_ARTIFACT_DIR)/path-utils-
	$(FUZZ_FILTER_CORE_BIN) -runs=$(FUZZ_RUNS) -artifact_prefix=$(FUZZ_ARTIFACT_DIR)/filter-core-

qa-fuzz:
	@command -v $(FUZZ_CC) >/dev/null || { echo "$(FUZZ_CC) is required for qa-fuzz"; exit 1; }
	$(MAKE_CMD) fuzz-smoke

# QA Targets
qa-clang:
	$(MAKE_CMD) QA_ON_BUILD=0 clean
	bear -- $(MAKE_CMD) QA_ON_BUILD=0 all
	clang-tidy $(CLANG_TIDY_SRCS) -p .

qa-cppcheck:
	cppcheck --enable=all --inconclusive --force --std=c99 -I include --error-exitcode=1 --suppressions-list=.cppcheck-suppressions.txt src include

qa-scan:
	$(MAKE_CMD) QA_ON_BUILD=0 clean
	scan-build --status-bugs $(MAKE_CMD) QA_ON_BUILD=0 all

qa-valgrind:
	$(MAKE_CMD) QA_ON_BUILD=0 clean
	$(MAKE_CMD) QA_ON_BUILD=0 all
	@echo "Running non-interactive valgrind smoke check (output in valgrind.log)."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 --log-file=valgrind.log ./build/ytnova --version >/dev/null

qa-valgrind-interactive:
	$(MAKE_CMD) QA_ON_BUILD=0 clean
	$(MAKE_CMD) QA_ON_BUILD=0 all
	@echo "Interactive valgrind run: exit ytnova cleanly to finish."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --error-exitcode=1 --log-file=valgrind.log ./build/ytnova .

qa-valgrind-full:
	$(MAKE_CMD) DEBUG=0 QA_ON_BUILD=0 clean
	$(MAKE_CMD) DEBUG=0 QA_ON_BUILD=0 all
	@echo "Running automated interactive valgrind session (output in valgrind.log)."
	$(PYTHON) scripts/valgrind_session.py

# Full pytest regression gate (entire suite).
qa-pytest: $(MAIN_BIN)
	TERM=$${TERM:-xterm} $(PYTEST)

# Fast targeted integrity/security pytest gate for file/archive mutations.
qa-fileops-integrity: $(MAIN_BIN)
	TERM=$${TERM:-xterm} $(PYTEST) -q -ra --tb=no \
		tests/test_fileops_integrity.py \
		tests/test_security_shell_paths.py \
		tests/test_security_tempfiles.py \
		tests/test_core.py::test_tagged_copy_overwrite_all_applies_to_remaining_conflicts \
		tests/test_core.py::test_tagged_move_overwrite_all_applies_to_remaining_conflicts \
		tests/test_tagged_action_regressions.py::test_tagged_copy_prompt_cancel_preserves_tagged_state \
		tests/test_tagged_action_regressions.py::test_tagged_move_prompt_cancel_preserves_tagged_state \
		tests/test_archive_write_parity.py \
		tests/test_archive_ui.py::test_archive_create_overwrite_prompt_respects_no_then_yes \
		tests/test_archive_ui.py::test_archive_create_overwrite_excludes_destination_from_payload \
		tests/test_archive_ui.py::test_archive_create_exclusion_empty_payload_shows_status_and_aborts \
		tests/test_archive_ui.py::test_archive_create_inside_source_round_trip_integrity

# Focused split-panel regression gate: canonical runtime split matrix.
qa-split-panel-gates: $(MAIN_BIN)
	TERM=$${TERM:-xterm} $(PYTEST) -q -ra --tb=no \
		tests/test_dir_window_dispatch_regressions.py::test_dir_window_split_and_tab_keeps_file_focus \
		tests/test_dir_window_dispatch_regressions.py::test_split_tab_refresh_rejects_stale_file_restore_snapshot \
		tests/test_file_window_dispatch_regressions.py::test_split_and_tab_dispatch_keeps_file_mode_footer \
		tests/test_panel_isolation.py::test_split_from_file_keeps_file_focus_on_tab \
		tests/test_panel_isolation.py::test_split_tab_from_small_file_does_not_expand_inactive_panel \
		tests/test_panel_isolation.py::test_split_same_directory_file_tags_are_panel_local

qa-pytest-coverage:
	$(MAKE_CMD) DEBUG=0 COVERAGE=1 QA_ON_BUILD=0 clean
	$(MAKE_CMD) DEBUG=0 COVERAGE=1 QA_ON_BUILD=0 all
	@command -v $(GCOV) >/dev/null || { echo "gcov is required for qa-pytest-coverage"; exit 1; }
	@command -v $(LCOV) >/dev/null || { echo "lcov is required for qa-pytest-coverage"; exit 1; }
	@mkdir -p coverage
	@rm -f "$(LCOV_INFO)" "$(LCOV_SUMMARY)"
	TERM=$${TERM:-xterm} $(PYTEST) -q -ra --tb=no
	$(LCOV) --capture --directory "$(OBJ_DIR)" --output-file "$(LCOV_INFO)" --gcov-tool "$(GCOV)"
	$(LCOV) --remove "$(LCOV_INFO)" '/usr/*' --output-file "$(LCOV_INFO)"
	$(LCOV) --summary "$(LCOV_INFO)" | tee "$(LCOV_SUMMARY)"

qa-sanitize:
	$(MAKE_CMD) DEBUG=0 SANITIZE=1 QA_ON_BUILD=0 clean
	$(MAKE_CMD) DEBUG=0 SANITIZE=1 QA_ON_BUILD=0 all
	YTNOVA_TUI_TIME_SCALE=$(SANITIZE_PYTEST_TIME_SCALE) \
	ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 \
	UBSAN_OPTIONS=halt_on_error=1 \
	TERM=$${TERM:-xterm} $(PYTEST)

qa-unsafe-apis:
	python3 scripts/check_c_unsafe_apis.py

qa-gitleaks:
	gitleaks detect --source . --redact --exit-code 1

qa-module-boundaries:
	python3 scripts/check_module_boundaries.py

qa-clean-code: qa-module-boundaries
	python3 scripts/check_clean_code.py

qa-dead-history-comments:
	python3 scripts/check_dead_history_comments.py

qa-compatibility-shims:
	python3 scripts/check_compatibility_shims.py

qa-appstate-contract:
	python3 scripts/check_appstate_contract.py

qa-ai-config:
	python3 scripts/check_project_ai_config.py

code-quality-hotspots:
	$(PYTHON) $(CODE_QUALITY_HOTSPOT_SCRIPT) $(HOTSPOT_ARGS)

qa-theme-catalog:
	$(PYTHON) $(THEME_CATALOG_SCRIPT) --source $(THEME_CATALOG_SRC) \
		--header $(THEME_CATALOG_HDR) --check

theme-catalog:
	$(PYTHON) $(THEME_CATALOG_SCRIPT) --source $(THEME_CATALOG_SRC) \
		--header $(THEME_CATALOG_HDR) --write

qa-profile-template:
	$(PYTHON) $(PROFILE_TEMPLATE_SCRIPT) --source $(PROFILE_TEMPLATE_SRC) \
		--header $(PROFILE_TEMPLATE_HDR) --check

profile-template:
	$(PYTHON) $(PROFILE_TEMPLATE_SCRIPT) --source $(PROFILE_TEMPLATE_SRC) \
		--header $(PROFILE_TEMPLATE_HDR) --write

qa-commands-catalog:
	$(PYTHON) $(COMMANDS_CATALOG_SCRIPT) --source $(COMMANDS_CATALOG_SRC) \
		--header $(COMMANDS_CATALOG_HDR) --check

commands-catalog:
	$(PYTHON) $(COMMANDS_CATALOG_SCRIPT) --source $(COMMANDS_CATALOG_SRC) \
		--header $(COMMANDS_CATALOG_HDR) --write

qa-applications-catalog:
	$(PYTHON) $(APPLICATIONS_CATALOG_SCRIPT) --source $(APPLICATIONS_CATALOG_SRC) \
		--header $(APPLICATIONS_CATALOG_HDR) --check

applications-catalog:
	$(PYTHON) $(APPLICATIONS_CATALOG_SCRIPT) --source $(APPLICATIONS_CATALOG_SRC) \
		--header $(APPLICATIONS_CATALOG_HDR) --write

qa-command-presets-catalog:
	$(PYTHON) $(COMMAND_PRESETS_SCRIPT) --source-dir $(COMMAND_PRESETS_SRC_DIR) \
		--header $(COMMAND_PRESETS_HDR) --check

command-presets-catalog:
	$(PYTHON) $(COMMAND_PRESETS_SCRIPT) --source-dir $(COMMAND_PRESETS_SRC_DIR) \
		--header $(COMMAND_PRESETS_HDR) --write

qa-help-assets:
	$(PYTHON) $(HELP_GENERATOR_SCRIPT) --f1-source $(HELP_F1_SOURCE) \
		$(foreach src,$(filter-out $(HELP_F1_SOURCE),$(HELP_F1_LOCALE_SOURCES)),--f1-locale-source $(src) ) \
		--man-source $(HELP_MAN_SOURCE) \
		--man-md $(HELP_MAN_MD) --usage-md $(HELP_USAGE_MD) \
		--runtime-header $(HELP_RUNTIME_HDR) --check

qa-test-contract-resilience:
	$(PYTHON) scripts/check_test_contract_resilience.py

qa-tracker-id-leaks:
	$(PYTHON) scripts/check_tracker_id_leaks.py

qa-code-quality: qa-unsafe-apis qa-dead-history-comments qa-compatibility-shims qa-clean-code qa-appstate-contract qa-ai-config qa-theme-catalog qa-profile-template qa-commands-catalog qa-applications-catalog qa-command-presets-catalog qa-help-assets qa-test-contract-resilience qa-tracker-id-leaks

ci-baseline: qa-code-quality qa-fileops-integrity qa-pytest-coverage qa-fuzz

# Comprehensive local gate: static/runtime checks + full pytest once.
# Run qa-fileops-integrity explicitly when file/archive mutation flows are touched.
qa-all: qa-clang qa-cppcheck qa-scan qa-valgrind qa-pytest qa-code-quality qa-gitleaks qa-fuzz

qa-all-log:
	@mkdir -p "$(dir $(QA_LOG))"
	/bin/bash -o pipefail -c '$(MAKE_CMD) qa-all 2>&1 | tee "$(QA_LOG)"'

qa-deep:
	bash scripts/qa-deep.sh "$(CURDIR)"

ci-repair-loop:
	python3 scripts/ci_repair_loop.py $(ARGS)

ci-repair-start:
	python3 scripts/ci_repair_loop.py --detach $(ARGS)

ci-repair-status:
	@if [ -f .agent/handoffs/ci-repair.current.md ]; then cat .agent/handoffs/ci-repair.current.md; \
	else echo "No ci-repair status file yet."; fi

ci-repair-log:
	@if [ -f .agent/handoffs/ci-repair.current.log ]; then tail -n 80 .agent/handoffs/ci-repair.current.log; \
	else echo "No ci-repair log yet."; fi

mcp-doctor:
	python3 scripts/mcp_doctor.py $(if $(filter 1,$(FIX)),--fix,)

py-requirements:
	bash scripts/update_python_requirements.sh
