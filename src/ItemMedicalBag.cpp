#include "ItemMedicalBag.h"

#include "Engine.h"
#include "Properties.h"
#include "ActorPlayer.h"
#include "Log.h"
#include "Renderer.h"
#include "Inventory.h"
#include "PlayerBonuses.h"
#include "Popup.h"
#include "GameTime.h"
#include "MapParsing.h"

bool MedicalBag::activateDefault(Actor* const actor) {
  (void)actor;

  vector<Actor*> SpottedEnemies;
  eng.player->getSpottedEnemies(SpottedEnemies);
  if(SpottedEnemies.empty() == false) {
    eng.log->addMsg("Not while an enemy is near.");
    return false;
  }

  curAction_ = playerChooseAction();

  if(curAction_ != endOfMedicalBagActions) {
    //Check if chosen action can be done
    vector<PropId> props;
    eng.player->getPropHandler().getAllActivePropIds(props);
    switch(curAction_) {
      case medicalBagAction_sanitizeInfection: {
        if(find(props.begin(), props.end(), propInfected) == props.end()) {
          eng.log->addMsg("I have no infections to sanitize.");
          curAction_ = endOfMedicalBagActions;
        }
      } break;

      case medicalBagAction_takeMorphine: {
        if(eng.player->getHp() >= eng.player->getHpMax(true)) {
          eng.log->addMsg("I am not in pain.");
          curAction_ = endOfMedicalBagActions;
        }
      } break;

      case medicalBagAction_treatWound: {
        if(find(props.begin(), props.end(), propWound) == props.end()) {
          eng.log->addMsg("I have no wounds to treat.");
          curAction_ = endOfMedicalBagActions;
        }
      } break;

      case endOfMedicalBagActions: {} break;
    }

    if(curAction_ != endOfMedicalBagActions) {
      if(getNrSuppliesNeededForAction(curAction_) > nrSupplies_) {
        eng.log->addMsg("I do not have enough supplies for that.");
        curAction_ = endOfMedicalBagActions;
      }
    }

    if(curAction_ != endOfMedicalBagActions) {
      //Action can be done
      nrTurnsLeft_ = getTotTurnsForAction(curAction_);
      eng.player->activeMedicalBag = this;

      switch(curAction_) {
        case medicalBagAction_sanitizeInfection: {
          eng.log->addMsg("I start to sanitize an infection...");
        } break;

        case medicalBagAction_takeMorphine: {
          eng.log->addMsg("I start to take Morphine...");
        } break;

        case medicalBagAction_treatWound: {
          eng.log->addMsg("I start to treat a wound...");
        } break;

        case endOfMedicalBagActions: {} break;
      }

      eng.gameTime->actorDidAct();
    }
  }

  return false;
}

MedicalBagAction MedicalBag::playerChooseAction() const {
  vector<string> choiceLabels;
  for(int actionNr = 0; actionNr < endOfMedicalBagActions; actionNr++) {
    string label = "";
    switch(MedicalBagAction(actionNr)) {
      case medicalBagAction_sanitizeInfection: {
        label = "Sanitize infection";
      } break;

      case medicalBagAction_takeMorphine: {
        label = "Take morphine";
      } break;

      case medicalBagAction_treatWound: {
        label = "Treat wound";
      } break;

      case endOfMedicalBagActions: {} break;
    }

    const int NR_TURNS_NEEDED =
      getTotTurnsForAction(MedicalBagAction(actionNr));
    const int NR_SUPPL_NEEDED =
      getNrSuppliesNeededForAction(MedicalBagAction(actionNr));
    label += " (" + toString(NR_SUPPL_NEEDED) + " suppl";
    label += "/"  + toString(NR_TURNS_NEEDED) + " turns)";
    choiceLabels.push_back(label);
  }
  choiceLabels.push_back("Cancel");

  const string suppliesMsg =
    toString(nrSupplies_) + " medical supplies available.";

  const int CHOICE_NR =
    eng.popup->showMenuMsg(suppliesMsg, true, choiceLabels, "Use medical bag");
  return MedicalBagAction(CHOICE_NR);
}

void MedicalBag::continueAction() {
  nrTurnsLeft_--;
  if(nrTurnsLeft_ <= 0) {
    finishCurAction();
  } else {
    eng.gameTime->actorDidAct();
  }
}

void MedicalBag::finishCurAction() {
  eng.player->activeMedicalBag = NULL;

  switch(curAction_) {
    case medicalBagAction_sanitizeInfection: {
      bool visionBlockers[MAP_W][MAP_H];
      MapParse::parse(CellPred::BlocksVision(eng), visionBlockers);
      eng.player->getPropHandler().endAppliedProp(
        propInfected, visionBlockers);
    } break;

    case medicalBagAction_treatWound: {
      Prop* prop =
        eng.player->getPropHandler().getProp(propWound, PropSrc::applied);
      if(prop == NULL) {
        trace << "[WARNING] No wound prop found, ";
        trace << "in MedicalBag::finishCurAction()" << endl;
      } else {
        dynamic_cast<PropWound*>(prop)->healOneWound();
      }
    } break;

    case medicalBagAction_takeMorphine: {
      eng.player->restoreHp(999);
      eng.log->addMsg("The morphine takes a toll on my mind.");
      eng.player->incrShock(ShockValue::shockValue_heavy, shockSrc_misc);
    } break;

    case endOfMedicalBagActions: {} break;
  }

  nrSupplies_ -= getNrSuppliesNeededForAction(curAction_);

  curAction_ = endOfMedicalBagActions;

  if(nrSupplies_ <= 0) {
    eng.player->getInv().removetemInGeneralWithPointer(this, true);
  }
}

void MedicalBag::interrupted() {
  eng.log->addMsg("My healing is disrupted.", clrWhite, false);

  nrTurnsLeft_ = -1;

  eng.player->activeMedicalBag = NULL;
}

int MedicalBag::getTotTurnsForAction(const MedicalBagAction action) const {
  const bool IS_HEALER =
    eng.playerBonHandler->hasTrait(traitHealer);

  switch(action) {
    case medicalBagAction_sanitizeInfection: {
      return 20 / (IS_HEALER ? 2 : 1);
    } break;

    case medicalBagAction_treatWound: {
      return 70 / (IS_HEALER ? 2 : 1);
    } break;

    case medicalBagAction_takeMorphine: {
      return 8 / (IS_HEALER ? 2 : 1);
    } break;

    case endOfMedicalBagActions: {} break;
  }
  return -1;
}

int MedicalBag::getNrSuppliesNeededForAction(
  const MedicalBagAction action) const {

  const bool IS_HEALER =
    eng.playerBonHandler->hasTrait(traitHealer);

  switch(action) {
    case medicalBagAction_sanitizeInfection: {
      return 4 / (IS_HEALER ? 2 : 1);
    } break;

    case medicalBagAction_treatWound: {
      return 10 / (IS_HEALER ? 2 : 1);
    } break;

    case medicalBagAction_takeMorphine: {
      return 4 / (IS_HEALER ? 2 : 1);
    } break;

    case endOfMedicalBagActions: {} break;
  }
  return -1;
}
