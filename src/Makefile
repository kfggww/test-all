SHELL := /bin/bash
CFLAGS := -fPIC

objs :=
shared_objs :=
deps :=

# libalgs
libalgs_srcs := $(addprefix src/algs/, sds.c)
libalgs_objs := $(libalgs_srcs:%.c=$(BUILD_DIR)/%.o)
libalgs_deps := $(libalgs_objs:%.o=%.d)
libalgs := $(BUILD_DIR)/src/algs/libalgs.so

objs += $(libalgs_objs)
shared_objs += $(libalgs)
deps += $(libalgs_deps)

# libevent
libevent_srcs := $(addprefix src/event/, epoll.c \
										 event.c)
libevent_objs := $(libevent_srcs:%.c=$(BUILD_DIR)/%.o)
libevent_deps := $(libevent_objs:%.o=%.d)
libevent := $(BUILD_DIR)/src/event/libevent.so

objs += $(libevent_objs)
shared_objs += $(libevent)
deps += $(libevent_deps)

# installed things
install_libraries := $(addprefix $(BUILD_DIR)/src/, algs/libalgs.so \
													event/libevent.so)
install_headers := algs/sds.h \
				   event/event.h

# targets
install: $(shared_objs)
	mkdir -p $(INSTALL_DIR)/{lib,include}
	cp $(install_libraries) $(INSTALL_DIR)/lib
	cp $(install_headers) $(INSTALL_DIR)/include

$(libalgs): $(libalgs_objs)
	$(CC) $^ -shared -o $@

$(libevent): $(libevent_objs)
	$(CC) $^ -shared -o $@

$(BUILD_DIR)/%.o: $(PROJECT_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) -c $< $(CFLAGS) -MM -MT $@ > $(@:%.o=%.d)
	$(CC) -c $< $(CFLAGS) -o $@

clean:
	rm -rf $(objs) $(shared_objs) $(deps)

.PHONY: install clean libalgs libevent
-include $(libalgs_deps) $(libevent_deps)