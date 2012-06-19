#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define    SWAP32(x)    (           \
    (((x) & 0x000000ff) << 24) |    \
    (((x) & 0x0000ff00) <<  8) |    \
    (((x) & 0x00ff0000) >>  8) |    \
    (((x) & 0xff000000) >> 24))

static void
handle_2400(void *arg, off_t nbytes, FILE *op)
{
    uint32_t narray = 0;
    uint32_t wo = 0;
    uint32_t *ptr = arg;
    off_t i;

    for (i = 0; i < (nbytes >> 2); i++) {
        ptr[i] = SWAP32(ptr[i]);
    }

    if (op) {
        fprintf(op, "static const uint32_t isp_2500_risc_code[] = {\n");
    }
    for (;;) {
        printf("load 0x%x words of code at load address 0x%x\n", ptr[3], ptr[2]);
        if (op) {
            for (i = 0; i < ptr[3]; i++, wo++) {
                if ((wo & 0x7) == 0 && wo) {
                    fputc('\n', op);
                }
                fprintf(op, " 0x%08x, ", ptr[i]);
            }
            fprintf(op, "\n /* end of array %d */\n", narray++);
            wo = 0;
        }
        if (ptr[1] == 0) {
            break;
        }
        ptr += ptr[3];
    }
    if (op) {
        fprintf(op, "};\n");
    }
}

static void
handle_2322(void *arg, off_t nbytes, FILE *op)
{
    uint16_t *ptr = arg;
    uint32_t nxtaddr;
    uint16_t segno = 0;
    uint32_t la = 0x0800;

    for (;;) {
        printf("load 0x%x words of code at load address 0x%x\n", ptr[3], la);
        if (++segno == 3) {
            break;
        }
        nxtaddr = ptr[3];
        ptr = &ptr[nxtaddr];
        la = ptr[5] | ((ptr[4] & 0x3f) << 16);
    }
}

static void
handle_2300(void *arg, off_t nbytes, FILE *op)
{
    uint16_t *ptr = arg;
    uint32_t la = 0x0800;

    printf("load 0x%x words of code at load address 0x%x\n", ptr[3], la);
}

static void
handle_other(void *arg, off_t nbytes, FILE *op)
{
    uint16_t *np = arg;
    uint32_t la = 0x1000;

    printf("load 0x%x words of code at load address 0x%x\n", np[0], la);
}

int
main(int a, char **v)
{
    int fd;
    FILE *op = NULL;
    void *arg;
    struct stat sb;

    if (a != 3 && a != 4) {
        fprintf(stderr, "usage: %s hba-type binary-firmware-file [ c output ]\n", v[0]);
        exit(1);
    }
    if (a == 4) {
        op = fopen(v[3], "w");
        if (op == NULL) {
            perror(v[3]);
            exit(1);
        }
    }

    fd = open(v[2], O_RDONLY);
    if (fd < 0) {
        perror(v[2]);
        exit(1);
    }

    if (fstat(fd, &sb) < 0) {
        perror("fstat");
        exit(1);
    }
    arg = mmap(NULL, sb.st_size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (arg == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    if (strncmp(v[1], "25", 2) == 0 || strncmp(v[1], "24", 2) == 0) {
        handle_2400(arg, sb.st_size, op);
    } else if (strcmp(v[1], "2322") == 0) {
        handle_2322(arg, sb.st_size, op);
    } else if (strncmp(v[1], "23", 2) == 0) {
        handle_2300(arg, sb.st_size, op);
    } else {
        handle_other(arg, sb.st_size, op);
    }
    (void) munmap(arg, sb.st_size);
    (void) close(fd);
    return (0);
}
/*
 * vim:ts=4:sw=4:expandtab
 */
