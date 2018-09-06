#include "wapi.h"
#include "k++/obj_list.h"

#define GEN_ADDR(r) \
    r.addr_space_id,r.register_bit_width,r.register_bit_offset,r.access_size, \
    r.addr

// Map ACPI SDT signatures to their real names.
struct acpi_name_table_entry
{
    uint32_t    signature;
    char        name[5];
};

// Tables whose signatures don't match their names.
static acpi_name_table_entry acpi_name_table[] = {
    {FADT_SIG, "FADT"},
    {MADT_SIG, "MADT"},
};

static const acpi_name_table_entry*
find_name_table_entry(uint32_t signature)
{
    for (auto& e : acpi_name_table)
    {
        if (e.signature == signature)
            return &e;
    }
    return NULL;
}

static void
get_acpi_table_name(uint32_t signature, char (&name)[5])
{
    auto* e = find_name_table_entry(signature);
    if (e)
    {
        strcpy(name,e->name);
        return;
    }
    else
    {
        memcpy(name,&signature,4);
        name[4] = '\0';
    }
}

struct sdt_node : public wapi::node
{
    const kernel::sdt_header* h;
    void handle_request(wapi::node* node, http::request* req, json::object* obj,
                        http::response* rsp);
    inline sdt_node(const kernel::sdt_header* h, const char* name):
        wapi::node(method_delegate(handle_request),METHOD_GET_MASK,
                   "%s",name),
        h(h)
    {
    }
};

static void
acpi_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    // GET /acpi
    rsp->printf("{\r\n"
                "    \"tables\" : [ \"RSDP\",");
    for (auto* h : kernel::acpi_sdts)
    {
        char name[5];
        get_acpi_table_name(h->signature,name);
        rsp->printf(" \"%s\",",name);
    }
    rsp->ks.shrink();
    rsp->printf("]\r\n}\r\n");
}

static void
rsdp_request(wapi::node* node, http::request* req, json::object* obj,
    http::response* rsp)
{
    // GET /acpi/RSDP
    rsp->printf("{\r\n"
                "    \"address\"   : \"0x%016lX\",\r\n"
                "    \"signature\" : \"%c%c%c%c%c%c%c%c\",\r\n"
                "    \"checksum\"  : \"0x%02X\",\r\n"
                "    \"oem_id\"    : \"%c%c%c%c%c%c\",\r\n"
                "    \"revision\"  : \"0x%02X\",\r\n"
                "    \"rsdt_base\" : \"0x%08X\"",
                kernel::virt_to_phys(kernel::acpi_rsdp),
                (char)(kernel::acpi_rsdp->signature >>  0),
                (char)(kernel::acpi_rsdp->signature >>  8),
                (char)(kernel::acpi_rsdp->signature >> 16),
                (char)(kernel::acpi_rsdp->signature >> 24),
                (char)(kernel::acpi_rsdp->signature >> 32),
                (char)(kernel::acpi_rsdp->signature >> 40),
                (char)(kernel::acpi_rsdp->signature >> 48),
                (char)(kernel::acpi_rsdp->signature >> 56),
                kernel::acpi_rsdp->checksum,
                kernel::acpi_rsdp->oem_id[0],
                kernel::acpi_rsdp->oem_id[1],
                kernel::acpi_rsdp->oem_id[2],
                kernel::acpi_rsdp->oem_id[3],
                kernel::acpi_rsdp->oem_id[4],
                kernel::acpi_rsdp->oem_id[5],
                kernel::acpi_rsdp->revision,
                kernel::acpi_rsdp->rsdt_base);
    if (kernel::acpi_rsdp->revision == 2)
    {
        auto* rsdp2 = (const kernel::rsdp_table_2*)kernel::acpi_rsdp;
        rsp->printf(",\r\n"
                    "    \"length\"    : \"0x%08X\",\r\n"
                    "    \"xsdt_base\" : \"0x%016lX\",\r\n"
                    "    \"xchecksum\" : \"0x%02X\"",
                    rsdp2->length,
                    rsdp2->xsdt_base,
                    rsdp2->xchecksum);
    }
    rsp->printf("\r\n}\r\n");
}

