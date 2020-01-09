#include "tiger/codegen/codegen.h"
#include "tiger/translate/translate.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"
#include "stdio.h"
#include "tiger/translate/tree.h"

#include <iostream>
#include <sstream>

namespace CG
{

static std::string fs;
static AS::InstrList *instrList = nullptr, *curtail = nullptr;
static TEMP::Temp *savedrbx, *savedrbp, *savedr12, *savedr13, *savedr14, *savedr15;

TEMP::TempList *L(TEMP::Temp *h, TEMP::TempList *t)
{
  return new TEMP::TempList(h, t);
}
static void emit(AS::Instr *inst)
{

  if (instrList == nullptr)
  {
    // fprintf(stdout, "init");
    instrList = new AS::InstrList(inst, nullptr);
    curtail = instrList;
  }
  else
  {
    curtail->tail = new AS::InstrList(inst, nullptr);
    curtail = curtail->tail;
  }
}
static TEMP::Temp *munchOpExp(T::BinopExp *exp)
{
  TEMP::Temp *left = munchExp(exp->left), *right = munchExp(exp->right);
  TEMP::Temp *r = TEMP::Temp::NewTemp();

  std::string op;
  switch (exp->op)
  {
  case T::MUL_OP:
  {
    emit(new AS::MoveInstr("movq `s0, `d0", L(r, nullptr), L(left, nullptr)));
    emit(new AS::OperInstr("imulq `s0,`d0", L(r, nullptr), L(right, nullptr), nullptr));

    return r;
  }
  case T::DIV_OP:
  {
    emit(new AS::MoveInstr("movq `s0, `d0", L(F::F_RAX(), nullptr), L(left, nullptr)));
    emit(new AS::OperInstr("cltd", L(F::F_RAX(), L(F::F_RDX(),nullptr)), L(F::F_RAX(),nullptr), new AS::Targets(nullptr)));
    emit(new AS::OperInstr("idivq `s0", nullptr,
                           L(right,nullptr), new AS::Targets((nullptr))));
    emit(new AS::MoveInstr("movq `s0, `d0", L(r, nullptr), L(F::F_RAX(), nullptr)));
    return r;
  }
  case T::PLUS_OP:
    op = "addq ";
    if (exp->left->kind == T::Exp::TEMP && left == F::F_FP() && exp->right->kind == T::Exp::CONST)
    {
      int rightvalue = ((T::ConstExp *)(exp->right))->consti;
      std::string instr;
      std::stringstream ioss;
      ioss << "leaq (" + fs + "-0x" << std::hex << -rightvalue << ")(`s0),`d0";

      instr = ioss.str();
      emit(new AS::OperInstr(instr,
                             L(r, nullptr), L(F::F_SP(), nullptr), nullptr));
    }
    else
    {

      emit(new AS::MoveInstr("movq `s0, `d0",
                             L(r, nullptr), L(left, nullptr)));
      emit(new AS::OperInstr(op + "`s0, `d0", L(r, nullptr),
                             L(right, L(r, nullptr)), new AS::Targets((nullptr))));
    }
    return r;
    break;
  case T::MINUS_OP:
  {
    op = "subq ";
    emit(new AS::MoveInstr("movq `s0, `d0",
                           L(r, nullptr), L(left, nullptr)));
    emit(new AS::OperInstr(op + "`s0, `d0", L(r, nullptr),
                           L(right, L(r, nullptr)), new AS::Targets((nullptr))));
    return r;
    break;
  }

  default:
    assert(0);
  }
  assert(0);
  return nullptr;
}
static TEMP::Temp *munchConstExp(T::ConstExp *exp)
{

  TEMP::Temp *r = TEMP::Temp::NewTemp();
  emit(new AS::OperInstr("movq $" + std::to_string(exp->consti) + " ,`d0",
                         L(r, nullptr),
                         nullptr, nullptr));
  return r;
}
static TEMP::Temp *munchMemExp(T::MemExp *exp)
{
  T::Exp *e = exp->exp;
  TEMP::Temp *r = TEMP::Temp::NewTemp();
  switch (e->kind)
  {
  case T::Exp::BINOP:
  {
    T::Exp *left = ((T::BinopExp *)e)->left;
    T::Exp *right = ((T::BinopExp *)e)->right;
    if (left->kind == T::Exp::TEMP && ((T::TempExp *)left)->temp == F::F_FP() && right->kind == T::Exp::CONST)
    {
      int offset = ((T::ConstExp *)right)->consti;
      std::string instr;
      std::stringstream ioss;
      ioss << "#BINOP F_FP munch mem\nmovq (" + fs + "-0x" << std::hex << -offset << ")(`s0),`d0";

      instr = ioss.str();
      emit(new AS::OperInstr(instr,
                             L(r, nullptr), L(F::F_SP(), nullptr), nullptr));
      return r;
    }
    else
    {
      TEMP::Temp *lefttemp = munchExp(left);
      TEMP::Temp *righttemp = munchExp(right);
      emit(new AS::OperInstr("#BINOP munch mem\naddq `s0,`d0", L(righttemp, nullptr),
                             L(lefttemp, L(righttemp, nullptr)), nullptr));
      emit(new AS::OperInstr("#BINOP munch mem\nmovq (`s0),`d0", L(r, nullptr), L(righttemp, nullptr), nullptr));
      return r;
    }
  }
  default:
  {
    TEMP::Temp *s = munchExp(e);
    emit(new AS::MoveInstr("#  munch mem\nmovq (`s0), `d0",
                           L(r, nullptr), L(s, nullptr)));
    return r;
  }
  }
  assert(0);
  return nullptr;
}
static TEMP::Temp *munchNameExp(T::NameExp *exp)
{
  TEMP::Temp *r = TEMP::Temp::NewTemp();
  std::string assem = "leaq " + exp->name->Name() + "(%rip),`d0";
  emit(new AS::OperInstr(assem,
                         L(r, nullptr), nullptr, nullptr));
  return r;
}
static void munchArgs(T::ExpList *list)
{
  int num = 1;
  TEMP::Temp *arg = TEMP::Temp::NewTemp();

  for (T::ExpList *l = list; l; l = l->tail, num++)
  {
    arg = munchExp(l->head);
    switch (num)
    {
    case 1:
      emit(new AS::MoveInstr("#munchArgs \nmovq `s0,`d0", L(F::F_RDI(), nullptr), L(arg, nullptr)));
      break;
    case 2:
      emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_RSI(), nullptr), L(arg, nullptr)));
      break;
    case 3:
      emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_RDX(), nullptr), L(arg, nullptr)));
      break;
    case 4:
      emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_RCX(), nullptr), L(arg, nullptr)));
      break;
    case 5:
      emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_R8(), nullptr), L(arg, nullptr)));
      break;
    case 6:
      emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_R9(), nullptr), L(arg, nullptr)));
      break;
    default:
      emit(new AS::OperInstr("pushq `s0", nullptr, L(arg, nullptr), nullptr));
      break;
    }
  }
}

