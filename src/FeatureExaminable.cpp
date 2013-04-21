#include "FeatureExaminable.h"

#include "Engine.h"
#include "Log.h"
#include "Render.h"
#include "Item.h"
#include "ItemWeapon.h"
#include "ItemFactory.h"
#include "ItemDrop.h"
#include "Popup.h"
#include "PlayerBonuses.h"
#include "ActorPlayer.h"
#include "Inventory.h"
#include "ActorFactory.h"
#include "ActorMonster.h"
#include "Explosion.h"

//------------------------------------------------------------------ BASE CLASS
FeatureExaminable::FeatureExaminable(Feature_t id, coord pos, Engine* engine) :
  FeatureStatic(id, pos, engine) {}

void FeatureExaminable::examine() {
  tracer << "FeatureExaminable::examine()..." << endl;

  featureSpecific_examine();
  eng->gameTime->letNextAct();

  tracer << "FeatureExaminable::examine() [DONE]" << endl;
}

EventRegularity_t FeatureExaminable::getEventRegularity() {
  const int TOT = eventRegularity_common + eventRegularity_rare + eventRegularity_veryRare;
  const int RND = eng->dice.getInRange(1, TOT);
  if(RND <= eventRegularity_common) {
    return eventRegularity_common;
  } else if(RND <= eventRegularity_rare) {
    return eventRegularity_rare;
  } else {
    return eventRegularity_veryRare;
  }
}

//------------------------------------------------------------------ ITEM CONTAINER CLASS
ExaminableItemContainer::ExaminableItemContainer() {items_.resize(0);}

ExaminableItemContainer::~ExaminableItemContainer() {
  for(unsigned int i = 0; i < items_.size(); i++) {
    delete items_.at(i);
  }
}

void ExaminableItemContainer::setRandomItemsForFeature(const Feature_t featureId,
    const int NR_ITEMS_TO_ATTEMPT, Engine* const engine) {
  for(unsigned int i = 0; i < items_.size(); i++) {
    delete items_.at(i);
  }
  items_.resize(0);

  if(NR_ITEMS_TO_ATTEMPT > 0) {
    while(items_.empty()) {
      vector<ItemId_t> itemCandidates;
      for(unsigned int i = 1; i < endOfItemIds; i++) {
        ItemDefinition* const currentDef = engine->itemData->itemDefinitions[i];
        for(unsigned int ii = 0; ii < currentDef->featuresCanBeFoundIn.size(); ii++) {
          pair<Feature_t, int> featuresFoundIn = currentDef->featuresCanBeFoundIn.at(ii);
          if(featuresFoundIn.first == featureId) {
            if(engine->dice.percentile() < featuresFoundIn.second) {
              if(engine->dice.percentile() < currentDef->chanceToIncludeInSpawnList) {
                itemCandidates.push_back(static_cast<ItemId_t>(i));
                break;
              }
            }
          }
        }
      }

      const int NR_CANDIDATES = static_cast<int>(itemCandidates.size());
      if(NR_CANDIDATES > 0) {
        for(int i = 0; i < NR_ITEMS_TO_ATTEMPT; i++) {
          const unsigned int ELEMENT = engine->dice.getInRange(0, NR_CANDIDATES - 1);
          Item* item = engine->itemFactory->spawnItem(itemCandidates.at(ELEMENT));
          engine->itemFactory->setItemRandomizedProperties(item);
          items_.push_back(item);
        }
      }
    }
  }
}

void ExaminableItemContainer::dropItems(const coord& pos, Engine* const engine) {
  for(unsigned int i = 0; i < items_.size(); i++) {
    engine->itemDrop->dropItemOnMap(pos, &(items_.at(i)));
  }
  items_.resize(0);
}

void ExaminableItemContainer::destroySingleFragile(Engine* const engine) {
  //TODO Generalize this function (perhaps isFragile variable for item defs)

  for(unsigned int i = 0; i < items_.size(); i++) {
    Item* const item = items_.at(i);
    const ItemDefinition& d = item->getDef();
    if(d.isQuaffable || d.id == item_molotov) {
      delete item;
      items_.erase(items_.begin() + i);
      engine->log->addMessage("I hear a muffled shatter.");
    }
  }
}

