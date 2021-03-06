#include "PopulateMonsters.h"

#include <algorithm>

#include "Engine.h"
#include "FeatureTrap.h"
#include "Map.h"
#include "ActorFactory.h"
#include "ActorMonster.h"
#include "ActorPlayer.h"
#include "FeatureFactory.h"
#include "MapGen.h"
#include "MapParsing.h"
#include "Utils.h"

using namespace std;

void PopulateMonsters::makeListOfMonstersEligibleForAutoSpawning(
  const int NR_LVLS_OUT_OF_DEPTH, vector<ActorId>& listRef) const {

  listRef.resize(0);

  const int DLVL = eng.map->getDlvl();
  const int EFFECTIVE_DLVL =
    max(1, min(LAST_CAVERN_LEVEL, DLVL + NR_LVLS_OUT_OF_DEPTH));

  for(unsigned int i = actor_player + 1; i < endOfActorIds; i++) {
    const ActorData& d = eng.actorDataHandler->dataList[i];
    if(
      d.isAutoSpawnAllowed &&
      d.nrLeftAllowedToSpawn != 0 &&
      EFFECTIVE_DLVL >= d.spawnMinDLVL &&
      EFFECTIVE_DLVL <= d.spawnMaxDLVL) {
      listRef.push_back((ActorId)(i));
    }
  }
}

void PopulateMonsters::spawnGroupOfRandomAt(
  const vector<Pos>& sortedFreeCellsVector,
  bool blockers[MAP_W][MAP_H],
  const int NR_LVLS_OUT_OF_DEPTH_ALLOWED,
  const bool IS_ROAMING_ALLOWED) const {

  vector<ActorId> idCandidates;
  makeListOfMonstersEligibleForAutoSpawning(
    NR_LVLS_OUT_OF_DEPTH_ALLOWED, idCandidates);

  if(idCandidates.empty() == false) {
    const ActorId id =
      idCandidates.at(Rnd::range(0, idCandidates.size() - 1));

    spawnGroupAt(id, sortedFreeCellsVector, blockers,
                 IS_ROAMING_ALLOWED);
  }
}

void PopulateMonsters::trySpawnDueToTimePassed() const {
  trace << "PopulateMonsters::trySpawnDueToTimePassed()..." << endl;

  bool blockers[MAP_W][MAP_H];
  MapParse::parse(CellPred::BlocksMoveCmn(true, eng), blockers);

  const int MIN_DIST_TO_PLAYER = FOV_STD_RADI_INT + 3;

  const Pos& playerPos = eng.player->pos;
  const int X0 = max(0, playerPos.x - MIN_DIST_TO_PLAYER);
  const int Y0 = max(0, playerPos.y - MIN_DIST_TO_PLAYER);
  const int X1 = min(MAP_W - 1, playerPos.x + MIN_DIST_TO_PLAYER);
  const int Y1 = min(MAP_H - 1, playerPos.y + MIN_DIST_TO_PLAYER);

  for(int x = X0; x <= X1; x++) {
    for(int y = Y0; y <= Y1; y++) {
      blockers[x][y] = true;
    }
  }

  vector<Pos> freeCellsVector;
  for(int y = 1; y < MAP_H - 1; y++) {
    for(int x = 1; x < MAP_W - 1; x++) {
      if(blockers[x][y] == false) {
        freeCellsVector.push_back(Pos(x, y));
      }
    }
  }

  if(freeCellsVector.empty() == false) {

    const int ELEMENT = Rnd::range(0, freeCellsVector.size() - 1);
    const Pos& origin = freeCellsVector.at(ELEMENT);

    makeSortedFreeCellsVector(origin, blockers, freeCellsVector);

    if(freeCellsVector.empty() == false) {
      if(eng.map->cells[origin.x][origin.y].isExplored) {
        const int NR_OOD = getRandomOutOfDepth();
        spawnGroupOfRandomAt(freeCellsVector, blockers, NR_OOD, true);
      }
    }
  }
  trace << "PopulateMonsters::trySpawnDueToTimePassed() [DONE]" << endl;
}

