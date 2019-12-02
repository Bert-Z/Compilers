#include "tiger/frame/frame.h"

#include <string>

namespace F
{

// Word size
static const int WordSize = 8;

class X64Frame : public Frame
{
  // TODO: Put your codes here (lab6).
};

// generate F::AccessList 参数链
static AccessList *makeFormals(U::BoolList *formals, int offset)
{
  if (formals)
    return new AccessList(new InFrameAccess(offset), makeFormals(formals->tail, offset + WordSize));
  else
    return nullptr;
}

Frame *Frame::newFrame(TEMP::Label *name, U::BoolList *formals)
{
  // 相对offset,空出来name的区域
  AccessList *accesslist = makeFormals(formals, WordSize);

  return new Frame(name,accesslist);
}

Access *allocLocal(Frame *frame, bool escape)
{
  if (escape)
  {
    (frame->size)++;
    int offset = -(frame->size * WordSize);
    return new InFrameAccess(offset);
  }
  else
  {
    return new InRegAccess(TEMP::Temp::NewTemp());
  }
}

T::Exp *getExp(Access *access, T::Exp *fp)
{
  if (access->kind == F::Access::Kind::INFRAME)
    return new T::MemExp(new T::BinopExp(T::BinOp::PLUS_OP, fp, new T::ConstExp(((InFrameAccess *)access)->offset)));
  else
    return new T::TempExp(((InRegAccess *)access)->reg);
}

// %rsp
TEMP::Temp *F_FP()
{
  return TEMP::Temp::NewTemp();
}

} // namespace F