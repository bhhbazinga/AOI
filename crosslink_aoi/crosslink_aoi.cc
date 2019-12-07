#include "crosslink_aoi/crosslink_aoi.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <random>

// It's almost guaranteed to be logn if the maximum number of nodes range in 0
// to 2^14
const int kMaxLevel = 14;

#define COMPARATOR_DEF(name, field)                                        \
  struct name {                                                            \
    bool operator()(const AOI::Unit* unit, const AOI::Unit* other) const { \
      if (fabs(unit->field - other->field) > 1e-6f) {                      \
        return unit->field < other->field;                                 \
      }                                                                    \
      return unit->id < other->id;                                         \
    }                                                                      \
  }

COMPARATOR_DEF(ComparatorX, x);
COMPARATOR_DEF(ComparatorY, y);

// See also https://github.com/bhhbazinga/SkipList,
// a generic skiplist implementation
class CrosslinkAOI::SkipList {
 public:
  struct SkipNode;

  typedef std::function<bool(const CrosslinkAOI::Unit*,
                             const CrosslinkAOI::Unit*)>
      Comparator;
  SkipList(Comparator&& comp)
      : head_(new SkipNode(kMaxLevel)),
        tail_(new SkipNode(kMaxLevel)),
        compare_(std::move(comp)) {
    for (int l = 0; l < kMaxLevel; ++l) {
      head_->nexts[l] = tail_;
      tail_->prevs[l] = head_;
    }
  }

  ~SkipList() {
    SkipNode* p = head_;
    SkipNode* temp;
    while (nullptr != p) {
      temp = p;
      p = p->nexts[0];
      delete temp;
    }
  }

  SkipList(const SkipList&) = delete;
  SkipList(SkipList&&) = delete;
  SkipList& operator=(const SkipList& other) = delete;
  SkipList& operator=(SkipList&& other) = delete;

  SkipNode* Insert(CrosslinkAOI::Unit* data) {
    SkipNode* new_node = new SkipNode(RandomLevel(), data);
    Insert(new_node);
    return new_node;
  }

  void Insert(SkipNode* new_node) {
    SkipNode* prevs[kMaxLevel];
    FindLastLess(new_node->data, prevs);

    for (int l = 0; l < new_node->level; ++l) {
      new_node->nexts[l] = prevs[l]->nexts[l];
      prevs[l]->nexts[l] = new_node;
      new_node->nexts[l]->prevs[l] = new_node;
      new_node->prevs[l] = prevs[l];
    }
  }

  void Erase(SkipNode* erase_node) {
    int erase_level = erase_node->level;
    for (int l = 0; l < erase_level; ++l) {
      erase_node->prevs[l]->nexts[l] = erase_node->nexts[l];
      erase_node->nexts[l]->prevs[l] = erase_node->prevs[l];
      erase_node->prevs[l] = nullptr;
      erase_node->nexts[l] = nullptr;
    }
  }

  void EraseAndDelete(SkipNode* erase_node) {
    Erase(erase_node);
    delete erase_node;
  }

  SkipNode* Next(const SkipNode* node) const { return node->nexts[0]; }

  SkipNode* Prev(const SkipNode* node) const { return node->prevs[0]; }

  typedef std::function<bool(const Unit* data)> ForeachFunction;
  void ForeachForward(const SkipNode* begin_node,
                      const ForeachFunction& func) const;
  void ForeachBackward(const SkipNode* begin_node,
                       const ForeachFunction& func) const;

  struct SkipNode {
    SkipNode(const int level_)
        : data(nullptr),
          level(level_),
          nexts(new SkipNode*[level]),
          prevs(new SkipNode*[level]) {
      memset(&nexts[0], 0, sizeof(nexts[0]) * level);
      memset(&prevs[0], 0, sizeof(prevs[0]) * level);
    }

    SkipNode(const int level_, const CrosslinkAOI::Unit* data_)
        : data(data_),
          level(level_),
          nexts(new SkipNode*[level]),
          prevs(new SkipNode*[level]) {
      memset(&nexts[0], 0, sizeof(nexts[0]) * level);
      memset(&prevs[0], 0, sizeof(prevs[0]) * level);
    }

    ~SkipNode() {
      delete[] nexts;
      delete[] prevs;
    }

    const CrosslinkAOI::Unit* data;
    int const level;
    SkipNode** nexts;
    SkipNode** prevs;
  };

 private:
  int RandomLevel() const {
    int level = 1;
    while (level < kMaxLevel && (rand() % 2 == 0)) {
      ++level;
    }
    return level;
  }

  bool Equals(const CrosslinkAOI::Unit* data,
              const CrosslinkAOI::Unit* other) const {
    return !compare_(data, other) && !compare_(other, data);
  }

  bool Less(const CrosslinkAOI::Unit* data,
            const CrosslinkAOI::Unit* other) const {
    return compare_(data, other);
  }

  bool Greater(const CrosslinkAOI::Unit* data,
               const CrosslinkAOI::Unit* other) const {
    return !Less(data, other) && !Equals(data, other);
  }

