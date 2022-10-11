#include <elf.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#define ROUND_DOWN(a, m) (((unsigned long)(a)) & (~((unsigned long)(m)-1)))

int main(int argc, char **argv)
{

    if (argc < 2)
    {
        printf("Usage: loader elf_file\n");
        return -1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0)
    {
        printf("Can NOT open ELF file: %s\n", argv[1]);
        return -1;
    }

    char buf[sizeof(Elf64_Ehdr)];
    if (read(fd, buf, sizeof(Elf64_Ehdr)) != sizeof(Elf64_Ehdr))
    {
        printf("Can NOT read ELF file: %s\n", argv[1]);
        close(fd);
        return -1;
    }

    Elf64_Ehdr *ehdr = mmap(NULL, ((Elf64_Ehdr *)buf)->e_phoff + ((Elf64_Ehdr *)buf)->e_phnum * ((Elf64_Ehdr *)buf)->e_phentsize, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(ehdr != MAP_FAILED);

    Elf64_Phdr *ephdr = (Elf64_Phdr *)((char *)ehdr + ehdr->e_phoff);

    void *seg = NULL;
    int prot = 0;
    for (int i = 0; i < ehdr->e_phnum; ++i)
    {
        if (ephdr->p_type == PT_LOAD)
        {
            prot = 0;
            prot |= ((ephdr->p_flags & PF_X) ? PROT_EXEC : 0);
            prot |= ((ephdr->p_flags & PF_W) ? PROT_WRITE : 0);
            prot |= ((ephdr->p_flags & PF_R) ? PROT_READ : 0);
            void *addr = (void *)ROUND_DOWN(ephdr->p_vaddr, ephdr->p_align);
            off_t offset = ROUND_DOWN(ephdr->p_offset, ephdr->p_align);
            seg = mmap(addr, ephdr->p_memsz + (ephdr->p_offset % ephdr->p_align), prot, MAP_PRIVATE, fd, offset);
            assert(seg != MAP_FAILED);
            memset((void *)(ephdr->p_vaddr + ephdr->p_filesz), 0, ephdr->p_memsz - ephdr->p_filesz);
        }

        ephdr += 1;
    }
    close(fd);

    static char stack[1024 * 8];
    void *sp = (void *)(stack + sizeof(stack) - 256);
    void *sp_exec = sp;

    *(long *)sp = 1;
    sp = ((long *)sp) + 1;
    *(const char **)sp = argv[1];
    sp = ((const char **)sp) + 1;
    *(const char **)sp = 0;

    asm volatile("mov $0, %%rdx;"
                 "mov %0, %%rsp;"
                 "jmp *%1;" ::"a"(sp_exec),
                 "b"(ehdr->e_entry)
                 :);

    return 0;
}