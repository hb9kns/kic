
// UNIX to DOS format conversion
// Unlike u2d, this takes arbitrarily long lines, and deletes old files

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

int
main(int argc, char **argv)
{
    char buf[256];
    int i;
    for (i = 1; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st)) {
            perror("stat");
            continue;
        }
        if (!(st.st_mode & S_IFREG))
            continue;

        strcpy(buf, argv[i]);
        strcat(buf, "~");
        if (rename(argv[i], buf)) {
            perror("rename");
            continue;
        }
        FILE *ip = fopen(buf, "rb");
        if (!ip) {
            fprintf(stderr, "Can't open %s\n", buf);
            continue;
        }
        FILE *op = fopen(argv[i], "wb");
        if (!op) {
            fprintf(stderr, "Can't open %s\n", argv[i]);
            fclose(ip);
            continue;
        }
        int c, lastc = -1;
        while ((c = getc(ip)) != EOF) {
            if (c == '\n' && lastc != '\r')
                fputc('\r', op);
            fputc(c, op);
            lastc = c;
        }
        fclose(ip);
        fclose(op);
        chmod(argv[i], st.st_mode & (S_IXUSR | S_IWUSR | S_IRUSR));
        chmod(buf, _S_IWRITE);
        unlink(buf);
    }
    return (0);
}
