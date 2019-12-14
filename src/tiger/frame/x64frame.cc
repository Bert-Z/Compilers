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

  return new Frame(name, accesslist);
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

// (level,frame point)-> exp *
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
  return F_RSP();
}

// %rax
TEMP::Temp *F_RV()
{
  return F_RAX();
}

T::Exp *F::externalCall(std::string s, T::ExpList *args)
{
  return new T::CallExp(new T::NameExp(TEMP::NamedLabel(s)), args);
}

TEMP::Temp *F_Arg(int idx)
{
  switch (idx)
  {
  case 0:
    return F_RDI();
  case 1:
    return F_RSI();
  case 2:
    return F_RDX();
  case 3:
    return F_RCX();
  case 4:
    return F_R8();
  case 5:
    return F_R9();
  default:
    assert(0);
  }
}

TEMP::TempList *F_Argregs()
{
  static TEMP::TempList *regs = nullptr;
  if (regs == nullptr)
  {
    regs = new TEMP::TempList(
        F_RDI(), new TEMP::TempList(
                     F_RSI(), new TEMP::TempList(
                                  F_RDX(), new TEMP::TempList(
                                               F_RCX(), new TEMP::TempList(
                                                            F_R8(), new TEMP::TempList(
                                                                        F_R9(), NULL))))));
  }
  return regs;
}

/* %rbx, %rbp, %r12, %r13, %r14, %r15 */
TEMP::TempList *Calleesaves()
{
  static TEMP::TempList *regs = NULL;
  if (regs == NULL)
  {
    regs = new TEMP::TempList(
        F_RBX(), new TEMP::TempList(
                     F_RBP(), new TEMP::TempList(
                                  F_R12(), new TEMP::TempList(
                                               F_R13(), new TEMP::TempList(
                                                            F_R14(), new TEMP::TempList(
                                                                         F_R15(), NULL))))));
  }
  return regs;
}

/* %r10, %r11 */
TEMP::TempList *Callersaves()
{
  static TEMP::TempList *regs = nullptr;
  if (regs == nullptr)
  {
    regs = new TEMP::TempList(F_R10(), new TEMP::TempList(F_R11(), nullptr));
  }
  return regs;
}

TEMP::Temp *F_RAX()
{
  static TEMP::Temp *rax = nullptr;
  if (!rax)
  {
    rax = TEMP::Temp::NewTemp();
  }
  return rax;
}
TEMP::Temp *F_RBX()
{
  static TEMP::Temp *rbx = nullptr;
  if (!rbx)
  {
    rbx = TEMP::Temp::NewTemp();
  }
  return rbx;
}
TEMP::Temp *F_RCX()
{
  static TEMP::Temp *rcx = nullptr;
  if (!rcx)
  {
    rcx = TEMP::Temp::NewTemp();
  }
  return rcx;
}
TEMP::Temp *F_RDX()
{
  static TEMP::Temp *rdx = nullptr;
  if (!rdx)
  {
    rdx = TEMP::Temp::NewTemp();
  }
  return rdx;
}
TEMP::Temp *F_RSI()
{
  static TEMP::Temp *rsi = nullptr;
  if (!rsi)
  {
    rsi = TEMP::Temp::NewTemp();
  }
  return rsi;
}
TEMP::Temp *F_RDI()
{
  static TEMP::Temp *rdi = nullptr;
  if (!rdi)
  {
    rdi = TEMP::Temp::NewTemp();
  }
  return rdi;
}
TEMP::Temp *F_RBP()
{
  static TEMP::Temp *rbp = nullptr;
  if (!rbp)
  {
    rbp = TEMP::Temp::NewTemp();
  }
  return rbp;
}
TEMP::Temp *F_RSP()
{
  static TEMP::Temp *rsp = nullptr;
  if (!rsp)
  {
    rsp = TEMP::Temp::NewTemp();
  }
  return rsp;
}
TEMP::Temp *F_R8()
{
  static TEMP::Temp *r8 = nullptr;
  if (!r8)
  {
    r8 = TEMP::Temp::NewTemp();
  }
  return r8;
}
TEMP::Temp *F_R9()
{
  static TEMP::Temp *r9 = nullptr;
  if (!r9)
  {
    r9 = TEMP::Temp::NewTemp();
  }
  return r9;
}
TEMP::Temp *F_R10()
{
  static TEMP::Temp *r10 = nullptr;
  if (!r10)
  {
    r10 = TEMP::Temp::NewTemp();
  }
  return r10;
}

