#include "DebugModeStatPrinter.h"

#include <vector>
#include <algorithm>

#include "Engine.h"
#include "ActorData.h"
#include "ActorFactory.h"
#include "Inventory.h"
#include "Item.h"
#include "PlayerSpellsHandler.h"
#include "ItemScroll.h"
#include "ItemData.h"
#include "Spells.h"
#include "ActorPlayer.h"
#include "DungeonMaster.h"

struct IsHigherSpawnMinLvl {
public:
  IsHigherSpawnMinLvl() {}
  bool operator()(const ActorData* const d1, const ActorData* const d2) {
    return d1->spawnMinDLVL < d2->spawnMinDLVL;
  }
};

void DebugModeStatPrinter::run() {
  const string separator = "--------------------------------------------------";
  const string indent1 = " ";
  const string indent2 = indent1 + "      ";

  statFile.open("debug_mode_stats_file.txt", ios::trunc);
  printLine("This file was created because Infra Arcana was run in Debug mode\n");
  printLine("Created on   : " + Utils::getCurrentTime().getTimeStr(time_minute, true));
  printLine("Game version : " + gameVersionStr);
  printLine("\n");

  printLine("SPELL MAX SPI COSTS");
  printLine(separator);
  for(int i = 0; i < endOfSpellId; i++) {
    Spell* const spell = eng.spellHandler->getSpellFromId(SpellId(i));
    string name = spell->getName();
    name.insert(name.end(), 24 - name.size(), ' ');
    Range cost = spell->getSpiCost(true, eng.player, eng);
    const string costStr =
      toString(cost.lower) + "-" + toString(cost.upper);
    delete spell;
    printLine(indent1 + name + costStr);
  }
  printLine("\n");

  vector<ActorData*> actorDataSorted;
  for(unsigned int i = actor_player + 1; i < endOfActorIds; i++) {
    actorDataSorted.push_back(&(eng.actorDataHandler->dataList[i]));
  }
  IsHigherSpawnMinLvl isHigherSpawnMinLvl;
  std::sort(actorDataSorted.begin(), actorDataSorted.end(), isHigherSpawnMinLvl);

  printLine("MONSTERS PER MIN DLVL");
  printLine(separator);
  printLine(indent1 + "LVL   NR");
  printLine(indent1 + "---------");

  vector<int> monstersPerMinDLVL(actorDataSorted.back()->spawnMinDLVL + 1, 0);
  for(unsigned int i = 0; i < actorDataSorted.size(); i++) {
    monstersPerMinDLVL.at(actorDataSorted.at(i)->spawnMinDLVL)++;
  }
  for(unsigned int i = 0; i < monstersPerMinDLVL.size(); i++) {
    const int LVL = i;
    string lvlStr = toString(LVL);
    lvlStr.insert(lvlStr.end(), 6 - lvlStr.size(), ' ');
    string nrStr = toString(monstersPerMinDLVL.at(i));
    nrStr.insert(nrStr.end(), 3, ' ');
    if(monstersPerMinDLVL.at(i) > 0) {
      nrStr.insert(nrStr.end(), monstersPerMinDLVL.at(i), '*');
    }
    printLine(indent1 + lvlStr + nrStr);
  }
  printLine("\n" + indent1 + "Total number of monsters: " +
            toString(actorDataSorted.size()));
  printLine("\n");

  printLine("STATS FOR EACH MONSTER");
  printLine(separator);
  printLine(indent1 + "Notes:");
  printLine(indent1 + "(U) = Unique monster");
  printLine(indent1 + "(M) = Melee weapon");
  printLine(indent1 + "(R) = Ranged weapon");
  printLine("");

  for(unsigned int i = 0; i < actorDataSorted.size(); i++) {
    ActorData& d = *(actorDataSorted.at(i));

    Actor* const actor = eng.actorFactory->makeActorFromId(d.id);
    actor->place(Pos(-1, -1), d);

    const string uniqueStr = d.isUnique ? " (U)" : "";
    printLine(indent1 + actor->getNameA() + uniqueStr);

    const string xpStr =
      "XP:" + toString(eng.dungeonMaster->getMonsterTotXpWorth(d));
    printLine(indent2 + xpStr);

    string hpStr = "HP:" + toString(d.hp);
    hpStr.insert(hpStr.end(), 8 - hpStr.size(), ' ');

    const int attackSkill =
      d.abilityVals.getVal(ability_accuracyMelee, false, *actor);
    const string attackSkillStr =
      "Attack skill:" + toString(attackSkill) + "%";
    printLine(indent2 + hpStr + attackSkillStr);

    const Inventory& inv = actor->getInv();
    const unsigned int NR_INTRINSIC_ATTACKS = inv.getIntrinsicsSize();
    for(unsigned int i_intr = 0; i_intr < NR_INTRINSIC_ATTACKS; i_intr++) {
      const Item* const item = inv.getIntrinsicInElement(i_intr);
      const ItemData& itemData = item->getData();
      const string meleeOrRangedStr = itemData.isRangedWeapon ? "(R)" : "(M)";
      const string attackNrStr = "Attack " + toString(i_intr + 1);
      const string dmgStr =
        toString(itemData.meleeDmg.first) + "d" +
        toString(itemData.meleeDmg.second);
      printLine(
        indent2 + attackNrStr + " " + meleeOrRangedStr + ": " + dmgStr);
    }
    delete actor;

    printLine("");
  }



  statFile.close();
}


void DebugModeStatPrinter::printLine(const string& line) {
  statFile << line << endl;
}
