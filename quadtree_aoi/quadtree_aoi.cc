#include "quadtree_aoi/quadtree_aoi.h"

#include <algorithm>
#include <cstring>
#include <queue>
#include <vector>

const int kMaxDegree = 5;
class QuadTreeAOI::QuadTree {
 public:
  struct Box;
  struct QuadTreeNode;

 public:
  QuadTree(float width, float height)
      : root_(new QuadTreeNode(0, true, 0, 0, width, height)) {}
  ~QuadTree() {}

  void Insert(const Unit* unit) { Insert(root_, unit); };
  void Insert(QuadTreeNode* parent, const Unit* unit);
  void Delete();
  void Search();

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
    QuadTreeNode(int depth_, bool leaf_, float x1, float y1, float x2, float y2)
        : depth(depth_), leaf(leaf_), box(Box(x1, y1, x2, y2)) {
      memset(&child_nodes[0], 0, sizeof(child_nodes[0]) * 4);
    }

    ~QuadTreeNode() {}

    int depth;
    bool leaf;
    Box const box;
    std::vector<Unit*> units;
    QuadTreeNode* child_nodes[4];
  };

 private:
  QuadTreeNode* const root_;
};

struct QuadTreeAOI::Unit : AOI::Unit {
  Unit(UnitID id, float x, float y) : AOI::Unit(id, x, y) {}
  ~Unit() {}

  QuadTree::QuadTreeNode* quad_tree_node;
};

void QuadTreeAOI::QuadTree::Insert(QuadTreeNode* parent, const Unit* unit) {
  if (parent->depth >= kMaxDegree ||
      (parent->leaf && parent->units.size() == 0)) {
    parent->units.push_back(const_cast<Unit*>(unit));
    return;
  }

  QuadTreeNode** child_nodes = parent->child_nodes;
  if (parent->leaf) {
    const Box& box = parent->box;
    float mid_x = (box.x1 + box.x2) / 2;
    float mid_y = (box.y1 + box.y2) / 2;
    child_nodes[0] =
        new QuadTreeNode(parent->depth, true, box.x1, mid_y, mid_x, box.y2);
    child_nodes[1] =
        new QuadTreeNode(parent->depth, true, mid_x, mid_y, box.x2, box.y2);
    child_nodes[2] =
        new QuadTreeNode(parent->depth, true, box.x1, box.y1, mid_x, mid_y);
    child_nodes[3] =
        new QuadTreeNode(parent->depth, true, mid_x, box.y1, box.x2, mid_y);
    parent->leaf = false;
  }

  float x = unit->x;
  float y = unit->y;
  for (int i = 0; i < 4; ++i) {
    if (child_nodes[i]->box.Contains(x, y)) {
      Insert(unit);
      break;
    }
  }
}

QuadTreeAOI::QuadTreeAOI(float width, float height, float visible_range,
                         const AOI::Callback& enter_callback,
                         const AOI::Callback& leave_callback)
    : AOI(width, height, visible_range, enter_callback, leave_callback),
      quad_tree_(new QuadTree(width, height)) {}

QuadTreeAOI::~QuadTreeAOI() {}

void QuadTreeAOI::AddUnit(UnitID id, float x, float y) {}

void QuadTreeAOI::UpdateUnit(UnitID id, float x, float y) {}

void QuadTreeAOI::RemoveUnit(UnitID id) {}

AOI::UnitSet QuadTreeAOI::FindNearbyUnit(AOI::Unit*, const float range) const {
  return UnitSet();
}

AOI::Unit* QuadTreeAOI::NewUnit(UnitID id, float x, float y) {
  return nullptr;
}

void QuadTreeAOI::DeleteUnit(AOI::Unit* unit) {}
