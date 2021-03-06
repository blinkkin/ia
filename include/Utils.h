#ifndef UTILS_H
#define UTILS_H

#include <vector>

#include "CommonData.h"
#include "Actor.h"

class Engine;

namespace Rnd {

//Note: If MTRand is not provided any parameters to the constructor,
//it will instead seed with current time. So seeding it manually is
//not necessary for normal gameplay purposes - only if seed should be
//controlled for testing purposes, or recreating a certain level, etc.
void seed(const unsigned long val);

int dice(const int ROLLS, const int SIDES);

int dice(const DiceParam& p);

bool coinToss();

bool fraction(const int NUMERATOR, const int DENOMINATOR);

bool oneIn(const int N);

int range(const int MIN, const int MAX);

int range(const Range& valueRange);

int percentile();

} //Rnd

enum TimeType {
  time_year,
  time_month,
  time_day,
  time_hour,
  time_minute,
  time_second
};

struct TimeData {
  TimeData() :
    year_(0), month_(0), day_(0), hour_(0), minute_(0), second_(0) {
  }

  TimeData(int year, int month, int day, int hour, int minute, int second) :
    year_(year), month_(month), day_(day), hour_(hour), minute_(minute),
    second_(second) {}

  string getTimeStr(const TimeType lowest, const bool ADD_SEPARATORS) const;

  int year_, month_, day_, hour_, minute_, second_;
};

namespace Utils {

bool isClrEq(const SDL_Color& clr1, const SDL_Color& clr2);

void resetArray(int a[MAP_W][MAP_H]);

void resetArray(Actor* a[MAP_W][MAP_H]);

void resetArray(bool a[MAP_W][MAP_H], const bool value);

void reverseBoolArray(bool array[MAP_W][MAP_H]);

void makeVectorFromBoolMap(const bool VALUE_TO_STORE,
                           bool a[MAP_W][MAP_H],
                           vector<Pos>& vectorRef);

Actor* getActorAtPos(const Pos& pos, Engine& eng,
                     ActorDeadState deadState = ActorDeadState::alive);

void getActorPositions(const vector<Actor*>& actors,
                       vector<Pos>& vectorRef);

void makeActorArray(Actor* a[MAP_W][MAP_H], Engine& eng);

bool isPosInsideMap(const Pos& pos);

bool isPosInside(const Pos& pos, const Rect& area);

bool isAreaInsideOther(const Rect& inner, const Rect& outer,
                       const bool COUNT_EQUAL_AS_INSIDE);

bool isAreaInsideMap(const Rect& area);

bool isPosAdj(const Pos& pos1, const Pos& pos2,
              const bool COUNT_SAME_CELL_AS_NEIGHBOUR);

Pos getClosestPos(const Pos& c, const vector<Pos>& positions);

Actor* getRandomClosestActor(const Pos& c, const vector<Actor*>& actors);

int chebyshevDist(const int X0, const int Y0, const int X1, const int Y1);

int chebyshevDist(const Pos& c1, const Pos& c2);

TimeData getCurrentTime();

} //Utils

namespace DirUtils {

Dir getDir(const Pos& offset);

Pos getOffset(const Dir dir);

void getCompassDirName(const Pos& fromPos, const Pos& toPos, string& strRef);

void getCompassDirName(const Dir dir, string& strRef);

void getCompassDirName(const Pos& offset, string& strRef);

} //DirUtils

#endif
