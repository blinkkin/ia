#ifndef AI_STEP_TO_LAIR_IF_HAS_LOS_TO_LAIR_H
#define AI_STEP_TO_LAIR_IF_HAS_LOS_TO_LAIR_H

#include "Engine.h"
#include "Fov.h"

class AI_stepToLairIfHasLosToLair {
public:
  static bool action(Monster& monster, const Pos& lairCell, Engine& engine) {
    if(monster.deadState == ActorDeadState::alive) {
      bool blockers[MAP_W][MAP_H];
      MapParse::parse(CellPred::BlocksVision(engine), blockers);
      const bool HAS_LOS_TO_LAIR =
        engine.fov->checkCell(blockers, lairCell, monster.pos, true);

      if(HAS_LOS_TO_LAIR) {
        Pos delta = lairCell - monster.pos;

        delta.x = delta.x == 0 ? 0 : (delta.x > 0 ? 1 : -1);
        delta.y = delta.y == 0 ? 0 : (delta.y > 0 ? 1 : -1);
        const Pos newPos = monster.pos + delta;

        MapParse::parse(CellPred::BlocksVision(engine), blockers);
        if(blockers[newPos.x][newPos.y]) {
          return false;
        } else {
          monster.moveDir(DirUtils::getDir(delta));
          return true;
        }
      }
    }

    return false;
  }

private:

};

#endif
