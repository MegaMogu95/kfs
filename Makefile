CC      := i686-elf-gcc
AS      := i686-elf-as

CFLAGS  := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-builtin -fno-stack-protector -g
LDFLAGS := -ffreestanding -O2 -nostdlib

KERNEL  := kfs
ISO     := kfs.iso
ISODIR  := isodir
GRUBCFG := grub.cfg

SRCS    := kernel.c vga.c terminal.c string.c ps2.c keyboard.c kbd_buffer.c printk.c
OBJS    := boot.o $(SRCS:.c=.o)

all: $(ISO)

boot.o: boot.s
	$(AS) boot.s -o boot.o

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

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
