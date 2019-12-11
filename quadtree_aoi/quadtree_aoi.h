
#ifndef QUADTREE_AOI_H
#define QUADTREE_AOI_H

#include "aoi.h"

class QuadTreeAOI : public AOI {
 private:
  class QuadTree;
  struct Unit;

 public:
  QuadTreeAOI(float width, float height, float visible_range,
              const AOI::Callback& enter_callback,
              const AOI::Callback& leave_callback);
  ~QuadTreeAOI() override;

  void AddUnit(UnitID id, float x, float y) override;
  void UpdateUnit(UnitID id, float x, float y) override;
  void RemoveUnit(UnitID id) override;

 protected:
  AOI::UnitSet FindNearbyUnit(const AOI::Unit* unit,
                              float range) const override;

 private:
  AOI::Unit* NewUnit(UnitID id, float x, float y) override;
  void DeleteUnit(AOI::Unit* unit) override;

  QuadTree* quad_tree_;
};
#endif  // QUADTREE_AOI_H