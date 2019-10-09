#include "straightline/slp.h"

namespace A {
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return std::max(stm1->MaxArgs(),stm2->MaxArgs());
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  // std::cout<<1<<std::endl;
  // Table *newt=t;
  // std::cout<<newt<<std::endl;
  // return nullptr;
  Table *t1=stm1->Interp(t);
  Table *t2=stm2->Interp(t1);
  return t2;
  // Table *newt=t;
  // if(stm1!=nullptr)
  //   newt=stm1->Interp(newt);
  
  // if(stm2!=nullptr)
  //   newt=stm2->Interp(newt);

  // return newt;
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  // return nullptr;
  Table *newt=t;
  if(newt==nullptr){
    newt=new Table(id,exp->Interp(nullptr)->i,newt);
  }else{
    newt=newt->Update(id,exp->Interp(newt)->i);
  }

  return newt;
}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exps->MaxArgs();
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return exps->Interp(t)->t;
}

int Table::Lookup(std::string key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(std::string key, int value) const {
  return new Table(key, value, this);
}


// IdExp
int A::IdExp::MaxArgs() const {
  return 0;
}

IntAndTable *A::IdExp::Interp(Table *t) const {
  return new IntAndTable(t->Lookup(id),t);
}


// NumExp
int A::NumExp::MaxArgs() const {
  return 0;
}

IntAndTable *A::NumExp::Interp(Table *t) const {
  return new IntAndTable(num,t);
}

// OpExp
int A::OpExp::MaxArgs() const {
  return std::max(left->MaxArgs(),right->MaxArgs());
}

IntAndTable *A::OpExp::Interp(Table *t) const {
  IntAndTable *leftIt= left->Interp(t);
  t=leftIt->t;
  IntAndTable *rightIt=right->Interp(t);
  t=rightIt->t;

  switch (oper)
  {
  case PLUS:
    return new IntAndTable(leftIt->i+rightIt->i,t);
  case MINUS:
    return new IntAndTable(leftIt->i-rightIt->i,t);
  case TIMES:
    return new IntAndTable(leftIt->i*rightIt->i,t);
  case DIV:
    return new IntAndTable(leftIt->i/rightIt->i,t);
  default:
    return nullptr;
  }
}

// EseqExp
int A::EseqExp::MaxArgs() const {
  return std::max(stm->MaxArgs(),exp->MaxArgs());
}

IntAndTable *A::EseqExp::Interp(Table *t) const {
  Table *newt=stm->Interp(t);
  return new IntAndTable(exp->Interp(newt)->i,t);
}

// PairExpList
int A::PairExpList::MaxArgs() const {
  return 1+tail->MaxArgs();
}

IntAndTable *A::PairExpList::Interp(Table *t) const {
  IntAndTable *headIt=head->Interp(t);
  // printf("%d",headIt->i);
  std::cout<<headIt->i<<" ";
  return new IntAndTable(tail->Interp(t)->i,t);
}

// LastExpList
int A::LastExpList::MaxArgs() const {
  return 1;
}

IntAndTable *A::LastExpList::Interp(Table *t) const {
  // printf("%d",last->Interp(t)->i);
  std::cout<<last->Interp(t)->i<<std::endl;;
  return new IntAndTable(last->Interp(t)->i,t);
}

}  // namespace A
