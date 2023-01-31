
$(subproj)_srcs := $(wildcard $(subproj)/*.c)
$(subproj)_objs := $(addprefix $(BUILD_DIR)/, $(patsubst %.c, %, $($(subproj)_srcs)))

obj_execs += $($(subproj)_objs)

$(BUILD_DIR)/$(subproj)/posix-mq-producer: LDFLAGS += -lrt
$(BUILD_DIR)/$(subproj)/posix-mq-consumer: LDFLAGS += -lrt

$(BUILD_DIR)/$(subproj)/posix-shm-producer: LDFLAGS += -lrt -lpthread
$(BUILD_DIR)/$(subproj)/posix-shm-consumer: LDFLAGS += -lrt -lpthread