#include "cpu/exec.h"

//data-mov.c
make_EHelper(mov);
make_EHelper(push);
make_EHelper(pop);
make_EHelper(lea);

make_EHelper(operand_size);

//control.c
make_EHelper(call);
make_EHelper(ret);

make_EHelper(inv);
make_EHelper(nemu_trap);

//arith.c
make_EHelper(sub);

//logic.c
make_EHelper(xor);