int TempListLength(TEMP::TempList *l)
{
  if (!l)
    return 0;
  return TempListLength(l->tail) + 1;
}

static TEMP::Temp *munchCallExp(T::CallExp *exp)
{

  T::Exp *func_exp = ((exp)->fun);
  assert(func_exp->kind == T::Exp::NAME);

  TEMP::Label *func = ((T::NameExp *)func_exp)->name;

  std::string func_name = TEMP::LabelString(((T::NameExp *)func_exp)->name);

  T::ExpList *args = (exp)->args;
  munchArgs(args);

  std::string assem = "call " + func_name;
  emit(new AS::OperInstr(assem, F::F_callerSaveRegs(), nullptr, nullptr));

  TEMP::Temp *r = TEMP::Temp::NewTemp();
  emit(new AS::MoveInstr("#return value\nmovq `s0, `d0", L(r, nullptr),
                         L(F::F_RAX(), nullptr)));

  return r;
}
static TEMP::Temp *munchExp(T::Exp *exp)
{
  switch (exp->kind)
  {
  case T::Exp::BINOP:
  {
    return munchOpExp((T::BinopExp *)exp);
  }
  case T::Exp::MEM:
  {
    return munchMemExp((T::MemExp *)exp);
  }
  case T::Exp::TEMP:
  {
    TEMP::Temp *t = ((T::TempExp *)exp)->temp;
    if (t == F::F_FP())
    {

      std::string inst = "leaq " + fs + "(`s0),`d0";
      t = TEMP::Temp::NewTemp();
      emit(new AS::OperInstr(inst, L(t, nullptr), L(F::F_SP(), nullptr), nullptr));
    }
    return t;
  }
  case T::Exp::ESEQ:
  {
    assert(0);
  }
  case T::Exp::NAME:
  {
    return munchNameExp((T::NameExp *)exp);
  }
  case T::Exp::CONST:
  {
    return munchConstExp((T::ConstExp *)exp);
  }
  case T::Exp::CALL:
  {
    return munchCallExp((T::CallExp *)exp);
  }
  }
}
static void munchMoveStm(T::MoveStm *stm)
{
  T::Exp *dst = stm->dst;
  T::Exp *src = stm->src;
  /*movq mem,reg; movq reg,mem; movq irr,mem; movq reg,reg*/
  if (dst->kind == T::Exp::TEMP)
  {

    TEMP::Temp *left = munchExp(src);
    emit(new AS::MoveInstr("movq `s0, `d0",
                           L(((T::TempExp *)dst)->temp, nullptr),
                           L(left, nullptr)));
    return;
  }
  if (dst->kind == T::Exp::MEM)
  {

    TEMP::Temp *left = munchExp(src);

    TEMP::Temp *right = munchExp(((T::MemExp *)dst)->exp);
    emit(new AS::MoveInstr("movq `s0, (`s1)", nullptr,
                           L(left, L(right, nullptr))));
    return;
  }
}
static void munchLabelStm(T::LabelStm *stm)
{
  TEMP::Label *label = stm->label;
  emit(new AS::LabelInstr(label->Name(), label));
}
static void munchStm(T::Stm *stm)
{
  switch (stm->kind)
  {
  /*movq mem,reg; movq reg,mem; movq irr,mem; movq reg,reg*/
  /* need to munch src exp first */
  case T::Stm::MOVE:
  {
    return munchMoveStm((T::MoveStm *)stm);
  }
  case T::Stm::LABEL:
  {
    return munchLabelStm((T::LabelStm *)stm);
  }
  case T::Stm::CJUMP:
  {
    std::string op;
    TEMP::Temp *left = munchExp(((T::CjumpStm *)stm)->left);
    TEMP::Temp *right = munchExp(((T::CjumpStm *)stm)->right);
    TEMP::Label *true_label = ((T::CjumpStm *)stm)->true_label;
    switch (((T::CjumpStm *)stm)->op)
    {
    case T::EQ_OP:
      op = "je ";
      break;
    case T::NE_OP:
      op = "jne ";
      break;
    case T::LT_OP:
      op = "jl ";
      break;
    case T::GT_OP:
      op = "jg ";
      break;
    case T::LE_OP:
      op = "jle ";
      break;
    case T::GE_OP:
      op = "jge ";
      break;
    }
    emit(new AS::OperInstr("cmp `s0,`s1", nullptr,
                           L(right, L(left, nullptr)),
                           nullptr));

    emit(new AS::OperInstr(op + TEMP::LabelString(true_label), nullptr, nullptr,
                           new AS::Targets(new TEMP::LabelList(true_label, nullptr))));
    return;
  }
  case T::Stm::JUMP:
  {
    std::string label = ((T::NameExp *)(((T::JumpStm *)stm)->exp))->name->Name();
    emit(new AS::OperInstr("jmp " + label, nullptr, nullptr,
                           new AS::Targets(((T::JumpStm *)stm)->jumps)));
    return;
  }
  case T::Stm::EXP:
  {
    munchExp(((T::ExpStm *)stm)->exp);
    return;
  }
  }
}

