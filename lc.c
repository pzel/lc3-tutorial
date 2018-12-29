#include <sys/select.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#define word uint16_t

/* Registers */

enum {
      R_R0 = 0,
      R_R1,
      R_R2,
      R_R3,
      R_R4,
      R_R5,
      R_R6,
      R_R7,
      R_PC, /* program counter */
      R_COND,
      R_COUNT
};

word memory[UINT16_MAX];
word reg[R_COUNT];

/* Opcodes */
enum {
      OP_BR = 0, /* branch */
      OP_ADD,    /* add (2 variants) */
      OP_LD,     /* load */
      OP_ST,     /* store */
      OP_JSR,    /* jump register */
      OP_AND,    /* bitwise and */
      OP_LDR,    /* load register */
      OP_STR,    /* store register */
      OP_RTI,    /* unused */
      OP_NOT,    /* bitwise not */
      OP_LDI,    /* load indirect */
      OP_STI,    /* store indirect */
      OP_JMP,    /* jump */
      OP_RES,    /* reserved (unused) */
      OP_LEA,    /* load effective address */
      OP_TRAP    /* execute trap */
};

/* condition flags */
enum {
      FL_POS = 1 << 0, /* p */
      FL_ZERO = 1 << 1, /* z */
      FL_NEG = 1 << 2
};

/* memory-mapped registers */
enum {
      MR_KBSR = 0xFE00, /* keyboard status reg */
      MR_KBDR = 0xFE02 /* keyboard data reg */
};

/* function prototypes */
word check_key();
void print_registers();
void mem_write(word, word);
word mem_read(word);
void read_image_file(FILE*);
void update_flags(word);
int read_image(const char*);
word swap16(word);
word sign_extend(word, int);


void update_flags(word r) {
  if (reg[r] == 0) {
    reg[R_COND] = FL_ZERO;
  } else if (reg[r] >> 15) { // 2s complement negative
    reg[R_COND] = FL_NEG;
  } else {
    reg[R_COND] = FL_POS;
  }
}

word sign_extend(word x, int bit_count) {
  if ((x >> (bit_count - 1)) & 1) {
    x |= (0xFFFF << bit_count);
  }
  return x;
}

void print_registers() {
  int i;
  for (i = 0; i<8; i++) {
    printf("R%d=x%04x ", i, reg[i]);
  }
  printf("\n");
  return;
}

void mem_write(word address, word val) {
  memory[address] = val;
}

word mem_read(word address) {
  if (address == MR_KBSR) {
    if (check_key()) {
      memory[MR_KBSR] = (1 << 15);
      memory[MR_KBDR] = getchar();
    } else {
      memory[MR_KBSR] = 0;
    }
  }
  return memory[address];
}

void read_image_file(FILE* file) {
  word origin;
  fread(&origin, sizeof(origin), 1, file);
  origin = swap16(origin);

  word max_read = UINT16_MAX - origin;
  word* p = memory + origin;
  size_t read = fread(p, sizeof(word), max_read, file);

  while (read-- > 0) {
    *p = swap16(*p);
    ++p;
  }
}

int read_image(const char* image_path) {
  FILE* file = fopen(image_path, "rb");
  if (!file) { return 0; }
  read_image_file(file);
  fclose(file);
  return 1;
}

word swap16(word x) {
  return (x << 8) | (x >> 8);
}


word check_key() {
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  return select(1, &readfds, NULL, NULL, &timeout) != 0;
}


int main(int argc, const char* argv[]){
  if(!read_image(argv[1])) {
    printf("could not load image\n");
    exit(1);
  }
  // setup (12)
  /* Set the PC to starting position */
  enum { PC_START = 0x3000 };
  reg[R_PC] = PC_START;

  int running = 1;
  while (running) {
    /* Fetch operation */
    word instr = mem_read(reg[R_PC]++);
    word op = instr >> 12;
    switch (op) {
    case OP_ADD:
      {
        word r0 = (instr >> 9) & 0x7;
        word r1 = (instr >> 6) & 0x7;
        word imm_flag = (instr >> 5) & 0x1;
        if (imm_flag) {
          word imm5 = sign_extend(instr & 0x1F, 5);
          reg[r0] = reg[r1] + imm5;
        } else {
          word r2 = instr & 0x7;
          reg[r0] = reg[r1] + reg[r2];
        }
        update_flags(r0);
      }

      break;
    case OP_AND:
      {
        word r0 = (instr >> 9) & 0x7;
        word r1 = (instr >> 6) & 0x7;
        word imm_flag = (instr >> 5) & 0x1;
        if (imm_flag) {
          word imm5 = sign_extend(instr & 0x1F, 5);
          reg[r0] = reg[r1] & imm5;
          printf("reg at %d is %x", r0, reg[r0]);
        } else {
          word r2 = instr & 0x7;
          reg[r0] = reg[r1] & reg[r2];
        }
        update_flags(r0);
      }
      break;
    default:
      printf("got other\n");
      running = 0;
      break;
    }
  }
  // shutdown (12)
  print_registers();
}