//------------------------------------------------------------------ SPECIFIC FEATURES
//--------------------------------------------------------- TOMB
Tomb::Tomb(Feature_t id, coord pos, Engine* engine) :
  FeatureExaminable(id, pos, engine), isContentKnown_(false),
  isTraitKnown_(false), chanceToPushLid_(100), appearance_(tombAppearance_common),
  trait_(endOfTombTraits) {

  //Contained items
  PlayerBonusHandler* const bonHandler = eng->playerBonusHandler;
  const int NR_ITEMS_MIN = eng->dice.percentile() < 30 ? 0 : 1;
  const int NR_ITEMS_MAX =
    NR_ITEMS_MIN + (bonHandler->isBonusPicked(playerBonus_treasureHunter) ? 1 : 0);

  itemContainer_.setRandomItemsForFeature(
    feature_tomb, eng->dice.getInRange(NR_ITEMS_MIN, NR_ITEMS_MAX), eng);

  //Exterior appearance
  if(engine->dice.percentile() < 20) {
    tracer << "Tomb: Setting random appearance" << endl;
    appearance_ = static_cast<TombAppearance_t>(engine->dice.getInRange(0, endOfTombAppearance - 1));
  } else {
    tracer << "Tomb: Setting appearance according to items contained (common if zero items)" << endl;
    for(unsigned int i = 0; i < itemContainer_.items_.size(); i++) {
      const ItemValue_t itemValue = itemContainer_.items_.at(i)->getDef().itemValue;
      if(itemValue == itemValue_majorTreasure) {
        tracer << "Tomb: Contains major treasure" << endl;
        appearance_ = tombAppearance_marvelous;
        break;
      } else if(itemValue == itemValue_minorTreasure) {
        tracer << "Tomb: Contains minor treasure" << endl;
        appearance_ = tombAppearance_impressive;
      }
    }
  }

  const bool IS_CONTAINING_ITEMS = itemContainer_.items_.empty() == false;

  chanceToPushLid_ = IS_CONTAINING_ITEMS ? engine->dice.getInRange(0, 75) : 90;

  if(IS_CONTAINING_ITEMS) {
    const int RND = engine->dice.percentile();

    if(RND < 15) {
      trait_ = tombTrait_forebodingCarvedSigns;
    } else if(RND < 45) {
      trait_ = tombTrait_stench;
    } else if(RND < 75) {
      trait_ = tombTrait_auraOfUnrest;
    }
    tracer << "Tomb: Set trait (" << trait_ << ")" << endl;
  }
}

void Tomb::featureSpecific_examine() {
  if(itemContainer_.items_.empty() && isContentKnown_) {
    eng->log->addMessage("The tomb is empty.");
  } else {
    vector<TombAction_t> possibleActions;
    getPossibleActions(possibleActions);

    vector<string> actionLabels;
    getChoiceLabels(possibleActions, actionLabels);

    string descr = "";
    getDescr(descr);

    const int CHOICE_NR = eng->popup->showMultiChoiceMessage(
                            descr, true, actionLabels, "A tomb");
    doAction(possibleActions.at(static_cast<unsigned int>(CHOICE_NR)));
  }
}

