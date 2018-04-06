/*
   pixorgel - eh18 edition

   public domain

   QnD hack to flood pixelflut with an virtuel X framebuffer display.
   start an virtual X server like
   Xvfb :1 -screen 0 256x256x24 -fbdir /var/tmp

   in that X start anything, e.g. xlock
   ./xlock/xlock -nolock -duration 13 -display :1
   or some relevant video
   DISPLAY=':1' mpv -fs -loop 0 https://www.youtube.com/embed/oHg5SJYRHA0

   then start pixorgel a few times (as many connections are allowed)
   watch -n 30 ./restart
   with
   cat ./restart
    #!/bin/bash
    # watch me
    for i in $(seq 10 ); do
        ./pixorgel 94.45.232.48 1234 &
    done
 */

#define X_FILE "/var/tmp/Xvfb_screen0"

#include <X11/XWDFile.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

void sendpx(int fd, int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
#if 0
    if (
            ( r == 0) &&
            ( g == 0) &&
            ( b == 0)
            ) return;
#endif
    dprintf(fd, "PX %d %d %02x%02x%02x\n", x, y, r, g, b);
}

int main(int argc, char ** argv)
{
    int fh = open(X_FILE, O_RDWR);
    XWDFileHeader xwd;
    read(fh, &xwd, sizeof(xwd));

    if (xwd.byte_order == 0)
    {
        /* swap endianess */
        xwd.pixmap_depth  = be32toh(xwd.pixmap_depth);
        xwd.pixmap_width  = be32toh(xwd.pixmap_width);
        xwd.pixmap_height = be32toh(xwd.pixmap_height);

        xwd.window_width  = be32toh(xwd.window_width);
        xwd.window_height = be32toh(xwd.window_height);

        xwd.window_x      = be32toh(xwd.window_x);
        xwd.window_y      = be32toh(xwd.window_y);
        xwd.bits_per_pixel      = be32toh(xwd.bits_per_pixel);
        xwd.bitmap_pad      = be32toh(xwd.bitmap_pad);
        xwd.bytes_per_line      = be32toh(xwd.bytes_per_line);
    }

    printf("%dx",  xwd.pixmap_width);
    printf("%dx",  xwd.pixmap_height);
    printf("%d\n", xwd.pixmap_depth);

    printf("ww   %d\n", xwd.window_width);
    printf("wh   %d\n", xwd.window_height);

    printf("wx   %d\n", xwd.window_x);
    printf("wy   %d\n", xwd.window_y);

    printf("bpp  %d\n", xwd.bits_per_pixel);
    printf("pad  %d\n", xwd.bitmap_pad);
    printf("bypl %d\n", xwd.bytes_per_line);

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <IP> <PORT>\n", argv[0]);
        exit(1);
    }

    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
        exit(1);
    }

    int portno = atoi(argv[2]);

    struct in_addr *server_ina = malloc(sizeof(struct in_addr));
    if (inet_pton(AF_INET, argv[1], server_ina) != 1) {
        fprintf(stderr, "Could not parse address %s\n", argv[1]);
        exit(1);
    }

    struct sockaddr_in *server_sin = calloc(1, sizeof(struct sockaddr_in));
    server_sin->sin_family = AF_INET;
    server_sin->sin_port = htons(portno);
    server_sin->sin_addr = *server_ina;

    if (connect(sockfd, (const struct sockaddr *)server_sin, sizeof(*server_sin)) != 0) {
        fprintf(stderr, "Could not connect: %s\n", strerror(errno));
        exit(1);
    }

    uint8_t * m; /* mmap'd file */

    int fs = lseek(fh, 0, SEEK_END);
    m = mmap(NULL, fs, PROT_READ, MAP_PRIVATE, fh, 0);

    int x_off = 920;
    int y_off = 843;

    x_off = 840;
    y_off =   0;

    x_off =   0;
    y_off =   0;

    int f_off = 0x000096a0;//sizeof(xwd);
    f_off = 3232;

    while (23)
    {
        for (int x = 0; x < xwd.pixmap_width; x++) {
            for (int y = 0; y < xwd.pixmap_height; y++) {
                sendpx(sockfd, x + x_off, y + y_off,
                        m[f_off + xwd.pixmap_width * y * 4 + (x*4) + 2],
                        m[f_off + xwd.pixmap_width * y * 4 + (x*4) + 1],
                        m[f_off + xwd.pixmap_width * y * 4 + (x*4) + 0]
                        );
            }
        }
    }
    return 0;
}