  SkipNode* FindLastLess(const CrosslinkAOI::Unit* data,
                         SkipNode** prevs) const {
    SkipNode* p = head_;
    int level = p->level - 1;
    while (level >= 0) {
      SkipNode* next = p->nexts[level];
      if (tail_ != next && Greater(data, next->data)) {
        p = next;
        level = p->level;
      } else {
        prevs[level] = p;
      }
      --level;
    }
    return p;
  }

  SkipNode* const head_;
  SkipNode* const tail_;
  Comparator const compare_;
};

#define FOR_EACH(field, end)        \
  do {                              \
    const SkipNode* p = begin_node; \
    while (end != p) {              \
      if (func(p->data)) {          \
        p = p->field[0];            \
      } else {                      \
        break;                      \
      }                             \
    }                               \
  } while (0)

void CrosslinkAOI::SkipList::ForeachForward(const SkipNode* begin_node,
                                            const ForeachFunction& func) const {
  FOR_EACH(nexts, tail_);
}

void CrosslinkAOI::SkipList::ForeachBackward(
    const SkipNode* begin_node, const ForeachFunction& func) const {
  FOR_EACH(prevs, head_);
}

struct CrosslinkAOI::Unit : AOI::Unit {
  Unit(UnitID id, float x, float y) : AOI::Unit(id, x, y) {}
  ~Unit() {}

  SkipList::SkipNode* x_skip_node;
  SkipList::SkipNode* y_skip_node;
};

AOI::Unit* CrosslinkAOI::NewUnit(UnitID id, float x, float y) {
  return reinterpret_cast<AOI::Unit*>(new Unit(id, x, y));
}

void CrosslinkAOI::DeleteUnit(AOI::Unit* unit) {
  delete reinterpret_cast<Unit*>(unit);
}

CrosslinkAOI::CrosslinkAOI(float width, float height, float visible_range,
                           const AOI::Callback& enter_callback,
                           const AOI::Callback& leave_callback)
    : AOI(width, height, visible_range, enter_callback, leave_callback),
      x_list_(new SkipList(ComparatorX())),
      y_list_(new SkipList(ComparatorY())) {}

CrosslinkAOI::~CrosslinkAOI() {
  std::vector<UnitID> unit_ids = get_unit_ids();
  for (auto id : unit_ids) {
    RemoveUnit(id);
  }

  delete x_list_;
  delete y_list_;
}

void CrosslinkAOI::AddUnit(UnitID id, float x, float y) {
  ValidatePosition(x, y);

  Unit* unit = reinterpret_cast<Unit*>(NewUnit(id, x, y));
  unit->x_skip_node = x_list_->Insert(unit);
  unit->y_skip_node = y_list_->Insert(unit);

  OnAddUnit(unit);
}

void CrosslinkAOI::UpdateUnit(UnitID id, float x, float y) {
  ValidatePosition(x, y);

  Unit* unit = reinterpret_cast<Unit*>(get_unit(id));
  SkipList::SkipNode* x_skip_node = unit->x_skip_node;
  SkipList::SkipNode* y_skip_node = unit->y_skip_node;
  x_list_->Erase(x_skip_node);
  y_list_->Erase(y_skip_node);
  unit->x = x;
  unit->y = y;
  x_list_->Insert(x_skip_node);
  y_list_->Insert(y_skip_node);

  OnUpdateUnit(unit);
}

void CrosslinkAOI::RemoveUnit(UnitID id) {
  Unit* unit = reinterpret_cast<Unit*>(get_unit(id));

  x_list_->EraseAndDelete(unit->x_skip_node);
  y_list_->EraseAndDelete(unit->y_skip_node);

  OnRemoveUnit(unit);
}

AOI::UnitSet CrosslinkAOI::FindNearbyUnit(const AOI::Unit* unit,
                                          float range) const {
  AOI::UnitSet x_set;
  auto x_for_func = [&](const Unit* other) {
    if (fabs(unit->x - other->x) <= range) {
      x_set.insert(const_cast<Unit*>(other));
      return true;
    }
    return false;
  };

  SkipList::SkipNode* x_skip_node =
      reinterpret_cast<Unit*>(const_cast<AOI::Unit*>(unit))->x_skip_node;
  x_list_->ForeachForward(x_list_->Next(x_skip_node), x_for_func);
  x_list_->ForeachBackward(x_list_->Prev(x_skip_node), x_for_func);

  AOI::UnitSet res_set;
  auto y_for_func = [&](const Unit* other) {
    if (fabs(unit->y - other->y) <= range) {
      if (x_set.find(const_cast<Unit*>(other)) != x_set.end()) {
        res_set.insert(const_cast<Unit*>(other));
      }
      return true;
    }
    return false;
  };

  SkipList::SkipNode* y_skip_node =
      reinterpret_cast<Unit*>(const_cast<AOI::Unit*>(unit))->y_skip_node;
  y_list_->ForeachForward(y_list_->Next(y_skip_node), y_for_func);
  y_list_->ForeachBackward(y_list_->Prev(y_skip_node), y_for_func);
  return res_set;
}
