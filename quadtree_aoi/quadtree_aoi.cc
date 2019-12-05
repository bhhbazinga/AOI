#include "quadtree_aoi/quadtree_aoi.h"

#include <algorithm>
#include <cstring>
#include <functional>
#include <queue>
#include <vector>

const int kMaxDegree = 5;

class QuadTreeAOI::QuadTree {
 public:
  struct Box;
  struct QuadTreeNode;

 public:
  QuadTree(float width, float height)
      : root_(new QuadTreeNode(0, true, 0, 0, width, height, nullptr)) {}
  ~QuadTree() {}

  QuadTreeNode* Insert(const Unit* unit) { return Insert(root_, unit); };

  void Delete(const Unit* unit);

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

    bool Contains(const Box& other) const {
      return other.x1 >= x1 && other.x2 <= x2 && other.y1 >= y1 &&
             other.y2 <= y2;
    }

    bool Intersects(const Box& other) const {
      float cx1 = std::max(x1, other.x1);
      float cy1 = std::max(y1, other.y1);
      float cx2 = std::min(x2, other.x2);
      float cy2 = std::min(y2, other.y2);
      return Contains(Box(cx1, cy1, cx2, cy2));
    }
  };

  struct QuadTreeNode {
    QuadTreeNode(int depth_, bool leaf_, float x1, float y1, float x2, float y2,
                 QuadTreeNode* parent_)
        : depth(depth_),
          leaf(leaf_),
          box(Box(x1, y1, x2, y2)),
          parent(parent_) {
      memset(&child_nodes[0], 0, sizeof(child_nodes[0]) * 4);
    }

    ~QuadTreeNode() {}
    QuadTreeNode*& top_left() { return child_nodes[0]; }
    QuadTreeNode*& top_right() { return child_nodes[1]; }
    QuadTreeNode*& bottom_left() { return child_nodes[2]; }
    QuadTreeNode*& bottom_right() { return child_nodes[3]; }
    void ForEachChilds(std::function<bool(const QuadTreeNode* child)> func) {
      for (int i = 0; i < 4; ++i) {
        if (!func(child_nodes[i])) {
          break;
        }
      }
    }

    int depth;
    bool leaf;
    Box const box;
    AOI::UnitSet unit_set;
    QuadTreeNode* parent;
    QuadTreeNode* child_nodes[4];
  };

 private:
  QuadTreeNode* Insert(QuadTreeNode* node, const Unit* unit);
  void Search(const QuadTreeNode* node, const Box& box, AOI::UnitSet& unit_set);

  QuadTreeNode* const root_;
};

struct QuadTreeAOI::Unit : AOI::Unit {
  Unit(UnitID id, float x, float y) : AOI::Unit(id, x, y) {}
  ~Unit() {}

  QuadTree::QuadTreeNode* quad_tree_node;
};

QuadTreeAOI::QuadTree::QuadTreeNode* QuadTreeAOI::QuadTree::Insert(
    QuadTreeNode* node, const Unit* unit) {
  AOI::UnitSet& unit_set = node->unit_set;
  if (node->depth >= kMaxDegree) {
    // Reach max degree
    unit_set.insert(const_cast<Unit*>(unit));
    return node;
  }

  if (node->leaf) {
    if (unit_set.empty()) {
      // Have no units
      unit_set.insert(const_cast<Unit*>(unit));
      return node;
    } else {
      if (node->leaf) {
        // Split current node
        const Box& box = node->box;
        float mid_x = (box.x1 + box.x2) / 2;
        float mid_y = (box.y1 + box.y2) / 2;
        node->top_left() = new QuadTreeNode(node->depth, true, box.x1, mid_y,
                                            mid_x, box.y2, node);
        node->top_right() = new QuadTreeNode(node->depth, true, mid_x, mid_y,
                                             box.x2, box.y2, node);
        node->bottom_left() = new QuadTreeNode(node->depth, true, box.x1,
                                               box.y1, mid_x, mid_y, node);
        node->top_right() = new QuadTreeNode(node->depth, true, mid_x, box.y1,
                                             box.x2, mid_y, node);
        node->leaf = false;
        for (auto unit : unit_set) {
          // Reinsert current units
          Insert(node, reinterpret_cast<Unit*>(unit));
        }
        unit_set.clear();
      }
    }
  }

  QuadTreeNode** child_nodes = node->child_nodes;
  for (int i = 0; i < 4; ++i) {
    if (child_nodes[i]->box.Contains(unit->x, unit->y)) {
      return Insert(child_nodes[i], unit);
    }
  }

  assert(false);
  return nullptr;
}

void QuadTreeAOI::QuadTree::Delete(const Unit* unit) {
  QuadTreeNode* node = unit->quad_tree_node;
  AOI::UnitSet& unit_set = node->unit_set;
  unit_set.erase(const_cast<Unit*>(unit));

  QuadTreeNode* parent = node->parent;
  QuadTreeNode** child_nodes = node->child_nodes;
  int size = 0;
  for (int i = 0; i < 4; ++i) {
    size += child_nodes[i]->unit_set.size();
    if (size > 1) {
      return;
    }
  }

  // Merge nodes
  parent->leaf = true;
  for (int i = 0; i < 4; ++i) {
    delete child_nodes[i];
  }
}

void QuadTreeAOI::QuadTree::Search(const QuadTreeNode* node, const Box& box,
                                   AOI::UnitSet& unit_set) {
  if (!node->box.Intersects(box)) {
    return;
  }

  QuadTreeNode* const* child_nodes = node->child_nodes;
  for (int i = 0; i < 4; ++i) {
    Search(child_nodes[i], box, unit_set);
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
  unit->quad_tree_node = quad_tree_->Insert(unit);

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
      std::min(unit->x - range, 0.0f), std::min(unit->y - range, 0.0f),
      std::min(unit->x + range, width), std::min(unit->y + range, height));
  return quad_tree_->Search(box);
}

AOI::Unit* QuadTreeAOI::NewUnit(UnitID id, float x, float y) {
  return new Unit(id, x, y);
}

void QuadTreeAOI::DeleteUnit(AOI::Unit* unit) {
  delete reinterpret_cast<Unit*>(unit);
}
