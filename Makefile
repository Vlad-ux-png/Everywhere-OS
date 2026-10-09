# Makefile for Everywhere OS
# Copyright (c) 2026  Everywhere Computing, Inc. All Rights Reserved.

CC      = gcc
LD      = ld
NASM    = nasm

HOST_CC = gcc

CFLAGS  = -c -ffreestanding -fno-builtin -fno-stack-protector -nostdlib \
          -m32 -Wall -Wextra \
          -I./src/minkernel/ntos/inc \
          -I./src/minkernel/ntos/mm \
          -I./src/minkernel/boot/kdcom \
          -I./src/shell/explorer \
          -I./src/minkernel/fs/evryfs \
          -I./src/onecore/drivers/input/keyboard/kbdclass \
          -I./src/onecore/drivers/input/mouse/mouclass

LDFLAGS = -m elf_i386 -T src/minkernel/ntos/init/kernel.ld
ASFLAGS = -f elf32

BUILD = obj/i386fre
BIN   = bin/i386fre/images
ISO   = iso
DISK_IMG   = $(BIN)/disk.img
FOLDER_ICO = src/shell/explorer/assets/folder.ico
MKDISK_SRC = src/tools/mkdisk.c
MKDISK_EXE = $(BUILD)/mkdisk

ENTRY_SRC = src/minkernel/ntos/init/entry.asm
ENTRY_OBJ = $(BUILD)/src/minkernel/ntos/init/entry.o

# Kernel core (src\minkernel\ntos\ke)
NTOS_SRC = src/minkernel/ntos/ke/video.c \
           src/minkernel/ntos/ke/font.c \
           src/minkernel/ntos/ke/window.c \
           src/minkernel/ntos/ke/time.c \
           src/minkernel/ntos/ke/bugcheck.c

# Kernel debugger COM port transport (src\minkernel\boot\kdcom)
KDCOM_SRC = src/minkernel/boot/kdcom/ixkdcom.c

# Memory Manager (src\minkernel\ntos\mm)
MM_SRC = src/minkernel/ntos/mm/mminit.c \
         src/minkernel/ntos/mm/miglobal.c \
         src/minkernel/ntos/mm/allocpag.c \
         src/minkernel/ntos/mm/pfnlist.c \
         src/minkernel/ntos/mm/pool.c \
         src/minkernel/ntos/mm/addrsup.c \
         src/minkernel/ntos/mm/vadtree.c \
         src/minkernel/ntos/mm/pagfault.c \
         src/minkernel/ntos/mm/wslist.c \
         src/minkernel/ntos/mm/wsmanage.c \
         src/minkernel/ntos/mm/allocvm.c \
         src/minkernel/ntos/mm/freevm.c \
         src/minkernel/ntos/mm/protect.c \
         src/minkernel/ntos/mm/queryvm.c \
         src/minkernel/ntos/mm/sysptes.c \
         src/minkernel/ntos/mm/hypermap.c \
         src/minkernel/ntos/mm/buildmdl.c \
         src/minkernel/ntos/mm/zeropage.c

FS_SRC = src/minkernel/fs/evryfs/ata.c \
         src/minkernel/fs/evryfs/super.c \
         src/minkernel/fs/evryfs/dirsup.c \
         src/minkernel/fs/evryfs/allocsup.c \
         src/minkernel/fs/evryfs/read.c \
         src/minkernel/fs/evryfs/write.c \
         src/minkernel/fs/evryfs/strsup.c
HAL_SRC = src/minkernel/hals/halx86/halinit.c \
          src/minkernel/hals/halx86/power.c
HAL_ASM_SRC = src/minkernel/hals/halx86/irq12.asm \
              src/minkernel/hals/halx86/i386/ixclock.asm
HAL_ASM_OBJ = $(BUILD)/src/minkernel/hals/halx86/irq12.o \
              $(BUILD)/src/minkernel/hals/halx86/i386/ixclock.o

