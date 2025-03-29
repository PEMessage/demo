# ====================================================
# Helper function
# ====================================================
MAKEFLAGS += --no-builtin-rules 

# ====================================================
# Debug subcmds handler
# ====================================================
# Check if 'debug' is in MAKECMDGOALS, 
ifneq (,$(filter debug,$(MAKECMDGOALS)))
  BUILD_MODE := debug
  # Remove 'debug' from arguments
  TEMP := $(filter-out debug,$(MAKECMDGOALS))
  # Add all target with `debug-prefix`, 
  DEBUG_GOALS := $(addprefix debug-,$(TEMP))
else ifneq (,$(filter run,$(MAKECMDGOALS)))
  $(warning $(MAKECMDGOALS))
  TEMP := $(filter-out run,$(MAKECMDGOALS))
  RUN_GOALS := $(addprefix run-,$(TEMP))
else
  # passthrough
endif

# ====================================================
# Configuration check
# ====================================================
OUT := out

MODULES := $(wildcard src/*)


help: 
	@echo avaiable target: $(MODULES)
debug: $(DEBUG_GOALS)
	@echo avaiable target: $(DEBUG_GOALS)
run: $(RUN_GOALS)
	@echo avaiable target: $(RUN_GOALS)
.PHONY: help debug run


define build_module
# Why: var-$(1)
# A:
# 	var-(1)而不能直接写
# 	PRIVATE :=(notdir $(1))
# 	是因为， 交给shell执行的这一部分，其中的variable 为惰性求值， 是基于运行时的变量。
var-$(1):
	$$(eval PRIVATE_M := $(notdir $(1)))
	$$(eval PRIVATE_BIN := $(OUT)/$$(PRIVATE_M)/src/$$(PRIVATE_M)/$$(PRIVATE_M).bin)

$(1): var-$(1)
	@echo "=================================="
	@echo "build module \"$$(PRIVATE_M)\""
	@echo "=================================="
	mkdir -p $(OUT)/$$(PRIVATE_M)
	cmake -B $(OUT)/$$(PRIVATE_M)
	cmake --build $(OUT)/$$(PRIVATE_M)

# Why: $$
# A:
# 	call 时候展开时，并不单单展开$1, 而是所有$表达式都展开，
# 	此时PRIVATE_M的赋值语句还没有被处理，所以PRIVATE_M还是空的
run-$(1): $(1)
	qemu-system-arm \
		-machine mps2-an385 \
		-cpu cortex-m3 \
		-kernel $$(PRIVATE_BIN) \
		-monitor none \
		-nographic -serial stdio

debug-$(1): $(1)
	qemu-system-arm \
		-machine mps2-an385 \
		-cpu cortex-m3 \
		-kernel $$(PRIVATE_BIN) \
		-monitor none \
		-nographic -serial stdio \
		-s -S
$()

endef

# instancize all module under src/*
$(foreach MODULE, \
	$(MODULES),\
	$(eval $(call build_module,$(MODULE))) \
)
	# $(info $(call build_module,$(MODULE))) \

.PHONY: $(MODULES)
