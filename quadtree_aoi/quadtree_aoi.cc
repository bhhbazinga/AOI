#include "quadtree_aoi/quadtree_aoi.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <queue>
#include <vector>

#define Log(fmt, ...)                  \
  do {                                 \
    fprintf(stderr, fmt, __VA_ARGS__); \
  } while (0)

const int kMaxDegree = 5;

class QuadTreeAOI::QuadTree {
 public:
  struct Box;
  struct QuadTreeNode;

 public:
  QuadTree(float width, float height)
      : root_(new QuadTreeNode(0, true, 0, 0, width, height, nullptr)) {}
  ~QuadTree() { Delete(root_); }

  void Insert(Unit* unit) { return Insert(root_, unit); };

  void Delete(Unit* unit);

  AOI::UnitSet Search(const Box& box) {
    AOI::UnitSet unit_set;
    Search(root_, box, unit_set);
    return unit_set;
  };

  struct Box {
    Box(float x1_, float y1_, float x2_, float y2_)
        : x1(x1_), y1(y1_), x2(x2_), y2(y2_) {}
    ~Box() {}

    Box(const Box& other) = default;
    Box(Box&& other) = default;

    float x1, y1;
    float x2, y2;

    bool Contains(float x, float y) const {
      return x >= x1 && x <= x2 && y >= y1 && y <= y2;
    }

    bool Intersects(const Box& other) const {
      return std::max(x1, other.x1) < std::min(x2, other.x2) &&
             std::max(y1, other.y1) < std::min(y2, other.y2);
    }
  };

  struct QuadTreeNode {
    QuadTreeNode(int depth_, bool leaf_, float x1, float y1, float x2, float y2,
                 QuadTreeNode* parent_)
        : depth(depth_),
          leaf(leaf_),
          box(Box(x1, y1, x2, y2)),
          parent(parent_) {
      // Log("%sQuadTreeNode\n", ">>>");
      memset(&child_nodes[0], 0, sizeof(child_nodes[0]) * 4);
    }

    ~QuadTreeNode() {
      // Log("%s~QuadTreeNode\n", ">>>");
    }
    QuadTreeNode*& top_left() { return child_nodes[0]; }
    QuadTreeNode*& top_right() { return child_nodes[1]; }
    QuadTreeNode*& bottom_left() { return child_nodes[2]; }
    QuadTreeNode*& bottom_right() { return child_nodes[3]; }

    int depth;
    bool leaf;
    Box const box;
    AOI::UnitSet unit_set;
    QuadTreeNode* parent;
    QuadTreeNode* child_nodes[4];
  };

 private:
  void Insert(QuadTreeNode* node, Unit* unit);
  void Search(const QuadTreeNode* node, const Box& box, AOI::UnitSet& unit_set);
  void Delete(QuadTreeNode* node);

  QuadTreeNode* const root_;
};

struct QuadTreeAOI::Unit : AOI::Unit {
  Unit(UnitID id, float x, float y) : AOI::Unit(id, x, y) {}
  ~Unit() {}

  QuadTree::QuadTreeNode* quad_tree_node;
};

void QuadTreeAOI::QuadTree::Insert(QuadTreeNode* node, Unit* unit) {
  AOI::UnitSet& unit_set = node->unit_set;
  QuadTreeNode** child_nodes = node->child_nodes;

  if (node->leaf) {
    if (unit_set.empty() || node->depth >= kMaxDegree) {
      // Have no units
      unit_set.insert(const_cast<Unit*>(unit));
      unit->quad_tree_node = node;
      // Log("insert, %d(%f,%f), node=%p\n", unit->id, unit->x, unit->y, node);
      return;
    } else {
      // Split current node
      const Box& box = node->box;
      float mid_x = (box.x1 + box.x2) / 2;
      float mid_y = (box.y1 + box.y2) / 2;
      node->top_left() = new QuadTreeNode(node->depth + 1, true, box.x1, mid_y,
                                          mid_x, box.y2, node);
      node->top_right() = new QuadTreeNode(node->depth + 1, true, mid_x, mid_y,
                                           box.x2, box.y2, node);
      node->bottom_left() = new QuadTreeNode(node->depth + 1, true, box.x1,
                                             box.y1, mid_x, mid_y, node);
      node->bottom_right() = new QuadTreeNode(node->depth + 1, true, mid_x,
                                              box.y1, box.x2, mid_y, node);
      node->leaf = false;
      for (auto old_unit : unit_set) {
        Insert(node, reinterpret_cast<Unit*>(old_unit));
      }
      unit_set.clear();
    }
  }

  for (int i = 0; i < 4; ++i) {
    if (child_nodes[i]->box.Contains(unit->x, unit->y)) {
      Insert(child_nodes[i], unit);
      break;
    }
  }
}

