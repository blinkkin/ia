#include "ItemPotion.h"

#include "Engine.h"
#include "Properties.h"
#include "ActorPlayer.h"
#include "Log.h"
#include "Map.h"
#include "ActorMonster.h"
#include "PlayerSpellsHandler.h"
#include "ItemScroll.h"

void PotionOfHealing::specificQuaff(Actor* const actor,
                                    Engine* const engine) {
  //End disease
  bool visionBlockers[MAP_X_CELLS][MAP_Y_CELLS];
  engine->mapTests->makeVisionBlockerArray(
    engine->player->pos, visionBlockers);
  actor->getPropHandler()->endAppliedProp(propDiseased, visionBlockers);

  //Attempt to heal the actor. If no hp was healed (already at full hp),
  //boost the hp instead.
  if(actor->restoreHp(engine->dice(2, 6) + 12, true) == false) {
    actor->changeMaxHp(1, true);
  }

  if(engine->player->checkIfSeeActor(*actor, NULL)) {
    identify(false, engine);
  }
}

void PotionOfHealing::specificCollide(const Pos& pos, Actor* const actor,
                                      Engine* const engine) {
  (void)pos;
  if(actor != NULL) {
    specificQuaff(actor, engine);
  }
}

void PotionOfSpirit::specificQuaff(Actor* const actor,
                                   Engine* const engine) {
  //Attempt to restore spirit. If no hp was healed (already at full hp),
  //boost the hp instead.
  if(actor->restoreSpi(engine->dice(2, 6) + 12, true) == false) {
    actor->changeMaxSpi(1, true);
  }

  if(engine->player->checkIfSeeActor(*actor, NULL)) {
    identify(false, engine);
  }
}

void PotionOfSpirit::specificCollide(const Pos& pos, Actor* const actor,
                                     Engine* const engine) {
  (void)pos;
  if(actor != NULL) {
    specificQuaff(actor, engine);
  }
}

//void PotionOfSorcery::specificQuaff(Actor* const actor, Engine* const engine) {
//  (void)actor;
//  bool isAnySpellRestored = false;
//
//  const unsigned int NR_OF_SCROLLS = engine->playerPowersHandler->getNrOfSpells();
//  for(unsigned int i = 0; i < NR_OF_SCROLLS; i++) {
//    Scroll* const scroll =  engine->playerPowersHandler->getScrollAt(i);
//    const ItemDef& d = scroll->getData();
//    if(
//      d.isScrollLearnable &&
//      d.isScrollLearned   &&
//      d.id != item_thaumaturgicAlteration) {
//      if(d.castFromMemoryCurrentBaseChance < CAST_FROM_MEMORY_CHANCE_LIM) {
//        scroll->setCastFromMemoryCurrentBaseChance(CAST_FROM_MEMORY_CHANCE_LIM);
//        isAnySpellRestored = true;
//      }
//    }
//  }
//
//  if(isAnySpellRestored) {
//    engine->log->addMessage("My magic is restored!");
//    identify(false, engine);
//  }
//}

void PotionOfBlindness::specificQuaff(Actor* const actor,
                                      Engine* const engine) {
  actor->getPropHandler()->tryApplyProp(
    new PropBlind(engine, propTurnsStandard));
  if(engine->player->checkIfSeeActor(*actor, NULL)) {
    identify(false, engine);
  }
}

void PotionOfBlindness::specificCollide(const Pos& pos, Actor* const actor,
                                        Engine* const engine) {
  (void)pos;
  if(actor != NULL) {specificQuaff(actor, engine);}
}

void PotionOfParalyzation::specificQuaff(Actor* const actor,
    Engine* const engine) {
  actor->getPropHandler()->tryApplyProp(
    new PropParalyzed(engine, propTurnsStandard));
  if(engine->player->checkIfSeeActor(*actor, NULL)) {
    identify(false, engine);
  }
}

void PotionOfParalyzation::specificCollide(const Pos& pos, Actor* const actor,
    Engine* const engine) {
  (void)pos;
  if(actor != NULL) {
    specificQuaff(actor, engine);
  }
}

void PotionOfDisease::specificQuaff(Actor* const actor,
                                    Engine* const engine) {
  actor->getPropHandler()->tryApplyProp(
    new PropDiseased(engine, propTurnsStandard));
  if(engine->player->checkIfSeeActor(*actor, NULL)) {
    identify(false, engine);
  }
}

void PotionOfConfusion::specificQuaff(Actor* const actor,
                                      Engine* const engine) {
  actor->getPropHandler()->tryApplyProp(
    new PropConfused(engine, propTurnsStandard));
  if(engine->player->checkIfSeeActor(*actor, NULL)) {
    identify(false, engine);
  }
}

void PotionOfConfusion::specificCollide(const Pos& pos, Actor* const actor,
                                        Engine* const engine) {
  (void)pos;
  if(actor != NULL) {
    specificQuaff(actor, engine);
  }
}

