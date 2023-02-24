
$(subproj)_srcs := $(wildcard $(subproj)/*.c)
$(subproj)_objs := $(addprefix $(BUILD_DIR)/, $(patsubst %.c, %.o, $($(subproj)_srcs)))
test_event := $(basename $(filter %test_event.o, $($(subproj)_objs)))

objs += $($(subproj)_objs)
obj_execs += $(test_event)

$(test_event): $($(subproj)_objs)
	$(Q)$(CC) $(LDFLAGS) $^ -o $@