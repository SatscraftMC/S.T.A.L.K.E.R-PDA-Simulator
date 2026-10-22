#include "poi.h"

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

/* The points of interest (BTC Map). The pack is a file of fixed size records,
 * mapped into memory - the same trick the map imagery uses, so the kernel
 * pages it out instead of it sitting in the heap. */

#define MAX_PLACES 8192

static poi_place_t * s_places = NULL;
static int s_count = 0;
static int s_alloc = 0;
static char s_source[64] = "";

/* the record layout must match tools/fetch_btc.py exactly */
_Static_assert(sizeof(poi_place_t) == 152, "the place layout must match the tool");

static void * map_file(const char * path, size_t * size_out)
{
    int fd = open(path, O_RDONLY);
    if(fd < 0) return NULL;
    struct stat st;
    if(fstat(fd, &st) != 0 || st.st_size <= 0) { close(fd); return NULL; }
    void * p = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if(p == MAP_FAILED) return NULL;
    *size_out = (size_t)st.st_size;
    return p;
}

/* the pack's manifest: "name <x>" and "source <x>" */
static bool cfg_get(const char * line, const char * key, char * val, size_t n)
{
    size_t klen = strlen(key);
    if(strncmp(line, key, klen) != 0) return false;
    const char * p = line + klen;
    while(*p == ' ' || *p == '\t') p++;
    size_t i = 0;
    while(p[i] && p[i] != '\n' && p[i] != '\r' && i < n - 1) {
        val[i] = p[i];
        i++;
    }
    val[i] = 0;
    return true;
}

int poi_load(const char * dir)
{
    const char * root = dir;
    if(!root) root = getenv("PDA_POIS");
    if(!root) root = "assets/poi";

    DIR * d = opendir(root);
    if(!d) return 0;

    struct dirent * e;
    while((e = readdir(d))) {
        if(e->d_name[0] == '.') continue;

        char dir_path[512], cfg_path[544], bin_path[576], line[512];
        snprintf(dir_path, sizeof(dir_path), "%s/%s", root, e->d_name);
        snprintf(cfg_path, sizeof(cfg_path), "%s/pack.cfg", dir_path);
        snprintf(bin_path, sizeof(bin_path), "%s/places.bin", dir_path);

        /* the manifest is what makes it a pack: a stray .bin is ignored */
        FILE * f = fopen(cfg_path, "r");
        if(!f) continue;
        char name[64] = "";
        while(fgets(line, sizeof(line), f)) {
            char val[64];
            if(cfg_get(line, "name", val, sizeof(val)))
                snprintf(name, sizeof(name), "%s", val);
            else if(cfg_get(line, "source", val, sizeof(val)))
                snprintf(s_source, sizeof(s_source), "%s", val);
        }
        fclose(f);
        if(!name[0]) continue;

        size_t size = 0;
        void * buf = map_file(bin_path, &size);
        if(!buf) continue;

        int count = (int)(size / sizeof(poi_place_t));
        if(count <= 0) { munmap(buf, size); continue; }
        if(s_count + count > MAX_PLACES) count = MAX_PLACES - s_count;
        if(count <= 0) { munmap(buf, size); continue; }

        /* the array grows with the packs that are there */
        if(s_count + count > s_alloc) {
            int want = s_count + count;
            poi_place_t * p = realloc(s_places, (size_t)want * sizeof(poi_place_t));
            if(!p) { munmap(buf, size); break; }
            s_places = p;
            s_alloc = want;
        }

        memcpy(s_places + s_count, buf, (size_t)count * sizeof(poi_place_t));
        s_count += count;
        munmap(buf, size);
    }
    closedir(d);
    return s_count;
}

int poi_count(void)
{
    return s_count;
}

const poi_place_t * poi_get(int i)
{
    if(i < 0 || i >= s_count) return NULL;
    return &s_places[i];
}

const char * poi_source(void)
{
    return s_source[0] ? s_source : "BTC Map";
}
