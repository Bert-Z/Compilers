#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <string>

#include "tiger/translate/tree.h"
#include "tiger/util/util.h"

#include "tiger/codegen/assem.h"

static const int wordsize = 8;

namespace F
{

// need to be declared at first
class AccessList;

class Frame
{
public:
  // Base class
  TEMP::Label *label;
  F::AccessList *formals;
  F::AccessList *locals;
  T::StmList *view_shift;
  int s_offset; //Which is commonly a minus number.
  Frame(TEMP::Label *label,
        F::AccessList *formals,
        F::AccessList *locals,
        T::StmList *view_shift, int s_offset)
      : label(label), formals(formals), locals(locals), view_shift(view_shift), s_offset(s_offset) {}
};

Frame *F_newFrame(TEMP::Label *name, U::BoolList *escape);

class Access
{
public:
  enum Kind
  {
    INFRAME,
    INREG
  };

  Kind kind;

  Access(Kind kind) : kind(kind) {}

  // Hints: You may add interface like
  virtual T::Exp *ToExp(T::Exp *framePtr) const = 0;
};

class InFrameAccess : public F::Access
{
public:
  int offset;
  T::Exp *ToExp(T::Exp *framePtr) const override
  {
    return new T::MemExp(new T::BinopExp(T::PLUS_OP, framePtr, new T::ConstExp(this->offset)));
  }

  InFrameAccess(int offset) : Access(INFRAME), offset(offset) {}
};

class InRegAccess : public F::Access
{
public:
  TEMP::Temp *reg;
  T::Exp *ToExp(T::Exp *framePtr) const override
  {
    return new T::TempExp(this->reg);
  }
  InRegAccess(TEMP::Temp *reg) : Access(INREG), reg(reg) {}
};

class AccessList
{
public:
  Access *head;
  AccessList *tail;

  AccessList(Access *head, AccessList *tail) : head(head), tail(tail) {}
};

/*
 * Fragments
 */

class Frag
{
public:
  enum Kind
  {
    STRING,
    PROC
  };

  Kind kind;

  Frag(Kind kind) : kind(kind) {}
};

class StringFrag : public Frag
{
public:
  TEMP::Label *label;
  std::string str;

  StringFrag(TEMP::Label *label, std::string str)
      : Frag(STRING), label(label), str(str) {}
};

class ProcFrag : public Frag
{
public:
  T::Stm *body;
  Frame *frame;

  ProcFrag(T::Stm *body, Frame *frame) : Frag(PROC), body(body), frame(frame) {}
};

class FragList
{
public:
  Frag *head;
  FragList *tail;

  FragList(Frag *head, FragList *tail) : head(head), tail(tail) {}
};

// 将每个传入寄存器的参数存放到从函数内看它的位置。
// SeqStm : |1(%rdi)||2(%rsi)||3||4||5||6||stm||const(0)|
T::Stm *F_procEntryExit1(Frame *frame, T::Stm *stm);

AS::InstrList *F_procEntryExit2(AS::InstrList *body);

// 把InstList包成一个Proc
AS::Proc *F_procEntryExit3(Frame *frame, AS::InstrList *inst);

// 分配局部变量
F::Access *F_allocLocal(Frame *frame, bool escape);

T::CallExp *F_externalCall(std::string s, T::ExpList *args);


/*************************************************/

TEMP::Temp *F_FP(void);
TEMP::Temp *F_RV(void);
TEMP::Temp *F_RAX(void);
TEMP::Temp *F_RDI();
TEMP::Temp *F_RSI();
TEMP::Temp *F_RDX();
TEMP::Temp *F_RCX();
TEMP::Temp *F_R8();
TEMP::Temp *F_R9();
TEMP::Temp *F_RBX();
TEMP::Temp *F_RBP();
TEMP::Temp *F_R10();
TEMP::Temp *F_R11();
TEMP::Temp *F_R12();
TEMP::Temp *F_R13();
TEMP::Temp *F_R14();
TEMP::Temp *F_R15();

TEMP::Temp *F_SP(void); 
TEMP::Temp *F_RV(void);
TEMP::TempList *F_callerSaveRegs();

} // namespace F

#endif