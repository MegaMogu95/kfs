CC		:= i686-elf-gcc
AS		:= i686-elf-as

CFLAGS	:= -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-builtin -fno-stack-protector -g
LDFLAGS	:= -ffreestanding -O2 -nostdlib
INCLUDE	:= include

KERNEL	:= kfs
ISO		:= kfs.iso
ISODIR	:= isodir
GRUBCFG	:= grub.cfg

CSRCS	:= kernel.c terminal/vga.c terminal/terminal.c utils/string.c utils/printk.c keyboard/ps2.c keyboard/keyboard.c keyboard/kbd_buffer.c
SSRCS	:= boot.s GDT/gdt.s
OBJS    := $(SSRCS:.s=.o) $(CSRCS:.c=.o)

all: $(ISO)

%.o: %.s
	$(AS) $< -o $@

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS) -I$(INCLUDE)

$(KERNEL): $(OBJS) linker.ld
	$(CC) -T linker.ld -o $(KERNEL) $(LDFLAGS) $(OBJS) -lgcc

$(ISO): $(KERNEL) $(GRUBCFG)
	@grub-file --is-x86-multiboot $(KERNEL) \
		|| { echo "ERROR: $(KERNEL) has no valid multiboot header — check boot.s / linker.ld"; exit 1; }
	mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL) $(ISODIR)/boot/$(KERNEL)
	cp $(GRUBCFG) $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISODIR) --install-modules="multiboot normal iso9660 biosdisk configfile" \
	--locales="" --fonts="" --themes=""

clean:
	rm -rf $(OBJS) $(ISODIR)

fclean: clean
	rm -rf $(KERNEL) $(ISO)

re: fclean all

.PHONY: all clean fclean re
