#ifndef LOOK_H
#define LOOK_H

#include "CommonTypes.h"

class Engine;
class Actor;
class Item;
class FeatureMob;
class FeatureStatic;
class Feature;

struct Entity {
public:
  Entity() :
    actor(NULL), item(NULL), feature(NULL),
    entityType(entityFeatureStatic) {}

  Entity(Actor* actor_) : actor(actor_), entityType(entityActor) {}

  Entity(Item* item_) : item(item_), entityType(entityItem) {}

  Entity(FeatureMob* feature_);
  Entity(FeatureStatic* feature_);

  const Actor* actor;
  Item* item;
  const Feature* feature;

  EntityType entityType;
};

class Look {
public:
  Look(Engine& engine) : eng(engine) {}

  void markerAtPos(const Pos& pos, const MarkerTask markerTask,
                   const Item* const itemThrown);
  void printExtraActorDescription(const Pos& pos) const;

private:
  Entity entityDescribed;

  void descrBriefActor(const Actor& actor, const MarkerTask markerTask,
                       const Item* const itemThrown) const;
  void descrBriefFeatureMob(const Feature& feature) const;
  void descrBriefFeatureStatic(const Feature& feature) const;
  void descrBriefItem(const Item& item) const;

  Entity getEntityToDescribe(const Pos pos);

  Engine& eng;
};

#endif
