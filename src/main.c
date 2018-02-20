#include <compiler.h>
#include <io.h>
#include <interrupts.h>
#include <string.h>
#include <stdio.h>
#include <multiboot2.h>

static int multiboot2_valid(u32 magic, u32 info_addr)
{
	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
		printf("Please use a multiboot2 compliant bootloader\n");
		return 0;
	}

	if (__align(info_addr, MULTIBOOT_INFO_ALIGN) != info_addr) {
		printf("Unaligned MBI: %#x\n", info_addr);
		return 0;
	}

	return 1;
}

static void *get_multiboot_infos(u32 info_addr, u8 tag_type)
{
	struct multiboot_tag *tag = (struct multiboot_tag *)(info_addr + 8);

	while (tag->type != MULTIBOOT_TAG_TYPE_END) {
		if (tag->type == tag_type)
			return tag;
		u32 size = __align_n(tag->size, MULTIBOOT_INFO_ALIGN);
		tag = (void *)((u8 *)tag + size);
	}

	return NULL;
}

static const char *multiboot_mmap_entry_types[] = {
	[1] = "AVAILABLE",
	[2] = "RESERVED",
	[3] = "ACPI_RECLAIMABLE",
	[4] = "NVS",
	[5] = "BADRAM",
};

static void dump_memory_map(struct multiboot_tag_mmap *mmap)
{
	multiboot_memory_map_t *m = mmap->entries;
	while ((u8 *)m < (u8 *)mmap + mmap->size) {
		printf("base_addr=0x%x%x, length=0x%x%x, type=%s\n",
				m->addr >> 32,
				m->addr & 0xffffffff,
				m->len >> 32,
				m->len & 0xffffffff,
				multiboot_mmap_entry_types[m->type]);

		m = (multiboot_memory_map_t *)((u8 *)m + mmap->entry_size);
	}
}

void hyper_main(u32 magic, u32 info_addr)
{
	if (!multiboot2_valid(magic, info_addr))
		return;

	struct multiboot_tag_string *c = get_multiboot_infos(info_addr,
				  MULTIBOOT_TAG_TYPE_CMDLINE);
	if (c)
		printf("Commandline: %s\n", c->string);

	struct multiboot_tag_mmap *mmap = get_multiboot_infos(info_addr,
				MULTIBOOT_TAG_TYPE_MMAP);
	if (mmap)
		dump_memory_map(mmap);

	init_idt();
	asm volatile ("int $0");

	char *buf = (void *)0xb8000;
	char *star = "|/-\\";
	for (u32 i = 0; ; i++)
		*buf = star[i++ % 4];

	for (;;)
		asm volatile ("hlt");
}