TEMP::Temp *F_R11()
{
  static TEMP::Temp *r11 = nullptr;
  if (!r11)
  {
    r11 = TEMP::Temp::NewTemp();
  }
  return r11;
}
TEMP::Temp *F_R12()
{
  static TEMP::Temp *r12 = nullptr;
  if (!r12)
  {
    r12 = TEMP::Temp::NewTemp();
  }
  return r12;
}
TEMP::Temp *F_R13()
{
  static TEMP::Temp *r13 = nullptr;
  if (!r13)
  {
    r13 = TEMP::Temp::NewTemp();
  }
  return r13;
}
TEMP::Temp *F_R14()
{
  static TEMP::Temp *r14 = nullptr;
  if (!r14)
  {
    r14 = TEMP::Temp::NewTemp();
  }
  return r14;
}
TEMP::Temp *F_R15()
{
  static TEMP::Temp *r15 = nullptr;
  if (!r15)
  {
    r15 = TEMP::Temp::NewTemp();
  }
  return r15;
}


/* 
 * use in translate.c, Tr_procEntryExit
 * 参考: p121页 第4/5/8条
 * 第4条：入口处理：将逃逸参数(包括静态链)保存至栈帧的指令，以及将非逃逸参数传送到新的临时寄存器的指令 
 * 第5条：入口处理：保存在此函数内用到的calleesave寄存器(包括返回地址寄存器)的存储指令 
 * 第6条：出口处理：用于恢复被调用者保护的寄存器的取数指令 
 * T_stm T_Move(T_exp dst, T_exp src)
 */
// T::Stm *F_procEntryExit1(F::Frame *frame, T::Stm *stm)
// {
// 	Temp_tempList saveTemps = NULL;
// 	Temp_tempList* saveTempsPtr = &saveTemps;

// 	// step 1 : callee save 
// 	// sample : move %rbx, temp
// 	T_stm save = NULL;
// 	Temp_tempList callees = F_Calleesaves();
	
// 	for(; callees; callees = callees->tail)
// 	{
// 		Temp_temp temp = Temp_newtemp();
// 		T_exp expp = T_Temp(temp);
// 		if (save){
// 			save = T_Seq(save, T_Move(expp, T_Temp(callees->head)));
// 		} else{
// 			save = T_Move(expp, T_Temp(callees->head));
// 		}
// 		*saveTempsPtr = Temp_TempList(temp, NULL);
// 		saveTempsPtr = &((*saveTempsPtr)->tail);
// 	}

// 	// step 2 : callee restore
// 	// sample : move temp, %rbx
// 	T_stm restore = NULL;
// 	callees = F_Calleesaves();

// 	for(; callees; callees = callees->tail)
// 	{
// 		T_exp expp = T_Temp(saveTemps->head);
// 		if (restore){
// 			restore = T_Seq(restore, T_Move(T_Temp(callees->head), expp));
// 		} else{
// 			restore = T_Move(T_Temp(callees->head), expp);
// 		}
// 		saveTemps = saveTemps->tail;
// 	}

// 	// step 3: combine save, prologue(view), stm, restore and return
// 	return T_Seq(save, T_Seq(frame->prologue, T_Seq(stm, restore)));
// }

/*
 * use in codegen.c, F_codegen
 * 参考: p153
 * 在函数体末未添加下沉指令，用以告诉寄存器分配器，某些寄存器在过程的出口是活跃的，可以防止寄存器分配器将其用于其他目的
 */
// static TEMP::TempList* returnSink = nullptr;
// AS::InstrList *F_procEntryExit2(F::Frame *frame, AS::InstrList *body)
// {
// 	//scan the whole proc body to find max arg's number
// 	AS_instrList tempBody = body;
// 	int max_num = 0;
// 	while(tempBody)
// 	{
// 		AS_instr instr = tempBody->head;
// 		if(instr->kind == I_OPER && !strncmp(instr->u.OPER.assem, " call", 4))
// 		{
// 			int num = 0;
// 			Temp_tempList tempList = instr->u.OPER.src;
// 			while(tempList){
// 				tempList = tempList->tail;
// 				num++;
// 			}
// 			max_num = (max_num > num ? max_num : num);
// 		}
// 		tempBody = tempBody->tail;
// 	}
// 	frame->max_arg_num = max_num;
// 	if(!returnSink){
// 		returnSink = Temp_TempList(F_RV(), Temp_TempList(F_SP(), F_Calleesaves()));
// 	}
// 	return AS_splice(body, AS_InstrList(AS_Oper("", NULL, returnSink, NULL), NULL));
// }

/*
 * use in main.c, doProc
 * 参考: p153
 * 我们生成的代码只处理过程体，而不处理入口和出口指令序列
 * struct AS_proc_ { string prolog; AS_instrList body; string epilog; }
 */
// AS::Proc* F_procEntryExit3(F::Frame *frame, AS::InstrList *body)
// {
// 	//static link
// 	frame->size += 8;
// 	if(frame->max_arg_num > 6){
// 		frame->size += (frame->max_arg_num - 6) * F_wordSize;
// 	}

// 	AS_rewriteFrameSize(frame, body);

// 	char prolog_buf[100], epilog_buf[100];
// 	sprintf(prolog_buf, "%s:\n", S_name(frame->label));
// 	sprintf(epilog_buf, " retq\n\n");

//     return AS_Proc(String(prolog_buf), body, String(epilog_buf));
// }

} // namespace F