void Tomb::doAction(const TombAction_t action) {
  StatusEffectsHandler* const statusHandler = eng->player->getStatusEffectsHandler();
  PlayerBonusHandler* const bonusHandler = eng->playerBonusHandler;

  const bool IS_TOUGH     = bonusHandler->isBonusPicked(playerBonus_tough);
  const bool IS_RUGGED    = bonusHandler->isBonusPicked(playerBonus_rugged);
  const bool IS_OBSERVANT = bonusHandler->isBonusPicked(playerBonus_observant);
  const bool IS_CONFUSED  = statusHandler->hasEffect(statusConfused);
  const bool IS_WEAK      = statusHandler->hasEffect(statusWeak);

  switch(action) {
    case tombAction_carveCurseWard: {
      const int CHANCE_OF_SUCCESS = 80;
      if(eng->dice.percentile() < CHANCE_OF_SUCCESS) {
        eng->log->addMessage("The curse is cleared.");
      } else {
        eng->log->addMessage("I make a misstake, the curse is doubled!");
        StatusCursed* const curse = new StatusCursed(eng);
        curse->turnsLeft *= 2;
        statusHandler->tryAddEffect(curse, true);
      }
      trait_ = endOfTombTraits;
    } break;

    case tombAction_leave: {
      eng->log->addMessage("I leave the tomb for now.");
    } break;

    case tombAction_pushLid: {
      eng->log->addMessage("I attempt to push the lid.");

      const int CHANCE_TO_SPRAIN    = 20;
      const int CHANCE_TO_PARALYZE  = 35;

      if(eng->dice.percentile() < CHANCE_TO_SPRAIN) {
        eng->log->addMessage("I sprain myself.", clrMessageBad);
        eng->player->hit(1, damageType_pure);
      }

      if(eng->player->deadState != actorDeadState_alive) {
        return;
      }

      if(eng->dice.percentile() < CHANCE_TO_PARALYZE) {
        eng->log->addMessage("I am off-balance.");
        statusHandler->tryAddEffect(new StatusParalyzed(2));
      }

      if(statusHandler->hasEffect(statusWeak)) {
        eng->log->addMessage("It seems futile.");
        return;
      }
      const int BON = IS_RUGGED ? 10 : (IS_TOUGH ? 5 : 0);
      if(eng->dice.percentile() < chanceToPushLid_ + BON) {
        eng->log->addMessage("The lid comes off!");
        openTomb();
      } else {
        eng->log->addMessage("The lid resists.");
      }
    } break;

    case tombAction_searchExterior: {
      const int CHANCE_TO_FIND = 40 + IS_OBSERVANT * 40 - IS_CONFUSED * 10;
      const bool IS_ROLL_SUCCESS = eng->dice.percentile() < CHANCE_TO_FIND;
      if(IS_ROLL_SUCCESS && trait_ != endOfTombTraits) {
        string traitDescr = "";
        getTraitDescr(traitDescr);
        eng->log->addMessage(traitDescr);
        isTraitKnown_ = true;
      } else {
        eng->log->addMessage("I find nothing significant.");
      }
    } break;

    case tombAction_smashLidWithSledgehammer: {
      eng->log->addMessage("I strike at the lid with a Sledgehammer.");
      const int CHANCE_TO_BREAK = IS_WEAK ? 10 : 90;
      if(eng->dice.percentile() < CHANCE_TO_BREAK) {
        eng->log->addMessage("The lid cracks open!");
        if(eng->dice.coinToss()) {
          itemContainer_.destroySingleFragile(eng);
        }
        openTomb();
      } else {
        eng->log->addMessage("The lid resists.");
      }
    } break;
  }
}

void Tomb::openTomb() {
  triggerTrap();
  if(itemContainer_.items_.size() > 0) {
    eng->log->addMessage("There are some items in the tomb.");
    itemContainer_.dropItems(pos_, eng);
  } else {
    eng->log->addMessage("The tomb is empty.");
  }
  eng->renderer->drawMapAndInterface(true);
  isContentKnown_ = isTraitKnown_ = true;
}