//void PotionOfCorruption::specificQuaff(Actor* const actor, Engine* const engine) {
//  const int CHANGE = -(engine->dice(1, 2));
//
//  actor->changeMaxHP(CHANGE, true);
//
//  if(engine->player->checkIfSeeActor(*actor, NULL)) {
//    identify(false, engine);
//  }
//}
//
//void PotionOfCorruption::specificCollide(const Pos& pos, Actor* const actor, Engine* const engine) {
//  if(actor != NULL) {
//    specificQuaff(actor, engine);
//  } else {
//    engine->map->switchToDestroyedFeatAt(pos);
//
//    if(engine->map->playerVision[pos.x][pos.y]) {
//      identify(false, engine);
//    }
//  }
//}

//void PotionOfTheCobra::specificQuaff(Actor* const actor, Engine* const engine) {
//  actor->getPropHandler()->tryApplyProp(
//    new StatusPerfectAim(engine, propTurnsStandard));
//  actor->getPropHandler()->tryApplyProp(
//    new StatusPerfectReflexes(engine, propTurnsStandard));
//  if(engine->player->checkIfSeeActor(*actor, NULL)) {
//    identify(false, engine);
//  }
//}

//void PotionOfTheCobra::specificCollide(const Pos& pos, Actor* const actor, Engine* const engine) {
//  (void)pos;
//  if(actor != NULL) {
//    specificQuaff(actor, engine);
//  }
//}

//void PotionOfFortitude::specificQuaff(Actor* const actor, Engine* const engine) {
//  actor->getPropHandler()->tryApplyProp(new StatusPerfectFortitude(engine));
//
//  bool visionBlockers[MAP_X_CELLS][MAP_Y_CELLS];
//  engine->mapTests->makeVisionBlockerArray(engine->player->pos, visionBlockers);
//  actor->getPropHandler()->endEffectsOfAbility(ability_resistStatusMind, visionBlockers);
//
//  bool isPhobiasCured = false;
//  for(unsigned int i = 0; i < endOfInsanityPhobias; i++) {
//    if(engine->player->insanityPhobias[i]) {
//      engine->player->insanityPhobias[i] = false;
//      isPhobiasCured = true;
//    }
//  }
//  if(isPhobiasCured) {
//    engine->log->addMessage("All my phobias are cured!");
//  }
//
//  bool isObsessionsCured = false;
//  for(unsigned int i = 0; i < endOfInsanityObsessions; i++) {
//    if(engine->player->insanityObsessions[i]) {
//      engine->player->insanityObsessions[i] = false;
//      isObsessionsCured = true;
//    }
//  }
//  if(isObsessionsCured) {
//    engine->log->addMessage("All my obsessions are cured!");
//  }
//
//  engine->player->restoreShock(999, false);
//  engine->log->addMessage("I feel more at ease.");
//
//  identify(false, engine);
//}
//
//void PotionOfFortitude::specificCollide(const Pos& pos, Actor* const actor, Engine* const engine) {
//  (void)pos;
//  if(actor != NULL) {
//    specificQuaff(actor, engine);
//  }
//}

//void PotionOfToughness::specificQuaff(Actor* const actor, Engine* const engine) {
//  actor->getPropHandler()->tryApplyProp(new StatusPerfectToughness(engine));
//
//  bool visionBlockers[MAP_X_CELLS][MAP_Y_CELLS];
//  engine->mapTests->makeVisionBlockerArray(engine->player->pos, visionBlockers);
//  actor->getPropHandler()->endEffectsOfAbility(ability_resistStatusBody, visionBlockers);
//
//  if(engine->player->checkIfSeeActor(*actor, NULL)) {
//    identify(false, engine);
//  }
//}
//
//void PotionOfToughness::specificCollide(const Pos& pos, Actor* const actor, Engine* const engine) {
//  (void)pos;
//  if(actor != NULL) {
//    specificQuaff(actor, engine);
//  }
//}

void PotionOfPoison::specificQuaff(Actor* const actor, Engine* const engine) {
  actor->getPropHandler()->tryApplyProp(
    new PropPoisoned(engine, propTurnsStandard));

  if(engine->player->checkIfSeeActor(*actor, NULL)) {
    identify(false, engine);
  }
}

void PotionOfPoison::specificCollide(const Pos& pos, Actor* const actor,
                                     Engine* const engine) {
  (void)pos;
  if(actor != NULL) {
    specificQuaff(actor, engine);
  }
}

void PotionOfKnowledge::specificQuaff(Actor* const actor, Engine* const engine) {
  (void)actor;
  engine->log->addMessage("I feel more insightful about the mystic powers!");
  engine->player->incrMth(4);
  identify(false, engine);
}

