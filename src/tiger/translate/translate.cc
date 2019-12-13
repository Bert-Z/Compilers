#include "tiger/translate/translate.h"
#include "tiger/errormsg/errormsg.h"

#include <cstdio>
#include <set>
#include <string>
#include <iostream>

#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/temp.h"
#include "tiger/semant/semant.h"
#include "tiger/semant/types.h"
#include "tiger/util/util.h"

extern EM::ErrorMsg errormsg;

namespace TR
{
// fraglist
static F::FragList *frags;

// wordsize
static const int WordSize = 8;

// main frame level
static Level *outermost = nullptr;

//staticLink: 当前层level和目标层level，得到目标level的静态链的fp
static T::Exp *staticLink(Level *level, Level *dest)
{
  T::Exp *exp = new T::TempExp(F::F_FP());
  Level *curlevel = level;
  while (curlevel && curlevel != dest)
  {
    if (curlevel == outermost)
    {
      exp = new T::MemExp(exp);
      break;
    }

    // 获得上一层frame的fp
    exp = F::getExp(curlevel->frame->formals->head, exp);
    curlevel = curlevel->parent;
  }
  return exp;
}

// generate TR::AccessList 生成TR中的formals
static AccessList *makeFormals(TR::Level *level, F::AccessList *formals)
{
  if (formals)
    return new TR::AccessList(new TR::Access(level, formals->head), makeFormals(level, formals->tail));
  else
    return nullptr;
}

Access *Access::allocLocal(Level *level, bool escape)
{
  return new Access(level, F::allocLocal(level->frame, escape));
}

Level *Level::NewLevel(Level *parent, TEMP::Label *name,
                       U::BoolList *formals)
{
  Level *level = new Level(F::Frame::newFrame(name, formals), parent);

  // maintain level formals
  AccessList *accesslist = makeFormals(level, level->frame->formals->tail);
  level->formals = accesslist;

  return level;
}

// ExExp
T::Exp *ExExp::UnEx() const
{
  return this->exp;
}

T::Stm *ExExp::UnNx() const
{
  return new T::ExpStm(this->exp);
}

Cx ExExp::UnCx() const
{
  //cx: if(ex), label的patch在translate过程中写入
  T::Stm *stm = new T::CjumpStm(T::RelOp::NE_OP, this->exp, new T::ConstExp(0), nullptr, nullptr);
  PatchList *trues = new PatchList(&(((T::CjumpStm *)stm)->true_label), nullptr);
  PatchList *falses = new PatchList(&(((T::CjumpStm *)stm)->false_label), nullptr);
  return Cx(trues, falses, stm);
}

// NxExp
T::Exp *NxExp::UnEx() const
{
  return new T::EseqExp(this->stm, new T::ConstExp(0));
}

T::Stm *NxExp::UnNx() const
{
  return this->stm;
}

Cx NxExp::UnCx() const
{
  // 不存在情况，应当拒绝
  printf("error: NxExp can not UnCx()!");
  return;
}

// CxExp
T::Exp *CxExp::UnEx() const
{
  TEMP::Temp *r = TEMP::Temp::NewTemp();
  TEMP::Label *t = TEMP::NewLabel();
  TEMP::Label *f = TEMP::NewLabel();

  do_patch(this->cx.trues, t);
  // join_patch?
  do_patch(this->cx.falses, f);
  return new T::EseqExp(new T::MoveStm(new T::TempExp(r), new T::ConstExp(1)),
                        new T::EseqExp(this->cx.stm,
                                       new T::EseqExp(new T::LabelStm(f),
                                                      new T::EseqExp(new T::MoveStm(new T::TempExp(r), new T::ConstExp(0)),
                                                                     new T::EseqExp(new T::LabelStm(t),
                                                                                    new T::TempExp(r))))));
}

T::Stm *CxExp::UnNx() const
{
  // 忽略跳转
  TEMP::Label *empty = TEMP::NewLabel();
  do_patch(this->cx.trues, empty);
  do_patch(this->cx.falses, empty);
  return new T::SeqStm(this->cx.stm, new T::LabelStm(empty));
}

Cx CxExp::UnCx() const
{
  return this->cx;
}

void do_patch(PatchList *tList, TEMP::Label *label)
{
  for (; tList; tList = tList->tail)
    *(tList->head) = label;
}

PatchList *join_patch(PatchList *first, PatchList *second)
{
  if (!first)
    return second;
  for (; first->tail; first = first->tail)
    ;
  first->tail = second;
  return first;
}

Level *Outermost()
{
  static Level *lv = nullptr;
  if (lv != nullptr)
    return lv;

  lv = new Level(nullptr, nullptr);
  return lv;
}

F::FragList *TranslateProgram(A::Exp *root)
{
  // TODO: Put your codes here (lab5).
  
  return nullptr;
}

T::Stm *procEntryExit(Exp *body, Level *level, AccessList *formals)
{
  return new T::MoveStm(new T::TempExp(F::F_RV()), body->UnEx());
}

void functionDec(TR::Exp *exp, TR::Level *level)
{
  T::Stm *stm = procEntryExit(exp, level, level->formals);
  F::Frag *head = new F::ProcFrag(stm, level->frame);
  frags = new F::FragList(head, frags);
}

} // namespace TR