void Tomb::triggerTrap() {
  vector<ActorId_t> actorCandidates;

  switch(trait_) {
    case tombTrait_auraOfUnrest: {
      for(unsigned int i = 1; i < endOfActorIds; i++) {
        const ActorDefinition& d = eng->actorData->actorDefinitions[i];
        if(d.isGhost && d.isAutoSpawnAllowed && d.isUnique == false) {
          actorCandidates.push_back(static_cast<ActorId_t>(i));
        }
      }
      eng->log->addMessage("Something rises from the tomb!");
    } break;

    case tombTrait_forebodingCarvedSigns: {
      eng->player->getStatusEffectsHandler()->tryAddEffect(new StatusCursed(eng));
    } break;

    case tombTrait_stench: {
      if(eng->dice.coinToss()) {
        eng->log->addMessage("Fumes burst out from the tomb!");
        StatusEffect* effect = NULL;
        sf::Color fumeClr = clrMagenta;
        const int RND = eng->dice.percentile();
        if(RND < 20) {
          effect = new StatusPoisoned(eng);
          fumeClr = clrGreenLight;
        } else if(RND < 40) {
          effect = new StatusDiseased(eng);
        } else {
          effect = new StatusParalyzed(eng);
          effect->turnsLeft *= 2;
        }
        eng->explosionMaker->runExplosion(pos_, false, effect, true, fumeClr);
      } else {
        for(unsigned int i = 1; i < endOfActorIds; i++) {
          const ActorDefinition& d = eng->actorData->actorDefinitions[i];
          if(d.moveType == moveType_ooze && d.isAutoSpawnAllowed && d.isUnique == false) {
            actorCandidates.push_back(static_cast<ActorId_t>(i));
          }
        }
        eng->log->addMessage("Something creeps up from the tomb!");
      }

    } break;

    default: {} break;
  }

  if(actorCandidates.size() > 0) {
    const unsigned int ELEM = eng->dice.getInRange(0, actorCandidates.size() - 1);
    Actor* const actor = eng->actorFactory->spawnActor(actorCandidates.at(ELEM), pos_);
    dynamic_cast<Monster*>(actor)->becomeAware();
  }
}

void Tomb::getPossibleActions(vector<TombAction_t>& possibleActions) const {
  possibleActions.push_back(tombAction_pushLid);

  const bool IS_WARLOCK = eng->playerBonusHandler->isBonusPicked(playerBonus_warlock);

  if(isTraitKnown_) {
    if(IS_WARLOCK) {
      if(trait_ == tombTrait_forebodingCarvedSigns) {
        possibleActions.push_back(tombAction_carveCurseWard);
      }
    }
  } else {
    possibleActions.push_back(tombAction_searchExterior);
  }

  const Inventory* const inv = eng->player->getInventory();
  bool hasSledgehammer = false;
  Item* item = inv->getItemInSlot(slot_wielded);
  if(item != NULL) {
    hasSledgehammer = item->getDef().id == item_sledgeHammer;
  }
  if(hasSledgehammer == false) {
    item = inv->getItemInSlot(slot_wieldedAlt);
    hasSledgehammer = item->getDef().id == item_sledgeHammer;
  }
  if(hasSledgehammer == false) {
    hasSledgehammer = inv->hasItemInGeneral(item_sledgeHammer);
  }
  if(hasSledgehammer) {
    possibleActions.push_back(tombAction_smashLidWithSledgehammer);
  }

  possibleActions.push_back(tombAction_leave);
}

void Tomb::getChoiceLabels(const vector<TombAction_t>& possibleActions,
                           vector<string>& actionLabels) const {
  actionLabels.resize(0);

  for(unsigned int i = 0; i < possibleActions.size(); i++) {
    const TombAction_t action = possibleActions.at(i);
    switch(action) {
      case tombAction_carveCurseWard: {
        actionLabels.push_back("Carve a curse ward");
      } break;
      case tombAction_leave: {
        actionLabels.push_back("Leave it");
      } break;
      case tombAction_pushLid: {
        actionLabels.push_back("Try pushing the lid");
      } break;
      case tombAction_searchExterior: {
        actionLabels.push_back("Search the exterior");
      } break;
      case tombAction_smashLidWithSledgehammer: {
        actionLabels.push_back("Smash the lid with a sledgehammer");
      } break;
    }
  }
}

void Tomb::getTraitDescr(string& descr) const {
  const bool IS_WARLOCK   = eng->playerBonusHandler->isBonusPicked(playerBonus_warlock);

  switch(trait_) {
    case tombTrait_auraOfUnrest: {
      descr = "It has a certain aura of unrest about it.";
    } break;

    case tombTrait_forebodingCarvedSigns: {
      if(IS_WARLOCK) {
        descr = "There is a curse carved on the box.";
      } else {
        descr = "There are some ominous runes carved on the box.";
      }
    } break;

    case tombTrait_stench: {
      descr = "There is a pungent stench.";
    } break;

    default: {} break;
  }
}

