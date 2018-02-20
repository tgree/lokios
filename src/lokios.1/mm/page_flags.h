#ifndef __KERNEL_PAGE_FLAGS_H
#define __KERNEL_PAGE_FLAGS_H

    // The PAT, PCD and PWT bits select an entry in the page attribute table.
    // The lokios bootloader reprograms the PAT MSR so that the PAT table is:
    //       0  1  2  3  4  5  6  7
    //      WB WT WC UC WB WT WC UC
    // In particular, since this configuration is repeated in both halves of
    // the MSR it means that the PAT bit in the page tables is "don't care".
    // We force it to 0 for 2M and 1G entries and force it to 1 for 4K entries
    // which has two effects:
    //  1. The physical address mask becomes consistent across all page table
    //     levels.
    //  2. Bit 7 can now be used at the 4K level to also indicate that no child
    //     node is present.  This is useful because we no longer need to
    //     special-case 4K pages.

    // These are all the user-visible flags.
#define PAGE_FLAG_NOEXEC        0x8000000000000000UL
#define PAGE_LEVEL_MASK         0x3000000000000000UL
#define PAGE_PADDR_MASK         0x000000FFFFFFF000UL
#define PAGE_FLAG_USER_PAGE     0x0000000000000080UL
#define PAGE_CACHE_MASK         0x000000000000000CUL
#define PAGE_CACHE_WB           0x0000000000000000UL
#define PAGE_CACHE_WT           0x0000000000000004UL
#define PAGE_CACHE_WC           0x0000000000000008UL
#define PAGE_CACHE_UC           0x000000000000000CUL
#define PAGE_FLAG_WRITEABLE     0x0000000000000002UL
#define PAGE_FLAG_PRESENT       0x0000000000000001UL

    // Flags that the user can modify; any other bits present when trying to
    // map a page are ignored.
#define PAGE_USER_FLAGS \
    (PAGE_FLAG_WRITEABLE | PAGE_FLAG_NOEXEC | PAGE_CACHE_MASK)

    // Flags to set for a data RAM mapping.
#define PAGE_FLAGS_DATA \
    (PAGE_FLAG_WRITEABLE | PAGE_FLAG_NOEXEC | PAGE_CACHE_WB)
#define PAGE_FLAGS_DATA_RO \
    (PAGE_FLAG_NOEXEC | PAGE_CACHE_WB)

    // Flags to set for an IO mapping.
#define PAGE_FLAGS_IO \
    (PAGE_FLAG_WRITEABLE | PAGE_FLAG_NOEXEC | PAGE_CACHE_UC)

    // Flags that are set when a page is mapped; excludes bits that the CPU
    // will update if a page is accessed or modified.
#define PAGE_KERNEL_FLAGS \
    (PAGE_FLAG_NOEXEC | PAGE_LEVEL_MASK | PAGE_PADDR_MASK | \
     PAGE_FLAG_USER_PAGE | PAGE_CACHE_MASK | PAGE_FLAG_WRITEABLE | \
     PAGE_FLAG_PRESENT)

    // Given a PTE, extract its page table level (and hence it's page size).
#define PTE_TO_LEVEL(pte)   (((pte) & PAGE_LEVEL_MASK) >> 60)

    // Number of levels in the page table.
#define PAGE_TABLE_HEIGHT   4

#endif /* __KERNEL_PAGE_FLAGS_H */
