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

  Frame(TEMP::Label *name, AccessList *formals) : name(name), formals(formals){}
  static Frame *newFrame(TEMP::Label *name,U::BoolList *formals);
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

class InFrameAccess : public Access {
 public:
  int offset;

  InFrameAccess(int offset) : Access(INFRAME), offset(offset) {}
};

class InRegAccess : public Access {
 public:
  TEMP::Temp* reg;

  InRegAccess(TEMP::Temp* reg) : Access(INREG), reg(reg) {}
};

T::Exp *getExp(Access *access,T::Exp *fp);
Access *allocLocal(Frame *frame,bool escape);

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

T::Exp *externalCall(std::string s,T::ExpList *args);

} // namespace F

#endif