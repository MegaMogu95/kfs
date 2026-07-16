CC      := i686-elf-gcc
AS      := i686-elf-as

CFLAGS  := -std=gnu99 -ffreestanding -O2 -Wall -Wextra -fno-builtin -fno-stack-protector -g
LDFLAGS := -ffreestanding -O2 -nostdlib

KERNEL  := kfs
ISO     := kfs.iso
OBJS    := boot.o kernel.o
ISODIR  := isodir
GRUBCFG := grub.cfg

all: $(ISO)

boot.o: boot.s
	$(AS) boot.s -o boot.o

kernel.o: kernel.c
	$(CC) -c kernel.c -o kernel.o $(CFLAGS)

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

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

run-kernel: $(KERNEL)
	qemu-system-i386 -kernel $(KERNEL)

clean:
	rm -rf $(OBJS) $(ISODIR)

fclean: clean
	rm -rf $(KERNEL) $(ISO)

re: fclean all

.PHONY: all run run-kernel clean fclean re
