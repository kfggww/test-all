
all: timer_test_x86 timer_test_riscv

timer_test_x86: timer_test.c
	gcc $< -lrt -lpthread -o $@

timer_test_riscv: timer_test.c
	riscv64-linux-gnu-gcc -static $< -lrt -lpthread -o $@

clean:
	-rm timer_test_x86 timer_test_riscv

.PHONY: clean all