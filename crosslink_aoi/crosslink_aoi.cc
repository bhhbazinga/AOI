#include "crosslink_aoi/crosslink_aoi.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <random>

// It's almost guaranteed to be logn if the maximum number of nodes range in 0
// to 2^14
const int kMaxLevel = 14;

#define COMPARATOR_DEF(name, field)                       \
  struct name {                                           \
    bool operator()(const AOI::Unit* const unit,          \
                    const AOI::Unit* const other) const { \
      if (abs(unit->field - other->field) > 1e-6f) {      \
        return unit->field < other->field;                \
      }                                                   \
      return unit->id < other->id;                        \
    }                                                     \
  }

COMPARATOR_DEF(ComparatorX, x);
COMPARATOR_DEF(ComparatorY, y);

class CrosslinkAOI::SkipList {
 private:
  struct SkipNode;
  friend CrosslinkAOI;

 public:
  typedef std::function<bool(const CrosslinkAOI::Unit* const,
                             const CrosslinkAOI::Unit* const)>
      Comparator;

  SkipList() : head_(new SkipNode(kMaxLevel)) {}

  SkipList(Comparator comp) : head_(new SkipNode(kMaxLevel)), compare_(comp) {
    memset(&head_->nexts[0], 0, sizeof(head_->nexts[0]) * kMaxLevel);
    memset(&head_->prevs[0], 0, sizeof(head_->prevs[0]) * kMaxLevel);
  }

  ~SkipList() {
    SkipNode* p = head_;
    SkipNode* temp;
    while (p) {
      temp = p;
      p = p->nexts[0];
      delete temp;
    }
  }

  SkipList(const SkipList&) = delete;
  SkipList(SkipList&&) = delete;
  SkipList& operator=(const SkipList& other) = delete;
  SkipList& operator=(SkipList&& other) = delete;

  SkipNode* Insert(CrosslinkAOI::Unit* const data) {
    SkipNode* new_node = new SkipNode(RandomLevel(), data);
    InsertForward(head_, new_node);
    return new_node;
  }

  void InsertForward(SkipNode* const begin_node, SkipNode* const new_node) {
    SkipNode* prevs[kMaxLevel];
    FindLastLessForward(begin_node, new_node->data, prevs);

    int new_level = new_node->level;
    for (int l = 0; l < new_level; ++l) {
      new_node->nexts[l] = prevs[l]->nexts[l];
      prevs[l]->nexts[l] = new_node;
      new_node->nexts[l]->prevs[l] = new_node;
    }
  }

  void InsertBackward(SkipNode* const begin_node, SkipNode* const new_node) {
    SkipNode* prevs[kMaxLevel];
    FindLastGreaterBackward(begin_node, new_node->data, prevs);

    int new_level = new_node->level;
    for (int l = 0; l < new_level; ++l) {
      new_node->prevs[l] = prevs[l]->prevs[l];
      prevs[l]->prevs[l] = new_node;
      new_node->prevs[l]->nexts[l] = new_node;
    }
  }

  void Erase(SkipNode* const erase_node) {
    int erase_level = erase_node->level;
    for (int l = 0; l < erase_level; ++l) {
      erase_node->prevs[l] = erase_node->nexts[l];
      erase_node->nexts[l]->prevs[l] = erase_node->prevs[l];
    }
  }

  typedef std::function<bool(Unit* const unit)> ForeachFunction;
  inline void ForeachForward(SkipNode* const begin_node, ForeachFunction func);
  inline void ForeachBackward(SkipNode* const begin_node, ForeachFunction func);

 private:
  int RandomLevel() const {
    int level = 1;
    while (level < kMaxLevel && (rand() % 2 == 0)) {
      ++level;
    }
    return level;
  }

  bool Equals(CrosslinkAOI::Unit* const data,
              CrosslinkAOI::Unit* const other) const {
    return !compare_(data, other) && !compare_(other, data);
  }

