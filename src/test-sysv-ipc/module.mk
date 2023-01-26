
$(subproj)_srcs := $(wildcard $(subproj)/*.c)
$(subproj)_objs := $(addprefix $(BUILD_DIR)/, $(patsubst %.c, %, $($(subproj)_srcs)))

obj_execs += $($(subproj)_objs)