void Tomb::getDescr(string& descr) const {
  switch(appearance_) {
    case tombAppearance_common:     {descr = "It looks ordinary.";} break;
    case tombAppearance_impressive: {descr = "It looks impressive.";} break;
    case tombAppearance_marvelous:  {descr = "It looks marvelous!";} break;
    default: {} break;
  }

  if(isTraitKnown_) {
    string traitDescr = "";
    getTraitDescr(traitDescr);
    descr += " " + traitDescr;
  }

  const bool IS_WEAK = eng->player->getStatusEffectsHandler()->hasEffect(statusWeak);

  if(chanceToPushLid_ < 10 || IS_WEAK) {
    descr += " The lid seems very heavy.";
  } else if(chanceToPushLid_ < 50) {
    descr += " The lid does not seem too heavy.";
  } else {
    descr += " I think I could move the lid with small effort.";
  }
}

//--------------------------------------------------------- CHEST
Chest::Chest(Feature_t id, coord pos, Engine* engine) :
  FeatureExaminable(id, pos, engine), isContentKnown_(false),
  isLocked_(false), isTrapped_(false), isTrapStatusKnown_(false) {

  PlayerBonusHandler* const bonHandler = eng->playerBonusHandler;
  const int CHANCE_FOR_EMPTY = 10;
  const int NR_ITEMS_MIN = eng->dice.percentile() < CHANCE_FOR_EMPTY ? 0 : 1;
  const int NR_ITEMS_MAX = bonHandler->isBonusPicked(playerBonus_treasureHunter) ? 4 : 3;
  itemContainer_.setRandomItemsForFeature(
    feature_chest, eng->dice.getInRange(NR_ITEMS_MIN, NR_ITEMS_MAX), eng);

  if(itemContainer_.items_.empty() == false) {
    const int CHANCE_FOR_LOCKED = 80;
    isLocked_ = eng->dice.percentile() < CHANCE_FOR_LOCKED;

    const int CHANCE_FOR_TRAPPED = 33;
    isTrapped_ = eng->dice.percentile() < CHANCE_FOR_TRAPPED ? true : false;
  }
}

//sf::Color Chest::getColor() const {
//  return material_ == doorMaterial_wood ? clrBrownDark : clrGray;
//}

//string Chest::getDescr(const bool DEFINITE_ARTICLE) const {
//}

void Chest::featureSpecific_examine() {
  if(itemContainer_.items_.empty() && isContentKnown_) {
    eng->log->addMessage("There is nothing of value in the chest.");
  } else {
    vector<ChestAction_t> possibleActions;
    getPossibleActions(possibleActions);

    vector<string> actionLabels;
    getChoiceLabels(possibleActions, actionLabels);

    string descr = "";
    getDescr(descr);

    const int CHOICE_NR = eng->popup->showMultiChoiceMessage(
                            descr, true, actionLabels, "A chest");

    doAction(possibleActions.at(static_cast<unsigned int>(CHOICE_NR)));
  }
}