  bool Less(CrosslinkAOI::Unit* const data,
            CrosslinkAOI::Unit* const other) const {
    return compare_(data, other);
  }

  bool Greater(CrosslinkAOI::Unit* const data,
               CrosslinkAOI::Unit* const other) const {
    return !Less(data, other) && !Equals(data, other);
  }

#define FIND(field, compare)                              \
  do {                                                    \
    SkipNode* p = begin_node;                             \
    int level = p->level - 1;                             \
    while (level >= 0) {                                  \
      SkipNode* next = p->field[level];                   \
      if (nullptr != next && compare(data, next->data)) { \
        p = next;                                         \
        level = p->level;                                 \
      } else {                                            \
        prevs[level] = p;                                 \
      }                                                   \
      --level;                                            \
    }                                                     \
    return p;                                             \
  } while (0)

  SkipNode* FindLastLessForward(SkipNode* const begin_node,
                                CrosslinkAOI::Unit* const data,
                                SkipNode** const prevs) const {
    FIND(nexts, Greater);
  }

  SkipNode* FindLastGreaterBackward(SkipNode* const begin_node,
                                    CrosslinkAOI::Unit* const data,
                                    SkipNode** const prevs) const {
    FIND(prevs, Less);
  }

  struct SkipNode {
    SkipNode(const int level_)
        : data(nullptr),
          level(level_),
          nexts(new SkipNode*[level]),
          prevs(new SkipNode*[level]) {}

    SkipNode(const int level_, CrosslinkAOI::Unit* const data_)
        : data(data_),
          level(level_),
          nexts(new SkipNode*[level]),
          prevs(new SkipNode*[level]) {}

    ~SkipNode() {
      delete[] nexts;
      delete[] prevs;
    }

    CrosslinkAOI::Unit* const data;
    int const level;
    SkipNode** const nexts;
    SkipNode** const prevs;
  };

  SkipNode* const head_;
  Comparator const compare_;
};

#define FOR_EACH(field)             \
  do {                              \
    const SkipNode* p = begin_node; \
    while (nullptr != p) {          \
      if (func(p->data)) {          \
        p = p->field[0];            \
      } else {                      \
        break;                      \
      }                             \
    }                               \
  } while (0)

inline void CrosslinkAOI::SkipList::ForeachForward(SkipNode* const begin_node,
                                                   ForeachFunction func) {
  FOR_EACH(nexts);
}

inline void CrosslinkAOI::SkipList::ForeachBackward(SkipNode* const begin_node,
                                                    ForeachFunction func) {
  FOR_EACH(prevs);
}

struct CrosslinkAOI::Unit : AOI::Unit {
  Unit(const int id, const float x, const float y) : AOI::Unit(id, x, y) {}
  ~Unit() {}

  SkipList::SkipNode* x_skip_node;
  SkipList::SkipNode* y_skip_node;
};

inline AOI::Unit* CrosslinkAOI::NewUnit(const int id, const float x,
                                        const float y) {
  return reinterpret_cast<AOI::Unit*>(new Unit(id, x, y));
}

CrosslinkAOI::CrosslinkAOI(const float width, const float height,
                           const float visible_range,
                           const AOI::Callback& enter_callback,
                           const AOI::Callback& leave_callback)
    : AOI(width, height, visible_range, enter_callback, leave_callback),
      x_list_(new SkipList(ComparatorX())),
      y_list_(new SkipList(ComparatorY())) {}

CrosslinkAOI::~CrosslinkAOI() {
  delete x_list_;
  delete y_list_;
}

inline AOI::Unit* CrosslinkAOI::AddUnit(int id, float x, float y) {
  AOI::Unit* const unit = AOI::AddUnit(id, x, y);
  reinterpret_cast<Unit*>(unit)->x_skip_node =
      x_list_->Insert(reinterpret_cast<Unit*>(unit));
  reinterpret_cast<Unit*>(unit)->y_skip_node =
      y_list_->Insert(reinterpret_cast<Unit*>(unit));

  AOI::UnitSet enter_set = FindNearbyUnit(unit, visible_range_);
  NotifyEnter(unit, enter_set);

  unit->subscribe_set = std::move(enter_set);
  return unit;
}

