#ifndef AOI_H
#define AOI_H

#include <cassert>
#include <functional>
#include <unordered_map>
#include <unordered_set>

class AOI {
 public:
  struct Unit;

  typedef int UnitID;
  typedef std::unordered_set<Unit*> UnitSet;
  typedef std::unordered_map<int, Unit*> UnitMap;
  struct Unit {
    Unit(UnitID id_, float x_, float y_) : id(id_), x(x_), y(y_) {}

    ~Unit(){};

    void Subscribe(const Unit* other) {
      subscribe_set.insert(const_cast<Unit*>(other));
    }
    void UnSubscribe(const Unit* other) {
      subscribe_set.erase(const_cast<Unit*>(other));
    }

    UnitID id;
    float x;
    float y;
    UnitSet subscribe_set;
  };

 public:
  typedef std::function<void(int, int)> Callback;

  AOI(float width, float height, float visible_range, Callback enter_callback,
      Callback leave_callback)
      : width_(width),
        height_(height),
        visible_range_(visible_range),
        enter_callback_(enter_callback),
        leave_callback_(leave_callback) {
    assert(width_ >= 0);
    assert(height_ >= 0);
    assert(visible_range >= 0);

    assert(nullptr != enter_callback);
    assert(nullptr != leave_callback);
  }

  virtual ~AOI(){};

  AOI(const AOI&) = delete;
  AOI(AOI&&) = delete;
  AOI& operator=(const AOI&) = delete;
  AOI& operator=(AOI&&) = delete;

  // Update when unit moved
  // id is a custom integer
  virtual void UpdateUnit(UnitID id, float x, float y) = 0;

  // Remove unit from AOI
  // id is a custom integer
  virtual void RemoveUnit(UnitID id) = 0;

  // Add unit to AOI
  // id is a custom integer
  virtual void AddUnit(UnitID id, float x, float y) = 0;

  // Find units in range near the given id, and exclude id itself
  std::unordered_set<int> FindNearbyUnit(UnitID id, float range) const {
    Unit* unit = get_unit(id);
    UnitSet unit_set = FindNearbyUnit(unit, range);
    std::unordered_set<int> id_set;
    for (const auto& unit : unit_set) {
      id_set.insert(unit->id);
    }
    return id_set;
  };

  // Find the ids of units in the subscribe set of given id
  std::unordered_set<int> GetSubScribeSet(UnitID id) const {
    Unit* unit = get_unit(id);
    UnitSet subscribe_set = unit->subscribe_set;
    std::unordered_set<int> id_set;
    for (const auto& unit : subscribe_set) {
      id_set.insert(unit->id);
    }
    return id_set;
  };

  const float& get_width() const { return width_; }
  const float& get_height() const { return height_; }

 protected:
  virtual UnitSet FindNearbyUnit(const Unit* unit, float range) const = 0;
  virtual Unit* NewUnit(UnitID id, float x, float y) = 0;
  virtual void DeleteUnit(Unit* unit) = 0;

  void ValidatePosition(float x, float y) {
    assert(x <= width_ && y <= height_);
  }

  Unit* get_unit(UnitID id) const {
    Unit* unit = unit_map_[id];
    assert(nullptr != unit);
    return unit;
  }

  const std::unordered_map<int, Unit*>& get_unit_map() const {
    return unit_map_;
  }

  std::vector<UnitID> get_unit_ids() const {
    const AOI::UnitMap& unit_map = get_unit_map();
    std::vector<UnitID> unit_ids(unit_map.size());

    int i = 0;
    for (const auto& pair : unit_map) {
      unit_ids[i++] = pair.first;
    }
    return unit_ids;
  }

  UnitSet Intersection(const UnitSet& set, const UnitSet& other) const {
    UnitSet res;
    for (const auto& unit : set) {
      if (other.find(unit) != other.end()) {
        res.insert(unit);
      }
    }
    return res;
  }

  UnitSet Difference(const UnitSet& set, const UnitSet& other) const {
    UnitSet res;
    for (const auto& unit : set) {
      if (other.find(unit) == other.end()) {
        res.insert(unit);
      }
    }
    return res;
  }

  void NotifyEnter(Unit* unit, const UnitSet& enter_set) const {
    for (const auto& other : enter_set) {
      enter_callback_(other->id, unit->id);
      other->Subscribe(unit);
      enter_callback_(unit->id, other->id);
      unit->Subscribe(other);
    }
  }

  void NotifyLeave(Unit* unit, const UnitSet& leave_set) const {
    for (const auto& other : leave_set) {
      leave_callback_(other->id, unit->id);
      other->UnSubscribe(unit);
      leave_callback_(unit->id, other->id);
      unit->UnSubscribe(other);
    }
  }

  void OnAddUnit(Unit* unit) {
    unit_map_.insert(std::pair(unit->id, unit));

    UnitSet enter_set = FindNearbyUnit(unit, visible_range_);
    NotifyEnter(unit, enter_set);
    unit->subscribe_set = std::move(enter_set);
  }

  void OnUpdateUnit(Unit* unit) {
    const UnitSet& old_set = unit->subscribe_set;
    UnitSet new_set =
        FindNearbyUnit(reinterpret_cast<Unit*>(unit), visible_range_);
    UnitSet move_set = Intersection(old_set, new_set);
    UnitSet enter_set = Difference(new_set, move_set);
    UnitSet leave_set = Difference(old_set, new_set);
    unit->subscribe_set = std::move(new_set);

    NotifyEnter(reinterpret_cast<Unit*>(unit), enter_set);
    NotifyLeave(reinterpret_cast<Unit*>(unit), leave_set);
  }

  void OnRemoveUnit(Unit* unit) {
    const UnitSet& subscribe_set = unit->subscribe_set;
    NotifyLeave(unit, UnitSet(subscribe_set));
    unit_map_.erase(unit->id);
    DeleteUnit(unit);
  }

 private:
  float width_;
  float height_;
  float visible_range_;
  mutable UnitMap unit_map_;
  Callback enter_callback_;
  Callback leave_callback_;
};

#endif  // AOI_H