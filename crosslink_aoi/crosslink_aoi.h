#ifndef CROSSLINK_AOI_H
#define CROSSLINK_AOI_H

#include "../aoi.h"
#include "skiplist.h"

// The cross-link model is optimized using skiplist
class CrosslinkAOI : public AOI {
 public:
  CrosslinkAOI(const float width, const float height,
               const float visible_range = kDefaultVisibleRange)
      : AOI(width, height, visible_range) {}
  ~CrosslinkAOI() override {}

  AOI::Unit* AddUnit(int id, float x, float y, AOI::Callback enter_callback_,
                     AOI::Callback leave_callback_) override;
  void UpdateUnit(int id, float x, float y) override;
  void RemoveUnit(int id) override;

 protected:
  AOI::UnitSet FindNearbyUnit(AOI::Unit*, const float range) const override;

 private:
  struct ComparatorX {
    bool operator()(const AOI::Unit* const unit,
                    const AOI::Unit* const other) const {
      if (abs(unit->x - other->x) > 1e-6) {
        return unit->x < other->x;
      }
      return unit->id < other->id;
    }
  };

  struct ComparatorY {
    bool operator()(const AOI::Unit* const unit,
                    const AOI::Unit* const other) const {
      if (abs(unit->y - other->y) > 1e-6) {
        return unit->y < other->y;
      }
      return unit->id < other->id;
    }
  };

  SkipList<Unit*, ComparatorX> x_list_;
  SkipList<Unit*, ComparatorY> y_list_;
};

#endif  // CROSSLINK_AOI_H