#define UPDATE(field)                                                \
  do {                                                               \
    SkipList::SkipNode* field##_skip_node = unit->field##_skip_node; \
                                                                     \
    if (field < old_##field) {                                       \
      SkipList::SkipNode* begin = field##_skip_node->nexts[0];       \
      if (nullptr != begin) {                                        \
        field##_list_->Erase(field##_skip_node);                     \
        field##_list_->InsertBackward(begin, field##_skip_node);     \
      } else {                                                       \
        begin = field##_skip_node->prevs[0];                         \
        if (nullptr != begin && field < begin->data->field) {        \
          field##_list_->Erase(field##_skip_node);                   \
          field##_list_->InsertBackward(begin, field##_skip_node);   \
        }                                                            \
      }                                                              \
    } else {                                                         \
      SkipList::SkipNode* begin = field##_skip_node->prevs[0];       \
      if (nullptr != begin) {                                        \
        field##_list_->Erase(field##_skip_node);                     \
        field##_list_->InsertForward(begin, field##_skip_node);      \
      } else {                                                       \
        begin = field##_skip_node->nexts[0];                         \
        if (nullptr != begin && field > begin->data->field) {        \
          field##_list_->Erase(field##_skip_node);                   \
          field##_list_->InsertForward(begin, field##_skip_node);    \
        }                                                            \
      }                                                              \
    }                                                                \
  } while (0)

void CrosslinkAOI::UpdateUnit(const int id, const float x, const float y) {
  AOI::UpdateUnit(id, x, y);
  Unit* const unit = reinterpret_cast<Unit*>(get_unit(id));
  const float old_x = unit->x;
  const float old_y = unit->y;
  unit->x = x;
  unit->y = y;

  UPDATE(x);
  UPDATE(y);

  const AOI::UnitSet& old_set = unit->subscribe_set;
  AOI::UnitSet new_set =
      FindNearbyUnit(reinterpret_cast<AOI::Unit*>(unit), visible_range_);
  AOI::UnitSet move_set = Intersection(old_set, new_set);
  AOI::UnitSet enter_set = Difference(new_set, move_set);
  AOI::UnitSet leave_set = Difference(old_set, new_set);
  unit->subscribe_set = std::move(new_set);

  NotifyAll(reinterpret_cast<AOI::Unit*>(unit), enter_set, leave_set);
}

AOI::UnitSet CrosslinkAOI::FindNearbyUnit(AOI::Unit* unit,
                                          const float range) const {
  AOI::UnitSet x_set;
  auto x_for_func = [&](Unit* const other) {
    if (abs(unit->x - other->x) < 1e-6f) {
      x_set.insert(reinterpret_cast<AOI::Unit*>(other));
      return true;
    }
    return false;
  };
  x_list_->ForeachForward(reinterpret_cast<Unit*>(unit)->x_skip_node->nexts[0],
                          x_for_func);
  x_list_->ForeachBackward(reinterpret_cast<Unit*>(unit)->x_skip_node->prevs[0],
                           x_for_func);

  AOI::UnitSet res_set;
  auto y_for_func = [&](Unit* const other) {
    if (x_set.find(other) != x_set.end() && abs(unit->y - other->y) < 1e-6f) {
      res_set.insert(reinterpret_cast<AOI::Unit*>(other));
      return true;
    }
    return false;
  };

  y_list_->ForeachForward(reinterpret_cast<Unit*>(unit)->y_skip_node->nexts[0],
                          y_for_func);
  y_list_->ForeachBackward(reinterpret_cast<Unit*>(unit)->y_skip_node->nexts[0],
                           y_for_func);
  return res_set;
}
