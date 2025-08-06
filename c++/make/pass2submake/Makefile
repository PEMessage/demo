# Top-level Makefile
# See: https://www.gnu.org/software/make/manual/html_node/Options_002fRecursion.html
# ‘-C’、‘-f’、‘-h’、‘-o’、‘-W’ will not be recusive pass to submake
#  -C dir, --directory=dir
#  -f file, --file=file, --makefile=FILE  Use file as a makefile.
$(warning -- MAKELEVEL is ===|$(MAKELEVEL)|===)
$(warning --  MAKEFLAGS is ===|$(MAKEFLAGS)|===)
$(warning --  MAKEGOALS is ===|$(MAKECMDGOALS)|===)
$(warning --  MAKEOVERRIDES is ===|$(MAKEOVERRIDES)|===)

# if we use make V=1, this will not work, similiar to cmake
# NOTICE: do not use inline command, to avoid extra space
V := 2
$(warning --  V is ===|$(V)|===)

ifeq ($(MAKELEVEL),0)
all:
	$(MAKE)
else ifeq ($(MAKELEVEL),1)
all:
	$(MAKE)
else ifeq ($(MAKELEVEL),2)
all:
	@echo [END] Recursive make end at level $(MAKELEVEL)
endif

# > make V=1 -d | grep ===
# Makefile:6: -- MAKELEVEL is ===|0|===
# Makefile:7: --  MAKEFLAGS is ===|d|===
# Makefile:8: --  MAKEGOALS is ===||===
# Makefile:9: --  MAKEOVERRIDES is ===|V=1|===
# Makefile:14: --  V is ===|1|===
# Makefile:6: -- MAKELEVEL is ===|1|===
# Makefile:7: --  MAKEFLAGS is ===|dw|===
# Makefile:8: --  MAKEGOALS is ===||===
# Makefile:9: --  MAKEOVERRIDES is ===|V=1|===
# Makefile:14: --  V is ===|1|===
# Makefile:6: -- MAKELEVEL is ===|2|===
# Makefile:7: --  MAKEFLAGS is ===|dw|===
# Makefile:8: --  MAKEGOALS is ===||===
# Makefile:9: --  MAKEOVERRIDES is ===|V=1|===
# Makefile:14: --  V is ===|1|===