namespace A
{

//finish
//EX, 栈上的变量，当前level
TR::ExpAndTy SimpleVar::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  E::EnvEntry *env_entry = venv->Look(this->sym);
  if (!env_entry)
  {
    errormsg.Error(this->pos, "undefined variable %s\n", this->sym->Name().c_str());
    return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
  }
  E::VarEntry *var_entry = (E::VarEntry *)env_entry;

  return TR::ExpAndTy(TR::Tr_SimpleVar(var_entry->access, level), var_entry->ty);
}

//finish
//EX, 首地址，field数, address of record
TR::ExpAndTy FieldVar::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *ltype = var->Translate(venv, tenv, level, label).ty;
  // check var
  if (ltype->kind != TY::Ty::RECORD)
  {
    errormsg.Error(this->pos, "not a record type");
    return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
  }

  // check field
  TY::FieldList *fields = ((TY::RecordTy *)ltype)->fields;
  TY::Field *field;

  // 首地址 field个数
  TR::Exp *addr = var->Translate(venv, tenv, level, label).exp;
  int depth = 0;

  for (; fields; fields = fields->tail)
  {
    field = fields->head;
    if (field->name == this->sym)
    {
      // return field->ty;
      T::Exp *exp = new T::MemExp(new T::BinopExp(T::PLUS_OP, addr->UnEx(), new T::ConstExp(depth * TR::WordSize)));

      return TR::ExpAndTy(new TR::ExExp(exp), field->ty);
    }
    depth++;
  }
  errormsg.Error(this->pos, "field %s doesn't exist", this->sym->Name().c_str());

  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

//finish
//EX, 首地址，取值数,address of array
//address (i-l)*s + a
TR::ExpAndTy SubscriptVar::Translate(S::Table<E::EnvEntry> *venv,
                                     S::Table<TY::Ty> *tenv, TR::Level *level,
                                     TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *ltype = var->Translate(venv, tenv, level, label).ty;
  TR::Exp *addr = var->Translate(venv, tenv, level, label).exp;
  TR::Exp *index = subscript->Translate(venv, tenv, level, label).exp;

  if (ltype->kind != TY::Ty::ARRAY)
  {
    errormsg.Error(this->pos, "array type required");
    return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
  }

  TY::Ty *exptyppe = subscript->Translate(venv, tenv, level, label).ty;

  if (exptyppe->kind != TY::Ty::INT)
  {
    errormsg.Error(this->pos, "array index must be integer");
    return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
  }

  T::Exp *exp = new T::MemExp(
      new T::BinopExp(T::PLUS_OP,
                      addr->UnEx(),
                      new T::BinopExp(
                          T::MUL_OP,
                          new T::BinopExp(
                              T::MINUS_OP,
                              index->UnEx(),
                              new T::ConstExp(0)),
                          new T::ConstExp(TR::WordSize))));

  return TR::ExpAndTy(new TR::ExExp(exp), ((TY::ArrayTy *)ltype)->ty);
}

//finish
//not sure
TR::ExpAndTy VarExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *var_type = var->Translate(venv, tenv, level, label).ty->ActualTy();

  TR::Exp *exp = var->Translate(venv, tenv, level, label).exp;

  return TR::ExpAndTy(exp, var_type);
}

//finish
TR::ExpAndTy NilExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(new TR::ExExp(new T::ConstExp(0)), TY::NilTy::Instance());
}

//finish
TR::ExpAndTy IntExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(new TR::ExExp(new T::ConstExp(i)), TY::NilTy::Instance());
}

//finish
TR::ExpAndTy StringExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TEMP::Label *r = TEMP::NewLabel();
  F::Frag *head = new F::StringFrag(r, s);
  TR::frags = new F::FragList(head, TR::frags);
  TR::ExExp *exp = new TR::ExExp(new T::NameExp(label));

  return TR::ExpAndTy(exp, TY::StringTy::Instance());
}

static T::ExpList *makeCallArgs(TR::ExpList *params)
{
  T::ExpList *exps = nullptr;
  for (; params; params = params->tail)
  {
    exps = new T::ExpList(params->head->UnEx(), exps);
  }
  return exps;
}

