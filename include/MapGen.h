#ifndef MAPBUILD_H
#define MAPBUILD_H

#include "CommonTypes.h"
#include "RoomTheme.h"
#include "Feature.h"
#include "MapTemplates.h"

using namespace std;

class Engine;
struct Region;

enum HorizontalVertical {
  horizontal,
  vertical
};

enum RoomReshapeType {
  roomReshape_trimCorners,
  roomReshape_pillarsRandom
};

struct Room {
public:
  Room(Rect dims) : roomTheme(roomTheme_plain), roomDescr(""), dims_(dims) {}

  Room() : roomTheme(roomTheme_plain), roomDescr(""),
    dims_(Rect(Pos(-1, -1), Pos(-1, -1))) {}

  inline Rect getDims()   const {return dims_;}
  inline int getX0()      const {return dims_.x0y0.x;}
  inline int getY0()      const {return dims_.x0y0.y;}
  inline int getX1()      const {return dims_.x1y1.x;}
  inline int getY1()      const {return dims_.x1y1.y;}
  inline Pos getX0Y0()    const {return dims_.x0y0;}
  inline Pos getX1Y1()    const {return dims_.x1y1;}

  RoomThemeId roomTheme;

  string roomDescr;

private:
  Rect dims_;
};

class MapGenUtilCorridorBuilder {
public:
  MapGenUtilCorridorBuilder(Engine& engine) : eng(engine) {}
  ~MapGenUtilCorridorBuilder() {}

  void buildZCorridorBetweenRooms(
    const Room& room1, const Room& room2, Dir cardinalDirToTravel,
    bool doorPosCandidates[MAP_W][MAP_H] = NULL);

private:
  Engine& eng;
};

class MapGen {
public:
  MapGen(Engine& engine) : eng(engine) {}
  ~MapGen() {}

  inline bool run() {return run_();}

protected:
  virtual bool run_() = 0;

  void buildFromTemplate(const Pos& pos, MapTemplate* t);
  void buildFromTemplate(const Pos& pos, MapTemplateId templateId);

  FeatureId backup[MAP_W][MAP_H];
  void backupMap();
  void restoreMap();

  void makePathByRandomWalk(
    int originX, int originY, int len, FeatureId featureToMake,
    const bool TUNNEL_THROUGH_ANY_FEATURE, const bool ONLY_STRAIGHT = true,
    const Pos& x0y0Lim = Pos(1, 1),
    const Pos& x1y1Lim = Pos(MAP_W - 2, MAP_H - 2));

  void makeStraightPathByPathfinder(
    const Pos& origin, const Pos& target, FeatureId feature, const bool SMOOTH,
    const bool TUNNEL_THROUGH_ANY_FEATURE);

  Engine& eng;
};

class MapGenBsp : public MapGen {
public:
  MapGenBsp(Engine& engine) : MapGen(engine) {}
  virtual ~MapGenBsp() {}

private:
  bool run_();

  void coverAreaWithFeature(const Rect& area, const FeatureId feature);

  int getNrStepsInDirUntilWallFound(Pos c, const Dir dir) const;

  bool isAllRoomsConnected();

  Room* buildRoom(const Rect& roomPoss);

  bool roomCells[MAP_W][MAP_H]; //Used for help building the map
  bool regionsToBuildCave[3][3];

  void makeCrumbleRoom(const Rect& roomAreaIncludingWalls,
                       const Pos& proxEventPos);

  void connectRegions(Region* regions[3][3]);
  void buildAuxRooms(Region* regions[3][3]);
  bool tryPlaceAuxRoom(
    const int X0, const int Y0, const int W, const int H,
    bool blockers[MAP_W][MAP_H], const Pos& doorPos);

  void buildMergedRegionsAndRooms(
    Region* regions[3][3], const int SPLIT_X1, const int SPLIT_X2,
    const int SPLIT_Y1, const int SPLIT_Y2);

  void buildCaves(Region* regions[3][3]);

  void placeDoorAtPosIfSuitable(const Pos& pos);

  void reshapeRoom(const Room& room);

  void buildRoomsInRooms();

  void postProcessFillDeadEnds();

  bool globalDoorPosCandidates[MAP_W][MAP_H];

//  void findEdgesOfRoom(const Rect roomPoss, vector<Pos>& vectorRef);

  bool isRegionFoundInCardinalDir(
    const Pos& pos, bool region[MAP_W][MAP_H]) const;

  bool isAreaFree(
    const Rect& area, bool blockingCells[MAP_W][MAP_H]);
  bool isAreaAndBorderFree(
    const Rect& areaWithBorder, bool blockingCells[MAP_W][MAP_H]);
  bool isAreaFree(
    const int X0, const int Y0, const int X1, const int Y1,
    bool blockingCells[MAP_W][MAP_H]);

  void decorate();

  void getAllowedStairCells(bool cellsToSet[MAP_W][MAP_H]) const;
  Pos placeStairs();
  void movePlayerToNearestAllowedPos();

//  void makeLevers();
//  void spawnLeverAdaptAndLinkDoor(const Pos& leverPos, Door& door);

  void revealAllDoorsBetweenPlayerAndStairs(const Pos& stairsPos);

//  void buildNaturalArea(Region* regions[3][3]);
//  void makeRiver(Region* regions[3][3]);

//  vector<Room*> rooms_;
  void deleteAndRemoveRoomFromList(Room* const room);
};

struct Region {
public:
  Region(const Pos& x0y0, const Pos& x1y1);
  Region();
  ~Region();

  Rect getRandomPossForRoom() const;
  Rect getRegionPoss() const {return Rect(x0y0_, x1y1_);}

  bool isRegionNeighbour(const Region& other);

  inline Pos getCenterPos() const {return (x1y1_ + x0y0_) / 2;}
  inline Pos getX0Y0()      const {return x0y0_;}
  inline Pos getX1Y1()      const {return x1y1_;}

  int getNrOfConnections();

  bool regionsConnectedTo[3][3];

  Room* mainRoom;

  bool isConnected;

private:
  Pos x0y0_, x1y1_;
};

class MapGenIntroForest : public MapGen {
public:
  MapGenIntroForest(Engine& engine) : MapGen(engine) {}
  ~MapGenIntroForest() {}

private:
  bool run_();

  void buildForestLimit();
  void buildForestOuterTreeline();
  void buildForestTreePatch();
  void buildForestTrees(const Pos& stairsPos);
};

class MapGenEgyptTomb : public MapGen {
public:
  MapGenEgyptTomb(Engine& engine) : MapGen(engine) {}
  ~MapGenEgyptTomb() {}

private:
  bool run_();
};

class MapGenCaveLvl : public MapGen {
public:
  MapGenCaveLvl(Engine& engine) : MapGen(engine) {}
  ~MapGenCaveLvl() {}

private:
  bool run_();
};

class MapGenTrapezohedronLvl : public MapGen {
public:
  MapGenTrapezohedronLvl(Engine& engine) : MapGen(engine) {}
  ~MapGenTrapezohedronLvl() {}

private:
  bool run_();
};

#endif
