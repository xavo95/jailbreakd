#include <CommonCrypto/CommonDigest.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <mach/machine.h>
#include "kmem.h"
#include "cs_blob.h"

static const int offsetof_p_textvp = 0x230;     /* proc::p_textvp */
static const int offsetof_vu_ubcinfo = 0x78;        /* vnode::v_un::vu_ubcinfo */
static const int offsetof_cs_blobs = 0x50;      /* ubc_info::cs_blobs */

#define SWAP32(p) __builtin_bswap32(p)

#if __arm64e__
int entitle(uint64_t proc, const char *ent, int verbose) {
    int rv;
    uint64_t off;
    uint32_t length, newlen;
    unsigned char *buf, digest[32];

    off = rk64(proc + offsetof_p_textvp);
    off = rk64(off + offsetof_vu_ubcinfo);
    off = rk64(off + offsetof_cs_blobs);

    struct cs_blob csb;
    kread(off, &csb, sizeof(csb));

    char *csb_mem_uaddr = calloc(1, csb.csb_mem_size);
    kread(csb.csb_mem_kaddr, csb_mem_uaddr, csb.csb_mem_size);

    CS_CodeDirectory *csb_cd = (CS_CodeDirectory *)(csb_mem_uaddr + (uint64_t)csb.csb_cd - csb.csb_mem_kaddr);
    CS_GenericBlob *csb_entitlements_blob = (CS_GenericBlob *)(csb_mem_uaddr + (uint64_t)csb.csb_entitlements_blob - csb.csb_mem_kaddr);

#if 1
    if (SWAP32(csb_cd->magic) != CSMAGIC_CODEDIRECTORY) {
        printf("bad magic\n");
        free(csb_mem_uaddr);
        return -1;
    }
    length = SWAP32(csb_entitlements_blob->length);
    if (length < 8) {
        printf("bad length\n");
        free(csb_mem_uaddr);
        return -1;
    }
    if (verbose) {
        printf("blob[%d]: {%.*s}\n", length, length - sizeof(CS_GenericBlob), csb_entitlements_blob->data);
    }
    buf = (unsigned char *)csb_cd + SWAP32(csb_cd->hashOffset) - CSSLOT_ENTITLEMENTS * csb_cd->hashSize;
    CC_SHA256(csb_entitlements_blob, length, digest);
    if (memcmp(buf, digest, sizeof(digest))) {
        printf("bad SHA2\n");
        free(csb_mem_uaddr);
        return -1;
    }
    newlen = snprintf(csb_entitlements_blob->data, length - sizeof(CS_GenericBlob),
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n"
"%s\n"
"</dict>\n"
"</plist>\n", ent);
    if (newlen >= length - sizeof(CS_GenericBlob)) {
        printf("too long\n");
        free(csb_mem_uaddr);
        return -1;
    }
    CC_SHA256(csb_entitlements_blob, length, buf);
#endif

    uint64_t csb_mem_kaddr = kalloc(csb.csb_mem_size);
    csb.csb_cd = (CS_CodeDirectory *)(csb_mem_kaddr + (uint64_t)csb.csb_cd - csb.csb_mem_kaddr);
    csb.csb_entitlements_blob = (CS_GenericBlob *)(csb_mem_kaddr + (uint64_t)csb.csb_entitlements_blob - csb.csb_mem_kaddr);
    csb.csb_teamid = (char *)(csb_mem_kaddr + (uint64_t)csb.csb_teamid - csb.csb_mem_kaddr);

    kwrite(csb_mem_kaddr, csb_mem_uaddr, csb.csb_mem_size);

    csb.csb_mem_kaddr = csb_mem_kaddr;
    kwrite(off, &csb, sizeof(csb));

    //kernel_call_7(pmap_cs_cd_unregister_ppl, 1, off + 0xB0);
    //kernel_call_7(pmap_cs_cd_register_ppl, 5, csb.csb_mem_kaddr, csb.csb_mem_size, csb.csb_cd - csb.csb_mem_kaddr, 0, off + 0xB0);

    rv = csops(getpid(), CS_OPS_ENTITLEMENTS_BLOB, csb_entitlements_blob, length);
    if (rv) {
        printf("bad blob\n");
    } else if (verbose) {
        printf("blob: {%.*s}\n", length - sizeof(CS_GenericBlob), csb_entitlements_blob->data);
    }
    free(csb_mem_uaddr);
    return rv;
}
#else	/* !__arm64e__ */
int entitle(uint64_t proc, const char *ent, int verbose) {
    int rv;
    CS_CodeDirectory cdir;
    CS_GenericBlob *blob;
    unsigned char buf[32];
    unsigned char digest[32];
    uint32_t length, newlen;
    uint64_t cdir_off, blob_off, off;

    off = rk64(proc + offsetof_p_textvp);
    off = rk64(off + offsetof_vu_ubcinfo);
    off = rk64(off + offsetof_cs_blobs);

    cdir_off = rk64(off + offsetof(struct cs_blob, csb_cd));
    blob_off = rk64(off + offsetof(struct cs_blob, csb_entitlements_blob));
    kread(cdir_off, &cdir, sizeof(cdir));

    if (SWAP32(cdir.magic) != CSMAGIC_CODEDIRECTORY) {
        printf("bad magic\n");
        return -1;
    }

    length = SWAP32(rk32(blob_off + 4));
    if (length < 8) {
        printf("bad length\n");
        return -1;
    }

    blob = malloc(length);
    if (!blob) {
        printf("no memory\n");
        return -1;
    }

    kread(blob_off, blob, length);

    if (verbose) {
        printf("blob[%d]: {%.*s}\n", length, length - 8, blob->data);
    }

    off = cdir_off + SWAP32(cdir.hashOffset) - CSSLOT_ENTITLEMENTS * cdir.hashSize;
    kread(off, buf, sizeof(buf));

    CC_SHA256(blob, length, digest);
    if (memcmp(buf, digest, sizeof(digest))) {
        printf("bad SHA2\n");
        free(blob);
        return -1;
    }

    newlen = snprintf(blob->data, length - 8,
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
"<plist version=\"1.0\">\n"
"<dict>\n"
"%s\n"
"</dict>\n"
"</plist>\n", ent);

    if (newlen >= length - 8) {
        printf("too long\n");
        free(blob);
        return -1;
    }

    CC_SHA256(blob, length, digest);

    kwrite(off, digest, sizeof(digest));
    kwrite(blob_off, blob, length);

    rv = csops(getpid(), CS_OPS_ENTITLEMENTS_BLOB, blob, length);
    if (rv) {
        printf("bad blob\n");
    } else if (verbose) {
        printf("blob: {%.*s}\n", length - 8, blob->data);
    }
    free(blob);
    return rv;
}
#endif	/* !__arm64e__ */