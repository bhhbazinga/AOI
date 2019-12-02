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

  inline AOI::Unit* AddUnit(const int id, const float x,
                            const float y) override;
  void UpdateUnit(const int id, const float x, const float y) override;
  void RemoveUnit(const int id) override {
    AOI::Unit* unit = get_unit(id);
    const AOI::UnitSet& subscribe_set = unit->subscribe_set;
    NotifyLeave(unit, subscribe_set);

    AOI::RemoveUnit(id);
  }

 protected:
  inline AOI::UnitSet FindNearbyUnit(AOI::Unit*,
                                     const float range) const override;

 private:
  AOI::Unit* NewUnit(const int id, const float x, const float y) override;

  SkipList* x_list_;
  SkipList* y_list_;
};
#endif  // CROSSLINK_AOI_H