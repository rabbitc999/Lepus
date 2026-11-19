# NO_LEPUS_* = 1 时加入对应宏；省略或 0 保持启用。直接 -D 定义按存在性裁剪。
LEPUS_TRIM_OPTIONS := NO_LEPUS_BIG_INT NO_LEPUS_BIG_FLOAT NO_LEPUS_DEFAULT_POOL NO_LEPUS_STD NO_LEPUS_STD_CAS NO_LEPUS_STD_MATH NO_LEPUS_STD_RANDOM NO_LEPUS_STD_REGEX NO_LEPUS_STD_SIGNAL NO_LEPUS_STD_JSON NO_LEPUS_STD_TOML NO_LEPUS_STD_BINARY NO_LEPUS_STD_LINALG NO_LEPUS_STD_PARSE NO_LEPUS_STD_PROCESS

override LEPUS_WITH_REFLECTION := 1
override RUNTIME_WITH_REFLECTION := 1
LEPUS_FEATURE_FLAGS = -DLEPUS_WITH_REFLECTION=$(LEPUS_WITH_REFLECTION) -DRUNTIME_WITH_REFLECTION=$(RUNTIME_WITH_REFLECTION)
LEPUS_TRIM_FLAGS = $(LEPUS_FEATURE_FLAGS) $(foreach option,$(LEPUS_TRIM_OPTIONS),$(if $(filter 1,$($(option))),-D$(option)))

PYTHON_NULL := $(if $(filter sh bash sh.exe bash.exe,$(notdir $(SHELL))),/dev/null,NUL)
PYTHON ?= $(or $(shell python3 -c "import sys; print('python3' if sys.version_info.major == 3 else '')" 2>$(PYTHON_NULL)), $(shell python -c "import sys; print('python' if sys.version_info.major == 3 else '')" 2>$(PYTHON_NULL)))
PYTHON := $(PYTHON)
