#include "mcfg.h"
#include "mm/mm.h"
#include "kern/console.h"
#include "kern/kassert.h"

using kernel::console::printf;

static inline size_t
config_len(size_t start_bus_num, size_t end_bus_num)
{
    return (end_bus_num - start_bus_num + 1)*0x00100000;
}

static inline void*
config_addr(void* vbase, uint8_t bus, uint8_t start_bus_num, uint8_t devfn,
    uint16_t offset)
{
    return (void*)((uint64_t)vbase | ((bus - start_bus_num) << 20) |
                   (devfn << 12) | offset);
}

kernel::mem_config_accessor::mem_config_accessor(uint64_t base,
    uint8_t start_bus_num, uint8_t end_bus_num):
        base(base),
        vbase(iomap_range(base,config_len(start_bus_num,end_bus_num))),
        start_bus_num(start_bus_num),
        end_bus_num(end_bus_num)
{
    printf("MCFG 0x%016lX [%02X:%02X]\n",base,start_bus_num,end_bus_num);
}

uint8_t
kernel::mem_config_accessor::config_read_8(uint8_t bus, uint8_t devfn,
    uint16_t offset)
{
    if (bus < start_bus_num || bus > end_bus_num)
        return 0xFF;
    return *(uint8_t*)config_addr(vbase,bus,start_bus_num,devfn,offset);
}

uint16_t
kernel::mem_config_accessor::config_read_16(uint8_t bus, uint8_t devfn,
    uint16_t offset)
{
    kassert((offset & 1) == 0);
    if (bus < start_bus_num || bus > end_bus_num)
        return 0xFFFF;
    return *(uint16_t*)config_addr(vbase,bus,start_bus_num,devfn,offset);
}

uint32_t
kernel::mem_config_accessor::config_read_32(uint8_t bus, uint8_t devfn,
    uint16_t offset)
{
    kassert((offset & 3) == 0);
    if (bus < start_bus_num || bus > end_bus_num)
        return 0xFFFFFFFF;
    return *(uint32_t*)config_addr(vbase,bus,start_bus_num,devfn,offset);
}

uint64_t
kernel::mem_config_accessor::config_read_64(uint8_t bus, uint8_t devfn,
    uint16_t offset)
{
    kassert((offset & 7) == 0);
    if (bus < start_bus_num || bus > end_bus_num)
        return 0xFFFFFFFFFFFFFFFF;
    return *(uint64_t*)config_addr(vbase,bus,start_bus_num,devfn,offset);
}

void
kernel::mem_config_accessor::config_write_8(uint8_t val, uint8_t bus,
    uint8_t devfn, uint16_t offset)
{
    *(uint8_t*)config_addr(vbase,bus,start_bus_num,devfn,offset) = val;
}

void
kernel::mem_config_accessor::config_write_16(uint16_t val, uint8_t bus,
    uint8_t devfn, uint16_t offset)
{
    kassert((offset & 1) == 0);
    *(uint16_t*)config_addr(vbase,bus,start_bus_num,devfn,offset) = val;
}

void
kernel::mem_config_accessor::config_write_32(uint32_t val, uint8_t bus,
    uint8_t devfn, uint16_t offset)
{
    kassert((offset & 3) == 0);
    *(uint32_t*)config_addr(vbase,bus,start_bus_num,devfn,offset) = val;
}

void
kernel::mem_config_accessor::config_write_64(uint64_t val, uint8_t bus,
    uint8_t devfn, uint16_t offset)
{
    kassert((offset & 7) == 0);
    *(uint64_t*)config_addr(vbase,bus,start_bus_num,devfn,offset) = val;
}

void
kernel::mem_config_accessor::handle_wapi_request(wapi::node* node,
    http::request* req, json::object* obj, http::response* rsp)
{
    // GET /pci/0000/cfg
    rsp->printf("{\r\n"
                "    \"type\"      : \"mcfg\",\r\n"
                "    \"base\"      : \"0x%016lX\",\r\n"
                "    \"first_bus\" : \"0x%02X\",\r\n"
                "    \"last_bus\"  : \"0x%02X\"\r\n"
                "}\r\n",
                base,start_bus_num,end_bus_num);
}
