#include <assert.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include "binlit.h"

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
      FL_POS = 1 << 0,
      FL_ZERO = 1 << 1,
      FL_NEG = 1 << 2
};

/* memory-mapped registers */
enum {
      MR_KBSR = 0xFE00, /* keyboard status reg */
      MR_KBDR = 0xFE02 /* keyboard data reg */
};

/* the model of the VM */
typedef struct vm {
  word memory[UINT16_MAX];
  word reg[R_COUNT];
} VM;

/* VM run state */
enum vm_exec_state {
      STOP = 0,
      RUNNING = 1
};
typedef enum vm_exec_state exec_st;

/* function prototypes */
exec_st process_instruction(VM*, word);
int read_image(VM*, const char*);
void mem_write(VM*, word, word);
void print_registers(VM*);
void read_image_file(VM*, FILE*);
void update_flags(VM*, word);
word mem_read(VM*, word);
word fetch_next_instr(VM*);
word check_key();
word sign_extend(word, int);
word swap16(word);

int run_vm(const char*);
int run_tests();


void update_flags(VM* vm, word r) {
  if (vm->reg[r] == 0) {
    vm->reg[R_COND] = FL_ZERO;
  } else if (vm->reg[r] >> 15) { // 2s complement negative
    vm->reg[R_COND] = FL_NEG;
  } else {
    vm->reg[R_COND] = FL_POS;
  }
}

word sign_extend(word x, int bit_count) {
  if ((x >> (bit_count - 1)) & 1) {
    x |= (0xFFFF << bit_count);
  }
  return x;
}

void print_registers(VM* vm) {
  int i;
  for (i = 0; i<8; i++) {
    printf("R%d=x%04x ", i, vm->reg[i]);
  }
  printf("\n");
  return;
}

void mem_write(VM* vm, word address, word val) {
  vm->memory[address] = val;
}

word mem_read(VM* vm, word address) {
  if (address == MR_KBSR) {
    if (check_key()) {
      vm->memory[MR_KBSR] = (1 << 15);
      vm->memory[MR_KBDR] = getchar();
    } else {
      vm->memory[MR_KBSR] = 0;
    }
  }
  return vm->memory[address];
}

void read_image_file(VM* vm, FILE* file) {
  word origin;
  fread(&origin, sizeof(origin), 1, file);
  origin = swap16(origin);

  word max_read = UINT16_MAX - origin;
  word* p = (vm->memory) + origin;
  size_t read = fread(p, sizeof(word), max_read, file);

  while (read-- > 0) {
    *p = swap16(*p);
    ++p;
  }
}

