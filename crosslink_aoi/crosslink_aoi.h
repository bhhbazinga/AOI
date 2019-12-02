#ifndef CROSSLINK_AOI_H
#define CROSSLINK_AOI_H

#include "aoi.h"
#include "crosslink_aoi/crosslink_aoi.h"

// The cross-link model is optimized using skiplist
class CrosslinkAOI : public AOI {
 private:
  class SkipList;
  struct Unit;

 public:
  CrosslinkAOI(const float width, const float height, const float visible_range,
               const AOI::Callback& enter_callback,
               const AOI::Callback& leave_callback);

  ~CrosslinkAOI() override;

  void AddUnit(const int id, const float x, const float y) override;
  void UpdateUnit(const int id, const float x, const float y) override;
  void RemoveUnit(const int id) override;

 protected:
  AOI::UnitSet FindNearbyUnit(AOI::Unit*, const float range) const override;

 private:
  AOI::Unit* NewUnit(const int id, const float x, const float y) override;
  void DeleteUnit(AOI::Unit* unit) override;

  SkipList* x_list_;
  SkipList* y_list_;
};
#endif  // CROSSLINK_AOI_H