void QuadTreeAOI::QuadTree::Delete(Unit* unit) {
  QuadTreeNode* node = unit->quad_tree_node;
  AOI::UnitSet& unit_set = node->unit_set;
  unit_set.erase(const_cast<Unit*>(unit));
  unit->quad_tree_node = nullptr;
}

void QuadTreeAOI::QuadTree::Delete(QuadTreeNode* node) {
  if (nullptr == node) {
    return;
  }

  QuadTreeNode** child_nodes = node->child_nodes;
  for (int i = 0; i < 4; ++i) {
    Delete(child_nodes[i]);
  }
  delete node;
}

void QuadTreeAOI::QuadTree::Search(const QuadTreeNode* node, const Box& box,
                                   AOI::UnitSet& unit_set) {
  if (!node->box.Intersects(box)) {
    return;
  }

  if (!node->leaf) {
    QuadTreeNode* const* child_nodes = node->child_nodes;
    for (int i = 0; i < 4; ++i) {
      Search(child_nodes[i], box, unit_set);
    }
    return;
  }

  for (auto unit : node->unit_set) {
    if (box.Contains(unit->x, unit->y)) {
      unit_set.insert(unit);
    }
  }
}

QuadTreeAOI::QuadTreeAOI(float width, float height, float visible_range,
                         const AOI::Callback& enter_callback,
                         const AOI::Callback& leave_callback)
    : AOI(width, height, visible_range, enter_callback, leave_callback),
      quad_tree_(new QuadTree(width, height)) {}

QuadTreeAOI::~QuadTreeAOI() {
  std::vector<UnitID> unit_ids = get_unit_ids();
  for (auto id : unit_ids) {
    RemoveUnit(id);
  }

  delete quad_tree_;
}

void QuadTreeAOI::AddUnit(UnitID id, float x, float y) {
  ValidatePosition(x, y);

  Unit* unit = reinterpret_cast<Unit*>(NewUnit(id, x, y));
  quad_tree_->Insert(unit);

  OnAddUnit(unit);
}

void QuadTreeAOI::UpdateUnit(UnitID id, float x, float y) {
  ValidatePosition(x, y);

  Unit* unit = reinterpret_cast<Unit*>(get_unit(id));
  quad_tree_->Delete(unit);
  unit->x = x;
  unit->y = y;
  quad_tree_->Insert(unit);

  OnUpdateUnit(unit);
}

void QuadTreeAOI::RemoveUnit(UnitID id) {
  Unit* unit = reinterpret_cast<Unit*>(get_unit(id));
  quad_tree_->Delete(unit);

  OnRemoveUnit(unit);
}

AOI::UnitSet QuadTreeAOI::FindNearbyUnit(const AOI::Unit* unit,
                                         float range) const {
  float width = get_width();
  float height = get_height();
  QuadTree::Box box(
      std::max(unit->x - range, 0.0f), std::max(unit->y - range, 0.0f),
      std::min(unit->x + range, width), std::min(unit->y + range, height));
  UnitSet unit_set = quad_tree_->Search(box);
  unit_set.erase(const_cast<AOI::Unit*>(unit));
  return unit_set;
}

AOI::Unit* QuadTreeAOI::NewUnit(UnitID id, float x, float y) {
  return new Unit(id, x, y);
}

void QuadTreeAOI::DeleteUnit(AOI::Unit* unit) {
  delete reinterpret_cast<Unit*>(unit);
}
