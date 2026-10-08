// Minimal reader for the flattened device tree (FDT) that the
// boot loader passes to the kernel.

#include "types.h"
#include "riscv.h"
#include "defs.h"

#define FDT_MAGIC       0xd00dfeed
#define FDT_BEGIN_NODE 1
#define FDT_END_NODE    2
#define FDT_PROP        3
#define FDT_NOP         4

// physical address of the device tree; set by entry.S from a1.
uint64 dtb_pa;

// every number in the device tree is stored big-endian.
static uint32 be32(const char *p)
{
    const uchar *b = (const uchar *)p;
    return ((uint32)b[0] << 24) | ((uint32)b[1] << 16 | ((uint32)b[2] << 8 | b[3]));
}

static uint64 be64(const char *p)
{
    return (((uint64)be32(p) << 32) | be32(p + 4));
}

// Return the size in bytes of physical memory, from the reg
// property of the /memory node, or 0 if it can't be found.
// Assumes #address-cells and #size-cells are both 2.
uint64 fdt_memsize(void)
{
    char *dtb = (char *)dtb_pa;

    if (dtb == 0 || be32(dtb) != FDT_MAGIC)
        return 0;

    char *p = dtb + be32(dtb + 8); // off_dt_struct
    char *strings = dtb + be32(dtb + 12); // off_dt_strings
    int depth = 0;
    int inmem = 0; // inside the /memory node?

    for (;;)
    {
        uint32 tok = be32(p);
        p += 4;
        if (tok == FDT_BEGIN_NODE) 
        {
            depth++;
            if ((depth == 2) && strncmp(p, "memory@", 7) == 0)
                inmem = 1;
            p += (strlen(p) + 1 + 3) & ~3; // name, padded to 4 bytes
        } else if (tok == FDT_END_NODE)
        {
            if (depth ==2)
                inmem = 0;
            depth--;
        }else if (tok == FDT_PROP)
        {
            uint32 len = be32(p);
            char *name = strings + be32(p+4);
            p+=8;
            
            if (inmem && depth == 2 && strncmp(name, "reg", 4) == 0 && len >= 16)
                return be64(p + 8); // reg = <address size>, 8 bytes each
            p += (len + 3) & ~3; // value, padded to 4 bytes
        } else if (tok != FDT_NOP)
        {
            return 0;  // FDT_END, or not a device tree
        }

    }
}
