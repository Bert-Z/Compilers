#include "tiger/semant/semant.h"
#include "tiger/errormsg/errormsg.h"

extern EM::ErrorMsg errormsg;

using VEnvType = S::Table<E::EnvEntry> *;
using TEnvType = S::Table<TY::Ty> *;

namespace
{
static TY::TyList *make_formal_tylist(TEnvType tenv, A::FieldList *params)
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

static TY::FieldList *make_fieldlist(TEnvType tenv, A::FieldList *fields)
{
  if (fields == nullptr)
  {
    return nullptr;
  }

  TY::Ty *ty = tenv->Look(fields->head->typ);
  return new TY::FieldList(new TY::Field(fields->head->name, ty),
                           make_fieldlist(tenv, fields->tail));
}

} // namespace

namespace A
{

TY::Ty *SimpleVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const
{
  // TODO: Put your codes here (lab4).
  E::EnvEntry *env_entry = venv->Look(this->sym);
  if (env_entry->kind == E::EnvEntry::VAR && env_entry)
  {
    E::VarEntry *var_entry = (E::VarEntry *)env_entry;
    return var_entry->ty;
  }
  else
  {
    errormsg.Error(this->pos, "undefined variable %s\n", this->sym->Name().c_str());
    return TY::VoidTy::Instance();
  }
}

TY::Ty *FieldVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *ltype = var->SemAnalyze(venv, tenv, labelcount)->ActualTy();
  // check var
  if (ltype->kind != TY::Ty::RECORD)
  {
    errormsg.Error(this->pos, "not a record type");
    return TY::VoidTy::Instance();
  }

  // check field
  TY::FieldList *fields = ((TY::RecordTy *)ltype)->fields;
  TY::Field *field;

  for (; fields; fields = fields->tail)
  {
    field = fields->head;
    if (field->name == this->sym)
    {
      return field->ty;
    }
  }
  errormsg.Error(this->pos, "field %s doesn't exist", this->sym->Name().c_str());

  return TY::VoidTy::Instance();
}

TY::Ty *SubscriptVar::SemAnalyze(VEnvType venv, TEnvType tenv,
                                 int labelcount) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *ltype = var->SemAnalyze(venv, tenv, labelcount);
  if (ltype->kind != TY::Ty::ARRAY)
  {
    errormsg.Error(this->pos, "array type required");
    return TY::VoidTy::Instance();
  }

  TY::Ty *exptyppe = subscript->SemAnalyze(venv, tenv, labelcount);
  if (exptyppe->kind != TY::Ty::INT)
  {
    errormsg.Error(this->pos, "array index must be integer");
    return TY::VoidTy::Instance();
  }

  return ((TY::ArrayTy *)ltype)->ty;
}

TY::Ty *VarExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *var_type = var->SemAnalyze(venv, tenv, labelcount);
  return var_type;
}

TY::Ty *NilExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).

  return TY::NilTy::Instance();
}

TY::Ty *IntExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  return TY::IntTy::Instance();
}

TY::Ty *StringExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const
{
  // TODO: Put your codes here (lab4).
  return TY::StringTy::Instance();
}

TY::Ty *CallExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                            int labelcount) const
{
  // TODO: Put your codes here (lab4).
  E::EnvEntry *env_entry = venv->Look(this->func);
  A::ExpList *args = this->args;
  if (!env_entry || env_entry->kind != E::EnvEntry::FUN)
  {
    errormsg.Error(this->pos, "undefined function %s", this->func->Name().c_str());
    return TY::VoidTy::Instance();
  }
  TY::TyList *funcargs = ((E::FunEntry *)env_entry)->formals;
  TY::Ty *funcarg;
  A::Exp *arg;
  for (; args && funcargs; args = args->tail, funcargs = funcargs->tail)
  {
    arg = args->head;
    funcarg = funcargs->head;
    TY::Ty *expty = arg->SemAnalyze(venv, tenv, labelcount);
    if (!expty->IsSameType(funcarg))
    {
      errormsg.Error(this->pos, "para type mismatch");
    }
  }
  if (!args)
  {
    errormsg.Error(this->pos, "too many params in function %s", this->func->Name().c_str());
  }

  return ((E::FunEntry *)env_entry)->result;
}