static void
sdt_request(const kernel::sdt_header* h, const char* name, http::response* rsp)
{
    rsp->printf("    \"address\"              : \"0x%016lX\",\r\n"
                "    \"length\"               : \"0x%08X\",\r\n"
                "    \"signature\"            : \"%c%c%c%c\",\r\n"
                "    \"name\"                 : \"%s\",\r\n"
                "    \"revision\"             : \"0x%02X\",\r\n"
                "    \"checksum\"             : \"0x%02X\",\r\n"
                "    \"oem_id\"               : \"%c%c%c%c%c%c\",\r\n"
                "    \"oem_table_id\"         : \"%c%c%c%c%c%c\",\r\n"
                "    \"oem_revision\"         : \"0x%08X\",\r\n"
                "    \"creator_id\"           : \"0x%08X\",\r\n"
                "    \"creator_revision\"     : \"0x%08X\"",
                kernel::virt_to_phys(h),
                h->length,
                (char)(h->signature >>  0),
                (char)(h->signature >>  8),
                (char)(h->signature >> 16),
                (char)(h->signature >> 24),
                name,
                h->revision,
                h->checksum,
                h->oem_id[0],
                h->oem_id[1],
                h->oem_id[2],
                h->oem_id[3],
                h->oem_id[4],
                h->oem_id[5],
                h->oem_table_id[0],
                h->oem_table_id[1],
                h->oem_table_id[2],
                h->oem_table_id[3],
                h->oem_table_id[4],
                h->oem_table_id[5],
                h->oem_revision,
                h->creator_id,
                h->creator_revision);
}

