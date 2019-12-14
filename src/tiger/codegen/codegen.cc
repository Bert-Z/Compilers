#include "tiger/codegen/codegen.h"

namespace CG
{

const int WORD_SIZE = 8;

AS::InstrList *ilist = nullptr;
AS::InstrList *last = nullptr;
F::Frame *frame = nullptr;

/* 构造寄存器链 */
TEMP::TempList *L(TEMP::Temp *h, TEMP::TempList *t)
{
  return new TEMP::TempList(h, t);
}

/* 将后面要返回的指令登记在指令表中 */
void emit(AS::Instr *inst)
{
  if (last != nullptr)
  {
    last->tail = new AS::InstrList(inst, nullptr);
    last = last->tail;
  }
  else
  {
    ilist = new AS::InstrList(inst, nullptr);
    last = ilist;
  }
}

void munchStm(T::Stm *s)
{
  std::string inst;

  switch (s->kind)
  {
  // store
  case T::Stm::MOVE:
  {
    /* struct {T_exp dst, src;} MOVE; */
    T::Exp *src = ((T::MoveStm *)s)->src;
    T::Exp *dst = ((T::MoveStm *)s)->dst;

    // didn't consider e1temp==F::F_FP()
    /* T_MOVE: 
        movq src, mem
    */
    if (dst->kind == T::Exp::MEM)
    {
      /* case 1 : MOVE(MEM(BINOP(PLUS,e1,CONST(i))), e2) */
      if (((T::MemExp *)dst)->exp->kind == T::Exp::BINOP && ((T::BinopExp *)((T::MemExp *)dst)->exp)->op == T::BinOp::PLUS_OP && ((T::BinopExp *)((T::MemExp *)dst)->exp)->right->kind == T::Exp::CONST)
      {
        T::Exp *e1 = ((T::BinopExp *)((T::MemExp *)dst)->exp)->left;
        T::Exp *e2 = src;
        TEMP::Temp *e2temp = munchExp(e2);
        TEMP::Temp *e1temp = munchExp(e1);

        if (e1temp == F::F_FP())
        {
          inst = " movq `s1, ?" + std::to_string(((T::ConstExp *)((T::BinopExp *)((T::MemExp *)dst)->exp)->right)->consti) + "#(`s0)";
        }
        else
        {
          inst = " movq `s1, " + std::to_string(((T::ConstExp *)((T::BinopExp *)((T::MemExp *)dst)->exp)->right)->consti) + "(`s0)";
        }

        emit(new AS::OperInstr(inst, nullptr, L(e1temp, L(e2temp, nullptr)), nullptr));
        return;
      }
      /* case 2 : MOVE(MEM(BINOP(PLUS,CONST(i),e1)), e2) */
      else if (((T::MemExp *)dst)->exp->kind == T::Exp::BINOP && ((T::BinopExp *)((T::MemExp *)dst)->exp)->op == T::BinOp::PLUS_OP && ((T::BinopExp *)((T::MemExp *)dst)->exp)->left->kind == T::Exp::CONST)
      {
        T::Exp *e1 = ((T::BinopExp *)((T::MemExp *)dst)->exp)->right;
        T::Exp *e2 = src;
        TEMP::Temp *e2temp = munchExp(e2);
        TEMP::Temp *e1temp = munchExp(e1);

        if (e1temp == F::F_FP())
        {
          inst = " movq `s1, ?" + std::to_string(((T::ConstExp *)((T::BinopExp *)((T::MemExp *)dst)->exp)->left)->consti) + "#(`s0)";
        }
        else
        {
          inst = " movq `s1, " + std::to_string(((T::ConstExp *)((T::BinopExp *)((T::MemExp *)dst)->exp)->left)->consti) + "(`s0)";
        }

        emit(new AS::OperInstr(inst, nullptr, L(e1temp, L(e2temp, nullptr)), nullptr));
        return;
      }
      /* case 3 : MOVE(MEM(e1), MEM(e2)) */
      else if (src->kind == T::Exp::MEM)
      {
        T::Exp *e1 = ((T::MemExp *)dst)->exp;
        T::Exp *e2 = ((T::MemExp *)src)->exp;
        TEMP::Temp *r = TEMP::Temp::NewTemp();
        TEMP::Temp *e2temp = munchExp(e2);
        TEMP::Temp *e1temp = munchExp(e1);

        // r = MEM(e2)
        if (e2temp == F::F_FP())
        {
          inst = " movq ?0#(`s0), `d0";
        }
        else
        {
          inst = " movq (`s0), `d0";
        }

        emit(new AS::OperInstr(inst, L(r, nullptr), L(e2temp, nullptr), nullptr));

        // MOVE(MEM(e1),r)
        if (e1temp == F::F_FP())
        {
          inst = " movq `s0, ?0#(`s1)";
        }
        else
        {
          inst = " movq `s0, (`s1)";
        }
        emit(new AS::OperInstr(inst, nullptr, L(r, L(e1temp, nullptr)), nullptr));

        return;
      }
      /* case 4 : MOVE(MEM(CONST(i)), e2) */
      else if (((T::MemExp *)dst)->exp->kind == T::Exp::CONST)
      {
        T::Exp *e2 = src;
        TEMP::Temp *e2temp = munchExp(e2);

        inst = " movq ` s0, (" + std::to_string(((T::ConstExp *)((T::MemExp *)dst)->exp)->consti) + ")";
        emit(new AS::OperInstr(inst, nullptr, L(e2temp, nullptr), nullptr));

        return;
      }
      /* case 5 : MOVE(MEM(e1), e2) */
      else
      {
        T::Exp *e1 = ((T::MemExp *)dst)->exp;
        T::Exp *e2 = src;
        TEMP::Temp *e2temp = munchExp(e2);
        TEMP::Temp *e1temp = munchExp(e1);

        if (e1temp == F::F_FP())
        {
          inst = " movq `s1, ?0#(`s0)";
        }
        else
        {
          inst = " movq `s1, (`s0)";
        }
        emit(new AS::OperInstr(inst, nullptr, L(e1temp, L(e2temp, nullptr)), nullptr));

        return;
      }
    }
    /* T_MOVE: MOVE(TEMP(i), e2)
        movq src, dst
    */
    else if (dst->kind == T::Exp::TEMP)
    {
      T::Exp *e2 = src;
      TEMP::Temp *e2temp = munchExp(e2);
      TEMP::Temp *e1temp = ((T::TempExp *)dst)->temp;

      // leaq 16(%rsp), %rax; movq %rax, (%rsp)
      // before call, pass static link
      if (e2temp == F::F_FP())
      {
        inst = " leaq ?0#(`s0), `d0";
      }
      else
      {
        inst = " movq `s0, `d0";
      }

      emit(new AS::MoveInstr(inst, L(e1temp, nullptr), L(e2temp, nullptr)));

      return;
    }
  }
  /* 
  * T_JUMP: while,for
  * jmp .L
  */
  case T::Stm::JUMP:
  {
    /* struct {T_exp exp; Temp_labelList jumps;} JUMP; */
    TEMP::LabelList *jumps = ((T::JumpStm *)s)->jumps;
    inst = " jmp `j0";
    emit(new AS::OperInstr(inst, nullptr, nullptr, new AS::Targets(jumps)));

    return;
  }
  /* 
  * T_CJUMP: if
  * cmp src0, src1
  * jle .L
  */
  case T::Stm::CJUMP:
  {
    /* struct {T_relOp op; T_exp left, right; Temp_label true, false;} CJUMP; */
    TEMP::Temp *left = munchExp(((T::CjumpStm *)s)->left);
    TEMP::Temp *right = munchExp(((T::CjumpStm *)s)->right);
    TEMP::Label *trues = ((T::CjumpStm *)s)->true_label;
    TEMP::Label *falses = ((T::CjumpStm *)s)->false_label;

    std::string oper = "";
    switch (((T::CjumpStm *)s)->op)
    {
    case T::RelOp::EQ_OP:
    {
      oper = "je";
      break;
    }
    case T::RelOp::NE_OP:
    {
      oper = "jne";
      break;
    }
    case T::RelOp::LT_OP:
    {
      oper = "jl";
      break;
    }
    case T::RelOp::GT_OP:
    {
      oper = "jg";
      break;
    }
    case T::RelOp::LE_OP:
    {
      oper = "jle";
      break;
    }
    case T::RelOp::GE_OP:
    {
      oper = "jge";
      break;
    }
    case T::RelOp::ULT_OP:
    {
      oper = "jb";
      break;
    }
    case T::RelOp::UGT_OP:
    {
      oper = "ja";
      break;
    }
    case T::RelOp::ULE_OP:
    {
      oper = "jbe";
      break;
    }
    case T::RelOp::UGE_OP:
    {
      oper = "jae";
      break;
    }
    default:
      // cannot reach here
      assert(0);
    }

    /* ZF, SF, OF, CF */
    /* eg: left > right, cmpq left, right, set SF=1, jl/jle is ok */
    inst = " cmpq `s1, `s0";
    emit(new AS::OperInstr(inst, nullptr, L(left, L(right, nullptr)), nullptr));

    inst = " " + oper + " `j0";
    emit(new AS::OperInstr(inst, nullptr, nullptr, new AS::Targets(new TEMP::LabelList(trues, new TEMP::LabelList(falses, nullptr)))));

    return;
  }
  /* 
  * T_LABEL: if,while,for
  * Label: 
  */
  case T::Stm::LABEL:
  {
    /* Temp_label LABEL; */
    TEMP::Label *label = ((T::LabelStm *)s)->label;
    inst = TEMP::LabelString(label);
    emit(new AS::LabelInstr(inst, label));
    return;
  }
  /* T_SEQ: */
  case T::Stm::SEQ:
  {
    /* union {struct {T_stm left, right;} SEQ; */
    munchStm(((T::SeqStm *)s)->left);
    munchStm(((T::SeqStm *)s)->right);
    return;
  }
  /* T_EXP: */
  case T::Stm::EXP:
  {
    /* T_exp EXP; */
    munchExp(((T::ExpStm *)s)->exp);
    return;
  }
  default:
    assert(0);
  }
}

/*
 * 翻译call时使用的参数
 */
TEMP::TempList *munchArgs(int cnt, T::ExpList *args)
{
  if (!args)
    return nullptr;

  std::string inst;
  TEMP::Temp *src = munchExp(args->head);

  /* use register pass argument */
  if (cnt > 6)
  {
    TEMP::TempList *argregs = F::F_Argregs();
    TEMP::Temp *dst = F::F_Arg(cnt);

    inst = " movq `s0, `d0";
    emit(new AS::MoveInstr(inst, L(dst, nullptr), L(munchExp(args->head), nullptr)));

    return L(dst, munchArgs(cnt + 1, args->tail));
  }
  /* use stack pass argument */
  else
  {
    inst = " movq `s0, " + std::to_string((cnt - 6) * WORD_SIZE) + "(%rsp)";
    emit(new AS::MoveInstr(inst, nullptr, L(src, L(F::F_RSP(), nullptr))));
    return munchArgs(cnt + 1, args->tail);
  }
}

/*
 * 翻译T_exp类型，返回Temp_temp
 */
TEMP::Temp *munchExp(T::Exp *e)
{
  std::string str;
  std::string inst;

  switch (e->kind)
  {
  /* load */
  case T::Exp::MEM:
  {
    /* T_exp MEM; */
    TEMP::Temp *r = TEMP::Temp::NewTemp();

    /* MEM(BINOP(PLUS, e1, CONST(i)))*/
    if (((T::MemExp *)e)->exp->kind == T::Exp::BINOP && ((T::BinopExp *)((T::MemExp *)e)->exp)->op == T::BinOp::PLUS_OP && ((T::BinopExp *)((T::MemExp *)e)->exp)->right->kind == T::Exp::CONST)
    {
      T::Exp *e1 = ((T::BinopExp *)((T::MemExp *)e)->exp)->left;
      T::Exp *e2 = ((T::BinopExp *)((T::MemExp *)e)->exp)->right;
      TEMP::Temp *e1temp = munchExp(e1);

      if (e1temp == F::F_FP())
      {
        inst = " movq ?" + std::to_string(((T::ConstExp *)e2)->consti) + "#(`s0), `d0";
      }
      else
      {
        inst = " movq " + std::to_string(((T::ConstExp *)e2)->consti) + "(`s0), `d0";
      }
      emit(new AS::OperInstr(inst, L(r, nullptr), L(e1temp, nullptr), nullptr));
    }
    /* MEM(BINOP(PLUS, CONST(i), e1))*/
    else if (((T::MemExp *)e)->exp->kind == T::Exp::BINOP && ((T::BinopExp *)((T::MemExp *)e)->exp)->op == T::BinOp::PLUS_OP && ((T::BinopExp *)((T::MemExp *)e)->exp)->left->kind == T::Exp::CONST)
    {
      T::Exp *e1 = ((T::BinopExp *)((T::MemExp *)e)->exp)->right;
      T::Exp *e2 = ((T::BinopExp *)((T::MemExp *)e)->exp)->left;
      TEMP::Temp *e1temp = munchExp(e1);

      if (e1temp == F::F_FP())
      {
        inst = " movq ?" + std::to_string(((T::ConstExp *)e2)->consti) + "#(`s0), `d0";
      }
      else
      {
        inst = " movq " + std::to_string(((T::ConstExp *)e2)->consti) + "(`s0), `d0";
      }

      emit(new AS::OperInstr(inst, L(r, nullptr), L(e1temp, nullptr), nullptr));
    }
    /* MEM(CONST(i)) */
    else if (((T::MemExp *)e)->exp->kind == T::Exp::CONST)
    {
      T::Exp *e1 = ((T::MemExp *)e)->exp;

      inst = " movq " + std::to_string(((T::ConstExp *)e1)->consti) + ", `d0";
      emit(new AS::OperInstr(inst, L(r, nullptr), nullptr, nullptr));
    }
    /* MEM(e1) */
    else
    {
      T::Exp *e1 = ((T::MemExp *)e)->exp;
      TEMP::Temp *e1temp = munchExp(e1);

      if (e1temp == F::F_FP())
      {
        inst = " movq ?0#(`s0), `d0";
      }
      else
      {
        inst = " movq (`s0), `d0";
      }

      emit(new AS::OperInstr(inst, L(r, nullptr), L(e1temp, nullptr), nullptr));
    }

    return r;
  }
  case T::Exp::BINOP:
  {
    T::BinopExp *ee = (T::BinopExp *)e;

    switch (ee->op)
    {
    /* T_Binop: a + b
        movq left, dst
        addq right, dst
    */
    case T::BinOp::PLUS_OP:
    {
      TEMP::Temp *temp = TEMP::Temp::NewTemp();

      if (ee->right->kind == T::Exp::CONST)
      {
        T::Exp *e1 = ee->left;
        TEMP::Temp *possible_fp = munchExp(e1);
        if (possible_fp == F::F_FP())
        {
          str = " leaq ?0#(`s0), `d0";
        }
        else
        {
          str = " movq `s0, `d0";
        }
        emit(new AS::MoveInstr(str, L(temp, nullptr), L(possible_fp, nullptr)));
        str = " addq $" + std::to_string(((T::ConstExp *)ee->right)->consti) + ", `d0";
        emit(new AS::OperInstr(str, L(temp, nullptr), L(temp, nullptr), nullptr));
      }
      else if (ee->left->kind == T::Exp::CONST)
      {
        T::Exp *e1 = ee->right;
        TEMP::Temp *possible_fp = munchExp(e1);

        if (possible_fp == F::F_FP())
        {
          str = " leaq ?0#(`s0), `d0";
        }
        else
        {
          str = " movq `s0, `d0";
        }
        emit(new AS::MoveInstr(str, L(temp, nullptr), L(possible_fp, nullptr)));
        str = " addq $" + std::to_string(((T::ConstExp *)ee->left)->consti) + ", `d0";
        emit(new AS::OperInstr(str, L(temp, nullptr), L(temp, nullptr), nullptr));
      }
      else
      {
        T::Exp *e1 = ee->left;
        T::Exp *e2 = ee->right;
        TEMP::Temp *possible_fp = munchExp(e1);
        if (possible_fp == F::F_FP())
        {
          str = " leaq ?0#(`s0), `d0";
        }
        else
        {
          str = " movq `s0, `d0";
        }
        emit(new AS::MoveInstr(str, L(temp, nullptr), L(possible_fp, nullptr)));
        str = " addq `s1, `d0";
        emit(new AS::OperInstr(str, L(temp, nullptr), L(temp, L(munchExp(e2), nullptr)), nullptr));
      }

      return temp;
    }
    case T::BinOp::MINUS_OP:
    {
      TEMP::Temp *temp = TEMP::Temp::NewTemp();

      if (ee->right->kind == T::Exp::CONST)
      {
        T::Exp *e1 = ee->left;
        std::string str;
        TEMP::Temp *possible_fp = munchExp(e1);
        if (possible_fp == F::F_FP())
        {
          str = " leaq ?0#(`s0), `d0";
        }
        else
        {
          str = " movq `s0, `d0";
        }

        emit(new AS::MoveInstr(str, L(temp, nullptr), L(possible_fp, nullptr)));
        str = " subq $" + std::to_string(((T::ConstExp *)ee->right)->consti) + ", `d0";
        emit(new AS::OperInstr(str, L(temp, nullptr), nullptr, nullptr));
      }
      else
      {
        T::Exp *e1 = ee->left;
        T::Exp *e2 = ee->right;
        TEMP::Temp *possible_fp = munchExp(e1);

        str = " movq `s0, `d0";
        emit(new AS::MoveInstr(str, L(temp, nullptr), L(possible_fp, nullptr)));
        str = " subq `s1, `d0";
        emit(new AS::OperInstr(str, L(temp, nullptr), L(temp, L(munchExp(e2), nullptr)), nullptr));
      }

      return temp;
    }
    case T::BinOp::MUL_OP:
    {
      TEMP::Temp *temp = TEMP::Temp::NewTemp();
      T::Exp *e1 = ee->left;
      T::Exp *e2 = ee->right;

      str = " movq `s0, %rax";
      emit(new AS::MoveInstr(str, L(F::F_RAX(), nullptr), L(munchExp(e1), nullptr)));
      str = " imulq `s0";
      emit(new AS::OperInstr(str, L(F::F_RAX(), L(F::F_RDX(), nullptr)), L(munchExp(e2), L(F::F_RAX(), nullptr)), nullptr));
      str = " movq %rax, `d0";
      emit(new AS::MoveInstr(str, L(temp, nullptr), L(F::F_RAX(), nullptr)));
      return temp;
    }
      /* T_Binop: a / b
        movq left, %rax
        clto            (dst: %rdx:%rax, src: %rax)
        idivq right     (dst: %rdx:%rax, src: right, %rdx:%rax)
        movq %rax, r 
        AS(..., dst, src)
      */
    case T::BinOp::DIV_OP:
    {
      TEMP::Temp *temp = TEMP::Temp::NewTemp();
      T::Exp *e1 = ee->left;
      T::Exp *e2 = ee->right;

      str = " movq `s0, %rax";
      emit(new AS::MoveInstr(str, L(F::F_RAX(), nullptr), L(munchExp(e1), nullptr)));
      // clear rdx
      str = " cqto";
      emit(new AS::OperInstr(str, L(F::F_RAX(), L(F::F_RDX(), nullptr)), L(F::F_RAX(), nullptr), nullptr));
      // div operation
      str = " idivq `s0";
      emit(new AS::OperInstr(str, L(F::F_RAX(), L(F::F_RDX(), nullptr)),
                             L(munchExp(e2), L(F::F_RAX(), L(F::F_RDX(), nullptr))), nullptr));

      str = " movq %rax, `d0";
      emit(new AS::MoveInstr(str, L(temp, nullptr), L(F::F_RAX(), nullptr)));
      return temp;
    }
    default:
      assert(0);
    }
  }
    /* T_Const: int i = 0
    * movq $imm, dst
    */
  case T::Exp::CONST:
  {
    /* int CONST; */
    TEMP::Temp *r = TEMP::Temp::NewTemp();
    inst = " movq $" + std::to_string(((T::ConstExp *)e)->consti) + ", `d0";
    emit(new AS::OperInstr(inst, L(r, nullptr), nullptr, nullptr));
    return r;
  }
  /* T_Temp: %reg */
  case T::Exp::TEMP:
  {
    /* Temp_temp TEMP; */
    return ((T::TempExp *)e)->temp;
  }
    /*  T_Name: char *s = "abc"
      .L: .string "abc"
      movq $.L, dst
      string Temp_labelstring(Temp_label s);
    */
  case T::Exp::NAME:
  {
    /* Temp_label NAME; */
    TEMP::Temp *r = TEMP::Temp::NewTemp();
    inst = " leaq " + TEMP::LabelString(((T::NameExp *)e)->name) + "(%rip), `d0";
    emit(new AS::OperInstr(inst, L(r, nullptr), nullptr, nullptr));
    return r;
  }
  /* T_Eseq: */
  case T::Exp::ESEQ:
  {
    /* struct {T_stm stm; T_exp exp;} ESEQ; */
    munchStm(((T::EseqExp *)e)->stm);
    return munchExp(((T::EseqExp *)e)->exp);
  }
  /* T_Call : func(args)
      pushq argk
      ...
      pushq arg7
      movq arg6, %r9d
      movq arg5, %r8d
      movq arg4, %rcx
      movq arg3, %rdx
      movq arg2, %rsi
      movq arg1, %rdi
      call func
      movq %rax, dst
  */
  case T::Exp::CALL:
  {
    /* struct {T_exp fun; T_expList args;} CALL; */
    T::CallExp *ee = (T::CallExp *)e;
    TEMP::Label *func = ((T::NameExp *)ee->fun)->name;
    T::ExpList *args = ee->args;
    TEMP::Temp *r = TEMP::Temp::NewTemp();

    if (!func)
      return F::F_RV();

    if (args->head->kind == T::Exp::TEMP)
    {
      inst = " leaq ?0#(%rsp), `d0";
      emit(new AS::OperInstr(inst, L(r, nullptr), L(F::F_RSP(), nullptr), nullptr));
      inst = " movq `s0, (%rsp)";
      emit(new AS::OperInstr(inst, nullptr, L(r, L(F::F_RSP(), nullptr)), nullptr));
    }
    else
    {
      inst = " movq `s0, (%rsp)";
      emit(new AS::OperInstr(inst, nullptr, L(munchExp(args->head), L(F::F_RSP(), nullptr)), nullptr));
    }

    //skip static link
    TEMP::TempList *srcregs = munchArgs(0, args->tail);
    //keep live in the out
    TEMP::TempList *dstregs = TEMP::unionTempList(F::F_Argregs(), L(F::F_RV(), F::Callersaves()));
    inst = " callq " + TEMP::LabelString(func);
    emit(new AS::OperInstr(inst, dstregs, srcregs, nullptr));

    return F::F_RV();
  }
  default:
    assert(0);
  }
}

AS::InstrList *Codegen(F::Frame *f, T::StmList *stmList)
{

  AS::InstrList *list;
  T::StmList *sl;
  frame = f;

  std::string prologue, epilogue;

  // prologue
  prologue = " subq $?0#, %rsp";
  emit(new AS::OperInstr(prologue, L(F::F_RSP(), nullptr), L(F::F_RSP(), nullptr), nullptr));

  // function body
  for (sl = stmList; sl; sl = sl->tail)
    munchStm(sl->head);

  // epilogue
  epilogue = " addq $?0#, %rsp";
  emit(new AS::OperInstr(epilogue, L(F::F_RSP(), nullptr), L(F::F_RSP(), nullptr), nullptr));

  list = ilist;
  last = nullptr;
  ilist = last;

  // retq
  ilist = F::F_procEntryExit2(f, ilist);
  frame = nullptr;

  return list;
}

} // namespace CG