void PopulateMonsters::populateCaveLevel() const {
  const int NR_GROUPS_ALLOWED = Rnd::range(6, 7);

  bool blockers[MAP_W][MAP_H];

  const int MIN_DIST_FROM_PLAYER = FOV_STD_RADI_INT - 2;
  MapParse::parse(CellPred::BlocksMoveCmn(true, eng), blockers);

  const Pos& playerPos = eng.player->pos;

  const int X0 = max(0, playerPos.x - MIN_DIST_FROM_PLAYER);
  const int Y0 = max(0, playerPos.y - MIN_DIST_FROM_PLAYER);
  const int X1 = min(MAP_W - 1, playerPos.x + MIN_DIST_FROM_PLAYER) - 1;
  const int Y1 = min(MAP_H - 1, playerPos.y + MIN_DIST_FROM_PLAYER) - 1;

  for(int y = Y0; y <= Y1; y++) {
    for(int x = X0; x <= X1; x++) {
      blockers[x][y] = true;
    }
  }

  for(int i = 0; i < NR_GROUPS_ALLOWED; i++) {
    vector<Pos> originCandidates;
    for(int y = 1; y < MAP_H - 1; y++) {
      for(int x = 1; x < MAP_W - 1; x++) {
        if(blockers[x][y] == false) {
          originCandidates.push_back(Pos(x, y));
        }
      }
    }
    const int ELEMENT = Rnd::range(0, originCandidates.size() - 1);
    const Pos origin = originCandidates.at(ELEMENT);
    vector<Pos> sortedFreeCellsVector;
    makeSortedFreeCellsVector(origin, blockers, sortedFreeCellsVector);
    if(sortedFreeCellsVector.empty() == false) {
      spawnGroupOfRandomAt(sortedFreeCellsVector, blockers,
                           getRandomOutOfDepth(), true);
    }
  }
}

void PopulateMonsters::populateIntroLevel() {
  const int NR_GROUPS_ALLOWED = 2; //Rnd::range(2, 3);

  bool blockers[MAP_W][MAP_H];

  const int MIN_DIST_FROM_PLAYER = FOV_STD_RADI_INT + 3;
  MapParse::parse(CellPred::BlocksMoveCmn(true, eng), blockers);

  const Pos& playerPos = eng.player->pos;

  const int X0 = max(0, playerPos.x - MIN_DIST_FROM_PLAYER);
  const int Y0 = max(0, playerPos.y - MIN_DIST_FROM_PLAYER);
  const int X1 = min(MAP_W - 1, playerPos.x + MIN_DIST_FROM_PLAYER) - 1;
  const int Y1 = min(MAP_H - 1, playerPos.y + MIN_DIST_FROM_PLAYER) - 1;
  for(int y = Y0; y <= Y1; y++) {
    for(int x = X0; x <= X1; x++) {
      blockers[x][y] = true;
    }
  }

  for(int i = 0; i < NR_GROUPS_ALLOWED; i++) {
    vector<Pos> originCandidates;
    for(int y = 1; y < MAP_H - 1; y++) {
      for(int x = 1; x < MAP_W - 1; x++) {
        if(blockers[x][y] == false) {
          originCandidates.push_back(Pos(x, y));
        }
      }
    }
    const int ELEMENT = Rnd::range(0, originCandidates.size() - 1);
    const Pos origin = originCandidates.at(ELEMENT);
    vector<Pos> sortedFreeCellsVector;
    makeSortedFreeCellsVector(origin, blockers, sortedFreeCellsVector);
    if(sortedFreeCellsVector.empty() == false) {
      spawnGroupAt(actor_wolf, sortedFreeCellsVector, blockers, true);
    }
  }
}