static void
fadt_request(const kernel::sdt_header* _h, const char* name,
    http::response* rsp)
{
    auto* h = (const kernel::fadt_table*)_h;
    rsp->printf(",\r\n"
                "    \"firmware_ctrl\"        : \"0x%08X\",\r\n"
                "    \"dstd\"                 : \"0x%08X\",\r\n"
                "    \"preferred_pm_profile\" : \"0x%02X\",\r\n"
                "    \"sci_int\"              : \"0x%04X\",\r\n"
                "    \"sci_cmd\"              : \"0x%08X\",\r\n"
                "    \"acpi_enable\"          : \"0x%02X\",\r\n"
                "    \"acpi_disable\"         : \"0x%02X\",\r\n"
                "    \"s4bios_req\"           : \"0x%02X\",\r\n"
                "    \"pstate_cnt\"           : \"0x%02X\",\r\n"
                "    \"pm1a_evt_blk\"         : \"0x%08X\",\r\n"
                "    \"pm1b_evt_blk\"         : \"0x%08X\",\r\n"
                "    \"pm1a_cnt_blk\"         : \"0x%08X\",\r\n"
                "    \"pm1b_cnt_blk\"         : \"0x%08X\",\r\n"
                "    \"pm2_cnt_blk\"          : \"0x%08X\",\r\n"
                "    \"pm_tmr_blk\"           : \"0x%08X\",\r\n"
                "    \"gpe0_blk\"             : \"0x%08X\",\r\n"
                "    \"gpe1_blk\"             : \"0x%08X\",\r\n"
                "    \"pm1_evt_len\"          : \"0x%02X\",\r\n"
                "    \"pm1_cnt_len\"          : \"0x%02X\",\r\n"
                "    \"pm2_cnt_len\"          : \"0x%02X\",\r\n"
                "    \"pm_tmr_len\"           : \"0x%02X\",\r\n"
                "    \"gpe0_blk_len\"         : \"0x%02X\",\r\n"
                "    \"gpe1_blk_len\"         : \"0x%02X\",\r\n"
                "    \"gpe1_base\"            : \"0x%02X\",\r\n"
                "    \"cst_cnt\"              : \"0x%02X\",\r\n"
                "    \"p_lvl2_lat\"           : \"0x%04X\",\r\n"
                "    \"p_lvl3_lat\"           : \"0x%04X\",\r\n"
                "    \"flush_size\"           : \"0x%04X\",\r\n"
                "    \"flush_stride\"         : \"0x%04X\",\r\n"
                "    \"duty_offset\"          : \"0x%02X\",\r\n"
                "    \"duty_width\"           : \"0x%02X\",\r\n"
                "    \"day_alrm\"             : \"0x%02X\",\r\n"
                "    \"mon_alrm\"             : \"0x%02X\",\r\n"
                "    \"century\"              : \"0x%02X\",\r\n"
                "    \"iapc_boot_arch\"       : \"0x%04X\",\r\n"
                "    \"flags\"                : \"0x%08X\"",
                h->firmware_ctrl,
                h->dsdt,
                h->preferred_pm_profile,
                h->sci_int,
                h->sci_cmd,
                h->acpi_enable,
                h->acpi_disable,
                h->s4bios_req,
                h->pstate_cnt,
                h->pm1a_evt_blk,
                h->pm1b_evt_blk,
                h->pm1a_cnt_blk,
                h->pm1b_cnt_blk,
                h->pm2_cnt_blk,
                h->pm_tmr_blk,
                h->gpe0_blk,
                h->gpe1_blk,
                h->pm1_evt_len,
                h->pm1_cnt_len,
                h->pm2_cnt_len,
                h->pm_tmr_len,
                h->gpe0_blk_len,
                h->gpe1_blk_len,
                h->gpe1_base,
                h->cst_cnt,
                h->p_lvl2_lat,
                h->p_lvl3_lat,
                h->flush_size,
                h->flush_stride,
                h->duty_offset,
                h->duty_width,
                h->day_alrm,
                h->mon_alrm,
                h->century,
                h->iapc_boot_arch,
                h->flags);

    if (offsetof(kernel::fadt_table,reset_reg) >= h->hdr.length)
        return;
    rsp->printf(",\r\n    \"reset_reg\"            : \"%u:%u:%u:%u:0x%016lX\"",
                GEN_ADDR(h->reset_reg));

    if (offsetof(kernel::fadt_table,reset_value) >= h->hdr.length)
        return;
    rsp->printf(",\r\n    \"reset_value\"          : \"0x%02X\",\r\n",
                h->reset_value);

    if (offsetof(kernel::fadt_table,x_firmware_ctrl) >= h->hdr.length)
        return;
    rsp->printf(",\r\n    \"x_firmware_ctrl\"      : \"0x%016lX\",\r\n",
                h->x_firmware_ctrl);

    if (offsetof(kernel::fadt_table,x_dsdt) >= h->hdr.length)
        return;
    rsp->printf(",\r\n    \"x_dsdt\"               : \"0x%016lX\",\r\n",
                h->x_dsdt);

    if (offsetof(kernel::fadt_table,x_pm1a_evt_blk) >= h->hdr.length)
        return;
    rsp->printf(",\r\n"
                "    \"x_pm1a_evt_blk\"       : \"%u:%u:%u:%u:0x%016lX\",\r\n",
                GEN_ADDR(h->x_pm1a_evt_blk));

    if (offsetof(kernel::fadt_table,x_pm1b_evt_blk) >= h->hdr.length)
        return;
    rsp->printf(",\r\n"
                "    \"x_pm1b_evt_blk\"       : \"%u:%u:%u:%u:0x%016lX\",\r\n",
                GEN_ADDR(h->x_pm1b_evt_blk));

    if (offsetof(kernel::fadt_table,x_pm1a_cnt_blk) >= h->hdr.length)
        return;
    rsp->printf(",\r\n"
                "    \"x_pm1a_cnt_blk\"       : \"%u:%u:%u:%u:0x%016lX\",\r\n",
                GEN_ADDR(h->x_pm1a_cnt_blk));

    if (offsetof(kernel::fadt_table,x_pm1b_cnt_blk) >= h->hdr.length)
        return;
    rsp->printf(",\r\n"
                "    \"x_pm1b_cnt_blk\"       : \"%u:%u:%u:%u:0x%016lX\",\r\n",
                GEN_ADDR(h->x_pm1b_cnt_blk));

    if (offsetof(kernel::fadt_table,x_pm2_cnt_blk) >= h->hdr.length)
        return;
    rsp->printf(",\r\n"
                "    \"x_pm2_cnt_blk\"        : \"%u:%u:%u:%u:0x%016lX\",\r\n",
                GEN_ADDR(h->x_pm2_cnt_blk));

    if (offsetof(kernel::fadt_table,x_pm_tmr_blk) >= h->hdr.length)
        return;
    rsp->printf(",\r\n"
                "    \"x_pm_tmr_blk\"         : \"%u:%u:%u:%u:0x%016lX\",\r\n",
                GEN_ADDR(h->x_pm_tmr_blk));

    if (offsetof(kernel::fadt_table,x_gpe0_blk) >= h->hdr.length)
        return;
    rsp->printf(",\r\n"
                "    \"x_gpe0_blk\"           : \"%u:%u:%u:%u:0x%016lX\",\r\n",
                GEN_ADDR(h->x_gpe0_blk));

    if (offsetof(kernel::fadt_table,x_gpe1_blk) >= h->hdr.length)
        return;
    rsp->printf(",\r\n"
                "    \"x_gpe1_blk\"           : \"%u:%u:%u:%u:0x%016lX\",\r\n",
                GEN_ADDR(h->x_gpe1_blk));

    if (offsetof(kernel::fadt_table,sleep_control_reg) >= h->hdr.length)
        return;
    rsp->printf(",\r\n"
                "    \"sleep_control_reg\"    : \"%u:%u:%u:%u:0x%016lX\",\r\n",
                GEN_ADDR(h->sleep_control_reg));

    if (offsetof(kernel::fadt_table,sleep_status_reg) >= h->hdr.length)
        return;
    rsp->printf(",\r\n"
                "    \"sleep_status_reg\"     : \"%u:%u:%u:%u:0x%016lX\",\r\n",
                GEN_ADDR(h->sleep_status_reg));
}

