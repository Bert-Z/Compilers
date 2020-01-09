#ifndef TIGER_CODEGEN_CODEGEN_H_
#define TIGER_CODEGEN_CODEGEN_H_

#include "tiger/codegen/assem.h"
#include "tiger/frame/frame.h"
#include "tiger/translate/tree.h"

namespace CG
{

AS::InstrList *Codegen(F::Frame *f, T::StmList *stmList);

TEMP::TempList *L(TEMP::Temp *h, TEMP::TempList *t);

/* function declare */
static void munchStm(T::Stm *s);
static TEMP::Temp *munchExp(T::Exp *e);
static TEMP::TempList *munchArgs(int cnt, T::ExpList *args);
static void restoreCallerRegs();
static void emit(AS::Instr *inst);
} // namespace CG
#endif