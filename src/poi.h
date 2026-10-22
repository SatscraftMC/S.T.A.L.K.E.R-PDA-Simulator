#ifndef PDA_POI_H
#define PDA_POI_H

#include <stdint.h>
#include <stdbool.h>

/* The points of interest: the places that accept bitcoin, from BTC Map
 * (btcmap.org) - the set of places tagged in OpenStreetMap with
 * currency:XBT=yes. tools/fetch_btc.py writes the pack; this is the reader.
 *
 * The places are read straight off the file, so the layout here and the one
 * in the tool must match. */

/* what a place takes */
#define POI_F_LIGHTNING     0x01    /* payment:lightning = yes */
#define POI_F_ONCHAIN       0x02    /* payment:onchain = yes */
#define POI_F_CONTACTLESS   0x04    /* payment:lightning_contactless = yes (NFC) */
#define POI_F_ATM           0x08    /* amenity = atm */
#define POI_F_VERIFIED      0x10    /* surveyed or checked within the last year */
#define POI_F_COMPANION     0x20    /* needs a third party app */

typedef struct {
    double lat, lon;        /* 16 */
    char name[48];          /* 48  NUL padded */
    char address[64];       /* 64  NUL padded */
    char category[16];      /* 16  NUL padded: "cafe", "restaurant", ... */
    uint8_t flags;          /*  1 */
    uint8_t pad;            /*  1 */
    uint16_t year;          /*  2  the year it was last checked, 0 = unknown */
} poi_place_t;

/* Find the packs under the directory (PDA_POIS overrides it) and map them.
 * Returns the number of places. */
int poi_load(const char * dir);

/* the places, in the order the pack has them */
int poi_count(void);
const poi_place_t * poi_get(int i);

/* the name of the pack, for the credit line */
const char * poi_source(void);

#endif
