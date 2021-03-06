#include "Hide.h"

#include <algorithm>

#include "Engine.h"

#include "GameTime.h"
#include "ActorPlayer.h"
#include "ActorMonster.h"
#include "Log.h"
#include "Utils.h"

void Hide::playerTryHide() {

  bool isHideSucceed = true;

  if(Rnd::oneIn(3)) {
    isHideSucceed = false;
  } else {
    vector<Actor*> actors; actors.resize(0);

    for(Actor * actor : eng.gameTime->actors_) {
      if(actor != eng.player) {actors.push_back(actor);}
    }

    //Cap number of monsters attempting to spot player
    const Pos& playerPos = eng.player->pos;
    sort(actors.begin(), actors.end(),
    [this, playerPos](const Actor * a1, const Actor * a2) {
      const int A1_DIST = Utils::chebyshevDist(a1->pos, playerPos);
      const int A2_DIST = Utils::chebyshevDist(a2->pos, playerPos);
      return A1_DIST < A2_DIST;
    });
    actors.resize(min(int(actors.size()), 3));

    for(Actor * actor : actors) {
      if(actor->isSpottingHiddenActor(*eng.player)) {
        isHideSucceed = false;
        break;
      }
    }
  }

  if(isHideSucceed) {
    for(Actor * actor : eng.gameTime->actors_) {
      if(actor != eng.player) {
        Monster* const monster = dynamic_cast<Monster*>(actor);
        monster->awareOfPlayerCounter_ = 0;
      }
    }
  }

//  eng.gameTime->actorDidAct();
}