//finish
//NX/EX, 函数名，变量序列
//T_exp T_Call(T_exp, T_expList);
//static T_exp staticlink(Tr_level level, Tr_level dest)
//传递静态链作为隐藏参数
//CALL(NAME lf, [sl, e1, e2, …,en])
//Both the level of f and the level of the function calling f are required to calculate sl
TR::ExpAndTy CallExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  E::EnvEntry *env_entry = venv->Look(this->func);
  A::ExpList *args = this->args;
  if (!env_entry || env_entry->kind != E::EnvEntry::FUN)
  {
    printf("%x undefined function %s", this->pos, this->func->Name().c_str());
    return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
  }
  TY::TyList *funcargs = ((E::FunEntry *)env_entry)->formals;
  TY::Ty *funcarg;
  A::Exp *arg;

  TR::ExpList *params = nullptr;

  for (; args && funcargs; args = args->tail, funcargs = funcargs->tail)
  {
    arg = args->head;
    funcarg = funcargs->head;
    TY::Ty *expty = arg->Translate(venv, tenv, level, label).ty;
    TR::Exp *argexp = arg->Translate(venv, tenv, level, label).exp;

    params = new TR::ExpList(argexp, params);

    if (!expty->IsSameType(funcarg))
    {
      errormsg.Error(this->pos, "para type mismatch");
    }
  }
  if (args)
  {
    errormsg.Error(this->pos, "too many params in function %s", this->func->Name().c_str());
  }

  TEMP::Label *func_label = ((E::FunEntry *)env_entry)->label;
  TR::Level *callee_level = ((E::FunEntry *)env_entry)->level;
  TR::Level *caller_level = level;

  T::Exp *ep = new T::CallExp(
      new T::NameExp(func),
      new T::ExpList(
          TR::staticLink(caller_level, callee_level->parent),
          makeCallArgs(params)));

  return TR::ExpAndTy(new TR::ExExp(ep), ((E::FunEntry *)env_entry)->result);
}

// 算术运算
static TR::Exp *TRCalOp(A::Oper op, TR::Exp *left, TR::Exp *right)
{
  T::Exp *le = left->UnEx();
  T::Exp *ri = right->UnEx();
  switch (op)
  {
  case A::Oper::PLUS_OP:
    return new TR::ExExp(new T::BinopExp(T::PLUS_OP, le, ri));
  case A::Oper::TIMES_OP:
    return new TR::ExExp(new T::BinopExp(T::MUL_OP, le, ri));
  case A::Oper::MINUS_OP:
    return new TR::ExExp(new T::BinopExp(T::MINUS_OP, le, ri));
  case A::Oper::DIVIDE_OP:
    return new TR::ExExp(new T::BinopExp(T::DIV_OP, le, ri));
  }
  assert(0);
}

// 比较运算
static TR::Exp *TRCondOp(A::Oper op, TR::Exp *left, TR::Exp *right)
{
  T::Exp *le = left->UnEx();
  T::Exp *ri = right->UnEx();
  T::Stm *stm;

  switch (op)
  {
  case A::Oper::LT_OP:
    stm = new T::CjumpStm(T::LT_OP, le, ri, nullptr, nullptr);
    break;
  case A::Oper::LE_OP:
    stm = new T::CjumpStm(T::LE_OP, le, ri, nullptr, nullptr);
    break;
  case A::Oper::GT_OP:
    stm = new T::CjumpStm(T::GT_OP, le, ri, nullptr, nullptr);
    break;
  case A::Oper::GE_OP:
    stm = new T::CjumpStm(T::GE_OP, le, ri, nullptr, nullptr);
    break;
  case A::Oper::EQ_OP:
    stm = new T::CjumpStm(T::EQ_OP, le, ri, nullptr, nullptr);
    break;
  case A::Oper::NEQ_OP:
    stm = new T::CjumpStm(T::NE_OP, le, ri, nullptr, nullptr);
    break;
  default:
    assert(0);
  }

  TR::PatchList *trues = new TR::PatchList(&(((T::CjumpStm *)stm)->true_label), nullptr);
  TR::PatchList *falses = new TR::PatchList(&(((T::CjumpStm *)stm)->false_label), nullptr);

  return new TR::CxExp(trues, falses, stm);
}

//finish
// OpExp
TR::ExpAndTy OpExp::Translate(S::Table<E::EnvEntry> *venv,
                              S::Table<TY::Ty> *tenv, TR::Level *level,
                              TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).

  TY::Ty *leftty = this->left->Translate(venv, tenv, level, label).ty;
  TY::Ty *rightty = this->right->Translate(venv, tenv, level, label).ty;

  // label not sure
  TR::Exp *lefttr = this->left->Translate(venv, tenv, level, label).exp;
  TR::Exp *righttr = this->right->Translate(venv, tenv, level, label).exp;

  switch (oper)
  {
  case A::Oper::PLUS_OP:
  case A::Oper::TIMES_OP:
  case A::Oper::MINUS_OP:
  case A::Oper::DIVIDE_OP:
  {
    if (leftty->kind != TY::Ty::INT)
      printf("%x integer required", this->pos);

    if (rightty->kind != TY::Ty::INT)
      printf("%x integer required", this->pos);

    // translate
    TR::Exp *exp = TRCalOp(oper, lefttr, righttr);

    return TR::ExpAndTy(exp, TY::IntTy::Instance());
  }
  case A::Oper::LT_OP:
  case A::Oper::LE_OP:
  case A::Oper::GT_OP:
  case A::Oper::GE_OP:
  case A::Oper::EQ_OP:
  case A::Oper::NEQ_OP:
  {
    if (leftty->kind != TY::Ty::INT && leftty->kind != TY::Ty::STRING)
      printf("%x integer or string required", this->pos);

    if (rightty->kind != TY::Ty::INT && rightty->kind != TY::Ty::STRING)
      printf("%x integer or string required", this->pos);

    if (!rightty->IsSameType(leftty))
      printf("same type required", this->pos);

    break;
  }
  default:
  {
    break;
  }
  }

  TR::Exp *ep = TRCondOp(oper, lefttr, righttr);

  return TR::ExpAndTy(ep, TY::IntTy::Instance());
}

