#include "Sound.h"

#include <iostream>

#include "Engine.h"
#include "Feature.h"
#include "Map.h"
#include "ActorPlayer.h"
#include "ActorMonster.h"
#include "GameTime.h"
#include "MapParsing.h"
#include "Utils.h"

namespace SndEmit {

//------------------------------------------------------------------------LOCAL
namespace {

int nrSndMsgPrintedCurTurn_;

bool isSndHeardAtRange(const int RANGE, const Snd& snd) {
  return snd.isLoud() ? (RANGE <= SND_DIST_LOUD) : (RANGE <= SND_DIST_NORMAL);
}

} //namespace

//-----------------------------------------------------------------------GLOBAL
void resetNrSoundMsgPrintedCurTurn() {nrSndMsgPrintedCurTurn_ = 0;}

void emitSnd(Snd snd, Engine& eng) {
  bool blockers[MAP_W][MAP_H];
  FeatureStatic* f = NULL;
  for(int y = 0; y < MAP_H; y++) {
    for(int x = 0; x < MAP_W; x++) {
      f = eng.map->cells[x][y].featureStatic;
      blockers[x][y] = f->isSoundPassable() == false;
    }
  }
  int floodFill[MAP_W][MAP_H];
  const Pos& origin = snd.getOrigin();
  FloodFill::run(origin, blockers, floodFill, 999, Pos(-1, -1));
  floodFill[origin.x][origin.y] = 0;

  for(Actor * actor : eng.gameTime->actors_) {
    const int FLOOD_VALUE_AT_ACTOR = floodFill[actor->pos.x][actor->pos.y];

    const bool IS_ORIGIN_SEEN_BY_PLAYER =
      eng.map->cells[origin.x][origin.y].isSeenByPlayer;

    if(isSndHeardAtRange(FLOOD_VALUE_AT_ACTOR, snd)) {
      if(actor == eng.player) {

        //Various conditions may clear the sound message
        if(
          nrSndMsgPrintedCurTurn_ >= 1 ||
          (IS_ORIGIN_SEEN_BY_PLAYER && snd.isMsgIgnoredIfOriginSeen())) {
          snd.clearMsg();
        }

        const Pos& playerPos = eng.player->pos;

        if(snd.getMsg().empty() == false) {
          //Add a direction string to the message (i.e. "(NW)", "(E)" , etc)
          if(playerPos != origin) {
            string dirStr;
            DirUtils::getCompassDirName(playerPos, origin, dirStr);
            snd.addString("(" + dirStr + ")");
          }
          nrSndMsgPrintedCurTurn_++;
        }

        const int SND_MAX_DISTANCE =
          snd.isLoud() ? SND_DIST_LOUD : SND_DIST_NORMAL;
        const int PERCENT_DISTANCE =
          (FLOOD_VALUE_AT_ACTOR * 100) / SND_MAX_DISTANCE;

        const Pos offset = (origin - playerPos).getSigns();
        const Dir dirToOrigin = DirUtils::getDir(offset);
        eng.player->hearSound(snd, IS_ORIGIN_SEEN_BY_PLAYER, dirToOrigin,
                              PERCENT_DISTANCE);
      } else {
        Monster* const monster = dynamic_cast<Monster*>(actor);
        monster->hearSound(snd);
      }
    }
  }
}

}
