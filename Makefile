# ==========================================
# Project Configuration
# ==========================================
PROJECT_NAME = CH559_CDPlayer
XRAM_SIZE    = 0x0800
XRAM_LOC     = 0x0600
CODE_SIZE    = 0xEFFF
FREQ_SYS     = 48000000

# Compiler & Tools (Linux環境に合わせて適宜変更してください)
CC       = sdcc
PACKIHX  = packihx
HEX2BIN  = hex2bin

FLASHER  = wchisp
FLASH_FLAGS = flash

# Flags
CFLAGS   = -mmcs51 --model-large --xram-size $(XRAM_SIZE) --xram-loc $(XRAM_LOC) --code-size $(CODE_SIZE) -I. -DFREQ_SYS=$(FREQ_SYS)

# Source files and Object files
SRCS     = main.c util.c USBHost.c uart.c msc.c mmc.c
OBJS     = $(SRCS:.c=.rel)

# ==========================================
# Build Targets
# ==========================================
.PHONY: all clean

all: config.h $(PROJECT_NAME).hex

# config.hの自動生成
config.h:
	@echo "// project config" > config.h

# 書き込みターゲットを追加
flash: $(PROJECT_NAME).bin
	$(FLASHER) $(FLASH_FLAGS) $(PROJECT_NAME).bin

# 各ソースファイルのコンパイル
%.rel: %.c
	$(CC) -c -V $(CFLAGS) $<

# リンクして .ihx 生成
$(PROJECT_NAME).ihx: $(OBJS)
	$(CC) $(OBJS) -V $(CFLAGS) -o $@


# ihx から hex および bin へ変換
$(PROJECT_NAME).hex: $(PROJECT_NAME).ihx
	$(PACKIHX) $< > $@
	makebin -p $(PROJECT_NAME).ihx $(PROJECT_NAME).bin
	@echo ""
	@echo "Build completed: $(PROJECT_NAME).hex and $(PROJECT_NAME).bin"
	@echo "Flash only after reviewing the hardware and test notes."

# クリーンアップ
clean:
	rm -f *.rel *.map *.mem *.asm *.lst *.rst *.sym *.lk *.ihx *.hex *.bin config.h *~
