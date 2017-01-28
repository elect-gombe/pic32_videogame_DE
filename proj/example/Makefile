PROJECT = EXAMPLE1

COMPILER_PATH = ../../pic32/compiler/pic32-tools/bin

CC = $(COMPILER_PATH)/pic32-gcc
LD = $(COMPILER_PATH)/pic32-gcc
SIZE = $(COMPILER_PATH)/pic32-size
HEX = $(COMPILER_PATH)/pic32-bin2hex

OBJDIR = ./obj
BINDIR = ./bin

USERSRCDIR = .
LIBSRCDIR = ../../library

C_SRC = $(wildcard $(USERSRCDIR)/*.c) $(wildcard $(LIBSRCDIR)/*.c)
INC = -I ./Include -I ../../library/

STARTUP_SRC = ../../library/crt0.S
STARTUP_OBJ = crt0.o

OBJ = 	$(addprefix $(OBJDIR)/, $(notdir $(C_SRC:.c=.o)))\
		$(OBJDIR)/$(STARTUP_OBJ)

LINKER_FILE = App-on-bootloader-170F.ld

LINKER_SCRIPT = ../../library/$(LINKER_FILE)

CPU = 32MX250F128B

COMPILE_FLAGS = -MMD -g -O3 -mprocessor=$(CPU) -c
COMPILE_CFLAGS = $(COMPILE_FLAGS) -x c -Wall

LD_FLAGS = -Os --gc-sections -mprocessor=$(CPU) -Map=$(BINDIR)/$(PROJECT).map -nostartfiles --report-mem

.PHONY: all clean size

all: $(BINDIR)/$(PROJECT).elf $(BINDIR)/$(PROJECT).hex size

size:$(BINDIR)/$(PROJECT).elf
	$(SIZE) $(BINDIR)/EXAMPLE1.elf

clean:
	rm $(OBJDIR) $(BINDIR) -r

$(BINDIR)/$(PROJECT).hex: $(BINDIR)/$(PROJECT).elf
	$(HEX) $(BINDIR)/EXAMPLE1.elf

$(OBJDIR)/$(STARTUP_OBJ): $(STARTUP_SRC)
	-mkdir -p $(OBJDIR)
	$(CC) $(COMPILE_FLAGS) -x assembler-with-cpp -o $@ $<

$(OBJDIR)/%.o: $(USERSRCDIR)/%.c 
	-mkdir -p $(OBJDIR)
	$(CC)  $(COMPILE_CFLAGS) $(INC) -o $@ $<

$(OBJDIR)/%.o: $(LIBSRCDIR)/%.c 
	-mkdir -p $(OBJDIR)
	$(CC)  $(COMPILE_CFLAGS) $(INC) -o $@ $<

$(BINDIR)/$(PROJECT).elf: $(OBJ)
	-mkdir -p $(BINDIR)
	$(LD) $(LD_FLAGS) -T $(LINKER_SCRIPT) -o $@ $^ $(LIBRARIES)

