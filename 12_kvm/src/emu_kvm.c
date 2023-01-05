#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

unsigned char code[] = {0xba, 0xf8, 0x03, 0x00, 0xd8, 0x04,
                        0x61, 0xee, 0xb0, 0x0a, 0xee, 0xf4};

int main(int argc, char **argv) {

    // open /dev/kvm
    int kvmfd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (kvmfd < 0) {
        printf("Can not open /dev/kvm\n");
        return errno;
    }

    printf("kvmfd is: %d\n", kvmfd);

    // get kvm api verison
    int version = ioctl(kvmfd, KVM_GET_API_VERSION);
    printf("kvm api version: %d\n", version);

    // create vm
    int vmfd = ioctl(kvmfd, KVM_CREATE_VM, 0);
    printf("vm created: %d\n", vmfd);

    // memory settings
    void *page =
        mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    if (page == MAP_FAILED) {
        perror("mmap failed because: ");
        return errno;
    }
    memcpy(page, code, sizeof(code));

    struct kvm_userspace_memory_region mem = {.slot = 0,
                                              .flags = 0,
                                              .guest_phys_addr = 4096,
                                              .memory_size = 4096,
                                              .userspace_addr = (__u64)page};
    int ret = ioctl(vmfd, KVM_SET_USER_MEMORY_REGION, &mem);
    if (ret) {
        perror("failed to set memory because: ");
        return errno;
    }
    printf("set vm memory done\n");

    // create and config vcpu
    int cpufd = ioctl(vmfd, KVM_CREATE_VCPU, 0);
    if (cpufd == -1) {
        perror("create vcpu failed because: ");
        return errno;
    }
    printf("cpufd: %d\n", cpufd);

    int mmap_size = ioctl(kvmfd, KVM_GET_VCPU_MMAP_SIZE, NULL);
    printf("vcpu mmap size: %d\n", mmap_size);

    struct kvm_run *run = (struct kvm_run *)mmap(
        NULL, mmap_size, PROT_READ | PROT_WRITE, MAP_SHARED, cpufd, 0);
    if (run == MAP_FAILED) {
        perror("failed to mmap on vcpu because: ");
        return errno;
    }

    struct kvm_sregs sregs;
    ret = ioctl(cpufd, KVM_GET_SREGS, &sregs);
    if (ret) {
        perror("failed to get sregs: ");
        return errno;
    }
    sregs.cs.base = 0;
    sregs.cs.selector = 0;

    ret = ioctl(cpufd, KVM_SET_SREGS, &sregs);
    if (ret) {
        perror("failed to set sregs because: ");
        return errno;
    }

    struct kvm_regs regs = {.rip = 4096, .rax = 1, .rbx = 1, .rflags = 0x2};
    ret = ioctl(cpufd, KVM_SET_REGS, &regs);
    if (ret) {
        perror("failed to set regs because: ");
        return errno;
    }

    // run binary code in vm
    int flag = 1;
    while (flag) {
        ret = ioctl(cpufd, KVM_RUN, NULL);
        if (ret) {
            perror("run vcpu failed because: ");
            return errno;
        }

        switch (run->exit_reason) {
        case KVM_EXIT_IO:
            if (run->io.direction == KVM_EXIT_IO_OUT && run->io.size == 1 &&
                run->io.port == 0x3f8 && run->io.count == 1) {
                putchar(*(((char *)run) + run->io.data_offset));
            } else {
                printf("unhandled KVM_EXIT_IO\n");
            }
            break;
        case KVM_EXIT_HLT:
            printf("KVM_EXIT_HLT\n");
            flag = 0;
            break;
        default:
            break;
        }
    }

    close(cpufd);
    close(vmfd);
    close(kvmfd);

    return 0;
}