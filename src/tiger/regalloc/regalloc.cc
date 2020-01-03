#include "tiger/regalloc/regalloc.h"

namespace RA
{

void init_regmap();
void allocate_reg(AS::InstrList *instr, TEMP::Map *map);
static void RewriteProgram(F::Frame *f, AS::InstrList *pil, TEMP::Map *map);
std::string *find_reg(bool def);

void addreg(TEMP::Map *m)
{

  std::string *addr = new std::string("%rdx");
  m->Enter(F::F_RDX(), addr);

  addr = new std::string("%rbx");
  m->Enter(F::F_RBX(), addr);

  addr = new std::string("%rbp");
  m->Enter(F::F_FP(), addr);
  m->Enter(F::F_RBP(), addr);

  addr = new std::string("%rsp");
  m->Enter(F::F_SP(), addr);
  addr = new std::string("%rax");
  m->Enter(F::F_RAX(), addr);
  m->Enter(F::F_RV(), addr);

  addr = new std::string("%rsi");
  m->Enter(F::F_RSI(), addr);
  addr = new std::string("%rcx");
  m->Enter(F::F_RCX(), addr);

  addr = new std::string("%rdi");
  m->Enter(F::F_RDI(), addr);
  addr = new std::string("%r8");
  m->Enter(F::F_R8(), addr);
  addr = new std::string("%r9");
  m->Enter(F::F_R9(), addr);
  addr = new std::string("%r10");
  m->Enter(F::F_R10(), addr);
  addr = new std::string("%r11");
  m->Enter(F::F_R11(), addr);
  addr = new std::string("%r12");
  m->Enter(F::F_R12(), addr);
  addr = new std::string("%r13");
  m->Enter(F::F_R13(), addr);
  addr = new std::string("%r14");
  m->Enter(F::F_R14(), addr);
  addr = new std::string("%r15");
  m->Enter(F::F_R15(), addr);
}

bool isreg(TEMP::Temp *t)
{

  if (t == F::F_RDX() ||
      t == F::F_RBX() ||
      t == F::F_FP() || t == F::F_RBP() ||
      t == F::F_SP() ||
      t == F::F_RAX() || t == F::F_RV() ||
      t == F::F_RSI() ||
      t == F::F_RCX() ||
      t == F::F_RDI() ||
      t == F::F_R8() ||
      t == F::F_R9() ||
      t == F::F_R10() ||
      t == F::F_R11() ||
      t == F::F_R12() ||
      t == F::F_R13() ||
      t == F::F_R14() ||
      t == F::F_R15())
  {
    return true;
  }

  return false;
}

static void add_temp(AS::InstrList *instr)
{

  while (instr)
  {

    TEMP::TempList *def = nullptr, *use = nullptr;
    switch (instr->head->kind)
    {
    case AS::Instr::MOVE:
      def = ((AS::MoveInstr *)instr->head)->dst;
      use = ((AS::MoveInstr *)instr->head)->src;

      break;
    case AS::Instr::OPER:
      def = ((AS::OperInstr *)instr->head)->dst;
      use = ((AS::OperInstr *)instr->head)->src;
      break;
    }
    while (def)
    {
      if (!isreg(def->head))
      {
        spilledNodes.insert(def->head);
      }
      def = def->tail;
    }
    while (use)
    {

      if (!isreg(use->head))
      {
        spilledNodes.insert(use->head);
      }
      use = use->tail;
    }
    instr = instr->tail;
  }
}

void regmap_setone(std::string s)
{
  for (std::map<std::string *, int>::iterator iter = regmap.begin(); iter != regmap.end(); iter++)
  {
    if (*(iter->first) == s)
    {
      iter->second = 1;
      return;
    }
  }
}

Result RegAlloc(F::Frame *f, AS::InstrList *il)
{
  // TODO: Put your codes here (lab6).
  TEMP::Map *map = TEMP::Map::Empty();
  addreg(map);
  add_temp(il);

  RewriteProgram(f, il, map);
  allocate_reg(il, map);

  return Result(map, il);
}

static TEMP::TempList *replaceTempList(TEMP::TempList *instr, TEMP::Temp *old, TEMP::Temp *new_t)
{
  if (instr)
  {
    if (instr->head == old)
    {
      return new TEMP::TempList(new_t, replaceTempList(instr->tail, old, new_t));
    }
    else
    {
      return new TEMP::TempList(instr->head, replaceTempList(instr->tail, old, new_t));
    }
  }
  else
  {
    return nullptr;
  }
}

bool intemp(TEMP::TempList *list, TEMP::Temp *temp)
{
  for (; list; list = list->tail)
  {
    if (list->head == temp)
      return true;
  }
  return false;
}
void RewriteProgram(F::Frame *f, AS::InstrList *pil, TEMP::Map *map)
{
  std::string fs = TEMP::LabelString(f->label) + "_framesize";

  AS::InstrList *il = pil;

  AS::InstrList *instr, //every instruction in il
      *last,            // last handled instruction
      *next,            //next instruction
      *new_instr;       //new_instruction after spilling.
  int off;
  int count = spilledNodes.size();
  printf("-------====tempcount:%d=====-----\n", count);

  while (!spilledNodes.empty())
  {
    TEMP::Temp *spilltemp = *(spilledNodes.begin());

    printf("-------====spilltemp :%d=====-----\n", spilltemp->Int());
    spilledNodes.erase(spilledNodes.begin());
    off = f->s_offset;
    f->s_offset -= 8;
    instr = il;
    last = nullptr;
    
    while (instr)
    {
      TEMP::Temp *t = nullptr;
      next = instr->tail;
      TEMP::TempList *def = nullptr, *use = nullptr;
      switch (instr->head->kind)
      {
      case AS::Instr::MOVE:
        def = ((AS::MoveInstr *)instr->head)->dst;
        use = ((AS::MoveInstr *)instr->head)->src;

        break;
      case AS::Instr::OPER:
        def = ((AS::OperInstr *)instr->head)->dst;
        use = ((AS::OperInstr *)instr->head)->src;
        break;
      }

      if (use && intemp(use, spilltemp))
      {

        t = TEMP::Temp::NewTemp();
        //  debug
        if (instr->head->kind == AS::Instr::OPER)
        {
          std::string assem = ((AS::OperInstr *)instr->head)->assem;
          printf("%s \n ", assem.c_str());
        }
        if (instr->head->kind == AS::Instr::MOVE)
        {
          std::string assem = ((AS::MoveInstr *)instr->head)->assem;
          printf("%s \n ", assem.c_str());
        }
        printf("-------====replace temp :%d=====-----\n", t->Int());

        //Replace spilledtemp by t.

        *use = *replaceTempList(use, spilltemp, t);

        std::string assem;
        std::stringstream ioss;
        ioss << "#use Spill load\nmovq (" + fs + "-0x" << std::hex << -off << ")(%rsp),`d0\n";

        assem = ioss.str();

        //Add the new instruction betfore the old one.
        AS::OperInstr *os_instr = new AS::OperInstr(assem, new TEMP::TempList(t, nullptr), nullptr, new AS::Targets(nullptr));
        new_instr = new AS::InstrList(os_instr, instr);
        if (last)
        {
          last->tail = new_instr;
        }
        else //instr is the first instruction of il.
        {
          il = new_instr;
        }

        std::string assem_store;
        std::stringstream ioss_store;
        ioss_store << "#use Spill store\nmovq `s0, (" + fs + "-0x" << std::hex << -off << ")(%rsp) \n";

        assem_store = ioss_store.str();

        //Add the new instruction betfore the old one.
        AS::OperInstr *os_instr_store;
        if (instr->head->kind == AS::Instr::MOVE && ((AS::MoveInstr *)instr->head)->assem == "movq `s0, (`s1)")
        {
          if (t == use->head)
          {
            os_instr_store = new AS::OperInstr(assem_store, nullptr, new TEMP::TempList(use->head, nullptr), new AS::Targets(nullptr));
          }
          else
          {
            os_instr_store = new AS::OperInstr("#free t" + assem_store, nullptr,
                                               new TEMP::TempList(use->head, new TEMP::TempList(t, nullptr)), new AS::Targets(nullptr));
          }
        }
        else
        {
          os_instr_store = new AS::OperInstr(assem_store, nullptr, new TEMP::TempList(t, nullptr), new AS::Targets(nullptr));
        }
        instr->tail = new AS::InstrList(os_instr_store, next);
        next = instr->tail;

        last = instr->tail;
      }
      else
      {

        last = instr;
      }

      if (def && intemp(def, spilltemp))
      {
        if (!t)
        {
          t = TEMP::Temp::NewTemp();
          printf("-------====DEF replace temp :%d=====-----\n", t->Int());
        }

        *def = *replaceTempList(def, spilltemp, t);

        assert(!intemp(def, spilltemp));

        std::string assem;
        std::stringstream ioss;
        ioss << "# Spill store\nmovq `s0, (" + fs + "-0x" << std::hex << -off << ")(%rsp) \n";

        assem = ioss.str();

        //Add the new instruction betfore the old one.
        AS::OperInstr *os_instr = new AS::OperInstr(assem, nullptr, new TEMP::TempList(t, nullptr), new AS::Targets(nullptr));
        instr->tail = new AS::InstrList(os_instr, next);

        last = instr->tail;
      }
      instr = next;
    }
  }

  f->s_offset += 8;
  *pil = *il;
}


void allocate_reg(AS::InstrList *instr, TEMP::Map *map)
{

  init_regmap();

  for (; instr; instr = instr->tail)
  {

    TEMP::TempList *def = nullptr, *use = nullptr;
    switch (instr->head->kind)
    {
    case AS::Instr::OPER:
      def = ((AS::OperInstr *)instr->head)->dst;
      use = ((AS::OperInstr *)instr->head)->src;
      break;
    case AS::Instr::MOVE:
      def = ((AS::MoveInstr *)instr->head)->dst;
      use = ((AS::MoveInstr *)instr->head)->src;
      break;
    default:
      def = nullptr;
      use = nullptr;
    }
    if (def == nullptr && use == nullptr)
    {
      continue;
    }
    if (instr->head->kind == AS::Instr::OPER)
    {
      std::string assem = ((AS::OperInstr *)instr->head)->assem;
      if (assem.size() >= 15 && assem.substr(0, 15) == "#use Spill load")
      {
        while (def)
        {
          if (!isreg(def->head))
          {
            //allocate register
            std::string *regname = find_reg(true);
            spillmap[def->head] = regname;
            assert(map->Look(def->head) == nullptr);
            map->Enter(def->head, regname);
            printf("%s Spill load allocated temp:%d\n", (*spillmap[def->head]).c_str(), def->head->Int());
          }
          def = def->tail;
        }

        continue;
      }
      if (assem.size() >= 16 && assem.substr(0, 16) == "#use Spill store")
      {
        while (use)
        {
          if (!isreg(use->head))
          {
            assert(spillmap.find(use->head) != spillmap.end());

            regmap[spillmap[use->head]] = 0;
            printf("%s free  temp:%d\n", (*spillmap[use->head]).c_str(), use->head->Int());
          }
          use = use->tail;
        }
        continue;
      }
      if (assem.size() >= 7 && assem.substr(0, 7) == "#free t")
      {
        assert(use->tail->head);
        regmap[spillmap[use->tail->head]] = 0;
        printf("%s #free t temp:%d\n", (*spillmap[use->tail->head]).c_str(), use->tail->head->Int());

        continue;
      }
    }

    while (def)
    {

      //debug

      if (!isreg(def->head))
      {
        //allocate register
        if (spillmap.find(def->head) != spillmap.end())
        {
        }
        else
        {
          //debug
          // if (instr->head->kind == AS::Instr::OPER)
          // {
          //   std::string assem = ((AS::OperInstr *)instr->head)->assem;
          //   printf("%s temp: %d\n ", assem.c_str(), def->head->Int());
          // }
          // if (instr->head->kind == AS::Instr::MOVE)
          // {
          //   std::string assem = ((AS::MoveInstr *)instr->head)->assem;
          //   printf("%s temp: %d\n ", assem.c_str(), def->head->Int());
          // }

          std::string *regname = find_reg(0);
          assert(map->Look(def->head) == nullptr);
          map->Enter(def->head, regname);
          // printf("%s allocated temp: %d  \n", (*regname).c_str(), def->head->Int());
        }
      }
      def = def->tail;
    }
  }
} // namespace RA
std::string *find_reg(bool set_one)
{
  for (std::map<std::string *, int>::iterator iter = regmap.begin(); iter != regmap.end(); iter++)
  {
    if (iter->second != 1)
    {
      if (set_one == true)
      {
        iter->second = 1;
      }
      return iter->first;
    }
  }
  assert(0);
}
void init_regmap()
{
  regmap[new std::string("%r10")] = 0;
  regmap[new std::string("%r11")] = 0;
  regmap[new std::string("%r12")] = 0;
  regmap[new std::string("%r13")] = 0;
  regmap[new std::string("%r14")] = 0;
  regmap[new std::string("%r15")] = 0;
  regmap[new std::string("%rbx")] = 0;
}
} // namespace RA
