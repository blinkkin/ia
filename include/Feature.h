#ifndef FEATURE_H
#define FEATURE_H

#include <vector>
#include <string>

#include <iostream>

#include "Colors.h"
#include "CommonData.h"
#include "ActorData.h"
#include "FeatureData.h"
#include "CommonTypes.h"

using namespace std;

class Engine;
class Actor;
class FeatureFactory;
class FeatureSpawnData;

class Feature {
public:
  virtual ~Feature() {}

  virtual void bump(Actor& actorBumping);
  virtual void newTurn();
  virtual bool canMoveCmn() const;
  virtual bool canMove(const vector<PropId>& actorsProps) const;
  virtual bool isSoundPassable() const;
  virtual bool isVisionPassable() const;
  virtual bool isProjectilePassable() const;
  virtual bool isSmokePassable() const;
  virtual bool isBottomless() const;
  virtual string getDescr(const bool DEFINITE_ARTICLE) const;
  virtual void hit(const int DMG, const DmgTypes dmgType);
  virtual SDL_Color getClr() const;
  virtual SDL_Color getClrBg() const;
  virtual char getGlyph() const;
  virtual TileId getTile() const;
  virtual void addLight(bool light[MAP_W][MAP_H]) const;
  virtual bool canHaveCorpse() const;
  virtual bool canHaveStaticFeature() const;
  virtual bool canHaveBlood() const;
  virtual bool canHaveGore() const;
  virtual bool canHaveItem() const;
  bool hasBlood() const;
  void setHasBlood(const bool HAS_BLOOD);
  FeatureId getId() const;
  virtual int getDodgeModifier() const;
  int getShockWhenAdjacent() const;
  virtual MaterialType getMaterialType() const;
protected:
  friend class Map;
  Feature(FeatureId id, Pos pos, Engine& engine,
          FeatureSpawnData* spawnData = NULL);

  Pos pos_;
  Engine& eng;
  const FeatureData* const data_;
  bool hasBlood_;
};

class FeatureMob: public Feature {
public:
  Pos getPos()  const {return pos_;}
  int getX()    const {return pos_.x;}
  int getY()    const {return pos_.y;}

  //For smoke etc
  bool shouldBeDeleted() {return shouldBeDeleted_;}

protected:
  friend class FeatureFactory;
  FeatureMob(FeatureId id, Pos pos, Engine& engine,
             FeatureSpawnData* spawnData = NULL) :
    Feature(id, pos, engine), shouldBeDeleted_(false) {
    (void)spawnData;
  }

  bool shouldBeDeleted_;
};

class FeatureStatic: public Feature {
public:
  virtual string getDescr(const bool DEFINITE_ARTICLE) const override;

  void setGoreIfPossible();

  inline TileId getGoreTile() const {return goreTile_;}

  inline char getGoreGlyph()  const {return goreGlyph_;}

  inline void clearGore() {
    goreTile_ = tile_empty;
    goreGlyph_ = ' ';
    hasBlood_ = false;
  }

  virtual void bash(Actor& actorTrying);
  virtual void bash_(Actor& actorTrying);
  virtual bool open() {return false;}
  virtual void disarm();
  virtual void examine();

protected:
  friend class FeatureFactory;
  friend class Map;
  FeatureStatic(FeatureId id, Pos pos, Engine& engine,
                FeatureSpawnData* spawnData = NULL) :
    Feature(id, pos, engine), goreTile_(tile_empty), goreGlyph_(' ') {
    (void)spawnData;
  }

  virtual void triggerTrap(Actor& actor) {(void)actor;}

  TileId goreTile_;
  char goreGlyph_;
};

class Grave: public FeatureStatic {
public:
  ~Grave() {}

  string getDescr(const bool DEFINITE_ARTICLE) const override;

  void setInscription(const string& str) {inscription_ = str;}

  void bump(Actor& actorBumping) override;

private:
  string inscription_;

  friend class FeatureFactory;
  Grave(FeatureId id, Pos pos, Engine& engine) :
    FeatureStatic(id, pos, engine) {}
};

class Stairs: public FeatureStatic {
public:
  ~Stairs() {}

  void bump(Actor& actorBumping) override;

private:
  friend class FeatureFactory;
  Stairs(FeatureId id, Pos pos, Engine& engine) :
    FeatureStatic(id, pos, engine) {}
};

#endif