static T::Stm *mk_record_array(TR::ExpList *fields, TEMP::Temp *r, int offset, int size)
{
  if (size > 1)
  {
    if (offset < size - 2)
    {
      return new T::SeqStm(
          new T::MoveStm(new T::BinopExp(T::PLUS_OP, new T::TempExp(r), new T::ConstExp(offset * TR::WordSize)), fields->head->UnEx()),
          mk_record_array(fields->tail, r, offset + 1, size));
    }
    else
    {
      return new T::SeqStm(
          new T::MoveStm(new T::BinopExp(T::PLUS_OP, new T::TempExp(r), new T::ConstExp(offset * TR::WordSize)), fields->head->UnEx()),
          new T::MoveStm(new T::BinopExp(T::PLUS_OP, new T::TempExp(r), new T::ConstExp((offset + 1) * TR::WordSize)), fields->tail->head->UnEx()));
    }
  }
  else
  {
    return new T::MoveStm(new T::BinopExp(T::PLUS_OP, new T::TempExp(r), new T::ConstExp(offset * TR::WordSize)), fields->head->UnEx());
  }
}

//finish
//EX, n是field的个数
//	It must be allocated on the heap
//	Call an external memory-allocation function
//	creates an n-word area
//	returns the pointer into a new temporary r
TR::ExpAndTy RecordExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *typty = tenv->Look(this->typ);
  if (!typty)
  {
    errormsg.Error(this->pos, "undefined type %s", this->typ->Name().c_str());
    return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
  }

  TR::ExpList *fields = new TR::ExpList(nullptr, nullptr);
  TR::ExpList *tail = fields;
  A::EFieldList *eflist = this->fields;
  TY::FieldList *fieldlist = ((TY::RecordTy *)typ)->fields;
  A::EField *efield;
  TY::Field *field;
  for (; eflist && fieldlist; eflist = eflist->tail, fieldlist = fieldlist->tail)
  {
    if (!fieldlist)
    {
      errormsg.Error(this->pos, "Too many efields in %s", this->typ->Name().c_str());
      break;
    }
    efield = eflist->head;
    field = fieldlist->head;
    TY::Ty *expty = efield->exp->Translate(venv, tenv, level, label).ty;
    if (!expty->IsSameType(field->ty))
    {
      errormsg.Error(this->pos, "record type unmatched");
    }
    tail->tail = new TR::ExpList(efield->exp->Translate(venv, tenv, level, label).exp, nullptr);
    tail = tail->tail;
  }

  fields = fields->tail;

  int count = 0;
  for (TR::ExpList *cnt_field = fields; cnt_field; cnt_field = cnt_field->tail, count++)
    ;
  TEMP::Temp *r = TEMP::Temp::NewTemp();
  T::Stm *stm =
      new T::MoveStm(
          new T::TempExp(r),
          F::externalCall("malloc", new T::ExpList(new T::ConstExp(count * TR::WordSize), nullptr)));
  stm = new T::SeqStm(stm, mk_record_array(fields, r, 0, count));

  return TR::ExpAndTy(new TR::ExExp(new T::EseqExp(stm, new T::TempExp(r))), typty);
}

static TR::Exp *Tr_Seq(TR::Exp *left, TR::Exp *right)
{
  T::Exp *e;
  if (right)
  {
    e = new T::EseqExp(left->UnNx(), right->UnEx());
  }
  else
  {
    e = new T::EseqExp(left->UnNx(), new T::ConstExp(0));
  }
  return new TR::ExExp(e);
}

// finish
TR::ExpAndTy SeqExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).

  // 初始化e1为nilExp
  TR::Exp *e1 = new TR::ExExp(new T::ConstExp(0));
  TR::Exp *e2;

  A::ExpList *explist = this->seq;
  if (!explist)
  {
    return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
  }
  TY::Ty *expty;
  for (; explist; explist = explist->tail)
  {
    expty = explist->head->Translate(venv, tenv, level, label).ty;
    e2 = explist->head->Translate(venv, tenv, level, label).exp;
    e1 = Tr_seqExp(e1, e2);
  }

  return TR::ExpAndTy(e1, expty);
}

TR::Exp *Tr_seqExp(TR::Exp *e1, TR::Exp *e2)
{
  T::Exp *exp = new T::EseqExp(e1->UnNx(), e2->UnEx());
  return new TR::ExExp(exp);
}