TY::Ty *OpExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *leftty = this->left->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *rightty = this->right->SemAnalyze(venv, tenv, labelcount);

  switch (oper)
  {
  case A::Oper::PLUS_OP:
  case A::Oper::TIMES_OP:
  case A::Oper::MINUS_OP:
  case A::Oper::DIVIDE_OP:
  {
    if (leftty->kind != TY::Ty::INT)
      errormsg.Error(this->pos, "integer required");

    if (rightty->kind != TY::Ty::INT)
      errormsg.Error(this->pos, "integer required");

    break;
  }
  case A::Oper::LT_OP:
  case A::Oper::LE_OP:
  case A::Oper::GT_OP:
  case A::Oper::GE_OP:
  case A::Oper::EQ_OP:
  case A::Oper::NEQ_OP:
  {
    if (leftty->kind != TY::Ty::INT && leftty->kind != TY::Ty::STRING)
      errormsg.Error(this->pos, "integer or string required");

    if (rightty->kind != TY::Ty::INT && rightty->kind != TY::Ty::STRING)
      errormsg.Error(this->pos, "integer or string required");

    if (!rightty->IsSameType(leftty))
      errormsg.Error(this->pos, "same type required");

    break;
  }
  default:
  {
    break;
  }
  }

  return TY::IntTy::Instance();
}

TY::Ty *RecordExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *typty = tenv->Look(this->typ);
  if (!typty)
  {
    errormsg.Error(this->pos, "undefined type %s", this->typ->Name().c_str());
    return TY::VoidTy::Instance();
  }

  if (typty->kind != TY::Ty::RECORD)
  {
    errormsg.Error(this->pos, "not a record type");
    return TY::VoidTy::Instance();
  }

  A::EFieldList *eflist = this->fields;
  TY::FieldList *fieldlist = ((TY::RecordTy *)typ)->fields;
  A::EField *efield;
  TY::Field *field;
  for (; eflist; eflist = eflist->tail, fieldlist = fieldlist->tail)
  {
    if (!fieldlist)
    {
      errormsg.Error(this->pos, "Too many efields in %s", this->typ->Name().c_str());
      break;
    }
    efield = eflist->head;
    field = fieldlist->head;
    TY::Ty *expty = efield->exp->SemAnalyze(venv, tenv, labelcount);
    if (!expty->IsSameType(field->ty))
    {
      errormsg.Error(this->pos, "record type unmatched");
    }
  }

  if (fieldlist != NULL)
  {
    errormsg.Error(this->pos, "Too little efields in %s", this->typ->Name().c_str());
  }

  return typty;
}

TY::Ty *SeqExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  A::ExpList *explist = this->seq;
  if (!explist)
  {
    return TY::VoidTy::Instance();
  }
  TY::Ty *expty;
  for (; explist; explist = explist->tail)
  {
    expty = explist->head->SemAnalyze(venv, tenv, labelcount);
  }
  return expty;
}

TY::Ty *AssignExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                              int labelcount) const
{
  // TODO: Put your codes here (lab4).
  if (this->var->kind == A::Var::SIMPLE)
  {
    E::EnvEntry *env_entry = venv->Look(((A::SimpleVar *)var)->sym);
    if (env_entry->readonly)
    {
      errormsg.Error(this->pos, "loop variable can't be assigned");
    }
  }
  TY::Ty *varty = var->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *expty = exp->SemAnalyze(venv, tenv, labelcount);
  if (!varty->IsSameType(expty))
  {
    errormsg.Error(this->pos, "unmatched assign exp");
  }
  return varty;
}

TY::Ty *IfExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  TY::Ty *testty = this->test->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *thenty = this->then->SemAnalyze(venv, tenv, labelcount);

  if (testty->kind != TY::Ty::INT)
  {
    errormsg.Error(this->pos, "integer required");
  }
  if (this->elsee && this->elsee->kind != A::Exp::NIL)
  {
    TY::Ty *elseety = this->elsee->SemAnalyze(venv, tenv, labelcount);
    if (!thenty->IsSameType(elseety))
    {
      errormsg.Error(this->pos, "then exp and else exp type mismatch");
    }
  }
  else if (thenty->kind != TY::Ty::VOID)
  {
    errormsg.Error(this->pos, "if-then exp's body must produce no value");
  }

  // TODO: Put your codes here (lab4).
  return thenty;
}

