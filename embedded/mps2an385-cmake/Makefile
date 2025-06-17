# ====================================================
# Basic 
# ====================================================
MAKEFLAGS += --no-builtin-rules 
# main purpose of this makefile is to call cmake,
# so let cmake to build parrelled is enougth
MAKEFLAGS += -j1

OUT := out
MODULES := $(wildcard src/*)

help: 
	@echo avaiable target: $(MODULES)

# ====================================================
# Handler subcmds
# ====================================================


define define_subcmd
  # Set BUILD_MODE if needed (optional)
  # Remove subcommand from arguments
  TEMP := $$(filter-out $(1),$$(MAKECMDGOALS))
  # Add prefix to all targets
  $(1)_GOALS := $$(addprefix $(1)-,$$(TEMP))
  
  # Create the subcommand target
$(1): $$($(1)_GOALS)
	@true # interface for $(1)-* targets
.PHONY: $(1)
endef

define define_subcmds
ifeq (dummy,)
  # pass
$(foreach cmd,$(1),
else ifneq (,$$(filter $(cmd),$$(MAKECMDGOALS)))
$(call define_subcmd,$(cmd)))
else
  # passthough
endif
endef

$(eval $(call define_subcmds,clean run debug))




# ====================================================
# module templete
# ====================================================

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
endef

define build_module_v2
# 在理解上面的v1的原理后，只需要确保call展开时，PRIVATE_M 有赋值就可以了
# 那么我们通过eval，就可以确保PRIVATE_M 在赋值时展开
$(eval PRIVATE_M := $(notdir $(1)))
$(eval PRIVATE_OUT := $(OUT)/$(PRIVATE_M))
$(eval PRIVATE_BIN := $(PRIVATE_OUT)/src/$(PRIVATE_M)/$(PRIVATE_M).bin)
$(eval PRIVATE_ELF := $(PRIVATE_OUT)/src/$(PRIVATE_M)/$(PRIVATE_M))

$(1):
	@echo "=================================="
	@echo "build module \"$(PRIVATE_M)\""
	@echo "=================================="
	mkdir -p $(OUT)/$(PRIVATE_M)
	cmake -B $(OUT)/$(PRIVATE_M) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
	cmake --build $(OUT)/$(PRIVATE_M) -- $(PRIVATE_M) -j4 && \
		rm -rf compile_commands.json && \
		ln -s $(PRIVATE_OUT)/compile_commands.json compile_commands.json

run-$(1):
	qemu-system-arm \
		-machine mps2-an385 \
		-cpu cortex-m3 \
		-kernel $(PRIVATE_BIN) \
		-serial stdio

debug-$(1):
	@echo '======================='
	@echo 'gdb-multiarch -x "target remote :1234" $(PRIVATE_ELF)'
	@echo '======================='
	qemu-system-arm \
		-machine mps2-an385 \
		-cpu cortex-m3 \
		-kernel $(PRIVATE_BIN) \
		-d int,cpu_reset \
		-nographic -serial stdio \
		-s -S
clean-$(1):
	cmake --build $(OUT)/$(PRIVATE_M) -- clean
endef

# ====================================================
# Instance all module under src/*
# ====================================================
$(foreach MODULE, \
	$(MODULES),\
	$(eval $(call build_module_v2,$(MODULE))) \
)

.PHONY: $(MODULES)
