LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_DEFINES += \
	PLATFORM_HAS_DYNAMIC_TIMER=1

MODULE_SRCS += \
	$(LOCAL_DIR)/x86_pit.c

include make/module.mk
