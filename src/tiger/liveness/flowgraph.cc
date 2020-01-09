#include "tiger/liveness/flowgraph.h"

namespace FG
{

TEMP::TempList *Def(G::Node<AS::Instr> *n)
{
  // TODO: Put your codes here (lab6).
  AS::Instr *instr = n->NodeInfo();
  switch (instr->kind)
  {
  case AS::Instr::OPER:
    return ((AS::OperInstr *)instr)->dst;
  case AS::Instr::MOVE:
    return ((AS::MoveInstr *)instr)->dst;
  default:
    return nullptr;
  }
}

TEMP::TempList *Use(G::Node<AS::Instr> *n)
{
  // TODO: Put your codes here (lab6).
  AS::Instr *instr = n->NodeInfo();
  switch (instr->kind)
  {
  case AS::Instr::OPER:
    return ((AS::OperInstr *)instr)->src;
  case AS::Instr::MOVE:
    return ((AS::MoveInstr *)instr)->src;
  default:
    return nullptr;
  }
}

bool IsMove(G::Node<AS::Instr> *n)
{

  AS::Instr *instr = n->NodeInfo();
  return instr->kind == AS::Instr::MOVE;
}

G::Graph<AS::Instr> *AssemFlowGraph(AS::InstrList *il, F::Frame *f)
{
  // TODO: Put your codes here (lab6).
  G::Graph<AS::Instr> *flowgragh = new G::Graph<AS::Instr>();
  TAB::Table<TEMP::Label, G::Node<AS::Instr>> *labelmap = new TAB::Table<TEMP::Label, G::Node<AS::Instr>>();
  AS::InstrList *instrl;
  AS::Instr *instr;
  G::Node<AS::Instr> *prev = NULL, *cur = NULL;

  /* Add Nodes and edges between sequential instructions to the flowgraph first */
  for (instrl = il; instrl; instrl = instrl->tail)
  {
    instr = instrl->head;
    cur = flowgragh->NewNode(instr);
    if (prev)
    {
      flowgragh->AddEdge(prev, cur);
    }

    /* Add labels to the TABtable */
    if (instr->kind == AS::Instr::LABEL)
    {
      labelmap->Enter(((AS::LabelInstr *)instr)->label, cur);
    }

    /* If the current instruction is jmp xxx, then do not addedge between current instruction
		and the next instruction, because after jmp xxx, the next instruction will not be executed. */
    if (instr->kind == AS::Instr::OPER)
    {
      std::string a = (((AS::OperInstr *)instr)->assem).substr(0, 3);
      if (a == "jmp")
      {
        prev = NULL;
        continue;
      }
    }

    prev = cur;
  }

  /* Add edges between jump instructions first*/
  G::NodeList<AS::Instr> *nodes = flowgragh->Nodes();

  for (; nodes; nodes = nodes->tail)
  {
    cur = nodes->head;
    instr = cur->NodeInfo();
    if (instr->kind == AS::Instr::OPER  && ((AS::OperInstr *)instr)->jumps->labels )
    {
      TEMP::LabelList* labels = ((AS::OperInstr *)instr)->jumps->labels;
      for (; labels; labels = labels->tail)
      { 
        G::Node<AS::Instr> *target =  labelmap->Look(labels->head);
        if (target)
          flowgragh->AddEdge(cur, target); 
        else
        {
          //printf("Cannot find label %s\nSee in runtime.s or undefined label\n", TEMP::LabelString(labels->head)); //For debugging
        }
      }
    }
  }
  return flowgragh;
}

} // namespace FG
