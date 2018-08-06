#ifndef __MODE32_E820_H
#define __MODE32_E820_H

struct e820_entry
{
    uint64_t    addr;
    uint64_t    len;
    uint32_t    type;
    uint32_t    rsrv;
};
KASSERT(sizeof(e820_entry) == 24);

struct e820_io
{
    uint32_t    sig;
    uint32_t    cookie;
    uint32_t    entry_len;
    e820_entry  entry;
};

struct __PACKED__ e820_map
{
    uint16_t    nentries;
    e820_entry  entries[];
};

extern "C"
{
    void e820_iter(e820_io* io);
}

#endif /* __MODE32_E820_H */