TY::Ty *WhileExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *testty = this->test->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *bodyty = this->body->SemAnalyze(venv, tenv, labelcount);

  if (test->kind != TY::Ty::INT)
  {
    errormsg.Error(this->pos, "integer required");
  }
  if (body->kind != TY::Ty::VOID)
  {
    errormsg.Error(this->pos, "while body must produce no value");
  }

  return bodyty;
}

TY::Ty *ForExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *loty = this->lo->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *hity = this->hi->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *bodyty = this->body->SemAnalyze(venv, tenv, labelcount);
  venv->BeginScope();
  if (!loty->IsSameType(hity) || loty->kind != TY::Ty::INT)
  {
    errormsg.Error(this->pos, "for exp's range type is not integer");
  }
  E::EnvEntry *env_entry = new E::VarEntry(loty, true);
  venv->Enter(this->var, env_entry);
  if (bodyty->kind != TY::Ty::VOID)
  {
    errormsg.Error(this->pos, "for body must produce no value");
  }
  venv->EndScope();

  return bodyty;
}

TY::Ty *BreakExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const
{
  // TODO: Put your codes here (lab4).
  return TY::VoidTy::Instance();
}

TY::Ty *LetExp::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  A::DecList *declist = this->decs;
  A::Exp *exp = this->body;
  A::Dec *dec;
  venv->BeginScope();
  tenv->BeginScope();
  for (dec = declist->head; dec; declist = declist->tail)
  {
    dec->SemAnalyze(venv, tenv, labelcount);
  }
  TY::Ty *expty = exp->SemAnalyze(venv, tenv, labelcount);
  tenv->EndScope();
  venv->EndScope();

  return expty;
}

TY::Ty *ArrayExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *arrayty = tenv->Look(this->typ);
  if (!arrayty)
  {
    errormsg.Error(this->pos, "undefined type %s", this->typ->Name().c_str());
    return TY::VoidTy::Instance();
  }

  if (arrayty->kind != TY::Ty::ARRAY)
  {
    errormsg.Error(this->pos, "not array type");
  }

  TY::Ty *sizety = this->size->SemAnalyze(venv, tenv, labelcount);
  TY::Ty *initty = this->init->SemAnalyze(venv, tenv, labelcount);
  if (!initty->IsSameType(((TY::ArrayTy *)arrayty)->ty))
  {
    errormsg.Error(this->pos, "type mismatch");
  }
  if (sizety->kind != TY::Ty::INT)
  {
    errormsg.Error(this->pos, "type of size expression should be int");
  }
  return arrayty;
}

