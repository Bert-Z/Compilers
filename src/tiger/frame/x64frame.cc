#include "tiger/frame/frame.h"

#include <iostream>
#include <sstream>
#include <string>

namespace F
{

static TEMP::Temp *rbp = nullptr;
static TEMP::Temp *rax = nullptr;
static TEMP::Temp *rdi = nullptr;
static TEMP::Temp *rsi = nullptr;
static TEMP::Temp *rdx = nullptr;
static TEMP::Temp *rcx = nullptr;
static TEMP::Temp *r8 = nullptr;
static TEMP::Temp *r9 = nullptr;

static TEMP::Temp *r10 = nullptr;
static TEMP::Temp *r11 = nullptr;
static TEMP::Temp *r12 = nullptr;
static TEMP::Temp *r13 = nullptr;
static TEMP::Temp *r14 = nullptr;
static TEMP::Temp *r15 = nullptr;

static TEMP::Temp *rsp = nullptr;
static TEMP::Temp *rbx = nullptr;

class X64Frame : public Frame
{
public:
  // TODO: Put your codes here (lab6).
  X64Frame(TEMP::Label *label,
           F::AccessList *formals,
           F::AccessList *locals,
           T::StmList *view_shift, int s_offset)
      : Frame(label, formals, locals, view_shift, s_offset) {}
};

Frame *F_newFrame(TEMP::Label *name, U::BoolList *escapes)
{
  F::AccessList *formals = new AccessList(nullptr, nullptr);
  T::StmList *view_shift = new T::StmList(nullptr, nullptr);

  T::StmList *vslist = view_shift;
  F::AccessList *formalslist = formals;
  bool escape;
  TEMP::Temp *temp = TEMP::Temp::NewTemp();

  // The seventh arg was located at 8(%rbp)
  int formal_off = wordsize; 
  
  X64Frame *newframe = new X64Frame(name, nullptr, nullptr, nullptr, -8);

  //the num of formals
  int num = 1; 
  
  /*If the formal is escape, then allocate it on the frame.
	  Else,allocate it on the temp.*/
  for (; escapes; escapes = escapes->tail, num++)
  {
    escape = escapes->head;
    if (escape)
    {
      switch (num)
      {
      case 1:
        vslist->tail = new T::StmList(new T::MoveStm(
                                          new T::MemExp(
                                              new T::BinopExp(
                                                  T::PLUS_OP, new T::TempExp(F::F_FP()), new T::ConstExp(newframe->s_offset))),
                                          new T::TempExp(F_RDI())),
                                      nullptr);
        newframe->s_offset -= wordsize;
        formalslist->tail = new AccessList(new F::InFrameAccess(newframe->s_offset), nullptr);
        formalslist = formalslist->tail;
        vslist = vslist->tail;
        break;
      case 2:
        vslist->tail = new T::StmList(new T::MoveStm(
                                          new T::MemExp(
                                              new T::BinopExp(
                                                  T::PLUS_OP, new T::TempExp(F::F_FP()), new T::ConstExp(newframe->s_offset))),
                                          new T::TempExp(F_RSI())),
                                      nullptr);
        newframe->s_offset -= wordsize;
        formalslist->tail = new AccessList(new F::InFrameAccess(newframe->s_offset), nullptr);
        formalslist = formalslist->tail;
        vslist = vslist->tail;
        break;
      case 3:
        vslist->tail = new T::StmList(new T::MoveStm(
                                          new T::MemExp(
                                              new T::BinopExp(
                                                  T::PLUS_OP, new T::TempExp(F::F_FP()), new T::ConstExp(newframe->s_offset))),
                                          new T::TempExp(F_RDX())),
                                      nullptr);
        newframe->s_offset -= wordsize;
        formalslist->tail = new AccessList(new F::InFrameAccess(newframe->s_offset), nullptr);
        formalslist = formalslist->tail;
        vslist = vslist->tail;
        break;
      case 4:
        vslist->tail = new T::StmList(new T::MoveStm(
                                          new T::MemExp(
                                              new T::BinopExp(
                                                  T::PLUS_OP, new T::TempExp(F::F_FP()), new T::ConstExp(newframe->s_offset))),
                                          new T::TempExp(F_RCX())),
                                      nullptr);
        newframe->s_offset -= wordsize;
        formalslist->tail = new AccessList(new F::InFrameAccess(newframe->s_offset), nullptr);
        formalslist = formalslist->tail;
        vslist = vslist->tail;
        break;
      case 5:
        vslist->tail = new T::StmList(new T::MoveStm(
                                          new T::MemExp(
                                              new T::BinopExp(
                                                  T::PLUS_OP, new T::TempExp(F::F_FP()), new T::ConstExp(newframe->s_offset))),
                                          new T::TempExp(F_R8())),
                                      nullptr);
        newframe->s_offset -= wordsize;
        formalslist->tail = new AccessList(new F::InFrameAccess(newframe->s_offset), nullptr);
        formalslist = formalslist->tail;
        vslist = vslist->tail;
        break;
      case 6:
        vslist->tail = new T::StmList(new T::MoveStm(
                                          new T::MemExp(
                                              new T::BinopExp(
                                                  T::PLUS_OP, new T::TempExp(F::F_FP()), new T::ConstExp(newframe->s_offset))),
                                          new T::TempExp(F_R9())),
                                      nullptr);
        newframe->s_offset -= wordsize;
        formalslist->tail = new AccessList(new F::InFrameAccess(newframe->s_offset), nullptr);
        formalslist = formalslist->tail;
        vslist = vslist->tail;
        break;
      default:
      {
        //sequence of formals here is reversed.
        formalslist->tail = new AccessList(new F::InFrameAccess(formal_off), nullptr); 
        formalslist = formalslist->tail;
        formal_off += wordsize;
      }
      }
    }
    else
    {
      //allocate it(escape) on the temp
    }
  }

  newframe = new X64Frame(name, formals->tail, nullptr, view_shift->tail, newframe->s_offset);
  return newframe;
}

T::Stm *F_procEntryExit1(Frame *frame, T::Stm *stm)
{
  //debug
  FILE *out = stdout;
  frame->view_shift->Print(out);

  printf("------====view_shift=====-------\n");

  T::StmList *iter = frame->view_shift;
  T::SeqStm *res = new T::SeqStm(stm, new T::ExpStm(new T::ConstExp(0)));
  while (iter && iter->head)
  {
    res = new T::SeqStm(iter->head, res);
    iter = iter->tail;
  }
  return res;
}

AS::InstrList *F_procEntryExit2(AS::InstrList *body)
{

  static TEMP::TempList *returnSink = nullptr;
  if (!returnSink)
    returnSink = new TEMP::TempList(F::F_SP(), new TEMP::TempList(F::F_RAX(), nullptr));
  return AS::InstrList::Splice(body, new AS::InstrList(new AS::OperInstr("#exit2", nullptr, returnSink, nullptr), nullptr));
}

AS::Proc *F_procEntryExit3(Frame *frame, AS::InstrList *inst)
{
  std::string fs = TEMP::LabelString(frame->label) + "_framesize";

  std::string prolog;
  std::stringstream ss;
  ss << "#exit3\n .set " + fs + ",0x" << std::hex << -frame->s_offset << "\n";
  ss << "subq $0x" << std::hex << -frame->s_offset << ",%rsp\n";

  prolog = ss.str();

  std::stringstream ess;
  ess << "addq $0x" << std::hex << -frame->s_offset << ",%rsp\nret\n\n";

  std::string epilog = ess.str();
  return new AS::Proc(prolog, inst, epilog);
}

F::Access *F_allocLocal(Frame *frame, bool escape)
{
  F::Access *local;
  if (escape)
  {
    local = new F::InFrameAccess(frame->s_offset);
    frame->s_offset -= wordsize;
  }
  else
  {
    local = new F::InRegAccess(TEMP::Temp::NewTemp());
  }
  return local;
}

T::CallExp *F_externalCall(std::string s, T::ExpList *args)
{
  return new T::CallExp(new T::NameExp(TEMP::NamedLabel(s)), args);
}

//caller_saved register: rax rdi rsi rdx rcx r8 r9 r10 r11
TEMP::TempList *F_callerSaveRegs()
{
  return new TEMP::TempList(F_RAX(),
                            new TEMP::TempList(F_RDI(),
                                               new TEMP::TempList(F_RSI(),
                                                                  new TEMP::TempList(F_RDX(),
                                                                                     new TEMP::TempList(F_RCX(),
                                                                                                        new TEMP::TempList(F_R8(),
                                                                                                                           new TEMP::TempList(F_R9(),
                                                                                                                                              new TEMP::TempList(F_R10(),
                                                                                                                                                                 new TEMP::TempList(F_R11(), nullptr)))))))));
}
TEMP::Temp *F_RBP(void)
{
  if (!rbp)
    rbp = TEMP::Temp::NewTemp();
  return rbp;
}

TEMP::Temp *F_FP(void)
{
  if (!rbp)
    rbp = TEMP::Temp::NewTemp();
  return rbp;
}
TEMP::Temp *F_SP(void)
{
  if (!rsp)
    rsp = TEMP::Temp::NewTemp();
  return rsp;
}
TEMP::Temp *F_RAX(void)
{
  if (!rax)
    rax = TEMP::Temp::NewTemp();
  return rax;
}
TEMP::Temp *F_RV(void) //return value of the callee
{
  if (!rax)
    rax = TEMP::Temp::NewTemp();
  return rax;
}

TEMP::Temp *F_RDI()
{
  if (!rdi)
    rdi = TEMP::Temp::NewTemp();
  return rdi;
}
TEMP::Temp *F_RSI()
{
  if (!rsi)
    rsi = TEMP::Temp::NewTemp();
  return rsi;
}
TEMP::Temp *F_RDX()
{
  if (!rdx)
    rdx = TEMP::Temp::NewTemp();
  return rdx;
}
TEMP::Temp *F_RCX()
{
  if (!rcx)
    rcx = TEMP::Temp::NewTemp();
  return rcx;
}
TEMP::Temp *F_R8()
{
  if (!r8)
    r8 = TEMP::Temp::NewTemp();
  return r8;
}
TEMP::Temp *F_R9()
{
  if (!r9)
    r9 = TEMP::Temp::NewTemp();
  return r9;
}
TEMP::Temp *F_R10()
{
  if (!r10)
    r10 = TEMP::Temp::NewTemp();
  return r10;
}

TEMP::Temp *F_R11()
{
  if (!r11)
    r11 = TEMP::Temp::NewTemp();
  return r11;
}
TEMP::Temp *F_R12()
{
  if (!r12)
    r12 = TEMP::Temp::NewTemp();
  return r12;
}

TEMP::Temp *F_R13()
{
  if (!r13)
    r13 = TEMP::Temp::NewTemp();
  return r13;
}

TEMP::Temp *F_R14()
{
  if (!r14)
    r14 = TEMP::Temp::NewTemp();
  return r14;
}

TEMP::Temp *F_R15()
{
  if (!r15)
    r15 = TEMP::Temp::NewTemp();
  return r15;
}

TEMP::Temp *F_RBX()
{
  if (!rbx)
    rbx = TEMP::Temp::NewTemp();
  return rbx;
}

} // namespace F