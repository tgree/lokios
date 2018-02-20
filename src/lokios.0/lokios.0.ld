EXTERN(_start)
SECTIONS
{
    /* PXE loads us into memory at 0x7c00, so link ourselves at that base
     * address. */
    . = 0x7c00;
    .bootsector_text :
    {
        *bootsector.a:entry.o(.text)
        *bootsector.a:*(*text)
    }
    .bootsector_data :
    {
        *timestamp.o(.data)
        *bootsector.a:*(.data)
    }
    ASSERT(. <= 0x7DB4, "boot sector crashed into disk ID")

    /* Include a valid MBR signature for booting off disk. */
    . = 0x7dfe;
    .mbr_sig :
    {
        BYTE(0x55)
        BYTE(0xAA)
    }

    /* Space for 0x7E00-0x7FFF is currently unused. */

    /* That's the end of the boot sector.  If we PXE-booted the rest of the
     * code is already in memory.  If we MBR-booted then the bootsector has
     * gone and loaded the rest of the code into memory.  In either case, they
     * will then branch into code at or above 0x8000.  We align the _smp_entry
     * routine at 0x8000 because the MP SIPI vector requires page alignment and
     * we want this to all be mapped read-only by the time lokios.1 is up and
     * running.  Due to alignment of other sections such as page table pages,
     * bumping the .text base address from 0x7E00 to 0x8000 doesn't actually
     * increase the size of the image. */
    . = 0x8000;
    .text :
    {
        *smp_entry.o(.text)
        *(.text)
    }
    .data :
    {
        *(.data)
    } = 0xCC

    /* Provide a global descriptor table to use for the swtich to 16-bit
     * unreal mode. */
    . = ALIGN(64);
    .gdt_unreal_16 :
    {
        _gdt_unreal_16 = .;
        QUAD(0x0000000000000000);
        QUAD(0x00CF92000000FFFF);

        _gdt_unreal_16_desc = .;
        SHORT(_gdt_unreal_16_desc - _gdt_unreal_16 - 1);
        LONG(_gdt_unreal_16);
    }

    /* Provide a global descriptor table to use for the switch to 64-bit
     * mode. */
    . = ALIGN(64);
    .gdt_64 :
    {
        _gdt_64 = .;
        QUAD(0x0000000000000000);
        QUAD(0x00209A0000000000);   /* Code descriptor */
        QUAD(0x0000920000000000);   /* Data descriptor */

        _gdt_64_desc = .;
        SHORT(_gdt_64_desc - _gdt_64 - 1);
        LONG(_gdt_64);
    }

    /* Provide an empty interrupt descriptor table.  TODO: Figure out what to
     * really put here. */
    . = ALIGN(64);
    .idt_64 :
    {
        _idt_64 = .;
        BYTE(0);
        _idt_64_desc = .;
        SHORT(_idt_64_desc - _idt_64 - 1);
        LONG(_idt_64);
    }

    /* Provide at least 4096 bytes to hold the E820 map.  This should be more
     * than enough (I hope).  Before we've loaded the E820 map, this region of
     * memory can also be used for a disk IO bounce buffer. */
    .e820_map :
    {
        _pre_e820_bounce_buffer = .;
        _e820_end = .;
        SHORT(0xFFFF);
        _e820_map = .;
        . += 4096;
        . = ALIGN(4096);
    } = 0xDD
    _e820_last_max_entry = . - 24;

    /* Provide a page table that identity-maps the first 64M of memory via 4K
     * and 2M pages. */
    .page_table :
    {
        _page_table = .;
        ASSERT(_page_table == ALIGN(4096), "page table misaligned");
        QUAD((_page_table + 0x00001000) | 0x0000000000000003);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD(0);
        QUAD((_page_table + 0x00001000) | 0x0000000000000003);

        . = ALIGN(4096);
        QUAD((_page_table + 0x00002000) | 0x1000000000000003);
        . = ALIGN(4096);
        QUAD((_page_table + 0x00003000) | 0x2000000000000003);
        QUAD(0x2000000000200083);
        QUAD(0x2000000000400083);
        QUAD(0x2000000000600083);
        . = ALIGN(4096);
        QUAD(0x3000000000000081);
        QUAD(0x3000000000001081);
        QUAD(0x3000000000002081);
        QUAD(0x3000000000003081);
        QUAD(0x3000000000004081);
        QUAD(0x3000000000005081);
        QUAD(0x3000000000006081);
        QUAD(0x3000000000007081);
        QUAD(0x3000000000008081);
        QUAD(0x3000000000009081);
        QUAD(0x300000000000A081);
        QUAD(0x300000000000B081);
        QUAD(0x300000000000C081);
        QUAD(0x300000000000D081);
        QUAD(0x300000000000E081);
        QUAD(0x300000000000F081);
        QUAD(0x3000000000010081);
        QUAD(0x3000000000011081);
        QUAD(0x3000000000012081);
        QUAD(0x3000000000013081);
        QUAD(0x3000000000014081);
        QUAD(0x3000000000015081);
        QUAD(0x3000000000016081);
        QUAD(0x3000000000017081);
        QUAD(0x3000000000018081);
        QUAD(0x3000000000019081);
        QUAD(0x300000000001A081);
        QUAD(0x300000000001B081);
        QUAD(0x300000000001C081);
        QUAD(0x300000000001D081);
        QUAD(0x300000000001E081);
        QUAD(0x300000000001F081);
        QUAD(0x3000000000020081);
        QUAD(0x3000000000021081);
        QUAD(0x3000000000022081);
        QUAD(0x3000000000023081);
        QUAD(0x3000000000024081);
        QUAD(0x3000000000025081);
        QUAD(0x3000000000026081);
        QUAD(0x3000000000027081);
        QUAD(0x3000000000028081);
        QUAD(0x3000000000029081);
        QUAD(0x300000000002A081);
        QUAD(0x300000000002B081);
        QUAD(0x300000000002C081);
        QUAD(0x300000000002D081);
        QUAD(0x300000000002E081);
        QUAD(0x300000000002F081);
        QUAD(0x3000000000030081);
        QUAD(0x3000000000031081);
        QUAD(0x3000000000032081);
        QUAD(0x3000000000033081);
        QUAD(0x3000000000034081);
        QUAD(0x3000000000035081);
        QUAD(0x3000000000036081);
        QUAD(0x3000000000037081);
        QUAD(0x3000000000038081);
        QUAD(0x3000000000039081);
        QUAD(0x300000000003A081);
        QUAD(0x300000000003B081);
        QUAD(0x300000000003C081);
        QUAD(0x300000000003D081);
        QUAD(0x300000000003E081);
        QUAD(0x300000000003F081);
        QUAD(0x3000000000040081);
        QUAD(0x3000000000041081);
        QUAD(0x3000000000042081);
        QUAD(0x3000000000043081);
        QUAD(0x3000000000044081);
        QUAD(0x3000000000045081);
        QUAD(0x3000000000046081);
        QUAD(0x3000000000047081);
        QUAD(0x3000000000048081);
        QUAD(0x3000000000049081);
        QUAD(0x300000000004A081);
        QUAD(0x300000000004B081);
        QUAD(0x300000000004C081);
        QUAD(0x300000000004D081);
        QUAD(0x300000000004E081);
        QUAD(0x300000000004F081);
        QUAD(0x3000000000050081);
        QUAD(0x3000000000051081);
        QUAD(0x3000000000052081);
        QUAD(0x3000000000053081);
        QUAD(0x3000000000054081);
        QUAD(0x3000000000055081);
        QUAD(0x3000000000056081);
        QUAD(0x3000000000057081);
        QUAD(0x3000000000058081);
        QUAD(0x3000000000059081);
        QUAD(0x300000000005A081);
        QUAD(0x300000000005B081);
        QUAD(0x300000000005C081);
        QUAD(0x300000000005D081);
        QUAD(0x300000000005E081);
        QUAD(0x300000000005F081);
        QUAD(0x3000000000060081);
        QUAD(0x3000000000061081);
        QUAD(0x3000000000062081);
        QUAD(0x3000000000063081);
        QUAD(0x3000000000064081);
        QUAD(0x3000000000065081);
        QUAD(0x3000000000066081);
        QUAD(0x3000000000067081);
        QUAD(0x3000000000068081);
        QUAD(0x3000000000069081);
        QUAD(0x300000000006A081);
        QUAD(0x300000000006B081);
        QUAD(0x300000000006C081);
        QUAD(0x300000000006D081);
        QUAD(0x300000000006E081);
        QUAD(0x300000000006F081);
        QUAD(0x3000000000070081);
        QUAD(0x3000000000071081);
        QUAD(0x3000000000072081);
        QUAD(0x3000000000073081);
        QUAD(0x3000000000074081);
        QUAD(0x3000000000075081);
        QUAD(0x3000000000076081);
        QUAD(0x3000000000077081);
        QUAD(0x3000000000078081);
        QUAD(0x3000000000079081);
        QUAD(0x300000000007A081);
        QUAD(0x300000000007B081);
        QUAD(0x300000000007C081);
        QUAD(0x300000000007D081);
        QUAD(0x300000000007E081);
        QUAD(0x300000000007F081);
        QUAD(0x3000000000080081);
        QUAD(0x3000000000081081);
        QUAD(0x3000000000082081);
        QUAD(0x3000000000083081);
        QUAD(0x3000000000084081);
        QUAD(0x3000000000085081);
        QUAD(0x3000000000086081);
        QUAD(0x3000000000087081);
        QUAD(0x3000000000088081);
        QUAD(0x3000000000089081);
        QUAD(0x300000000008A081);
        QUAD(0x300000000008B081);
        QUAD(0x300000000008C081);
        QUAD(0x300000000008D081);
        QUAD(0x300000000008E081);
        QUAD(0x300000000008F081);
        QUAD(0x3000000000090081);
        QUAD(0x3000000000091081);
        QUAD(0x3000000000092081);
        QUAD(0x3000000000093081);
        QUAD(0x3000000000094081);
        QUAD(0x3000000000095081);
        QUAD(0x3000000000096081);
        QUAD(0x3000000000097081);
        QUAD(0x3000000000098081);
        QUAD(0x3000000000099081);
        QUAD(0x300000000009A081);
        QUAD(0x300000000009B081);
        QUAD(0x300000000009C081);
        QUAD(0x300000000009D081);
        QUAD(0x300000000009E081);
        QUAD(0x300000000009F081);
        QUAD(0x30000000000A0081);
        QUAD(0x30000000000A1081);
        QUAD(0x30000000000A2081);
        QUAD(0x30000000000A3081);
        QUAD(0x30000000000A4081);
        QUAD(0x30000000000A5081);
        QUAD(0x30000000000A6081);
        QUAD(0x30000000000A7081);
        QUAD(0x30000000000A8081);
        QUAD(0x30000000000A9081);
        QUAD(0x30000000000AA081);
        QUAD(0x30000000000AB081);
        QUAD(0x30000000000AC081);
        QUAD(0x30000000000AD081);
        QUAD(0x30000000000AE081);
        QUAD(0x30000000000AF081);
        QUAD(0x30000000000B0081);
        QUAD(0x30000000000B1081);
        QUAD(0x30000000000B2081);
        QUAD(0x30000000000B3081);
        QUAD(0x30000000000B4081);
        QUAD(0x30000000000B5081);
        QUAD(0x30000000000B6081);
        QUAD(0x30000000000B7081);
        QUAD(0xB0000000000B8087); /* VRAM */
        QUAD(0xB0000000000B9087); /* VRAM */
        QUAD(0xB0000000000BA087); /* VRAM */
        QUAD(0xB0000000000BB087); /* VRAM */
        QUAD(0xB0000000000BC087); /* VRAM */
        QUAD(0xB0000000000BD087); /* VRAM */
        QUAD(0xB0000000000BE087); /* VRAM */
        QUAD(0xB0000000000BF087); /* VRAM */
        QUAD(0x30000000000C0081);
        QUAD(0x30000000000C1081);
        QUAD(0x30000000000C2081);
        QUAD(0x30000000000C3081);
        QUAD(0x30000000000C4081);
        QUAD(0x30000000000C5081);
        QUAD(0x30000000000C6081);
        QUAD(0x30000000000C7081);
        QUAD(0x30000000000C8081);
        QUAD(0x30000000000C9081);
        QUAD(0x30000000000CA081);
        QUAD(0x30000000000CB081);
        QUAD(0x30000000000CC081);
        QUAD(0x30000000000CD081);
        QUAD(0x30000000000CE081);
        QUAD(0x30000000000CF081);
        QUAD(0x30000000000D0081);
        QUAD(0x30000000000D1081);
        QUAD(0x30000000000D2081);
        QUAD(0x30000000000D3081);
        QUAD(0x30000000000D4081);
        QUAD(0x30000000000D5081);
        QUAD(0x30000000000D6081);
        QUAD(0x30000000000D7081);
        QUAD(0x30000000000D8081);
        QUAD(0x30000000000D9081);
        QUAD(0x30000000000DA081);
        QUAD(0x30000000000DB081);
        QUAD(0x30000000000DC081);
        QUAD(0x30000000000DD081);
        QUAD(0x30000000000DE081);
        QUAD(0x30000000000DF081);
        QUAD(0x30000000000E0081);
        QUAD(0x30000000000E1081);
        QUAD(0x30000000000E2081);
        QUAD(0x30000000000E3081);
        QUAD(0x30000000000E4081);
        QUAD(0x30000000000E5081);
        QUAD(0x30000000000E6081);
        QUAD(0x30000000000E7081);
        QUAD(0x30000000000E8081);
        QUAD(0x30000000000E9081);
        QUAD(0x30000000000EA081);
        QUAD(0x30000000000EB081);
        QUAD(0x30000000000EC081);
        QUAD(0x30000000000ED081);
        QUAD(0x30000000000EE081);
        QUAD(0x30000000000EF081);
        QUAD(0x30000000000F0081);
        QUAD(0x30000000000F1081);
        QUAD(0x30000000000F2081);
        QUAD(0x30000000000F3081);
        QUAD(0x30000000000F4081);
        QUAD(0x30000000000F5081);
        QUAD(0x30000000000F6081);
        QUAD(0x30000000000F7081);
        QUAD(0x30000000000F8081);
        QUAD(0x30000000000F9081);
        QUAD(0x30000000000FA081);
        QUAD(0x30000000000FB081);
        QUAD(0x30000000000FC081);
        QUAD(0x30000000000FD081);
        QUAD(0x30000000000FE081);
        QUAD(0x30000000000FF081);
        QUAD(0x3000000000100081);
        QUAD(0x3000000000101081);
        QUAD(0x3000000000102081);
        QUAD(0x3000000000103081);
        QUAD(0x3000000000104081);
        QUAD(0x3000000000105081);
        QUAD(0x3000000000106081);
        QUAD(0x3000000000107081);
        QUAD(0x3000000000108081);
        QUAD(0x3000000000109081);
        QUAD(0x300000000010A081);
        QUAD(0x300000000010B081);
        QUAD(0x300000000010C081);
        QUAD(0x300000000010D081);
        QUAD(0x300000000010E081);
        QUAD(0x300000000010F081);
        QUAD(0x3000000000110081);
        QUAD(0x3000000000111081);
        QUAD(0x3000000000112081);
        QUAD(0x3000000000113081);
        QUAD(0x3000000000114081);
        QUAD(0x3000000000115081);
        QUAD(0x3000000000116081);
        QUAD(0x3000000000117081);
        QUAD(0x3000000000118081);
        QUAD(0x3000000000119081);
        QUAD(0x300000000011A081);
        QUAD(0x300000000011B081);
        QUAD(0x300000000011C081);
        QUAD(0x300000000011D081);
        QUAD(0x300000000011E081);
        QUAD(0x300000000011F081);
        QUAD(0x3000000000120081);
        QUAD(0x3000000000121081);
        QUAD(0x3000000000122081);
        QUAD(0x3000000000123081);
        QUAD(0x3000000000124081);
        QUAD(0x3000000000125081);
        QUAD(0x3000000000126081);
        QUAD(0x3000000000127081);
        QUAD(0x3000000000128081);
        QUAD(0x3000000000129081);
        QUAD(0x300000000012A081);
        QUAD(0x300000000012B081);
        QUAD(0x300000000012C081);
        QUAD(0x300000000012D081);
        QUAD(0x300000000012E081);
        QUAD(0x300000000012F081);
        QUAD(0x3000000000130081);
        QUAD(0x3000000000131081);
        QUAD(0x3000000000132081);
        QUAD(0x3000000000133081);
        QUAD(0x3000000000134081);
        QUAD(0x3000000000135081);
        QUAD(0x3000000000136081);
        QUAD(0x3000000000137081);
        QUAD(0x3000000000138081);
        QUAD(0x3000000000139081);
        QUAD(0x300000000013A081);
        QUAD(0x300000000013B081);
        QUAD(0x300000000013C081);
        QUAD(0x300000000013D081);
        QUAD(0x300000000013E081);
        QUAD(0x300000000013F081);
        QUAD(0x3000000000140081);
        QUAD(0x3000000000141081);
        QUAD(0x3000000000142081);
        QUAD(0x3000000000143081);
        QUAD(0x3000000000144081);
        QUAD(0x3000000000145081);
        QUAD(0x3000000000146081);
        QUAD(0x3000000000147081);
        QUAD(0x3000000000148081);
        QUAD(0x3000000000149081);
        QUAD(0x300000000014A081);
        QUAD(0x300000000014B081);
        QUAD(0x300000000014C081);
        QUAD(0x300000000014D081);
        QUAD(0x300000000014E081);
        QUAD(0x300000000014F081);
        QUAD(0x3000000000150081);
        QUAD(0x3000000000151081);
        QUAD(0x3000000000152081);
        QUAD(0x3000000000153081);
        QUAD(0x3000000000154081);
        QUAD(0x3000000000155081);
        QUAD(0x3000000000156081);
        QUAD(0x3000000000157081);
        QUAD(0x3000000000158081);
        QUAD(0x3000000000159081);
        QUAD(0x300000000015A081);
        QUAD(0x300000000015B081);
        QUAD(0x300000000015C081);
        QUAD(0x300000000015D081);
        QUAD(0x300000000015E081);
        QUAD(0x300000000015F081);
        QUAD(0x3000000000160081);
        QUAD(0x3000000000161081);
        QUAD(0x3000000000162081);
        QUAD(0x3000000000163081);
        QUAD(0x3000000000164081);
        QUAD(0x3000000000165081);
        QUAD(0x3000000000166081);
        QUAD(0x3000000000167081);
        QUAD(0x3000000000168081);
        QUAD(0x3000000000169081);
        QUAD(0x300000000016A081);
        QUAD(0x300000000016B081);
        QUAD(0x300000000016C081);
        QUAD(0x300000000016D081);
        QUAD(0x300000000016E081);
        QUAD(0x300000000016F081);
        QUAD(0x3000000000170081);
        QUAD(0x3000000000171081);
        QUAD(0x3000000000172081);
        QUAD(0x3000000000173081);
        QUAD(0x3000000000174081);
        QUAD(0x3000000000175081);
        QUAD(0x3000000000176081);
        QUAD(0x3000000000177081);
        QUAD(0x3000000000178081);
        QUAD(0x3000000000179081);
        QUAD(0x300000000017A081);
        QUAD(0x300000000017B081);
        QUAD(0x300000000017C081);
        QUAD(0x300000000017D081);
        QUAD(0x300000000017E081);
        QUAD(0x300000000017F081);
        QUAD(0x3000000000180081);
        QUAD(0x3000000000181081);
        QUAD(0x3000000000182081);
        QUAD(0x3000000000183081);
        QUAD(0x3000000000184081);
        QUAD(0x3000000000185081);
        QUAD(0x3000000000186081);
        QUAD(0x3000000000187081);
        QUAD(0x3000000000188081);
        QUAD(0x3000000000189081);
        QUAD(0x300000000018A081);
        QUAD(0x300000000018B081);
        QUAD(0x300000000018C081);
        QUAD(0x300000000018D081);
        QUAD(0x300000000018E081);
        QUAD(0x300000000018F081);
        QUAD(0x3000000000190081);
        QUAD(0x3000000000191081);
        QUAD(0x3000000000192081);
        QUAD(0x3000000000193081);
        QUAD(0x3000000000194081);
        QUAD(0x3000000000195081);
        QUAD(0x3000000000196081);
        QUAD(0x3000000000197081);
        QUAD(0x3000000000198081);
        QUAD(0x3000000000199081);
        QUAD(0x300000000019A081);
        QUAD(0x300000000019B081);
        QUAD(0x300000000019C081);
        QUAD(0x300000000019D081);
        QUAD(0x300000000019E081);
        QUAD(0x300000000019F081);
        QUAD(0x30000000001A0081);
        QUAD(0x30000000001A1081);
        QUAD(0x30000000001A2081);
        QUAD(0x30000000001A3081);
        QUAD(0x30000000001A4081);
        QUAD(0x30000000001A5081);
        QUAD(0x30000000001A6081);
        QUAD(0x30000000001A7081);
        QUAD(0x30000000001A8081);
        QUAD(0x30000000001A9081);
        QUAD(0x30000000001AA081);
        QUAD(0x30000000001AB081);
        QUAD(0x30000000001AC081);
        QUAD(0x30000000001AD081);
        QUAD(0x30000000001AE081);
        QUAD(0x30000000001AF081);
        QUAD(0x30000000001B0081);
        QUAD(0x30000000001B1081);
        QUAD(0x30000000001B2081);
        QUAD(0x30000000001B3081);
        QUAD(0x30000000001B4081);
        QUAD(0x30000000001B5081);
        QUAD(0x30000000001B6081);
        QUAD(0x30000000001B7081);
        QUAD(0x30000000001B8081);
        QUAD(0x30000000001B9081);
        QUAD(0x30000000001BA081);
        QUAD(0x30000000001BB081);
        QUAD(0x30000000001BC081);
        QUAD(0x30000000001BD081);
        QUAD(0x30000000001BE081);
        QUAD(0x30000000001BF081);
        QUAD(0x30000000001C0081);
        QUAD(0x30000000001C1081);
        QUAD(0x30000000001C2081);
        QUAD(0x30000000001C3081);
        QUAD(0x30000000001C4081);
        QUAD(0x30000000001C5081);
        QUAD(0x30000000001C6081);
        QUAD(0x30000000001C7081);
        QUAD(0x30000000001C8081);
        QUAD(0x30000000001C9081);
        QUAD(0x30000000001CA081);
        QUAD(0x30000000001CB081);
        QUAD(0x30000000001CC081);
        QUAD(0x30000000001CD081);
        QUAD(0x30000000001CE081);
        QUAD(0x30000000001CF081);
        QUAD(0x30000000001D0081);
        QUAD(0x30000000001D1081);
        QUAD(0x30000000001D2081);
        QUAD(0x30000000001D3081);
        QUAD(0x30000000001D4081);
        QUAD(0x30000000001D5081);
        QUAD(0x30000000001D6081);
        QUAD(0x30000000001D7081);
        QUAD(0x30000000001D8081);
        QUAD(0x30000000001D9081);
        QUAD(0x30000000001DA081);
        QUAD(0x30000000001DB081);
        QUAD(0x30000000001DC081);
        QUAD(0x30000000001DD081);
        QUAD(0x30000000001DE081);
        QUAD(0x30000000001DF081);
        QUAD(0x30000000001E0081);
        QUAD(0x30000000001E1081);
        QUAD(0x30000000001E2081);
        QUAD(0x30000000001E3081);
        QUAD(0x30000000001E4081);
        QUAD(0x30000000001E5081);
        QUAD(0x30000000001E6081);
        QUAD(0x30000000001E7081);
        QUAD(0x30000000001E8081);
        QUAD(0x30000000001E9081);
        QUAD(0x30000000001EA081);
        QUAD(0x30000000001EB081);
        QUAD(0x30000000001EC081);
        QUAD(0x30000000001ED081);
        QUAD(0x30000000001EE081);
        QUAD(0x30000000001EF081);
        QUAD(0x30000000001F0081);   /* Guard page */
        QUAD(0xB0000000001F1083);   /* Kernel stack */
        QUAD(0xB0000000001F2083);   /* Kernel stack */
        QUAD(0xB0000000001F3083);   /* Kernel stack */
        QUAD(0xB0000000001F4083);   /* Kernel stack */
        QUAD(0xB0000000001F5083);   /* Kernel stack */
        QUAD(0xB0000000001F6083);   /* Kernel stack */
        QUAD(0xB0000000001F7083);   /* Kernel stack */
        QUAD(0xB0000000001F8083);   /* Kernel stack */
        QUAD(0xB0000000001F9083);   /* Kernel stack */
        QUAD(0xB0000000001FA083);   /* Kernel stack */
        QUAD(0xB0000000001FB083);   /* Kernel stack */
        QUAD(0xB0000000001FC083);   /* Kernel stack */
        QUAD(0xB0000000001FD083);   /* Kernel stack */
        QUAD(0xB0000000001FE083);   /* Kernel stack */
        QUAD(0x30000000001FF081);   /* Guard page - read only, holds kargs */
        . = ALIGN(4096);
    }

    /* We'll switch to a 3K stack from 0xF000 - 0xFC00. */
    _stack_bottom = 0xF000;
    _stack_top    = 0xFC00;

    /* Make sure we didn't crash into the stack. */
    ASSERT(. <= _stack_bottom, "image crashed into the low stack address")

    /* Compute the number of sectors the MBR loader needs to read. */
    _extra_sectors = (. - 0x7e00) / 512;

    /* This is where we are going to load the kernel. */
    _kernel_base = 0x00200000;
    _kernel_bsp_entry = _kernel_base + 0x10;
    _kernel_ap_entry = _kernel_base + 0x20;
    _kernel_stack = 0x001FF000;
    _kernel_params = _kernel_stack;
}