static void saveCalleeRegs()
{

  savedrbx = TEMP::Temp::NewTemp();
  savedrbp = TEMP::Temp::NewTemp(); //Saved rbp here.
  savedr12 = TEMP::Temp::NewTemp();
  savedr13 = TEMP::Temp::NewTemp();
  savedr14 = TEMP::Temp::NewTemp();
  savedr15 = TEMP::Temp::NewTemp();
  emit(new AS::MoveInstr("#saveCalleeRegs\nmovq `s0,`d0", L(savedrbx, nullptr), L(F::F_RBX(), nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(savedrbp, nullptr), L(F::F_RBP(), nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(savedr12, nullptr), L(F::F_R12(), nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(savedr13, nullptr), L(F::F_R13(), nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(savedr14, nullptr), L(F::F_R14(), nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(savedr15, nullptr), L(F::F_R15(), nullptr)));
}
static void restoreCalleeRegs(void)
{
  emit(new AS::MoveInstr("#restoreCalleeRegs\nmovq `s0,`d0", L(F::F_RBX(), nullptr), L(savedrbx, nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_RBP(), nullptr), L(savedrbp, nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_R12(), nullptr), L(savedr12, nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_R13(), nullptr), L(savedr13, nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_R14(), nullptr), L(savedr14, nullptr)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_R15(), nullptr), L(savedr15, nullptr)));
}
AS::InstrList *Codegen(F::Frame *f, T::StmList *stmList)
{
  // TODO: Put your codes here (lab6).
  fs = TEMP::LabelString(f->label) + "_framesize";
  instrList = nullptr;
  saveCalleeRegs();
  //push reg in stack
  for (; stmList; stmList = stmList->tail)
  {
    munchStm(stmList->head);
  }

  restoreCalleeRegs();
  return F::F_procEntryExit2(instrList);
}

} // namespace CG