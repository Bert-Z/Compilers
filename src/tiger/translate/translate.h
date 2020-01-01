#ifndef TIGER_TRANSLATE_TRANSLATE_H_
#define TIGER_TRANSLATE_TRANSLATE_H_

#include "tiger/absyn/absyn.h"
#include "tiger/frame/frame.h"

#include "tiger/frame/temp.h"
/* Forward Declarations */
namespace A {
class Exp;
}  // namespace A

namespace TR {

// need to be declared at first
class Level;

class Access {
 public:
  Level *level;
  F::Access *access;

  Access(Level *level, F::Access *access) : level(level), access(access) {}
  
  static Access *allocLocal(Level *level, bool escape);
};

class AccessList {
 public:
  Access *head;
  AccessList *tail;

  AccessList(Access *head, AccessList *tail) : head(head), tail(tail) {}
};

class Level {
 public:
  F::Frame *frame;
  Level *parent;

  Level(F::Frame *frame, Level *parent) : frame(frame), parent(parent) {}
  AccessList *Formals(Level *level) { return nullptr; }

  static Level *NewLevel(Level *parent, TEMP::Label *name,
                         U::BoolList *formals);
};

class PatchList {
 public:
  TEMP::Label **head;
  PatchList *tail;

  PatchList(TEMP::Label **head, PatchList *tail) : head(head), tail(tail) {}
};

class Cx {
 public:
  PatchList *trues;
  PatchList *falses;
  T::Stm *stm;

  Cx(PatchList *trues, PatchList *falses, T::Stm *stm)
      : trues(trues), falses(falses), stm(stm) {}
}; 

class Exp {
 public:
  enum Kind { EX, NX, CX };

  Kind kind;

  Exp(Kind kind) : kind(kind) {}

  virtual T::Exp *UnEx() const = 0;
  virtual T::Stm *UnNx() const = 0;
  virtual Cx UnCx() const = 0;
};

class ExpList {
  public:
    Exp *head;
    ExpList *tail;

    ExpList(Exp *head,ExpList *tail):head(head),tail(tail){}
};

class ExpAndTy {
 public:
  TR::Exp *exp;
  TY::Ty *ty;

  ExpAndTy(TR::Exp *exp, TY::Ty *ty) : exp(exp), ty(ty) {}
};

class ExExp : public Exp {
 public:
  T::Exp *exp;

  ExExp(T::Exp *exp) : Exp(EX), exp(exp) {}

  T::Exp *UnEx() const override; 
  T::Stm *UnNx() const override;
  Cx UnCx() const override;
};

class NxExp : public Exp {
 public:
  T::Stm *stm;

  NxExp(T::Stm *stm) : Exp(NX), stm(stm) {}

  T::Exp *UnEx() const override;
  T::Stm *UnNx() const override;
  Cx UnCx() const override;
};

class CxExp : public Exp {
 public:
  Cx cx;

  CxExp(struct Cx cx) : Exp(CX), cx(cx) {}
  CxExp(PatchList *trues, PatchList *falses, T::Stm *stm)
      : Exp(CX), cx(trues, falses, stm) {}

  T::Exp *UnEx() const override;
  T::Stm *UnNx() const override;
  Cx UnCx() const override;
};

PatchList *join_patch(PatchList *first, PatchList *second);

Level* Outermost();

F::FragList* TranslateProgram(A::Exp*);

TR::Exp *Nil();

void do_patch(PatchList *tList, TEMP::Label *label);

void procEntryExit(Level *level, TR::Exp *func_body);

TR::NxExp *generate_assign(TR::Exp *leftvalue, TR::Exp *right);

Exp *Tr_Seq(Exp *left, Exp *right);

}  // namespace TR

#endif