SHELL_SRC = src/shell/explorer/desktop.c \
            src/shell/explorer/taskbar.c \
            src/shell/explorer/shell.c \
            src/shell/explorer/notes.c \
            src/shell/explorer/snake.c \
            src/shell/explorer/input.c \
            src/shell/explorer/files.c \
            src/shell/explorer/icon.c

KBDCLASS_SRC = src/onecore/drivers/input/keyboard/kbdclass/kbdclass.c
MOUCLASS_SRC = src/onecore/drivers/input/mouse/mouclass/mouclass.c

# Runtime Library (src/minkernel/ntos/rtl)
RTL_SRC = src/minkernel/ntos/rtl/movemem.c \
          src/minkernel/ntos/rtl/string.c \
          src/minkernel/ntos/rtl/bitmapex.c \
          src/minkernel/ntos/rtl/random.c

# Executive (src/minkernel/ntos/ex)
EX_SRC = src/minkernel/ntos/ex/exinit.c \
         src/minkernel/ntos/ex/fmutex.c \
         src/minkernel/ntos/ex/lookasid.c \
         src/minkernel/ntos/ex/luid.c

# Main entry
MAIN_SRC = src/minkernel/ntos/init/kernel.c

ALL_C_SRC = $(NTOS_SRC) $(KDCOM_SRC) $(MM_SRC) $(HAL_SRC) $(FS_SRC) $(SHELL_SRC) $(KBDCLASS_SRC) $(MOUCLASS_SRC) $(RTL_SRC) $(EX_SRC) $(MAIN_SRC)
ALL_C_OBJ = $(patsubst %.c,$(BUILD)/%.o,$(ALL_C_SRC))

KERNEL_ELF = $(BUILD)/kernel.elf
OS_ISO     = $(BUILD)/os.iso

TEST_ENTRY_SRC  = src/minkernel/ntos/mm/tests/entry.asm
TEST_ENTRY_OBJ  = $(BUILD)/src/minkernel/ntos/mm/tests/entry.o
TEST_MMTEST_SRC = src/minkernel/ntos/mm/tests/mmtest.c
TEST_MMTEST_OBJ = $(BUILD)/src/minkernel/ntos/mm/tests/mmtest.o
TEST_ELF        = $(BUILD)/mmtest.elf

KERNEL_ELF = $(BUILD)/kernel.elf
OS_ISO     = $(BIN)/os.iso

QEMU_TESTFLAGS  = -display none -m 64M -no-reboot

.PHONY: all clean run test

$(shell mkdir -p $(BUILD)/src/minkernel/ntos/init)
$(shell mkdir -p $(BUILD)/src/minkernel/ntos/ke)
$(shell mkdir -p $(BUILD)/src/minkernel/ntos/rtl)
$(shell mkdir -p $(BUILD)/src/minkernel/ntos/ex)
$(shell mkdir -p $(BUILD)/src/minkernel/boot/kdcom)
$(shell mkdir -p $(BUILD)/src/minkernel/ntos/mm)
$(shell mkdir -p $(BUILD)/src/minkernel/ntos/mm/tests)
$(shell mkdir -p $(BUILD)/src/minkernel/hals/halx86/i386)
$(shell mkdir -p $(BUILD)/src/minkernel/fs/evryfs)
$(shell mkdir -p $(BUILD)/src/shell/explorer)
$(shell mkdir -p $(BUILD)/src/onecore/drivers/input/keyboard/kbdclass)
$(shell mkdir -p $(BUILD)/src/onecore/drivers/input/mouse/mouclass)
$(shell mkdir -p $(BIN))
$(shell mkdir -p $(ISO)/boot/grub)

all: $(OS_ISO)

$(ENTRY_OBJ): $(ENTRY_SRC)
	$(NASM) $(ASFLAGS) $< -o $@

$(BUILD)/%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/src/minkernel/hals/halx86/irq12.o: src/minkernel/hals/halx86/irq12.asm
	$(NASM) $(ASFLAGS) $< -o $@

