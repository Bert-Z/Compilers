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
static AS::InstrList *instrList = NULL, *cur = NULL;
static TEMP::Temp *savedrbx, *savedrbp, *savedr12, *savedr13, *savedr14, *savedr15;

TEMP::TempList *L(TEMP::Temp *h, TEMP::TempList *t)
{
  assert(h);
  return new TEMP::TempList(h, t);
}
static void emit(AS::Instr *inst)
{

  if (instrList == NULL)
  {
    // fprintf(stdout, "init");
    instrList = new AS::InstrList(inst, NULL);
    cur = instrList;
  }
  else
  {
    cur->tail = new AS::InstrList(inst, NULL);
    cur = cur->tail;
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
    emit(new AS::MoveInstr("movq `s0, `d0", L(r, NULL), L(left, NULL)));
    emit(new AS::OperInstr("imulq `s0,`d0", L(r, NULL), L(right, NULL), new AS::Targets(nullptr)));

    return r;
  }
  case T::DIV_OP:
  {
    TEMP::TempList *divident = L(F::F_RAX(), L(F::F_RDX(), NULL));
    emit(new AS::MoveInstr("movq `s0, `d0", L(F::F_RAX(), NULL), L(left, NULL)));
    emit(new AS::OperInstr("cltd", NULL, NULL, new AS::Targets(nullptr)));
    emit(new AS::OperInstr("idivq `s0", NULL,
                           L(right, NULL), new AS::Targets((NULL))));
    emit(new AS::MoveInstr("movq `s0, `d0", L(r, NULL), L(F::F_RAX(), NULL)));
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
                             L(r, nullptr), L(F::F_SP(), nullptr), new AS::Targets(NULL)));
    }
    else
    {

      emit(new AS::MoveInstr("movq `s0, `d0",
                             L(r, NULL), L(left, NULL)));
      emit(new AS::OperInstr(op + "`s0, `d0", L(r, nullptr),
                             L(right, L(r, nullptr)), new AS::Targets((NULL))));
    }
    return r;
    break;
  case T::MINUS_OP:
  {
    op = "subq ";
    emit(new AS::MoveInstr("movq `s0, `d0",
                           L(r, NULL), L(left, NULL)));
    emit(new AS::OperInstr(op + "`s0, `d0", L(r, nullptr),
                           L(right, L(r, nullptr)), new AS::Targets((NULL))));
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
                         NULL, NULL));
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
    if (left->kind == T::Exp::TEMP && ((T::TempExp *)left)->temp == F::F_FP())
    {
      assert(right->kind == T::Exp::CONST);
      int offset = ((T::ConstExp *)right)->consti;
      std::string instr;
      std::stringstream ioss;
      ioss << "#BINOP F_FP munch mem\nmovq (" + fs + "-0x" << std::hex << -offset << ")(`s0),`d0";

      instr = ioss.str();
      emit(new AS::OperInstr(instr,
                             L(r, nullptr), L(F::F_SP(), nullptr), new AS::Targets(NULL)));
      return r;
    }
    else
    {
      TEMP::Temp *lefttemp = munchExp(left);
      TEMP::Temp *righttemp = munchExp(right);
      emit(new AS::OperInstr("#BINOP munch mem\naddq `s0,`d0", L(righttemp, NULL),
                             L(lefttemp, L(righttemp, NULL)), new AS::Targets(NULL)));
      emit(new AS::OperInstr("#BINOP munch mem\nmovq (`s0),`d0", L(r, NULL), L(righttemp, NULL), new AS::Targets(NULL)));
      return r;
    }
  }
  default:
  {
    TEMP::Temp *s = munchExp(e);
    emit(new AS::MoveInstr("#  munch mem\nmovq (`s0), `d0",
                           L(r, NULL), L(s, NULL)));
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
                         L(r, NULL), NULL, new AS::Targets(nullptr)));
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
      emit(new AS::MoveInstr("#munchArgs \nmovq `s0,`d0", L(F::F_RDI(), NULL), L(arg, NULL)));
      break;
    case 2:
      emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_RSI(), NULL), L(arg, NULL)));
      break;
    case 3:
      emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_RDX(), NULL), L(arg, NULL)));
      break;
    case 4:
      emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_RCX(), NULL), L(arg, NULL)));
      break;
    case 5:
      emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_R8(), NULL), L(arg, NULL)));
      break;
    case 6:
      emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_R9(), NULL), L(arg, NULL)));
      break;
    default:
      emit(new AS::OperInstr("pushq `s0", NULL, L(arg, NULL), new AS::Targets(nullptr)));
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
  emit(new AS::OperInstr(assem, F::F_callerSaveRegs(), nullptr, new AS::Targets(NULL)));

  TEMP::Temp *r = TEMP::Temp::NewTemp();
  emit(new AS::MoveInstr("#return value\nmovq `s0, `d0", L(r, NULL),
                         L(F::F_RAX(), NULL)));

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
      emit(new AS::OperInstr(inst, L(t, NULL), L(F::F_SP(), NULL), new AS::Targets(NULL)));
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
                           L(((T::TempExp *)dst)->temp, NULL),
                           L(left, NULL)));
    return;
  }
  if (dst->kind == T::Exp::MEM)
  {

    TEMP::Temp *left = munchExp(src);

    TEMP::Temp *right = munchExp(((T::MemExp *)dst)->exp);
    emit(new AS::MoveInstr("movq `s0, (`s1)", NULL,
                           L(left, L(right, NULL))));
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
    emit(new AS::OperInstr("cmp `s0,`s1", NULL,
                           L(right, L(left, NULL)),
                           new AS::Targets(NULL)));

    emit(new AS::OperInstr(op + TEMP::LabelString(true_label), NULL, NULL,
                           new AS::Targets(new TEMP::LabelList(true_label, NULL))));
    return;
  }
  case T::Stm::JUMP:
  {
    std::string label = ((T::NameExp *)(((T::JumpStm *)stm)->exp))->name->Name();
    emit(new AS::OperInstr("jmp " + label, NULL, NULL,
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
  emit(new AS::MoveInstr("#saveCalleeRegs\nmovq `s0,`d0", L(savedrbx, NULL), L(F::F_RBX(), NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(savedrbp, NULL), L(F::F_RBP(), NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(savedr12, NULL), L(F::F_R12(), NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(savedr13, NULL), L(F::F_R13(), NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(savedr14, NULL), L(F::F_R14(), NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(savedr15, NULL), L(F::F_R15(), NULL)));
}
static void restoreCalleeRegs(void)
{
  emit(new AS::MoveInstr("#restoreCalleeRegs\nmovq `s0,`d0", L(F::F_RBX(), NULL), L(savedrbx, NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_RBP(), NULL), L(savedrbp, NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_R12(), NULL), L(savedr12, NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_R13(), NULL), L(savedr13, NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_R14(), NULL), L(savedr14, NULL)));
  emit(new AS::MoveInstr("movq `s0,`d0", L(F::F_R15(), NULL), L(savedr15, NULL)));
}
AS::InstrList *Codegen(F::Frame *f, T::StmList *stmList)
{
  // TODO: Put your codes here (lab6).
  fs = TEMP::LabelString(f->label) + "_framesize";
  instrList = NULL;
  //saveCalleeRegs();
  //push reg in stack
  for (; stmList; stmList = stmList->tail)
  {
    munchStm(stmList->head);
  }

  //restoreCalleeRegs();
  return F::F_procEntryExit2(instrList);
}

} // namespace CG