TR::Exp *Tr_SimpleVar(TR::Access *access, TR::Level *level)
{
  T::Exp *fp = TR::staticLink(level, access->level);
  T::Exp *ex = F::getExp(access->access, fp);
  return new TR::ExExp(ex);
}

TR::Exp *Tr_Assign(TR::Exp *var, TR::Exp *exp)
{
  return new TR::NxExp(new T::MoveStm(var->UnEx(), exp->UnEx()));
}

//finish
TR::ExpAndTy AssignExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).

  if (this->var->kind == A::Var::SIMPLE)
  {
    E::EnvEntry *env_entry = venv->Look(((A::SimpleVar *)var)->sym);
    if (((E::VarEntry *)env_entry)->readonly)
    {
      errormsg.Error(this->pos, "loop variable can't be assigned");
    }
  }
  TY::Ty *varty = var->Translate(venv, tenv, level, label).ty;
  TY::Ty *expty = exp->Translate(venv, tenv, level, label).ty;
  if (!varty->IsSameType(expty))
  {
    errormsg.Error(this->pos, "unmatched assign exp");
  }

  TR::Exp *vartr = var->Translate(venv, tenv, level, label).exp;
  TR::Exp *exptr = exp->Translate(venv, tenv, level, label).exp;
  TR::Exp *ep = TR::Tr_Assign(vartr, exptr);
  return TR::ExpAndTy(ep, varty);
}

//finish
TR::ExpAndTy IfExp::Translate(S::Table<E::EnvEntry> *venv,
                              S::Table<TY::Ty> *tenv, TR::Level *level,
                              TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *testty = this->test->Translate(venv, tenv, level, label).ty;
  TY::Ty *thenty = this->then->Translate(venv, tenv, level, label).ty;

  if (testty->kind != TY::Ty::INT)
  {
    errormsg.Error(this->pos, "integer required");
  }
  if (this->elsee && this->elsee->kind != A::Exp::NIL)
  {
    TY::Ty *elseety = this->elsee->Translate(venv, tenv, level, label).ty;
    if (!thenty->IsSameType(elseety))
    {
      errormsg.Error(this->pos, "then exp and else exp type mismatch");
    }
  }
  else if (thenty->kind != TY::Ty::VOID)
  {
    errormsg.Error(this->pos, "if-then exp's body must produce no value");
  }

  TR::Exp *testexp = this->test->Translate(venv, tenv, level, label).exp;
  TR::Exp *thenexp = this->then->Translate(venv, tenv, level, label).exp;

  TR::Cx test_ = testexp->UnCx();
  TEMP::Temp *r = TEMP::Temp::NewTemp();
  TEMP::Label *trues = TEMP::NewLabel();
  TEMP::Label *falses = TEMP::NewLabel();

  TR::do_patch(test_.trues, trues);
  TR::do_patch(test_.falses, falses);

  if (elsee)
  {
    TR::Exp *elseeexp = this->elsee->Translate(venv, tenv, level, label).exp;

    TEMP::Label *meeting = TEMP::NewLabel();
    T::Exp *e =
        new T::EseqExp(test_.stm,
                       new T::EseqExp(new T::LabelStm(trues),
                                      new T::EseqExp(new T::MoveStm(new T::TempExp(r), thenexp->UnEx()),
                                                     new T::EseqExp(new T::JumpStm(new T::NameExp(meeting), new TEMP::LabelList(meeting, NULL)),
                                                                    new T::EseqExp(new T::LabelStm(falses),
                                                                                   new T::EseqExp(new T::MoveStm(new T::TempExp(r), elseeexp->UnEx()),
                                                                                                  new T::EseqExp(new T::JumpStm(new T::NameExp(meeting), new TEMP::LabelList(meeting, NULL)),
                                                                                                                 new T::EseqExp(new T::LabelStm(meeting),
                                                                                                                                new T::TempExp(r)))))))));

    return TR::ExpAndTy(new TR::ExExp(e), thenty);
  }
  else
  {
    T::Stm *s =
        new T::SeqStm(test_.stm,
                      new T::SeqStm(new T::LabelStm(trues),
                                    new T::SeqStm(thenexp->UnNx(), new T::LabelStm(falses))));

    return TR::ExpAndTy(new TR::NxExp(s), thenty);
  }

  return TR::ExpAndTy(nullptr, thenty);
}

