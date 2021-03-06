# Find the local dir of the make file
GET_LOCAL_DIR    = $(patsubst %/,%,$(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))

# makes sure the target dir exists
MKDIR = if [ ! -d $(dir $@) ]; then mkdir -p $(dir $@); fi

# prepends the BUILD_DIR var to each item in the list
TOBUILDDIR = $(addprefix $(BUILDDIR)/,$(1))

# converts specified variable to boolean value
TOBOOL = $(if $(filter-out 0 false,$1),true,false)

COMMA := ,
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)

define NEWLINE


endef

STRIP_TRAILING_COMMA = $(if $(1),$(subst $(COMMA)END_OF_LIST_MARKER_FOR_STRIP_TRAILING_COMMA,,$(strip $(1))END_OF_LIST_MARKER_FOR_STRIP_TRAILING_COMMA))

# test if two files are different, replacing the first
# with the second if so
# args: $1 - temporary file to test
#       $2 - file to replace
define TESTANDREPLACEFILE
	if [ -f "$2" ]; then \
		if cmp "$1" "$2"; then \
			rm -f $1; \
		else \
			mv $1 $2; \
		fi \
	else \
		mv $1 $2; \
	fi
endef

# generate a header file at $1 with an expanded variable in $2
define MAKECONFIGHEADER
	$(MKDIR); \
	echo generating $1; \
	rm -f $1.tmp; \
	LDEF=`echo $1 | tr '/\\.-' '_' | sed "s/C++/CPP/g;s/c++/cpp/g"`; \
	echo \#ifndef __$${LDEF}_H > $1.tmp; \
	echo \#define __$${LDEF}_H >> $1.tmp; \
	for d in `echo $($2) | tr '[:lower:]' '[:upper:]'`; do \
		echo "#define $$d" | sed "s/=/\ /g;s/-/_/g;s/\//_/g;s/\./_/g;s/\//_/g;s/C++/CPP/g" >> $1.tmp; \
	done; \
	echo \#endif >> $1.tmp; \
	$(call TESTANDREPLACEFILE,$1.tmp,$1)
endef

# Map LK's arch names into a more common form.
define standard_name_for_arch
ifeq ($(2),arm)
$(1) := arm
else
ifeq ($(2),arm64)
$(1) := aarch64
else
ifeq ($(2),x86)
ifeq ($(3),x86-64)
$(1) := x86_64
else
ifeq ($(3),x86-32)
$(1) := i386
else
$$(error "unknown arch: $(2) / $(3)")
endif
endif
else
$$(error "unknown arch: $(2) / $(3)")
endif
endif
endif
endef