void Chest::doAction(const ChestAction_t action) {
  const bool IS_NIMBLE = eng->playerBonusHandler->isBonusPicked(playerBonus_nimbleHanded);
  const bool IS_OBSERVANT = eng->playerBonusHandler->isBonusPicked(playerBonus_observant);
  const bool IS_CONFUSED = eng->player->getStatusEffectsHandler()->hasEffect(statusConfused);

  switch(action) {
    case chestAction_open: {
      if(isTrapped_) {
        triggerTrap();
      } else {
        if(itemContainer_.items_.empty()) {
          eng->log->addMessage("There is nothing of value in the chest.");
        } else {
          eng->log->addMessage("There are some items in the chest.");
          itemContainer_.dropItems(pos_, eng);
          eng->renderer->drawMapAndInterface(true);
        }
        isContentKnown_ = true;
        isTrapStatusKnown_ = true;
      }
    }
    break;
    case chestAction_searchForTrap: {
      const int CHANCE_TO_FIND = 20 + IS_OBSERVANT * 40 - IS_CONFUSED * 10;
      const bool IS_ROLL_SUCCESS = eng->dice.percentile() < CHANCE_TO_FIND;
      if(IS_ROLL_SUCCESS && isTrapped_) {
        eng->log->addMessage("The chest has a hidden trap mechanism!");
        isTrapStatusKnown_ = true;
      } else {
        eng->log->addMessage("I find no indication that the chest is trapped.");
      }
    }
    break;
    case chestAction_disarmTrap: {

    }
    break;
    case chestAction_forceLock: {

    }
    break;
    case chestAction_kick: {

    }
    break;
    case chestAction_leave: {
      eng->log->addMessage("I leave the chest for now.");
    }
    break;
  }
}

void Chest::getPossibleActions(vector<ChestAction_t>& possibleActions) const {
  if(isTrapStatusKnown_ == false) {
    possibleActions.push_back(chestAction_searchForTrap);
  } else if(isTrapped_) {
    possibleActions.push_back(chestAction_disarmTrap);
  }

  if(isLocked_) {
    possibleActions.push_back(chestAction_kick);

    //TODO check wielded weapon
    possibleActions.push_back(chestAction_forceLock);

  } else {
    possibleActions.push_back(chestAction_open);
  }

  possibleActions.push_back(chestAction_leave);
}

void Chest::getChoiceLabels(const vector<ChestAction_t>& possibleActions,
                            vector<string>& actionLabels) const {
  actionLabels.resize(0);

  for(unsigned int i = 0; i < possibleActions.size(); i++) {
    ChestAction_t action = possibleActions.at(i);
    switch(action) {
      case chestAction_open:          {actionLabels.push_back("Open it");} break;
      case chestAction_searchForTrap: {actionLabels.push_back("Search it for traps");} break;
      case chestAction_disarmTrap:    {actionLabels.push_back("Disarm the trap");} break;
      case chestAction_forceLock:     {actionLabels.push_back("Force the lock");} break;
      case chestAction_kick:          {actionLabels.push_back("Kick the lid");} break;
      case chestAction_leave:         {actionLabels.push_back("Leave it");} break;
    }
  }
}

void Chest::getDescr(string& descr) const {
  const string lockDescr = isLocked_ ? "locked" : "not locked";
  const string trapDescr = isTrapped_ && isTrapStatusKnown_ ?
                           " There appears to be a trap mechanism." : "";
  descr = "It is " + lockDescr + "." + trapDescr;
}

void Chest::triggerTrap() {
  isTrapped_ = false;
  isTrapStatusKnown_ = true;
}

//--------------------------------------------------------- CABINET
Cabinet::Cabinet(Feature_t id, coord pos, Engine* engine) :
  FeatureExaminable(id, pos, engine) {

}

void Cabinet::featureSpecific_examine() {
}

//--------------------------------------------------------- COCOON
Cocoon::Cocoon(Feature_t id, coord pos, Engine* engine) :
  FeatureExaminable(id, pos, engine) {}

void Cocoon::featureSpecific_examine() {
}

//--------------------------------------------------------- ALTAR
Altar::Altar(Feature_t id, coord pos, Engine* engine) :
  FeatureExaminable(id, pos, engine) {}

void Altar::featureSpecific_examine() {
}

//--------------------------------------------------------- CARVED PILLAR
//CarvedPillar::CarvedPillar(Feature_t id, coord pos, Engine* engine) :
//  FeatureExaminable(id, pos, engine) {}
//
//void CarvedPillar::featureSpecific_examine() {
//}

//--------------------------------------------------------- BARREL
//Barrel::Barrel(Feature_t id, coord pos, Engine* engine) :
//  FeatureExaminable(id, pos, engine) {}
//
//void Barrel::featureSpecific_examine() {
//}