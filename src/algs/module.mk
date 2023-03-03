$(d)_srcs := $(wildcard $(d)/*.c)
$(d)_objs := $($(d)_srcs:%.c=$(BUILD_DIR)/%.o)
$(d)_target := $(BUILD_DIR)/$(d)/libalgs.so

$($(d)_objs): CFLAGS += -fPIC

objs += $($(d)_objs)
shared_objs += $($(d)_target)
interface_hdrs += $(d)/sds.h
deps += $($(d)_objs:%.o=%.d)

install_src: libalgs

libalgs: $($(d)_target)

$($(d)_target): $($(d)_objs)
	$(CC) -shared $^ -o $@

.PHONY: libalgs