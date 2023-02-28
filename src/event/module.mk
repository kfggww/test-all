$(d)_srcs := $(wildcard $(d)/*.c)
$(d)_objs := $($(d)_srcs:%.c=$(BUILD_DIR)/%.o)
$(d)_target := $(BUILD_DIR)/$(d)/libevent.so

$($(d)_objs): CFLAGS += -fPIC

objs += $($(d)_objs)
shared_objs += $($(d)_target)
deps += $($(d)_objs:%.o=%.d)

install_src: libevent

libevent: $($(d)_target)

$($(d)_target): $($(d)_objs)
	$(CC) $(LDFLAGS) -shared $^ -o $@

.PHONY: libevent