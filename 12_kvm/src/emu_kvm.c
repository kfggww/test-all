#include <stdio.h>

#include <errno.h>
#include <linux/kvm.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv) {

    // open /dev/kvm
    int kvmfd = open("/dev/kvm", O_RDWR | O_CLOEXEC);
    if (kvmfd < 0) {
        printf("Can not open /dev/kvm\n");
        return EPERM;
    }

    printf("kvmfd is: %d\n", kvmfd);

    // get kvm api verison
    int version = ioctl(kvmfd, KVM_GET_API_VERSION);
    printf("kvm api version: %d\n", version);

    // create vm
    int vmfd = ioctl(kvmfd, KVM_CREATE_VM, 0);
    printf("vm created: %d\n", vmfd);

    // memory settings

    // create vcpu

    // run binary code in vm

    close(vmfd);
    close(kvmfd);

    return 0;
}