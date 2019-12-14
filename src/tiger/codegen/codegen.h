#ifndef TIGER_CODEGEN_CODEGEN_H_
#define TIGER_CODEGEN_CODEGEN_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/translate/tree.h"

namespace CG
{

AS::InstrList *Codegen(F::Frame *f, T::StmList *stmList);

TEMP::TempList *L(TEMP::Temp *h, TEMP::TempList *t);
void emit(AS::Instr *inst);

/* function declare */
void munchStm(T::Stm *s);
TEMP::Temp *munchExp(T::Exp *e);
TEMP::TempList *munchArgs(int cnt, T::ExpList *args);
} // namespace CG
#endif