int read_image(VM* vm, const char* image_path) {
  FILE* file = fopen(image_path, "rb");
  if (!file) { return 0; }
  read_image_file(vm, file);
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

word fetch_next_instr(VM* vm) {
  return mem_read(vm, vm->reg[R_PC]++);
}

int main(int argc, const char* argv[]){
#ifdef TEST
  return run_tests();
#else
  return run_vm(argv[1]);
#endif
}

int op_add(VM* vm, word operands) {
  word r0 = (operands >> 9) & 0x7;
  word r1 = (operands >> 6) & 0x7;
  word imm_flag = (operands >> 5) & 0x1;
  if (imm_flag) {
    word imm5 = sign_extend(operands & 0x1F, 5);
    vm->reg[r0] = vm->reg[r1] + imm5;
  } else {
    word r2 = operands & 0x7;
    vm->reg[r0] = vm->reg[r1] + vm->reg[r2];
  }
  update_flags(vm, r0);
}

int op_and(VM* vm, word operands) {
  word r0 = (operands >> 9) & 0x7;
  word r1 = (operands >> 6) & 0x7;
  word imm_flag = (operands >> 5) & 0x1;
  if (imm_flag) {
    word imm5 = sign_extend(operands & 0x1F, 5);
    vm->reg[r0] = vm->reg[r1] & imm5;
  } else {
    word r2 = operands & 0x7;
    vm->reg[r0] = vm->reg[r1] & vm->reg[r2];
  }
  update_flags(vm, r0);
}

exec_st process_instruction(VM* vm, word instr) {
  int z = 0;
  exec_st exec_st = RUNNING;
  word op = instr >> 12;
  word payload = instr & 0xFFF;
  switch (op) {
  case OP_ADD: op_add(vm, payload); break;
  case OP_AND: op_and(vm, payload); break;
  case OP_LD: {
      word r0 = (instr >> 9) & 0x7;
      word source = sign_extend(instr & 0x1ff, 9);
      vm->reg[r0] = mem_read(vm, vm->reg[R_PC] + source);
      update_flags(vm, r0);
      break;
  }
  default:; {
      printf("got other, stopping\n");
      exec_st = STOP;
      break;
  }
  }
  return exec_st;
}

int run_vm(const char* image) {
  VM* vm = malloc(sizeof(vm));

  if(!read_image(vm, image)) {
    printf("could not load image\n");
    exit(1);
  }

  /* Set the PC to starting position */
  enum { PC_START = 0x3000 };
  vm->reg[R_PC] = PC_START;

  exec_st exec_st = RUNNING;
  while (exec_st) {
    word instr = fetch_next_instr(vm);
    exec_st = process_instruction(vm, instr);
  }
  /* Shutdown (12) */
  print_registers(vm);
  free(vm);
}

#ifdef TEST
#define test(name)

int run_tests() {
  test("sign_extend preserves sign"){
    // 00000 == 0000 0000
    assert(sign_extend(0x0, 5) == (word)(0));

    // 11111 == 1111 1111 == -1
    assert(sign_extend(0x1F, 5) == ((word)(-1)));

    // 01111 == 0000 1111 == 15
    assert(sign_extend(0xF, 5) == ((word)(15)));
  }

  test("all registers start at zero"){
    VM* vm = calloc(sizeof(vm), 0);
    for(int i = 0; i<R_COUNT; i++) {
      assert(vm->reg[i] == 0);
    }
    free(vm);
  }

  test("ADD with immediate value adds to dest register"){
    VM* vm = calloc(sizeof(vm), 0);
    word i = word_from_string("0001 000 000 1 00001");
                           //   ADD  dr  sr f  imm5
    process_instruction(vm, i);

    assert(vm->reg[0] == 1);
    free(vm);
  }

  test("ADD with max immediate value ADDs to dr"){
    VM* vm = calloc(sizeof(vm), 0);
    word i = word_from_string("0001 000 000 1 01111");
                             // ADD           imm5=0xf
    process_instruction(vm, i);

    assert(vm->reg[0] == 0xf);
    free(vm);
  }

  test("ADD with negative immediate value decrements dr"){
    VM* vm = calloc(sizeof(vm), 0);
    word i = word_from_string("0001 000 000 1 11111");
                             //               (-1) in 2's comp
    process_instruction(vm, i);

    assert(vm->reg[0] == (word)(-1));
    free(vm);
  }

  test("ADD with other register ADDs register values"){
    VM* vm = calloc(sizeof(vm), 0);
    word i = word_from_string("0001 000 001 0 00 010");
                            //  ADD  dr  sr  f zz  tr
    vm->reg[1] = 1;
    vm->reg[2] = 2;
    process_instruction(vm, i);

    assert(vm->reg[0] == 3);
    free(vm);
  }

  test("AND with immediate performs bitwise AND"){
    VM* vm = calloc(sizeof(vm), 0);
    word i = word_from_string("0101 000 001 1 01111");
                            //  AND  dr  sr f  imm5
    vm->reg[1] = 0x9; // 1001
    process_instruction(vm, i);

    assert(vm->reg[0] == 0x9);
    free(vm);
  }

  test("AND performs bitwise AND in register-addressed mode"){
    VM* vm = calloc(sizeof(vm), 0);
    word i = word_from_string("0101 000 001 0 00 010");
                            //  AND  dr  sr f zz  tr
    vm->reg[1] = 1;
    vm->reg[2] = 3;
    process_instruction(vm, i);

    assert(vm->reg[0] == 1);
    free(vm);
  }

  printf("ALL UNIT TESTS PASSED.\n\n");
  return 0;
}
#endif