void PopulateMonsters::populateRoomAndCorridorLevel() const {
  const int NR_GROUPS_ALLOWED_ON_MAP = Rnd::range(5, 9);
  int nrGroupsSpawned = 0;

  bool blockers[MAP_W][MAP_H];

  const int MIN_DIST_FROM_PLAYER = FOV_STD_RADI_INT - 1;

  MapParse::parse(CellPred::BlocksMoveCmn(true, eng), blockers);

  const Pos& playerPos = eng.player->pos;

  const int X0 = max(0, playerPos.x - MIN_DIST_FROM_PLAYER);
  const int Y0 = max(0, playerPos.y - MIN_DIST_FROM_PLAYER);
  const int X1 = min(MAP_W - 1, playerPos.x + MIN_DIST_FROM_PLAYER);
  const int Y1 = min(MAP_H - 1, playerPos.y + MIN_DIST_FROM_PLAYER);
  for(int y = Y0; y <= Y1; y++) {
    for(int x = X0; x <= X1; x++) {
      blockers[x][y] = true;
    }
  }

  //First, attempt to populate all non-plain themed rooms
  for(Room * const room : eng.map->rooms) {
    if(room->roomTheme != roomTheme_plain) {

      const int ROOM_W = room->getX1() - room->getX0() + 1;
      const int ROOM_H = room->getY1() - room->getY0() + 1;
      const int NR_CELLS_IN_ROOM = ROOM_W * ROOM_H;

      const int MAX_NR_GROUPS_IN_ROOM = 2;
      for(int i = 0; i < MAX_NR_GROUPS_IN_ROOM; i++) {
        //Randomly pick a free position inside the room
        vector<Pos> originCandidates;
        for(int y = room->getY0(); y <= room->getY1(); y++) {
          for(int x = room->getX0(); x <= room->getX1(); x++) {
            if(
              blockers[x][y] == false &&
              eng.roomThemeMaker->themeMap[x][y] == room->roomTheme) {
              originCandidates.push_back(Pos(x, y));
            }
          }
        }

        //If room is too full (due to spawned monsters and features),
        //stop spawning in this room
        const int NR_ORIGIN_CANDIDATES = originCandidates.size();
        if(NR_ORIGIN_CANDIDATES < (NR_CELLS_IN_ROOM / 3)) {break;}

        //Spawn monsters in room
        if(NR_ORIGIN_CANDIDATES > 0) {
          const int ELEMENT = Rnd::range(0, NR_ORIGIN_CANDIDATES - 1);
          const Pos& origin = originCandidates.at(ELEMENT);
          vector<Pos> sortedFreeCellsVector;
          makeSortedFreeCellsVector(origin, blockers, sortedFreeCellsVector);

          if(spawnGroupOfRandomNativeToRoomThemeAt(
                room->roomTheme, sortedFreeCellsVector, blockers, false)) {
            nrGroupsSpawned++;
            if(nrGroupsSpawned >= NR_GROUPS_ALLOWED_ON_MAP) {
              return;
            }
          }
        }
      }

      //After attempting to populate a non-plain themed room,
      //mark that area as forbidden
      for(int y = room->getY0(); y <= room->getY1(); y++) {
        for(int x = room->getX0(); x <= room->getX1(); x++) {
          blockers[x][y] = true;
        }
      }
    }
  }

  //Second, place groups randomly in plain-themed areas until
  //no more groups to place
  while(nrGroupsSpawned < NR_GROUPS_ALLOWED_ON_MAP) {
    vector<Pos> originCandidates;
    for(int y = 1; y < MAP_H - 1; y++) {
      for(int x = 1; x < MAP_W - 1; x++) {
        if(
          blockers[x][y] == false &&
          eng.roomThemeMaker->themeMap[x][y] == roomTheme_plain) {
          originCandidates.push_back(Pos(x, y));
        }
      }
    }
    const int ELEMENT = Rnd::range(0, originCandidates.size() - 1);
    const Pos origin  = originCandidates.at(ELEMENT);
    vector<Pos> sortedFreeCellsVector;
    makeSortedFreeCellsVector(origin, blockers, sortedFreeCellsVector);
    if(spawnGroupOfRandomNativeToRoomThemeAt(
          roomTheme_plain, sortedFreeCellsVector, blockers, true)) {
      nrGroupsSpawned++;
    }
  }
}

