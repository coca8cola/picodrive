// Pico Library - Header File

// (c) Copyright 2004 Dave, All rights reserved.
// (c) Copyright 2006 notaz, All rights reserved.
// Free for non-commercial use.

// For commercial use, separate licencing terms must be obtained.


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Pico.h"


// to select core, define EMU_C68K, EMU_M68K or EMU_A68K in your makefile

#ifdef __cplusplus
extern "C" {
#endif


// ----------------------- 68000 CPU -----------------------
#ifdef EMU_C68K
#include "../cpu/Cyclone/Cyclone.h"
extern struct Cyclone PicoCpu;
#define SekCyclesLeft PicoCpu.cycles // cycles left for this run
#define SekSetCyclesLeft(c) PicoCpu.cycles=c
#define SekPc (PicoCpu.pc-PicoCpu.membase)
#endif

#ifdef EMU_A68K
void __cdecl M68000_RUN();
// The format of the data in a68k.asm (at the _M68000_regs location)
struct A68KContext
{
  unsigned int d[8],a[8];
  unsigned int isp,srh,ccr,xc,pc,irq,sr;
  int (*IrqCallback) (int nIrq);
  unsigned int ppc;
  void *pResetCallback;
  unsigned int sfc,dfc,usp,vbr;
  unsigned int AsmBank,CpuVersion;
};
struct A68KContext M68000_regs;
extern int m68k_ICount;
#define SekCyclesLeft m68k_ICount
#define SekSetCyclesLeft(c) m68k_ICount=c
#define SekPc M68000_regs.pc
#endif

#ifdef EMU_M68K
#include "../cpu/musashi/m68kcpu.h"
extern m68ki_cpu_core PicoM68kCPU; // MD's CPU
extern m68ki_cpu_core PicoS68kCPU; // Mega CD's CPU
#ifndef SekCyclesLeft
#define SekCyclesLeft m68k_cycles_remaining()
#define SekSetCyclesLeft(c) SET_CYCLES(c)
#define SekPc m68k_get_reg(&PicoM68kCPU, M68K_REG_PC)
#define SekPcS68k m68k_get_reg(&PicoS68kCPU, M68K_REG_PC)
#endif
#endif

extern int SekCycleCnt; // cycles done in this frame
extern int SekCycleAim; // cycle aim
extern unsigned int SekCycleCntT; // total cycle counter, updated once per frame

#define SekCyclesReset() {SekCycleCntT+=SekCycleCnt;SekCycleCnt=SekCycleAim=0;}
#define SekCyclesBurn(c)  SekCycleCnt+=c
#define SekCyclesDone()  (SekCycleAim-SekCyclesLeft)    // nuber of cycles done in this frame (can be checked anywhere)
#define SekCyclesDoneT() (SekCycleCntT+SekCyclesDone()) // total nuber of cycles done for this rom

#define SekEndRun(after) { \
	SekCycleCnt -= SekCyclesLeft - after; \
	if(SekCycleCnt < 0) SekCycleCnt = 0; \
	SekSetCyclesLeft(after); \
}

extern int SekCycleCntS68k;
extern int SekCycleAimS68k;

#define SekCyclesResetS68k() {SekCycleCntS68k=SekCycleAimS68k=0;}

// does not work as expected
//extern int z80ExtraCycles; // extra z80 cycles, used when z80 is [en|dis]abled

extern int PicoMCD;

// ---------------------------------------------------------

// main oscillator clock which controls timing
#define OSC_NTSC 53693100
#define OSC_PAL  53203424 // not accurate

struct PicoVideo
{
  unsigned char reg[0x20];
  unsigned int command; // 32-bit Command
  unsigned char pending; // 1 if waiting for second half of 32-bit command
  unsigned char type; // Command type (v/c/vsram read/write)
  unsigned short addr; // Read/Write address
  int status; // Status bits
  unsigned char pending_ints; // pending interrupts: ??VH????
  unsigned char pad[0x13];
};

