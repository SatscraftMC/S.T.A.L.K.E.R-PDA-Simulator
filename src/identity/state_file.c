#include "identity/state_file.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool state_ensure_dir(const char * path)
{
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", path ? path : "");
    char * slash = strrchr(dir, '/');
    if(!slash) return true;             /* no directory in the path */
    *slash = 0;
    if(!dir[0]) return true;            /* "/file" - the root is there */
    if(mkdir(dir, 0755) == 0) return true;
    return errno == EEXIST;
}

bool state_write(const char * path, const char * data, size_t len)
{
    if(!path || !*path || !data) return false;
    if(!state_ensure_dir(path)) return false;

    char tmp[600];
    if(snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int)sizeof(tmp)) return false;

    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if(fd < 0) return false;

    bool ok = true;
    size_t off = 0;
    while(off < len) {
        ssize_t w = write(fd, data + off, len - off);
        if(w < 0) {
            if(errno == EINTR) continue;
            ok = false;
            break;
        }
        off += (size_t)w;
    }
    if(ok && fsync(fd) != 0) ok = false;
    if(close(fd) != 0) ok = false;

    if(ok && rename(tmp, path) != 0) ok = false;
    if(!ok) unlink(tmp);
    return ok;
}
