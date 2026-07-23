CC		:= i686-elf-gcc
AS		:= i686-elf-as

SRCDIR	:= src
INCLUDE	:= include
BUILD	:= build

CFLAGS	:= -std=gnu99 -ffreestanding -fno-builtin -fno-stack-protector -O2 \
		   -Wall -Wextra -Werror -I$(INCLUDE) -MMD -MP
LDFLAGS	:= -ffreestanding -O2 -nostdlib

KERNEL	:= kfs
ISO		:= kfs.iso
ISODIR	:= isodir
GRUBCFG	:= grub.cfg

SSRCS	:=	$(SRCDIR)/boot.s \
			$(SRCDIR)/gdt.s

CSRCS	:=	$(SRCDIR)/kernel.c \
			\
			$(SRCDIR)/drivers/vga.c \
			$(SRCDIR)/drivers/terminal.c \
			$(SRCDIR)/drivers/ps2.c \
			$(SRCDIR)/drivers/keyboard.c \
			$(SRCDIR)/drivers/kbd_buffer.c \
			\
			$(SRCDIR)/libk/printk.c \
			$(SRCDIR)/libk/strlen.c \
			$(SRCDIR)/libk/strcmp.c \
			$(SRCDIR)/libk/memset.c \
			$(SRCDIR)/libk/memcpy.c \
			$(SRCDIR)/libk/memmove.c \
			\
			$(SRCDIR)/debug/hexdump.c \
			$(SRCDIR)/debug/stack.c \
			\
			$(SRCDIR)/shell/shell_loop.c \
			$(SRCDIR)/shell/commands.c

# Objects mirror the source tree under build/, so the sources stay clean and
# `make clean` is a single rm -rf.  -MMD -MP writes a .d beside each .o so a
# header edit rebuilds every file that includes it.
OBJS	:= $(SSRCS:%.s=$(BUILD)/%.o) $(CSRCS:%.c=$(BUILD)/%.o)
DEPS	:= $(OBJS:.o=.d)

all: $(ISO)

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS)

$(BUILD)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(CC) -T linker.ld -o $(KERNEL) $(LDFLAGS) $(OBJS) -lgcc

$(ISO): $(KERNEL) $(GRUBCFG)
	@grub-file --is-x86-multiboot $(KERNEL) \
		|| { echo "ERROR: $(KERNEL) has no valid multiboot header — check src/boot.s / linker.ld"; exit 1; }
	mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL) $(ISODIR)/boot/$(KERNEL)
	cp $(GRUBCFG) $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) $(ISODIR) --install-modules="multiboot normal iso9660 biosdisk configfile" \
	--locales="" --fonts="" --themes=""

clean:
	rm -rf $(BUILD) $(ISODIR)

fclean: clean
	rm -rf $(KERNEL) $(ISO)

re: fclean all

-include $(DEPS)

.PHONY: all clean fclean re
