
$(d)_srcs := $(wildcard $(d)/*.c)
$(d)_objs := $($(d)_srcs:%.c=$(BUILD_DIR)/%.o)
$(d)_deps := $($(d)_objs:%.o=%.d)
$(d)_tests := $($(d)_objs:%.o=%)

$($(d)_objs): CFLAGS += -I$(INSTALL_DIR)/include
$($(d)_tests): LDFLAGS += -levent -lcutest

install_tests: $($(d)_tests)

$($(d)_objs): third-party install_src

objs += $($(d)_objs)
deps += $($(d)_deps)
tests += $($(d)_tests)