//finish
//NX, 条件判断，body
//T_stm T_Jump(T_exp exp, Temp_labelList labels);
//done label是在参数中提供的
TR::ExpAndTy WhileExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *testty = this->test->Translate(venv, tenv, level, label).ty;
  TY::Ty *bodyty = this->body->Translate(venv, tenv, level, label).ty;

  if (test->kind != A::Exp::Kind::INT)
  {
    errormsg.Error(this->pos, "integer required");
  }
  if (body->kind != A::Exp::Kind::VOID)
  {
    errormsg.Error(this->pos, "while body must produce no value");
  }

  TR::Exp *testexp = this->test->Translate(venv, tenv, level, label).exp;
  TR::Exp *bodyexp = this->body->Translate(venv, tenv, level, label).exp;

  TR::Cx cx = testexp->UnCx();
  //label: loop/body, test, done
  TEMP::Label *l = TEMP::NewLabel();
  TEMP::Label *t = TEMP::NewLabel();
  TEMP::Label *done = TEMP::NewLabel();

  TR::do_patch(cx.trues, l);
  TR::do_patch(cx.falses, done);

  T::Stm *nx = new T::SeqStm(new T::LabelStm(t),
                             new T::SeqStm(cx.stm,
                                           new T::SeqStm(new T::LabelStm(l),
                                                         new T::SeqStm(bodyexp->UnNx(),
                                                                       new T::SeqStm(new T::JumpStm(new T::NameExp(t), new TEMP::LabelList(t, NULL)),
                                                                                     new T::LabelStm(done))))));

  return TR::ExpAndTy(new TR::NxExp(nx), bodyty);
}

//finish
//NX, 循环变量i，low值，high值，body
//semant接口需要传i的access
/* if i > limit goto done
  body
  if i == limit goto done
Loop:	i := i + 1
	if i <= limit goto Loop
 done:
*/
TR::ExpAndTy ForExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *loty = this->lo->Translate(venv, tenv, level, label).ty;
  TY::Ty *hity = this->hi->Translate(venv, tenv, level, label).ty;
  venv->BeginScope();
  if (!loty->IsSameType(hity) || loty->kind != TY::Ty::INT)
  {
    errormsg.Error(this->pos, "for exp's range type is not integer");
  }
  E::EnvEntry *env_entry = new E::VarEntry(loty, true);
  venv->Enter(this->var, env_entry);

  TY::Ty *bodyty = this->body->Translate(venv, tenv, level, label).ty;

  if (bodyty->kind != TY::Ty::VOID)
  {
    errormsg.Error(this->pos, "for body must produce no value");
  }
  venv->EndScope();

  TR::Exp *loexp = lo->Translate(venv, tenv, level, label).exp;
  TR::Exp *hiexp = hi->Translate(venv, tenv, level, label).exp;
  TR::Exp *bodyxp = body->Translate(venv, tenv, level, label).exp;
  TR::Access *access = TR::Access::allocLocal(level, escape);
  TEMP::Label *done = TEMP::NewLabel();

  //DECLARE loop variable, init
  //T_exp ex = F_Exp(access->access, fp);
  T::Exp *fp = TR::staticLink(level, access->level);
  T::Exp *ex = F::getExp(access->access, fp);
  TR::Exp *i = new TR::ExExp(ex);
  TEMP::Temp *limit = TEMP::Temp::NewTemp();
  //label: loop/body
  TEMP::Label *l = TEMP::NewLabel();
  TEMP::Label *b = TEMP::NewLabel();

  T::Stm *nx = new T::SeqStm(new T::MoveStm(i->UnEx(), loexp->UnEx()),
                             new T::SeqStm(new T::MoveStm(new T::TempExp(limit), hiexp->UnEx()),
                                           new T::SeqStm(new T::CjumpStm(T::GT_OP, i->UnEx(), new T::TempExp(limit), done, b),
                                                         new T::SeqStm(new T::LabelStm(b),
                                                                       new T::SeqStm(bodyxp->UnNx(),
                                                                                     new T::SeqStm(new T::CjumpStm(T::EQ_OP, i->UnEx(), new T::TempExp(limit), done, l),
                                                                                                   new T::SeqStm(new T::LabelStm(l),
                                                                                                                 new T::SeqStm(new T::MoveStm(i->UnEx(), new T::BinopExp(T::PLUS_OP, i->UnEx(), new T::ConstExp(1))),
                                                                                                                               new T::SeqStm(new T::CjumpStm(T::LT_OP, i->UnEx(), new T::TempExp(limit), l, done),
                                                                                                                                             new T::LabelStm(done))))))))));

  return TR::ExpAndTy(new TR::NxExp(nx), bodyty);
}

//finish
TR::ExpAndTy BreakExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  T::Stm *nx = new T::JumpStm(new T::NameExp(label), new TEMP::LabelList(label, NULL));
  return TR::ExpAndTy(new TR::NxExp(nx), TY::VoidTy::Instance());
}

//finish
TR::ExpAndTy LetExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  A::DecList *declist = this->decs;
  A::Exp *exp = this->body;
  A::Dec *dec;
  venv->BeginScope();
  tenv->BeginScope();
  TR::Exp *trSeq = new TR::ExExp(new T::ConstExp(0));
  for (; declist; declist = declist->tail)
  {
    dec = declist->head;
    trSeq = Tr_Seq(trSeq, dec->Translate->Translate(venv, tenv, level, label).exp);
  }
  TY::Ty *expty = exp->Translate(venv, tenv, level, label).ty;
  TR::Exp *trlet = Tr_Seq(trSeq, body->Translate(venv, tenv, level, label).exp);
  tenv->EndScope();
  venv->EndScope();

  return TR::ExpAndTy(trlet, expty->ActualTy());
}

