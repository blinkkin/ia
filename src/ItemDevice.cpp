#include "ItemDevice.h"

#include "Engine.h"
#include "ActorPlayer.h"
#include "Renderer.h"
#include "GameTime.h"
#include "Log.h"
#include "Knockback.h"
#include "Inventory.h"
#include "Map.h"
#include "Audio.h"
#include "Utils.h"

//---------------------------------------------------- BASE CLASS
Device::Device(ItemData* const itemData, Engine& engine) :
  Item(itemData, engine), isActivated_(false), nrTurnsToNextGoodEffect_(-1),
  nrTurnsToNextBadEffect_(-1) {}

bool Device::activateDefault(Actor* const actor) {
  (void)actor;

  if(data_->isIdentified) {
    bool isDestroyed = toggle();
    eng.gameTime->actorDidAct();
    return isDestroyed;
  } else {
    eng.log->addMsg("I cannot yet use this.");
    return false;
  }
}

bool Device::toggle() {
  printToggleMessage();

  if(isActivated_) {
    isActivated_ = false;
    nrTurnsToNextGoodEffect_ = nrTurnsToNextBadEffect_ = -1;
    toggle_();
  } else {
    isActivated_ = true;
    nrTurnsToNextGoodEffect_ = getRandomNrTurnsToNextGoodEffect();
    nrTurnsToNextBadEffect_ = getRandomNrTurnsToNextBadEffect();
    toggle_();
    const string message = getSpecificActivateMessage();
    if(message.empty() == false) {
      eng.log->addMsg(message);
    }
  }
  return false;
}

void Device::printToggleMessage() {
  const string name_a =
    eng.itemDataHandler->getItemRef(*this, itemRef_a, true);
  eng.log->addMsg(
    (isActivated_ ? "I deactivate " : "I activate ") + name_a + ".");
}

int Device::getRandomNrTurnsToNextGoodEffect() const {
  return Rnd::range(6, 9);
}

int Device::getRandomNrTurnsToNextBadEffect() const {
  return Rnd::range(12, 16);
}

void Device::newTurnInInventory() {
  if(isActivated_) {

    newTurnInInventory_();

    if(--nrTurnsToNextGoodEffect_ <= 0) {
      nrTurnsToNextGoodEffect_ = getRandomNrTurnsToNextGoodEffect();
      runGoodEffect();
    }
    if(--nrTurnsToNextBadEffect_ <= 0) {
      nrTurnsToNextBadEffect_ = getRandomNrTurnsToNextBadEffect();
      runBadEffect();
    }
  }
}

void Device::runBadEffect() {
  const string name =
    eng.itemDataHandler->getItemRef(*this, itemRef_plain, true);

  const int RND = Rnd::percentile();
  if(RND < 2) {
    eng.log->addMsg("The " + name + " breaks!");
    eng.player->getInv().removetemInGeneralWithPointer(this, false);
  } else if(RND < 40) {
    eng.log->addMsg(
      "I am hit with a jolt of electricity from the " + name +
      ".", clrMsgBad, true);
    eng.player->getPropHandler().tryApplyProp(
      new PropParalyzed(eng, propTurnsSpecific, 2));
    eng.player->hit(Rnd::range(1, 2), dmgType_electric, false);
  } else {
    eng.log->addMsg("The " + name + " hums ominously.");
  }
}

void Device::addSaveLines_(vector<string>& lines) {
  lines.push_back(isActivated_ ? "1" : "0");
  deviceSpecificAddSaveLines(lines);
}

void Device::setParamsFromSaveLines_(vector<string>& lines) {
  isActivated_ = lines.back() == "1";
  lines.erase(lines.begin());
  deviceSpecificSetParamsFromSaveLines(lines);
}

void Device::identify(const bool IS_SILENT_IDENTIFY) {
  (void)eng;
  (void)IS_SILENT_IDENTIFY;

  data_->isIdentified = true;
}

//---------------------------------------------------- SENTRY
string DeviceSentry::getSpecificActivateMessage() {
  return "It seems to peruse area.";
}

