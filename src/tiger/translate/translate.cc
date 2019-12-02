#include "tiger/translate/translate.h"

#include <cstdio>
#include <set>
#include <string>

#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/temp.h"
#include "tiger/semant/semant.h"
#include "tiger/semant/types.h"
#include "tiger/util/util.h"

namespace TR
{
static Level *outermost = nullptr;

//staticLink: 当前层level和目标层level
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

} // namespace TR

namespace A
{

//EX, 栈上的变量，当前level
TR::ExpAndTy SimpleVar::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  E::EnvEntry *env_entry = venv->Look(this->sym);
  if (!env_entry)
  {
    printf("%x undefined variable %s\n", this->pos, this->sym->Name().c_str());
    return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
  }
  E::VarEntry *var_entry = (E::VarEntry *)env_entry;

  T::Exp *fp = TR::staticLink(level, )

      return TR::ExpAndTy(nullptr, var_entry->ty);
}

TR::ExpAndTy FieldVar::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy SubscriptVar::Translate(S::Table<E::EnvEntry> *venv,
                                     S::Table<TY::Ty> *tenv, TR::Level *level,
                                     TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy VarExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy NilExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy IntExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy StringExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy CallExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy OpExp::Translate(S::Table<E::EnvEntry> *venv,
                              S::Table<TY::Ty> *tenv, TR::Level *level,
                              TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy RecordExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy SeqExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy AssignExp::Translate(S::Table<E::EnvEntry> *venv,
                                  S::Table<TY::Ty> *tenv, TR::Level *level,
                                  TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy IfExp::Translate(S::Table<E::EnvEntry> *venv,
                              S::Table<TY::Ty> *tenv, TR::Level *level,
                              TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy WhileExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy ForExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy BreakExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy LetExp::Translate(S::Table<E::EnvEntry> *venv,
                               S::Table<TY::Ty> *tenv, TR::Level *level,
                               TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy ArrayExp::Translate(S::Table<E::EnvEntry> *venv,
                                 S::Table<TY::Ty> *tenv, TR::Level *level,
                                 TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::ExpAndTy VoidExp::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return TR::ExpAndTy(nullptr, TY::VoidTy::Instance());
}

TR::Exp *FunctionDec::Translate(S::Table<E::EnvEntry> *venv,
                                S::Table<TY::Ty> *tenv, TR::Level *level,
                                TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return nullptr;
}

TR::Exp *VarDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                           TR::Level *level, TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return nullptr;
}

TR::Exp *TypeDec::Translate(S::Table<E::EnvEntry> *venv, S::Table<TY::Ty> *tenv,
                            TR::Level *level, TEMP::Label *label) const
{
  // TODO: Put your codes here (lab5).
  return nullptr;
}

TY::Ty *NameTy::Translate(S::Table<TY::Ty> *tenv) const
{
  // TODO: Put your codes here (lab5).
  return TY::VoidTy::Instance();
}

TY::Ty *RecordTy::Translate(S::Table<TY::Ty> *tenv) const
{
  // TODO: Put your codes here (lab5).
  return TY::VoidTy::Instance();
}

TY::Ty *ArrayTy::Translate(S::Table<TY::Ty> *tenv) const
{
  // TODO: Put your codes here (lab5).
  return TY::VoidTy::Instance();
}

} // namespace A
