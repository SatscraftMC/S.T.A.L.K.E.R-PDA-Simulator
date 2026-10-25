#include "identity/contacts.h"
#include "identity/state_file.h"
#include "ui/prefs.h"
#include "chat/chat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* The trust list. A name is a label; the key is the identity - which is why
   the table is keyed by the 64-hex pubkey (prefs' 32-char keys cannot hold
   one, so contacts never ride inside prefs). */

typedef struct {
    char pub[65];      /* lowercase hex, always */
    char label[64];    /* control-char stripped, trimmed, <= 63 chars */
} contact_t;

static contact_t s_ct[CONTACT_MAX];
static int s_n = 0;
static unsigned s_version = 0;

static void contacts_path(char * out, size_t outn)
{
    char dir[512];
    snprintf(dir, sizeof(dir), "%s", prefs_path());
    char * slash = strrchr(dir, '/');
    if(slash) *slash = 0;
    snprintf(out, outn, "%s%scontacts.conf", dir[0] ? dir : "", dir[0] ? "/" : "");
}

/* lowercase + must be exactly 64 hex: the wire says lowercase, but a
   foreign uppercase line must not silently miss */
static bool norm_pub(const char * in, char * out /* 65 */)
{
    if(!in || strlen(in) != 64) return false;
    for(int i = 0; i < 64; i++) {
        char c = in[i];
        if(c >= 'A' && c <= 'F') c = (char)(c - 'A' + 'a');
        if(!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
        out[i] = c;
    }
    out[64] = 0;
    return true;
}

static void clean_label(const char * in, char * out, size_t outn)
{
    size_t o = 0;
    if(in) {
        for(const char * p = in; *p && o + 1 < outn; p++) {
            unsigned char c = (unsigned char)*p;
            if(c < 0x20 || c == 0x7f) continue;
            out[o++] = *p;
        }
    }
    out[o] = 0;
    /* trim, and an empty label never drops the trust: the key is the trust */
    char * s = out;
    while(*s == ' ' || *s == '\t') s++;
    size_t n = strlen(s);
    while(n && (s[n - 1] == ' ' || s[n - 1] == '\t')) s[--n] = 0;
    if(s != out) memmove(out, s, n + 1);
    if(!out[0]) snprintf(out, outn, "%s", "UNKNOWN");
}

static int find(const char * pub_lower)
{
    for(int i = 0; i < s_n; i++)
        if(strcmp(s_ct[i].pub, pub_lower) == 0) return i;
    return -1;
}

/* the whole file, as it stands in memory */
static bool serialize(char * out, size_t outn)
{
    size_t o = 0;
    out[0] = 0;
    for(int i = 0; i < s_n; i++) {
        int w = snprintf(out + o, outn - o, "%s = %s\n", s_ct[i].pub, s_ct[i].label);
        if(w < 0 || (size_t)w >= outn - o) return false;
        o += (size_t)w;
    }
    return true;
}

bool contacts_save(void)
{
    char path[600];
    contacts_path(path, sizeof(path));

    char body[8192];
    if(!serialize(body, sizeof(body))) return false;
    if(!state_write(path, body, strlen(body))) {
        chat_system("contacts could not be written - nothing was saved");
        return false;
    }
    return true;
}

/* write this table out, and only then make it the live one */
static bool commit(const contact_t * next, int n)
{
    contact_t keep[CONTACT_MAX];
    int keep_n = s_n;
    memcpy(keep, s_ct, sizeof(s_ct));      /* the old table, if we must roll back */

    memcpy(s_ct, next, sizeof(s_ct));
    s_n = n;

    if(!contacts_save()) {
        memcpy(s_ct, keep, sizeof(s_ct));
        s_n = keep_n;
        return false;
    }
    s_version++;
    return true;
}

bool contacts_load(void)
{
    s_n = 0;
    memset(s_ct, 0, sizeof(s_ct));

    char path[600];
    contacts_path(path, sizeof(path));

    struct stat st;
    if(stat(path, &st) != 0) return true;    /* a first run is not scolded */

    FILE * f = fopen(path, "r");
    if(!f) {
        chat_system("contacts.conf exists but could not be read - starting empty");
        return false;
    }

    int skipped = 0;
    char line[256];
    while(fgets(line, sizeof(line), f)) {
        char * eq = strchr(line, '=');
        if(!eq) { skipped++; continue; }
        *eq = 0;
        char * key = line;
        char * val = eq + 1;
        size_t kl = strlen(key);
        while(kl && (key[kl - 1] == ' ' || key[kl - 1] == '\t' ||
                     key[kl - 1] == '\n' || key[kl - 1] == '\r'))
            key[--kl] = 0;
        while(*val == ' ' || *val == '\t') val++;

        char pub[65];
        if(!norm_pub(key, pub)) { skipped++; continue; }

        char label[64];
        clean_label(val, label, sizeof(label));

        int at = find(pub);                  /* duplicate keys: last wins */
        if(at < 0) {
            if(s_n >= CONTACT_MAX) { skipped++; continue; }
            at = s_n++;
            snprintf(s_ct[at].pub, sizeof(s_ct[at].pub), "%s", pub);
        }
        snprintf(s_ct[at].label, sizeof(s_ct[at].label), "%s", label);
    }
    fclose(f);

    if(skipped > 0) {
        char why[96];
        snprintf(why, sizeof(why),
                 "contacts.conf: %d line(s) skipped (not a valid entry)", skipped);
        chat_system(why);
    }
    return true;
}

bool contacts_has(const char * pub_hex)
{
    char pub[65];
    if(!norm_pub(pub_hex, pub)) return false;
    return find(pub) >= 0;
}

const char * contacts_name(const char * pub_hex)
{
    char pub[65];
    if(!norm_pub(pub_hex, pub)) return NULL;
    int at = find(pub);
    return at < 0 ? NULL : s_ct[at].label;
}

int contacts_count(void)
{
    return s_n;
}

const char * contacts_at(int i)
{
    if(i < 0 || i >= s_n) return NULL;
    return s_ct[i].pub;
}

unsigned contacts_version(void)
{
    return s_version;
}

bool contacts_trust(const char * pub_hex, const char * label)
{
    char pub[65];
    if(!norm_pub(pub_hex, pub)) return false;

    int at = find(pub);
    char clean[64];
    clean_label(label, clean, sizeof(clean));

    if(at >= 0) {
        /* write only on change: a message that changes nothing costs no
           file write and no list rebuild (the version is that check) */
        if(strcmp(s_ct[at].label, clean) == 0) return true;
        contact_t next[CONTACT_MAX];
        memcpy(next, s_ct, sizeof(next));
        snprintf(next[at].label, sizeof(next[at].label), "%s", clean);
        return commit(next, s_n);
    }

    if(s_n >= CONTACT_MAX) {
        chat_system("contacts full - that trust was not saved");
        return false;
    }
    contact_t next[CONTACT_MAX];
    memcpy(next, s_ct, sizeof(next));
    snprintf(next[s_n].pub, sizeof(next[s_n].pub), "%s", pub);
    snprintf(next[s_n].label, sizeof(next[s_n].label), "%s", clean);
    return commit(next, s_n + 1);
}

bool contacts_untrust(const char * pub_hex)
{
    char pub[65];
    if(!norm_pub(pub_hex, pub)) return false;
    int at = find(pub);
    if(at < 0) return false;

    contact_t next[CONTACT_MAX];
    memcpy(next, s_ct, sizeof(next));
    for(int i = at; i < s_n - 1; i++) next[i] = next[i + 1];
    memset(&next[s_n - 1], 0, sizeof(next[0]));
    return commit(next, s_n - 1);
}
