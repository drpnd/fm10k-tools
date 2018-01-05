/*_
 * Copyright (c) 2018 Hirochika Asai <asai@jar.jp>
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "fm10k.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

/* 64 MiB */
#define FM10K_BAR4_SIZE 0x4000000

/*
 * Usage
 */
void
usage(const char *prog)
{
    fprintf(stderr, "Usage: %s /dev/<uioX>\n", prog);
    exit(EXIT_FAILURE);
}

/*
 * Initialize FM10K
 */

/*
 * Main routine
 */
int
main(int argc, const char *const argv[])
{
    int fd;
    const char *prog;
    const char *uiodev;
    long pagesize;
    void *ptr;
    size_t size;

    prog = argv[0];
    if ( argc < 2 ) {
        usage(prog);
    }
    uiodev = argv[1];

    /* Open and memory map uio device */
    fd = open (uiodev, O_RDWR);
    if ( fd < 0 ) {
        perror(uiodev);
        return EXIT_FAILURE;
    }
    pagesize = sysconf(_SC_PAGESIZE);
    size = (FM10K_BAR4_SIZE + pagesize - 1) / pagesize * pagesize;
    ptr = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    /* Testing */
    printf("%x\n", *((uint32_t *)(ptr + 0x10)));

    /* Unmap and close */
    (void)munmap(ptr, size);
    (void)close(fd);

    return 0;
}

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: sw=4 ts=4 fdm=marker
 * vim<600: sw=4 ts=4
 */
