#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include "permissions.h"

int perm_ensure_dir(const char *path, mode_t mode)
{
    struct stat st;
    if (stat(path, &st) == 0) return S_ISDIR(st.st_mode) ? 0 : -1;
    return mkdir(path, mode);
}

int perm_get_mode(const char *path, mode_t *mode)
{
    struct stat st;
    if (stat(path, &st) == -1) return -1;
    *mode = st.st_mode & 07777;
    return 0;
}

int perm_set_mode(const char *path, mode_t mode)
{
    return chmod(path, mode);
}

void perm_to_string(mode_t m, char out[10])
{
    const char *r = "rwxrwxrwx";
    for (int i = 0; i < 9; i++)
        out[i] = (m & (1u << (8 - i))) ? r[i] : '-';
    out[9] = '\0';
}

/* Copy src to dst; dst keeps the same permissions and timestamps as src. */
int perm_backup_file(const char *src, const char *dst)
{
    struct stat st;
    if (stat(src, &st) == -1) return -1;

    int in = open(src, O_RDONLY);
    if (in < 0) return -1;
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (out < 0) { close(in); return -1; }

    char buf[4096];
    ssize_t n;
    int rc = 0;
    while ((n = read(in, buf, sizeof(buf))) > 0) {
        ssize_t off = 0;
        while (off < n) {
            ssize_t w = write(out, buf + off, (size_t)(n - off));
            if (w < 0) {
                if (errno == EINTR) continue;
                rc = -1;
                goto done;
            }
            off += w;
        }
    }
    if (n < 0) rc = -1;
done:
    if (rc == 0) {
        fchmod(out, st.st_mode & 07777);              /* preserve permissions */
        struct timespec ts[2] = { st.st_atim, st.st_mtim };
        futimens(out, ts);                             /* preserve timestamps  */
    }
    close(in);
    close(out);
    return rc;
}