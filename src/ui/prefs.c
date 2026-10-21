#include "ui/prefs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct {
    char key[32];
    char value[128];
    bool used;
} pref_t;

static pref_t s_prefs[PREFS_MAX];
static char s_path[512];
static bool s_loaded = false;

static void resolve_path(void)
{
    if(s_path[0]) return;
    const char * env = getenv("PDA_PREFS");
    if(env && *env) {
        snprintf(s_path, sizeof(s_path), "%s", env);
        return;
    }
    const char * xdg = getenv("XDG_CONFIG_HOME");
    if(xdg && *xdg) {
        snprintf(s_path, sizeof(s_path), "%s/pda/prefs.conf", xdg);
        return;
    }
    const char * home = getenv("HOME");
    if(home && *home) {
        snprintf(s_path, sizeof(s_path), "%s/.config/pda/prefs.conf", home);
        return;
    }
    snprintf(s_path, sizeof(s_path), "pda-prefs.conf");
}

const char * prefs_path(void)
{
    resolve_path();
    return s_path;
}

static pref_t * find(const char * key)
{
    for(int i = 0; i < PREFS_MAX; i++)
        if(s_prefs[i].used && strcmp(s_prefs[i].key, key) == 0) return &s_prefs[i];
    return NULL;
}

static pref_t * take(const char * key)
{
    pref_t * p = find(key);
    if(p) return p;
    for(int i = 0; i < PREFS_MAX; i++) {
        if(s_prefs[i].used) continue;
        s_prefs[i].used = true;
        snprintf(s_prefs[i].key, sizeof(s_prefs[i].key), "%s", key);
        s_prefs[i].value[0] = 0;
        return &s_prefs[i];
    }
    return NULL;
}

void prefs_init(void)
{
    resolve_path();
    prefs_load();
}

void prefs_load(void)
{
    resolve_path();
    memset(s_prefs, 0, sizeof(s_prefs));
    FILE * f = fopen(s_path, "r");
    if(!f) {
        s_loaded = true;
        return;
    }
    char line[256];
    while(fgets(line, sizeof(line), f)) {
        char * eq = strchr(line, '=');
        if(!eq) continue;
        *eq = 0;
        char * key = line;
        char * val = eq + 1;
        while(*key == ' ' || *key == '\t') key++;
        size_t kl = strlen(key);
        while(kl && (key[kl - 1] == ' ' || key[kl - 1] == '\t')) key[--kl] = 0;
        while(*val == ' ' || *val == '\t') val++;
        size_t vl = strlen(val);
        while(vl && (val[vl - 1] == '\n' || val[vl - 1] == '\r' || val[vl - 1] == ' ')) val[--vl] = 0;
        if(!key[0]) continue;
        pref_t * p = take(key);
        if(p) snprintf(p->value, sizeof(p->value), "%s", val);
    }
    fclose(f);
    s_loaded = true;
}

void prefs_save(void)
{
    if(!s_loaded) return;
    resolve_path();

    /* make sure the directory exists */
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", s_path);
    char * slash = strrchr(dir, '/');
    if(slash) {
        *slash = 0;
        if(dir[0]) mkdir(dir, 0755);
    }

    FILE * f = fopen(s_path, "w");
    if(!f) return;
    for(int i = 0; i < PREFS_MAX; i++) {
        if(!s_prefs[i].used) continue;
        fprintf(f, "%s = %s\n", s_prefs[i].key, s_prefs[i].value);
    }
    fclose(f);
}

const char * prefs_str(const char * key, const char * fallback)
{
    pref_t * p = find(key);
    if(p && p->value[0]) return p->value;
    return fallback;
}

bool prefs_bool(const char * key, bool fallback)
{
    pref_t * p = find(key);
    if(!p || !p->value[0]) return fallback;
    return (p->value[0] == '1' || p->value[0] == 't' || p->value[0] == 'y' ||
            p->value[0] == 'T' || p->value[0] == 'Y');
}

void prefs_set_str(const char * key, const char * value)
{
    pref_t * p = take(key);
    if(!p) return;
    /* the file is one setting per line: a value with a control character in
       it would break the file (and the name that comes out of it) */
    size_t o = 0;
    for(const char * src = value ? value : ""; *src && o + 1 < sizeof(p->value); src++) {
        unsigned char c = (unsigned char)*src;
        if(c < 0x20 || c == 0x7f) continue;
        p->value[o++] = *src;
    }
    p->value[o] = 0;
    prefs_save();
}

void prefs_set_bool(const char * key, bool value)
{
    prefs_set_str(key, value ? "1" : "0");
}