static void
mcfg_request(const kernel::sdt_header* _h, const char* name,
    http::response* rsp)
{
    auto* h = (const kernel::mcfg_table*)_h;
    if (h->hdr.length < sizeof(*h))
        return;

    size_t entries_len = h->hdr.length - sizeof(*h);
    size_t nentries    = entries_len/sizeof(kernel::mcfg_entry);
    rsp->printf(",\r\n"
                "    \"entries\" = [");
    for (size_t i=0; i<nentries; ++i)
    {
        auto* e = &h->entries[i];
        rsp->printf("\r\n"
                    "        { \"addr\"      : \"0x%016lX\",\r\n"
                    "          \"domain\"    : \"0x%04X\",\r\n"
                    "          \"first_bus\" : \"0x%02X\",\r\n"
                    "          \"last_bus\"  : \"0x%02X\" },",
                    e->addr,e->domain,e->start_bus_num,e->end_bus_num);
    }
    rsp->ks.shrink();
    rsp->printf("\r\n    ]");
}

static void
madt_request(const kernel::sdt_header* _h, const char* name,
    http::response* rsp)
{
    auto* h = (const kernel::madt_table*)_h;
    rsp->printf(",\r\n"
                "    \"local_apic_addr\"      : \"0x%08X\",\r\n"
                "    \"flags\"                : \"0x%08X\",\r\n"
                "    \"records\"              : [ ",
                h->local_apic_addr,h->flags);
    for (auto& r : *h)
    {
        rsp->printf("\r\n        { \"type\" : \"0x%02X/",r.type);
        switch (r.type)
        {
            case kernel::MADT_TYPE_LAPIC:
                rsp->printf("LAPIC\", "
                            "\"acpi_processor_id\" : \"0x%02X\", "
                            "\"apic_id\" : \"0x%02X\", "
                            "\"flags\" : \"0x%08X\" ",
                            r.type0.acpi_processor_id,r.type0.apic_id,
                            r.type0.flags);
            break;

            case kernel::MADT_TYPE_IOAPIC:
                rsp->printf("IOAPIC\", "
                            "\"io_apic_id\" : \"0x%02X\", "
                            "\"io_apic_addr\" : \"0x%08X\", "
                            "\"interrupt_base\" : \"0x%08X\" ",
                            r.type1.io_apic_id,r.type1.io_apic_addr,
                            r.type1.interrupt_base);
            break;

            case kernel::MADT_TYPE_INTERRUPT_OVERRIDE:
                rsp->printf("INTERRUPT_OVERRIDE\", "
                            "\"bus_source\" : \"0x%02X\", "
                            "\"irq_source\" : \"0x%02X\", "
                            "\"interrupt_num\" : \"0x%08X\", "
                            "\"flags\" : \"0x%04X\" ",
                            r.type2.bus_source,r.type2.irq_source,
                            r.type2.interrupt_num,r.type2.flags);
            break;

            case kernel::MADT_TYPE_LAPIC_NMI:
                rsp->printf("LAPIC_NMI\", "
                            "\"processor\" : \"0x%02X\", "
                            "\"flags\" : \"0x%04X\", "
                            "\"lint_num\" : \"0x%02X\" ",
                            r.type4.processor,r.type4.flags,r.type4.lint_num);
            break;

            case kernel::MADT_TYPE_LAPIC_ADDRESS_OVERRIDE:
                rsp->printf("LAPIC_ADDRESS_OVERRIDE\", "
                            "\"local_apic_addr\" : \"0x%016lX\" ",
                            r.type5.local_apic_addr);
            break;

            default:
                rsp->printf("???\", "
                            "\"length\" : \"0x%02X\" ",r.len);
            break;
        }
        rsp->printf("},");
    }
    rsp->ks.shrink();
    rsp->printf("\r\n    ]");
}

void
sdt_node::handle_request(wapi::node* node, http::request* req,
    json::object* obj, http::response* rsp)
{
    // GET /acpi/XXXX
    rsp->printf("{\r\n");

    char name[5];
    get_acpi_table_name(h->signature,name);
    sdt_request(h,name,rsp);
    if (!strcmp(name,"FADT"))
        fadt_request(h,name,rsp);
    else if (!strcmp(name,"MCFG"))
        mcfg_request(h,name,rsp);
    else if (!strcmp(name,"MADT"))
        madt_request(h,name,rsp);

    rsp->printf("\r\n}\r\n");
}

wapi::node kernel::acpi_node(func_delegate(acpi_request),METHOD_GET_MASK,
                             "acpi");
static wapi::node rsdp_node(func_delegate(rsdp_request),METHOD_GET_MASK,"RSDP");
static kernel::obj_list<sdt_node> acpi_sdt_nodes;

void
kernel::init_acpi_wapi()
{
    acpi_node.register_child(&rsdp_node);
    for (auto* h : kernel::acpi_sdts)
    {
        char name[5];
        get_acpi_table_name(h->signature,name);
        auto iter = acpi_sdt_nodes.emplace_back(h,name);
        acpi_node.register_child(&(*iter));
    }

    wapi::root_node.register_child(&kernel::acpi_node);
}