bool PopulateMonsters::spawnGroupOfRandomNativeToRoomThemeAt(
  const RoomThemeId roomTheme, const vector<Pos>& sortedFreeCellsVector,
  bool blockers[MAP_W][MAP_H], const bool IS_ROAMING_ALLOWED) const {

  trace << "PopulateMonsters::spawnGroupOfRandomNativeToRoomThemeAt()" << endl;
  const int NR_LEVELS_OUT_OF_DEPTH_ALLOWED = getRandomOutOfDepth();
  vector<ActorId> idCandidates;
  makeListOfMonstersEligibleForAutoSpawning(
    NR_LEVELS_OUT_OF_DEPTH_ALLOWED, idCandidates);

  for(size_t i = 0; i < idCandidates.size(); i++) {
    const ActorData& d = eng.actorDataHandler->dataList[idCandidates.at(i)];
    bool isMonsterNativeToRoom = false;
    for(size_t iNative = 0; iNative < d.nativeRooms.size(); iNative++) {
      if(d.nativeRooms.at(iNative) == roomTheme) {
        isMonsterNativeToRoom = true;
        break;
      }
    }
    if(isMonsterNativeToRoom == false) {
      idCandidates.erase(idCandidates.begin() + i);
      i--;
    }
  }

  if(idCandidates.empty()) {
    trace << "PopulateMonsters: Found no valid monsters to spawn ";
    trace << "at room theme (" + toString(roomTheme) + ")" << endl;
    return false;
  } else {
    const int ELEMENT = Rnd::range(0, idCandidates.size() - 1);
    const ActorId id = idCandidates.at(ELEMENT);
    spawnGroupAt(id, sortedFreeCellsVector, blockers,
                 IS_ROAMING_ALLOWED);
    return true;
  }
}

void PopulateMonsters::spawnGroupAt(
  const ActorId id, const vector<Pos>& sortedFreeCellsVector,
  bool blockers[MAP_W][MAP_H], const bool IS_ROAMING_ALLOWED) const {

  const ActorData& d = eng.actorDataHandler->dataList[id];

  int maxNrInGroup = 1;

  switch(d.groupSize) {
    case monsterGroupSizeFew:    maxNrInGroup = Rnd::range(1, 2);    break;
    case monsterGroupSizeGroup:  maxNrInGroup = Rnd::range(3, 4);    break;
    case monsterGroupSizeHorde:  maxNrInGroup = Rnd::range(6, 7);    break;
    case monsterGroupSizeSwarm:  maxNrInGroup = Rnd::range(10, 12);  break;
    default: {} break;
  }

  Actor* originActor = NULL;

  const int NR_FREE_CELLS = sortedFreeCellsVector.size();
  const int NR_CAN_BE_SPAWNED = min(NR_FREE_CELLS, maxNrInGroup);
  for(int i = 0; i < NR_CAN_BE_SPAWNED; i++) {
    const Pos& pos = sortedFreeCellsVector.at(i);

    Actor* const actor = eng.actorFactory->spawnActor(id, pos);
    Monster* const monster = dynamic_cast<Monster*>(actor);
    monster->isRoamingAllowed_ = IS_ROAMING_ALLOWED;

    if(i == 0) {
      originActor = actor;
    } else {
      monster->leader = originActor;
    }

    blockers[pos.x][pos.y] = true;
  }
}

void PopulateMonsters::makeSortedFreeCellsVector(
  const Pos& origin, const bool blockers[MAP_W][MAP_H],
  vector<Pos>& vectorRef) const {

  vectorRef.resize(0);

  const int RADI = 10;
  const int X0 = getConstrInRange(1, origin.x - RADI, MAP_W - 2);
  const int Y0 = getConstrInRange(1, origin.y - RADI, MAP_H - 2);
  const int X1 = getConstrInRange(1, origin.x + RADI, MAP_W - 2);
  const int Y1 = getConstrInRange(1, origin.y + RADI, MAP_H - 2);

  for(int y = Y0; y <= Y1; y++) {
    for(int x = X0; x <= X1; x++) {
      if(blockers[x][y] == false) {
        vectorRef.push_back(Pos(x, y));
      }
    }
  }

  IsCloserToOrigin sorter(origin, eng);
  std::sort(vectorRef.begin(), vectorRef.end(), sorter);
}

int PopulateMonsters::getRandomOutOfDepth() const {
  const int DLVL = eng.map->getDlvl();

  if(DLVL == 0)                   {return 0;}
  if(Rnd::oneIn(40) && DLVL > 1)  {return 5;}
  if(Rnd::oneIn(5))               {return 2;}

  return 0;
}