//finish
//EX, 数组大小，数组初值
//T_exp F_externalCall(string s, T_expList args)
//	详细介绍在ppt里面
//	returns the pointer into a new temporary r
//	initArray(n, b), n:array length b:init value
//	CALL(NAME(Temp_namedlabel(“initArray”)),
//		T_ExpList(n, T_ExpList(b, NULL)))
TR::ExpAndTy ArrayExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *arrayty = tenv->Look(this->typ);
  if (!arrayty)
  {
    errormsg.Error(this->pos, "undefined type %s", this->typ->Name().c_str());
  }

  // need actualty
  arrayty = arrayty->ActualTy();
  if (arrayty->kind != TY::Ty::ARRAY)
  {
    errormsg.Error(this->pos, "not array type");
  }

  TY::Ty *sizety = this->size->Translate(venv, tenv, level, label).ty;
  TY::Ty *initty = this->init->Translate(venv, tenv, level, label).ty;
  if (!initty->IsSameType(((TY::ArrayTy *)arrayty)->ty))
  {
    errormsg.Error(this->pos, "type mismatch");
  }
  if (sizety->kind != TY::Ty::INT)
  {
    errormsg.Error(this->pos, "type of size expression should be int");
  }

  TR::Exp *sizeexp = this->size->Translate(venv, tenv, level, label).exp;
  TR::Exp *initexp = this->init->Translate(venv, tenv, level, label).exp;

  TEMP::Temp *r; //return value
  TEMP::Temp *s = TEMP::Temp::NewTemp();
  TEMP::Temp *i = TEMP::Temp::NewTemp();

  T::Exp *ex = new T::EseqExp(new T::MoveStm(new T::TempExp(s), sizeexp->UnEx()),
                              new T::EseqExp(new T::MoveStm(new T::TempExp(i), initexp->UnEx()),
                                             new T::EseqExp(new T::MoveStm(new T::TempExp(r),
                                                                           F::externalCall("initArray",
                                                                                           new T::ExpList(new T::TempExp(s), new T::ExpList(new T::TempExp(i), NULL)))),
                                                            new T::TempExp(r))));

  return TR::ExpAndTy(new TR::ExExp(ex), arrayty);
}

//finish
TR::ExpAndTy VoidExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

//finish
TR::Exp *FunctionDec::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  A::FunDecList *funcs = this->functions;
  A::FunDec *func;

  for (; funcs; funcs = funcs->tail)
  {
    func = funcs->head;
    if (venv->Look(func->name))
    {
      errormsg.Error(this->pos, "two functions have the same name");
      continue;
    }

    TY::TyList *formaltys = make_formal_tylist(tenv, func->params);

    // make new level
    TEMP::Label *name = TEMP::NewLabel();
    U::BoolList *args = nullptr;

    for (TY::TyList *cal_arg = formaltys; cal_arg; cal_arg = cal_arg->tail)
    {
      args = new U::BoolList(true, args);
    }
    TR::Level *newl = TR::Level::NewLevel(level, name, args);

    if (func->result)
    {
      TY::Ty *resultTy = tenv->Look(func->result);
      if (resultTy->kind == TY::Ty::VOID)
      {
        errormsg.Error(this->pos, "undefined return type %s", func->result);
        continue;
      }
      venv->Enter(func->name, new E::FunEntry(newl, name, formaltys, resultTy));
    }
    else
    {
      venv->Enter(func->name, new E::FunEntry(newl, name, formaltys, TY::VoidTy::Instance()));
    }
  }

  for (funcs = this->functions; funcs; funcs = funcs->tail)
  {
    func = funcs->head;
    E::FunEntry *funentry = (E::FunEntry *)venv->Look(func->name);
    TY::TyList *formaltys = funentry->formals;

    venv->BeginScope();
    A::FieldList *fields;
    TY::TyList *tys;
    TR::AccessList *accesslist = funentry->level->formals;
    for (fields = func->params, tys = formaltys; fields && tys; fields = fields->tail, tys = tys->tail)
    {
      venv->Enter(fields->head->name, new E::VarEntry(accesslist->head, tys->head));
      accesslist = accesslist->tail;
    }
    TY::Ty *bodyty = func->body->Translate(venv, tenv, funentry->level, funentry->label).ty;
    E::EnvEntry *func_entry = venv->Look(func->name);
    if (!((E::FunEntry *)func_entry)->result->IsSameType(bodyty))
    {
      if (((E::FunEntry *)func_entry)->result->kind == TY::Ty::VOID)
      {
        errormsg.Error(this->pos, "procedure returns value");
      }
      else
      {
        errormsg.Error(this->pos, "procedure returns unexpected type");
      }
    }
    venv->EndScope();
  }
  return nullptr;
}