struct PicoMisc
{
  unsigned char rotate;
  unsigned char z80Run;
  unsigned char padTHPhase[2]; // phase of gamepad TH switches
  short scanline; // 0 to 261||311; -1 in fast mode
  char dirtyPal; // Is the palette dirty (1 - change @ this frame, 2 - some time before)
  unsigned char hardware; // Hardware value for country
  unsigned char pal; // 1=PAL 0=NTSC
  unsigned char sram_reg; // SRAM mode register. bit0: allow read? bit1: deny write? bit2: EEPROM?
  unsigned short z80_bank68k;
  unsigned short z80_lastaddr; // this is for Z80 faking
  unsigned char  z80_fakeval;
  unsigned char  pad0;
  unsigned char  padDelay[2];  // gamepad phase time outs, so we count a delay
  unsigned short sram_addr;  // EEPROM address register
  unsigned char sram_cycle;  // EEPROM SRAM cycle number
  unsigned char sram_slave;  // EEPROM slave word for X24C02 and better SRAMs
  unsigned char prot_bytes[2]; // simple protection fakeing
  unsigned short dma_bytes;  //
  unsigned char pad1[6];
};

// some assembly stuff depend on these, do not touch!
struct Pico
{
  unsigned char ram[0x10000];  // 0x00000 scratch ram
  unsigned short vram[0x8000]; // 0x10000
  unsigned char zram[0x2000];  // 0x20000 Z80 ram
  unsigned char ioports[0x10];
  unsigned int pad[0x3c];      // unused
  unsigned short cram[0x40];   // 0x22100
  unsigned short vsram[0x40];  // 0x22180

  unsigned char *rom;          // 0x22200
  unsigned int romsize;        // 0x22204

  struct PicoMisc m;
  struct PicoVideo video;
};

// sram
struct PicoSRAM
{
  unsigned char *data; // actual data
  unsigned int start;  // start address in 68k address space
  unsigned int end;
  unsigned char resize; // 1=SRAM size changed and needs to be reallocated on PicoReset
  unsigned char reg_back; // copy of Pico.m.sram_reg to set after reset
  unsigned char changed;
  unsigned char pad;
};

// MCD
#include "cd/cd_sys.h"
#include "cd/LC89510.h"

typedef struct
{
	unsigned char bios[0x20000];
	union {
		unsigned char prg_ram[0x80000];
		unsigned char prg_ram_b[4][0x20000];
	};
	unsigned char word_ram[0x40000];
	unsigned char s68k_regs[0x200];
	unsigned char m68k_regs[0x10];
	CDD  cdd;
	CDC  cdc;
	_scd scd;
} mcd_state;

#define Pico_mcd ((mcd_state *)Pico.rom)


// Draw.c
int PicoLine(int scan);
void PicoFrameStart();

// Draw2.c
void PicoFrameFull();

// Memory.c
int PicoInitPc(unsigned int pc);
unsigned int CPU_CALL PicoRead32(unsigned int a);
int  PicoMemInit();
void PicoMemReset();
void PicoDasm(int start,int len);
unsigned char z80_read(unsigned short a);
unsigned short z80_read16(unsigned short a);
void z80_write(unsigned char data, unsigned short a);
void z80_write16(unsigned short data, unsigned short a);

// cd/Memory.c
unsigned char  PicoReadCD8 (unsigned int a);
unsigned short PicoReadCD16(unsigned int a);
unsigned int   PicoReadCD32(unsigned int a);
void PicoWriteCD8 (unsigned int a, unsigned char d);
void PicoWriteCD16(unsigned int a, unsigned short d);
void PicoWriteCD32(unsigned int a, unsigned int d);

// Pico.c
extern struct Pico Pico;
extern struct PicoSRAM SRam;
extern int emustatus;

// cd/Pico.c
int  PicoInitMCD(void);
void PicoExitMCD(void);
int  PicoResetMCD(int hard);

// Sek.c
int SekInit(void);
int SekReset(void);
int SekInterrupt(int irq);
void SekState(unsigned char *data);

// cd/Sek.c
int SekInitS68k(void);
int SekResetS68k(void);
int SekInterruptS68k(int irq);

// VideoPort.c
void PicoVideoWrite(unsigned int a,unsigned short d);
unsigned int PicoVideoRead(unsigned int a);

// Misc.c
void SRAMWriteEEPROM(unsigned int d);
unsigned int SRAMReadEEPROM();
void SRAMUpdPending(unsigned int a, unsigned int d);


#ifdef __cplusplus
} // End of extern "C"
#endif