void DeviceSentry::runGoodEffect() {
  const int DMG = Rnd::dice(1, 6) + 2;

  vector<Actor*> targetCandidates;
  eng.player->getSpottedEnemies(targetCandidates);
  const unsigned int NR_CANDIDATES = targetCandidates.size();
  if(NR_CANDIDATES > 0) {
    const int ELEMENT = Rnd::range(0, NR_CANDIDATES - 1);
    Actor* const actor = targetCandidates.at(ELEMENT);
    const Pos& pos = actor->pos;
    eng.log->addMsg(actor->getNameThe() + " is hit by a bolt of lightning!",
                    clrMsgGood, true);
    Renderer::drawBlastAnimAtPositionsWithPlayerVision(
      vector<Pos>(1, pos), clrYellow);
    actor->hit(DMG, dmgType_electric, false);
  }
}

//---------------------------------------------------- REPELLER
string DeviceRepeller::getSpecificActivateMessage() {
  return "I feel a certain tension in the air around me.";
}

void DeviceRepeller::runGoodEffect() {
  const Pos& playerPos = eng.player->pos;
  for(Actor* actor : eng.gameTime->actors_) {
    if(actor != eng.player) {
      const Pos& otherPos = actor->pos;
      if(Utils::isPosAdj(playerPos, otherPos, false)) {
        eng.knockBack->tryKnockBack(*actor, playerPos, false, true);
      }
    }
  }
}

int DeviceRepeller::getRandomNrTurnsToNextGoodEffect() const {
  return Rnd::range(2, 4);
}

//---------------------------------------------------- REJUVENATOR
string DeviceRejuvenator::getSpecificActivateMessage() {
  return "It seems to attempt repairing my flesh.";
}

void DeviceRejuvenator::runGoodEffect() {
//  const string name = eng.itemData->getItemRef(this, itemRef_plain, true);
//  eng.log->addMsg(name + " repairs my wounds.");
  eng.player->restoreHp(1, false);
}

//---------------------------------------------------- TRANSLOCATOR
string DeviceTranslocator::getSpecificActivateMessage() {
  return "";
}

void DeviceTranslocator::runGoodEffect() {
  Player* const player = eng.player;
  vector<Actor*> SpottedEnemies;
  player->getSpottedEnemies(SpottedEnemies);
  if(
    player->getHp() <= player->getHpMax(true) / 4 &&
    SpottedEnemies.empty() == false) {
    const string name = eng.itemDataHandler->getItemRef(
                          *this, itemRef_plain, true);
    eng.log->addMsg("The " + name + " makes a droning noise...");
    player->teleport(true);
  }
}

//---------------------------------------------------- ELECTRIC LANTERN
void DeviceElectricLantern::newTurnInInventory_() {
  if(isActivated_ && malfunctCooldown_ > 0) {
    malfunctCooldown_--;
    if(malfunctCooldown_ <= 0) {
      eng.gameTime->updateLightMap();
      eng.player->updateFov();
      Renderer::drawMapAndInterface();
    }
  }
}

void DeviceElectricLantern::printToggleMessage() {
  const string toggleStr = isActivated_ ? "I turn off" : "I turn on";
  eng.log->addMsg(toggleStr + " an Electric Lantern.");
}

void DeviceElectricLantern::toggle_() {
  Audio::play(SfxId::electricLantern);
  eng.gameTime->updateLightMap();
  eng.player->updateFov();
  Renderer::drawMapAndInterface();
}

bool DeviceElectricLantern::isGivingLight() const {
  return isActivated_ && malfunctCooldown_ <= 0;
}

void DeviceElectricLantern::runBadEffect() {
  if(malfunctCooldown_ <= 0) {
    bool isVisionUpdateNeeded = false;
    bool isItemDestroyed = false;

    const int RND = Rnd::percentile();
    if(RND < 6) {
      eng.log->addMsg("My Electric Lantern breaks!");
      eng.player->getInv().removetemInGeneralWithPointer(this, false);
      isVisionUpdateNeeded = true;
      isItemDestroyed = true;
    } else if(RND < 20) {
      eng.log->addMsg("My Electric Lantern malfunctions.");
      malfunctCooldown_ = Rnd::range(3, 4);
      isVisionUpdateNeeded = true;
    } else if(RND < 50) {
      eng.log->addMsg("My Electric Lantern flickers.");
      malfunctCooldown_ = 2;
      isVisionUpdateNeeded = true;
    }

    if(isVisionUpdateNeeded) {
      eng.gameTime->updateLightMap();
      eng.player->updateFov();
      Renderer::drawMapAndInterface();
    }
    if(isItemDestroyed) {
      delete this;
    }
  }
}