//finish
TR::Exp *VarDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                           TR::Level *level, TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  A::Exp *exp = this->init;
  TY::Ty *expty = exp->Translate(venv, tenv, level, label).ty;
  TR::Access *access;
  if (!typ)
  {
    if (expty->kind == TY::Ty::NIL)
    {
      errormsg.Error(this->pos, "init should not be nil without type specified");
    }
    access = TR::Access::allocLocal(level, true);
    venv->Enter(var, new E::VarEntry(access, expty));
  }
  else
  {
    TY::Ty *typety = tenv->Look(this->typ);
    if (!typety)
    {
      errormsg.Error(this->pos, "undefined type %s", this->typ->Name().c_str());
      access = TR::Access::allocLocal(level, true);
      venv->Enter(var, new E::VarEntry(access, expty->ActualTy()));
    }
    else
    {
      if (expty->kind == TY::Ty::NIL && typety->ActualTy()->kind != TY::Ty::RECORD)
        errormsg.Error(this->pos, "init should not be nil without type specified");

      if (!typety->IsSameType(expty) && expty->kind != TY::Ty::NIL)
        errormsg.Error(this->pos, "type mismatch");

      access = TR::Access::allocLocal(level, true);
      venv->Enter(var, new E::VarEntry(access, expty->ActualTy()));
    }
  }

  return TR::Tr_Assign(TR::Tr_SimpleVar(access, level), exp->Translate(venv, tenv, level, label).exp);
}

//finish
TR::Exp *TypeDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                            TR::Level *level, TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  A::NameAndTyList *ntys = this->types;
  A::NameAndTy *nty;
  //  push type names into tenv to handle recursive type.
  for (; ntys; ntys = ntys->tail)
  {
    nty = ntys->head;
    TY::Ty *ty = tenv->Look(nty->name);
    if (ty)
    {
      errormsg.Error(this->pos, "two types have the same name");
      continue;
    }
    else
    {
      tenv->Enter(nty->name, new TY::NameTy(nty->name, nullptr));
    }
  }

  for (ntys = this->types; ntys; ntys = ntys->tail)
  {
    TY::Ty *ty = tenv->Look(ntys->head->name);
    ((TY::NameTy *)ty)->ty = ntys->head->ty->Translate(tenv);
  }

  //  Find wrong recursive typedec.
  ntys = types;
  bool isOk = true;
  while (ntys && isOk)
  {
    TY::Ty *ty = tenv->Look(ntys->head->name);
    if (ty->kind == TY::Ty::NAME)
    {
      TY::Ty *tyTy = ((TY::NameTy *)ty)->ty;
      while (tyTy->kind == TY::Ty::NAME)
      {
        TY::NameTy *nameTy = (TY::NameTy *)tyTy;
        if (!nameTy->sym->Name().compare(ntys->head->name->Name()))
        {
          errormsg.Error(pos, "illegal type cycle");
          isOk = false;
          break;
        }
        tyTy = nameTy->ty;
      }
    }

    ntys = ntys->tail;
  }
  //return TR::nil
  return new TR::ExExp(new T::ConstExp(0));
}

//finish
TY::Ty *NameTy::Translate(S::Table<TY::Ty> *tenv) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *namety = tenv->Look(this->name);
  if (!namety)
  {
    errormsg.Error(this->pos, "undefined type %s", this->name->Name().c_str());
    return TY::VoidTy::Instance();
  }
  return new TY::NameTy(this->name, namety);
}

//finish
TY::Ty *RecordTy::Translate(S::Table<TY::Ty> *tenv) const
{
  // TODO: Put your codes here (lab5).
  TY::FieldList *tyFields = make_fieldlist(tenv, record);
  return new TY::RecordTy(tyFields);
}

//finish
TY::Ty *ArrayTy::Translate(S::Table<TY::Ty> *tenv) const
{
  // TODO: Put your codes here (lab5).
  TY::Ty *arrayty = tenv->Look(this->array);
  if (!arrayty)
  {
    errormsg.Error(this->pos, "undefined type %s", this->array->Name().c_str());
    return TY::VoidTy::Instance();
  }

  return new TY::ArrayTy(arrayty);
}

static TY::TyList *make_formal_tylist(S::Table<TY::Ty> *tenv, A::FieldList *params)
{
  if (params == nullptr)
  {
    return nullptr;
  }

  TY::Ty *ty = tenv->Look(params->head->typ);
  if (ty == nullptr)
  {
    errormsg.Error(params->head->pos, "undefined type %s",
                   params->head->typ->Name().c_str());
  }

  return new TY::TyList(ty->ActualTy(), make_formal_tylist(tenv, params->tail));
}

static TY::FieldList *make_fieldlist(S::Table<TY::Ty> *tenv, A::FieldList *fields)
{
  if (fields == nullptr)
  {
    return nullptr;
  }

  TY::Ty *ty = tenv->Look(fields->head->typ);
  if (ty == nullptr)
  {
    errormsg.Error(fields->head->pos, "undefined type %s",
                   fields->head->typ->Name().c_str());
  }
  return new TY::FieldList(new TY::Field(fields->head->name, ty),
                           make_fieldlist(tenv, fields->tail));
}

} // namespace A