void PotionNameHandler::setColorAndFalseName(ItemData* d) {
  const unsigned int NR_NAMES = m_falseNames.size();

  const unsigned int ELEMENT = (unsigned int)(eng->dice(1, NR_NAMES) - 1);

  const string DESCRIPTION = m_falseNames.at(ELEMENT).str;
  const SDL_Color clr = m_falseNames.at(ELEMENT).clr;

  m_falseNames.erase(m_falseNames.begin() + ELEMENT);

  d->name.name = DESCRIPTION + " potion";
  d->name.name_plural = DESCRIPTION + " potions";
  d->name.name_a = "a " + DESCRIPTION + " potion";
  d->color = clr;
}

void PotionNameHandler::addSaveLines(vector<string>& lines) const {
  for(unsigned int i = 1; i < endOfItemIds; i++) {
    ItemData* const d = eng->itemDataHandler->dataList[i];
    if(d->isQuaffable) {
      lines.push_back(d->name.name);
      lines.push_back(d->name.name_plural);
      lines.push_back(d->name.name_a);
      lines.push_back(intToString(d->color.r));
      lines.push_back(intToString(d->color.g));
      lines.push_back(intToString(d->color.b));
    }
  }
}

void PotionNameHandler::setParametersFromSaveLines(vector<string>& lines) {
  for(unsigned int i = 1; i < endOfItemIds; i++) {
    ItemData* const d = eng->itemDataHandler->dataList[i];
    if(d->isQuaffable) {
      d->name.name = lines.front();
      lines.erase(lines.begin());
      d->name.name_plural = lines.front();
      lines.erase(lines.begin());
      d->name.name_a = lines.front();
      lines.erase(lines.begin());
      d->color.r = stringToInt(lines.front());
      lines.erase(lines.begin());
      d->color.g = stringToInt(lines.front());
      lines.erase(lines.begin());
      d->color.b = stringToInt(lines.front());
      lines.erase(lines.begin());
    }
  }
}

void Potion::identify(const bool IS_SILENT_IDENTIFY,
                      Engine* const engine) {
  if(data_->isIdentified == false) {
    const string REAL_TYPE_NAME = getRealTypeName();

    const string REAL_NAME = "Potion of " + REAL_TYPE_NAME;
    const string REAL_NAME_PLURAL = "Potions of " + REAL_TYPE_NAME;
    const string REAL_NAME_A = "a potion of " + REAL_TYPE_NAME;

    data_->name.name = REAL_NAME;
    data_->name.name_plural = REAL_NAME_PLURAL;
    data_->name.name_a = REAL_NAME_A;

    if(IS_SILENT_IDENTIFY == false) {
      engine->log->addMessage("It was a " + REAL_NAME + ".");
      engine->player->incrShock(shockValue_heavy);
    }

    data_->isIdentified = true;
  }
}

void Potion::collide(const Pos& pos, Actor* const actor,
                     Engine* const engine) {
  if(engine->map->featuresStatic[pos.x][pos.y]->isBottomless() == false ||
      actor != NULL) {
//    ItemData* const potData =
//      engine->itemDataHandler->dataList[d.id];

    const bool PLAYER_SEE_CELL = engine->map->playerVision[pos.x][pos.y];

    if(PLAYER_SEE_CELL) {
      // TODO Use standard animation
      engine->renderer->drawGlyph('*', panel_map, pos, data_->color);

      if(actor != NULL) {
        if(actor->deadState == actorDeadState_alive) {
          engine->log->addMessage(
            "The potion shatters on " +
            actor->getNameThe() + ".");
        }
      } else {
        Feature* const f = engine->map->featuresStatic[pos.x][pos.y];
        engine->log->addMessage(
          "The potion shatters on " + f->getDescription(true) + ".");
      }
    }
    //If the blow from the bottle didn't kill the actor, apply what's inside
    if(actor != NULL) {
      if(actor->deadState == actorDeadState_alive) {
        specificCollide(pos, actor, engine);
        if(
          actor->deadState == actorDeadState_alive &&
          data_->isIdentified == false && PLAYER_SEE_CELL) {
          engine->log->addMessage("It had no apparent effect...");
        }
      }
    }
  }
}

void Potion::quaff(Actor* const actor, Engine* const engine) {
  if(actor == engine->player) {
    data_->isTried = true;

    engine->player->incrShock(shockValue_heavy);

    if(data_->isIdentified) {
      engine->log->addMessage("I drink " + data_->name.name_a + "...");
    } else {
      engine->log->addMessage("I drink an unknown " + data_->name.name + "...");
    }
  }

  specificQuaff(actor, engine);

  if(engine->player->deadState == actorDeadState_alive) {
    engine->gameTime->endTurnOfCurrentActor();
  }
}

void Potion::failedToLearnRealName(Engine* const engine,
                                   const string overrideFailString) {
  if(data_->isIdentified == false) {
    if(overrideFailString != "") {
      engine->log->addMessage(overrideFailString);
    } else {
      engine->log->addMessage("It doesn't seem to affect me.");
    }
  }
}