TY::Ty *VoidExp::SemAnalyze(VEnvType venv, TEnvType tenv,
                            int labelcount) const
{
  // TODO: Put your codes here (lab4).
  return TY::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(VEnvType venv, TEnvType tenv,
                             int labelcount) const
{
  // TODO: Put your codes here (lab4).
  A::FunDecList *funcs = this->functions;
  A::FunDec *func;

  for (; func; funcs = funcs->tail)
  {
    func = funcs->head;
    if (venv->Look(func->name))
    {
      errormsg.Error(this->pos, "two functions have the same name");
      continue;
    }
    TY::TyList *formaltys = make_formal_tylist(tenv, func->params);
    if (func->result)
    {
      TY::Ty *resultTy = tenv->Look(func->result);
      if (resultTy->kind == TY::Ty::VOID)
      {
        errormsg.Error(this->pos, "undefined return type %s", func->result);
        continue;
      }
      venv->Enter(func->name, new E::FunEntry(formaltys, resultTy));
    }
    else
    {
      venv->Enter(func->name, new E::FunEntry(formaltys, TY::VoidTy::Instance()));
    }
  }

  for (funcs = this->functions; funcs; funcs = funcs->tail)
  {
    func = funcs->head;
    TY::TyList *formaltys = make_formal_tylist(tenv, func->params);

    venv->BeginScope();
    A::FieldList *fields;
    TY::TyList *tys;
    for (fields = func->params, tys = formaltys; fields; fields = fields->tail, tys = tys->tail)
    {
      venv->Enter(fields->head->name, new E::VarEntry(tys->head));
    }
    TY::Ty *bodyty = func->body->SemAnalyze(venv, tenv, labelcount);
    E::EnvEntry *func_entry = venv->Look(func->name);
    if (func_entry->kind != bodyty->kind)
    {
      if (func_entry->kind == TY::Ty::VOID)
      {
        errormsg.Error(this->pos, "procedure returns value");
      }
      errormsg.Error(this->pos, "procedure returns unexpected type");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  A::Exp *exp = this->init;
  TY::Ty *expty = exp->SemAnalyze(venv, tenv, labelcount);
  if (!this->typ)
  {
    if (expty->kind == TY::Ty::NIL)
    {
      errormsg.Error(this->pos, "init should not be nil without type specified");
      return;
    }
    venv->Enter(this->var, new E::VarEntry(expty));
    return;
  }
  else
  {
    TY::Ty *typety = tenv->Look(this->typ);
    if (!typety)
    {
      errormsg.Error(this->pos, "undefined type %s", this->typ->Name().c_str());
      return;
    }

    if (expty->kind == TY::Ty::NIL && typety->kind != TY::Ty::RECORD)
      errormsg.Error(this->pos, "init should not be nil without type specified");

    if (!typety->IsSameType(expty) && expty->kind != TY::Ty::NIL)
      errormsg.Error(this->pos, "type mismatch");

    venv->Enter(this->var, new E::VarEntry(expty));
  }
}

void TypeDec::SemAnalyze(VEnvType venv, TEnvType tenv, int labelcount) const
{
  // TODO: Put your codes here (lab4).
  A::NameAndTyList *ntys = this->types;
  A::NameAndTy *nty;
  //  push type names into tenv to handle recursive type.
  for (; ntys; ntys->tail)
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
      tenv->Enter(nty->name, new TY::NameTy(nty->name, nty->ty->SemAnalyze(tenv)));
    }
  }

  //  Find wrong recursive typedec.
  for (ntys = this->types; ntys; ntys = ntys->tail)
  {
    nty = ntys->head;

    TY::Ty *def_name = tenv->Look(nty->name);
    TY::Ty *def_value;
    while (def_name->kind == TY::Ty::NAME)
    {
      def_value = ((TY::NameTy *)def_name)->ty;
      if (def_value->IsSameType(def_name))
      {
        errormsg.Error(this->pos, "illegal type cycle");
        // set as voidty
        tenv->Set(nty->name, TY::VoidTy::Instance());
        break;
      }
    }
  }
}

TY::Ty *NameTy::SemAnalyze(TEnvType tenv) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *namety = tenv->Look(this->name);
  if (!namety)
  {
    errormsg.Error(this->pos, "undefined type %s", this->name->Name().c_str());
    return TY::VoidTy::Instance();
  }
  return new TY::NameTy(this->name, namety);
}

TY::Ty *RecordTy::SemAnalyze(TEnvType tenv) const
{
  // TODO: Put your codes here (lab4).
  A::FieldList *records = this->record;
  A::Field *field;
  for (; records; records = records->tail)
  {
    field = records->head;
    TY::Ty *fieldty = tenv->Look(field->typ);
    if (!field)
    {
      errormsg.Error(this->pos, "undefined type %s", field->typ->Name().c_str());
      tenv->Set(field->typ, TY::VoidTy::Instance());
    }
    TY::Field tyfield = TY::Field(field->name, fieldty);
  }
  TY::FieldList *fieldlistTy = make_fieldlist(tenv, records);

  return new TY::RecordTy(fieldlistTy);
}

TY::Ty *ArrayTy::SemAnalyze(TEnvType tenv) const
{
  // TODO: Put your codes here (lab4).
  TY::Ty *arrayty = tenv->Look(this->array);
  if (!arrayty)
  {
    errormsg.Error(this->pos, "undefined type %s", this->array->Name().c_str());
    return TY::VoidTy::Instance();
  }

  return new TY::ArrayTy(arrayty);
}

} // namespace A

namespace SEM
{
void SemAnalyze(A::Exp *root)
{
  if (root)
    root->SemAnalyze(E::BaseVEnv(), E::BaseTEnv(), 0);
}

} // namespace SEM
