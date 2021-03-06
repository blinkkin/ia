#include "ItemDrop.h"

#include "Engine.h"
#include "CommonTypes.h"
#include "GameTime.h"
#include "ActorPlayer.h"
#include "Log.h"
#include "Map.h"
#include "Inventory.h"
#include "ItemFactory.h"
#include "MapParsing.h"
#include "Utils.h"

#include <algorithm>

void ItemDrop::dropAllCharactersItems(Actor* actor, bool died) {
  (void)died;
  actor->getInv().dropAllNonIntrinsic(actor->pos, true, eng);
}

void ItemDrop::dropItemFromInventory(Actor* actorDropping, const int ELEMENT,
                                     const int NR_ITEMS_TO_DROP) {
  Inventory& inv = actorDropping->getInv();
  Item* itemToDrop = inv.getItemInElement(ELEMENT);

  if(itemToDrop != NULL) {
    const bool IS_STACKABLE = itemToDrop->getData().isStackable;
    const int NR_ITEMS_BEFORE_DROP = itemToDrop->nrItems;
    const bool IS_WHOLE_STACK_DROPPED =
      IS_STACKABLE == false ||
      NR_ITEMS_TO_DROP == -1 ||
      (NR_ITEMS_TO_DROP >= NR_ITEMS_BEFORE_DROP);

    string itemRef = "";

    if(IS_WHOLE_STACK_DROPPED) {
      itemRef = eng.itemDataHandler->getItemRef(*itemToDrop, itemRef_plural);
      inv.removeItemInElementWithoutDeletingInstance(ELEMENT);
      dropItemOnMap(actorDropping->pos, *itemToDrop);
    } else {
      Item* itemToKeep = itemToDrop;
      itemToDrop = eng.itemFactory->copyItem(itemToKeep);
      itemToDrop->nrItems = NR_ITEMS_TO_DROP;
      itemRef = eng.itemDataHandler->getItemRef(*itemToDrop, itemRef_plural);
      itemToKeep->nrItems = NR_ITEMS_BEFORE_DROP - NR_ITEMS_TO_DROP;
      dropItemOnMap(actorDropping->pos, *itemToDrop);
    }

    //Messages
    const Actor* const curActor = eng.gameTime->getCurrentActor();
    if(curActor == eng.player) {
      eng.log->clearLog();
      eng.log->addMsg(
        "I drop " + itemRef + ".", clrWhite, false, true);
    } else {
      bool blockers[MAP_W][MAP_H];
      MapParse::parse(CellPred::BlocksVision(eng), blockers);
      if(eng.player->isSeeingActor(*curActor, blockers)) {
        eng.log->addMsg(
          "I see " + curActor->getNameThe() + " drop " + itemRef + ".");
      }
    }

    //End turn
    eng.gameTime->actorDidAct();
  }
}

Item* ItemDrop::dropItemOnMap(const Pos& intendedPos, Item& item) {
  //If target cell is bottomless, just destroy the item
  const Feature* const targetFeature =
    eng.map->cells[intendedPos.x][intendedPos.y].featureStatic;
  if(targetFeature->isBottomless()) {
    delete &item;
    return NULL;
  }

  //Make a vector of all cells on map with no blocking feature
  bool freeCellArray[MAP_W][MAP_H];
  for(int y = 0; y < MAP_H; y++) {
    for(int x = 0; x < MAP_W; x++) {
      FeatureStatic* const f = eng.map->cells[x][y].featureStatic;
      freeCellArray[x][y] = f->canHaveItem() && f->isBottomless() == false;
    }
  }
  vector<Pos> freeCells;
  Utils::makeVectorFromBoolMap(true, freeCellArray, freeCells);

  //Sort the vector according to distance to origin
  IsCloserToOrigin isCloserToOrigin(intendedPos, eng);
  sort(freeCells.begin(), freeCells.end(), isCloserToOrigin);

  Pos curPos;
  Pos stackPos;
  const bool ITEM_STACKS = item.getData().isStackable;
  int ii = 0;
  const unsigned int vectorSize = freeCells.size();
  for(unsigned int i = 0; i < vectorSize; i++) {
    //First look in all cells that has distance to origin equal to cell i
    //to try and merge the item if it stacks
    if(ITEM_STACKS) {
      //While ii cell is not further away than i cell
      while(isCloserToOrigin(freeCells.at(i), freeCells.at(ii)) == false) {
        stackPos = freeCells.at(ii);
        Item* itemFoundOnFloor = eng.map->cells[stackPos.x][stackPos.y].item;
        if(itemFoundOnFloor != NULL) {
          if(itemFoundOnFloor->getData().id == item.getData().id) {
            item.nrItems += itemFoundOnFloor->nrItems;
            delete itemFoundOnFloor;
            eng.map->cells[stackPos.x][stackPos.y].item = &item;
            return &item;
          }
        }
        ii++;
      }
    } else {
      item.appplyDropEffects();
    }

    if(&item == NULL) {break;}

    curPos = freeCells.at(i);
    if(eng.map->cells[curPos.x][curPos.y].item == NULL) {

      eng.map->cells[curPos.x][curPos.y].item = &item;

      const bool IS_PLAYER_POS    = eng.player->pos == curPos;
      const bool IS_INTENDED_POS  = curPos == intendedPos;
      if(IS_PLAYER_POS && IS_INTENDED_POS == false) {
        eng.log->addMsg("I feel something by my feet.");
      }

      i = 99999;
    }

    if(i == vectorSize - 1) {
      delete &item;
      return NULL;
    }
  }
  return NULL;
}

