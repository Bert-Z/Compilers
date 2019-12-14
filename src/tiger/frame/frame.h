#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <string>

#include "tiger/codegen/assem.h"
#include "tiger/translate/tree.h"
#include "tiger/util/util.h"

namespace F
{

class Frame
{
  // Base class
public:
  AccessList *formals;
  TEMP::Label *name;
  int size = 0;

  Frame(TEMP::Label *name, AccessList *formals) : name(name), formals(formals) {}
  static Frame *newFrame(TEMP::Label *name, U::BoolList *formals);
};

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
  //        `virtual T::Exp* ToExp(T::Exp* framePtr) const = 0`
  // virtual T::Exp *ToExp(T::Exp *frame_ptr) const = 0;
};

class InFrameAccess : public Access
{
public:
  int offset;

  InFrameAccess(int offset) : Access(INFRAME), offset(offset) {}
};

class InRegAccess : public Access
{
public:
  TEMP::Temp *reg;

  InRegAccess(TEMP::Temp *reg) : Access(INREG), reg(reg) {}
};

T::Exp *getExp(Access *access, T::Exp *fp);
Access *allocLocal(Frame *frame, bool escape);

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

// %rsp
TEMP::Temp *F_FP();

// %rax
TEMP::Temp *F_RV();

// TEMP::TempList *F_Specialregs(void);
TEMP::Temp *F_Arg(int idx);
TEMP::TempList *F_Argregs();
TEMP::TempList *Calleesaves();
TEMP::TempList *Callersaves();

TEMP::TempList *F_registers(void);

T::Exp *externalCall(std::string s, T::ExpList *args);

TEMP::Temp *F_RSP();
TEMP::Temp *F_RAX();
TEMP::Temp *F_RBX();
TEMP::Temp *F_RCX();
TEMP::Temp *F_RDX();
TEMP::Temp *F_RSI();
TEMP::Temp *F_RDI();
TEMP::Temp *F_RBP();
TEMP::Temp *F_RSP();
TEMP::Temp *F_R8();
TEMP::Temp *F_R9();
TEMP::Temp *F_R10();
TEMP::Temp *F_R11();
TEMP::Temp *F_R12();
TEMP::Temp *F_R13();
TEMP::Temp *F_R14();
TEMP::Temp *F_R15();

T::Stm *F_procEntryExit1(F::Frame *frame, T::Stm *stm);
AS::InstrList *F_procEntryExit2(F::Frame *frame, AS::InstrList *body);
AS::Proc *F_procEntryExit3(F::Frame *frame, AS::InstrList *body);

} // namespace F

#endif