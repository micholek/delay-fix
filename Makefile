OUTPUT_DIR = output
DEPS_DIR = deps

COMMON_OPTIONS = /nologo /EHsc /std:c++latest /Zi /W4 /Fd$(OUTPUT_DIR)/ /Fo$(OUTPUT_DIR)/
TEST_INCLUDE_OPTIONS = $(addprefix /I$(DEPS_DIR)/include/, boost-1_84 detours)
TEST_LIB_OPTIONS = /libpath:$(DEPS_DIR)/lib

APP_TARGET = $(OUTPUT_DIR)/main.exe
TEST_TARGET = $(OUTPUT_DIR)/test.exe

.PHONY: run
run: build
	$(APP_TARGET)

.PHONY: build
build:
	cl main.cpp reg.cpp $(COMMON_OPTIONS) /Fe$(OUTPUT_DIR)/main.exe /link advapi32.lib

.PHONY: test
test:
	cl test.cpp reg.cpp $(COMMON_OPTIONS) /MD /Fe$(OUTPUT_DIR)/test.exe $(TEST_INCLUDE_OPTIONS) /link $(TEST_LIB_OPTIONS) /subsystem:console advapi32.lib detours.lib
	$(TEST_TARGET) -l unit_scope

.PHONY: clean
clean:
	del /q $(OUTPUT_DIR)