$(BUILD)/src/minkernel/hals/halx86/i386/ixclock.o: src/minkernel/hals/halx86/i386/ixclock.asm
	$(NASM) $(ASFLAGS) $< -o $@

$(KERNEL_ELF): $(ENTRY_OBJ) $(ALL_C_OBJ) $(HAL_ASM_OBJ)
	$(LD) $(LDFLAGS) $^ -o $@

$(ISO)/boot/kernel.elf: $(KERNEL_ELF)
	cp $< $@

$(ISO)/boot/grub/grub.cfg:
	echo 'set timeout=0' > $@
	echo 'set default=0' >> $@
	echo '' >> $@
	echo 'insmod all_video' >> $@
	echo 'insmod vbe' >> $@
	echo 'insmod vga' >> $@
	echo '' >> $@
	echo 'if loadfont /boot/grub/fonts/unicode.pf2; then' >> $@
	echo '    set gfxmode=auto' >> $@
	echo '    insmod gfxterm' >> $@
	echo '    terminal_output gfxterm' >> $@
	echo 'fi' >> $@
	echo '' >> $@
	echo 'menuentry "Everywhere OS" {' >> $@
	echo '    set gfxpayload=keep' >> $@
	echo '    multiboot /boot/kernel.elf' >> $@
	echo '    boot' >> $@
	echo '}' >> $@

$(OS_ISO): $(ISO)/boot/kernel.elf $(ISO)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISO)

$(MKDISK_EXE): $(MKDISK_SRC)
	$(HOST_CC) -o $@ $<

$(DISK_IMG): $(MKDISK_EXE) $(FOLDER_ICO)
	$(MKDISK_EXE) $(FOLDER_ICO) $@

run: $(OS_ISO) $(DISK_IMG)
	qemu-system-i386 -cdrom $(OS_ISO) -hda $(DISK_IMG) #-full-screen

$(TEST_ENTRY_OBJ): $(TEST_ENTRY_SRC)
	$(NASM) $(ASFLAGS) $< -o $@

$(TEST_MMTEST_OBJ): $(TEST_MMTEST_SRC)
	$(CC) $(CFLAGS) $< -o $@

$(TEST_ELF): $(TEST_ENTRY_OBJ) \
             $(BUILD)/src/minkernel/ntos/mm/mminit.o \
             $(BUILD)/src/minkernel/ntos/mm/miglobal.o \
             $(BUILD)/src/minkernel/ntos/mm/allocpag.o \
             $(BUILD)/src/minkernel/ntos/mm/pfnlist.o \
             $(BUILD)/src/minkernel/ntos/mm/pool.o \
             $(BUILD)/src/minkernel/ntos/mm/addrsup.o \
             $(BUILD)/src/minkernel/ntos/mm/vadtree.o \
             $(BUILD)/src/minkernel/ntos/mm/pagfault.o \
             $(BUILD)/src/minkernel/ntos/mm/wslist.o \
             $(BUILD)/src/minkernel/ntos/mm/wsmanage.o \
             $(BUILD)/src/minkernel/ntos/mm/allocvm.o \
             $(BUILD)/src/minkernel/ntos/mm/freevm.o \
             $(BUILD)/src/minkernel/ntos/mm/protect.o \
             $(BUILD)/src/minkernel/ntos/mm/queryvm.o \
             $(BUILD)/src/minkernel/ntos/mm/sysptes.o \
             $(BUILD)/src/minkernel/ntos/mm/hypermap.o \
             $(BUILD)/src/minkernel/ntos/mm/buildmdl.o \
             $(BUILD)/src/minkernel/ntos/mm/zeropage.o \
             $(TEST_MMTEST_OBJ)
	$(LD) $(LDFLAGS) $^ -o $@

test: $(TEST_ELF)
	timeout 30 qemu-system-i386 -kernel $< $(QEMU_TESTFLAGS) \
	    -serial file:$(BUILD)/test.log || true
	@cat $(BUILD)/test.log
	grep -q "^PASS" $(BUILD)/test.log

clean:
	rm -rf $(BUILD) $(BIN) $(ISO)
