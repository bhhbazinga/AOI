#include <cstring>

#include "quadtree_aoi/quadtree_aoi.h"

const int kMaxDegree = 5;

class QuadTreeAOI::QuadTree {
 public:
  struct Box;
  struct QuadTreeNode;

 public:
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

  QuadTree(float width, float height)
      : root_(new QuadTreeNode(0, Box(0, 0, width, height), nullptr)) {}
  ~QuadTree() { Destruct(root_); }

  void Insert(Unit* unit) { return Insert(root_, unit); };

  AOI::UnitSet Search(const Box& box) const {
    AOI::UnitSet unit_set;
    Search(root_, box, unit_set);
    return unit_set;
  };

  void Delete(Unit* unit);

  struct QuadTreeNode {
    QuadTreeNode(int depth_, Box box_, QuadTreeNode* parent_);
    ~QuadTreeNode();

    QuadTreeNode*& top_left() { return child_nodes[0]; }
    QuadTreeNode*& top_right() { return child_nodes[1]; }
    QuadTreeNode*& bottom_left() { return child_nodes[2]; }
    QuadTreeNode*& bottom_right() { return child_nodes[3]; }

    void Insert(Unit* insert_unit);
    void Delete(Unit* delete_unit);
    bool Empty();

    void Foreach(std::function<bool(QuadTreeNode* node)> func) {
      for (int i = 0; i < 4; ++i) {
        if (!func(child_nodes[i])) {
          break;
        }
      }
    }

    int depth;
    bool leaf;
    Box const box;
    Unit* head;
    Unit* tail;
    QuadTreeNode* parent;
    QuadTreeNode* child_nodes[4];
  };

 private:
  void Insert(QuadTreeNode* node, Unit* unit);
  void Search(const QuadTreeNode* node, const Box& box,
              AOI::UnitSet& unit_set) const;
  void Destruct(QuadTreeNode* node) {
    if (nullptr == node) {
      return;
    }

    node->Foreach([this](QuadTreeNode* child_node) {
      Destruct(child_node);
      return true;
    });

    delete node;
  }

  QuadTreeNode* const root_;
};

struct QuadTreeAOI::Unit : AOI::Unit {
  Unit(UnitID id, float x, float y)
      : AOI::Unit(id, x, y),
        next(nullptr),
        prev(nullptr),
        quad_tree_node(nullptr) {}
  ~Unit() {}

  // Link to other units in the same node
  QuadTreeAOI::Unit* next;
  QuadTreeAOI::Unit* prev;
  QuadTree::QuadTreeNode* quad_tree_node;
};

QuadTreeAOI::QuadTree::QuadTreeNode::QuadTreeNode(int depth_, Box box_,
                                                  QuadTreeNode* parent_)
    : depth(depth_),
      leaf(true),
      box(box_),
      head(new Unit(0, 0, 0)),
      tail(new Unit(0, 0, 0)),
      parent(parent_) {
  memset(&child_nodes[0], 0, sizeof(child_nodes[0]) * 4);
  head->next = tail;
  tail->prev = head;
  head->prev = nullptr;
  tail->next = nullptr;
}

QuadTreeAOI::QuadTree::QuadTreeNode::~QuadTreeNode() {
  assert(Empty());
  delete head;
  delete tail;
}

void QuadTreeAOI::QuadTree::QuadTreeNode::Insert(Unit* insert_unit) {
  insert_unit->next = head->next;
  insert_unit->next->prev = insert_unit;
  insert_unit->prev = head;
  head->next = insert_unit;
}

void QuadTreeAOI::QuadTree::QuadTreeNode::Delete(Unit* delete_unit) {
  delete_unit->prev->next = delete_unit->next;
  delete_unit->next->prev = delete_unit->prev;
  delete_unit->prev = delete_unit->next = nullptr;
}

bool QuadTreeAOI::QuadTree::QuadTreeNode::Empty() { return head->next == tail; }

void QuadTreeAOI::QuadTree::Insert(QuadTreeNode* node, Unit* unit) {
  if (node->leaf) {
    if (node->Empty() || node->depth >= kMaxDegree) {
      // Have no units
      node->Insert(unit);
      unit->quad_tree_node = node;
      return;
    } else {
      // Split current node
      const Box& box = node->box;
      float mid_x = (box.x1 + box.x2) / 2;
      float mid_y = (box.y1 + box.y2) / 2;
      node->top_left() = new QuadTreeNode(
          node->depth + 1, Box(box.x1, mid_y, mid_x, box.y2), node);
      node->top_right() = new QuadTreeNode(
          node->depth + 1, Box(mid_x, mid_y, box.x2, box.y2), node);
      node->bottom_left() = new QuadTreeNode(
          node->depth + 1, Box(box.x1, box.y1, mid_x, mid_y), node);
      node->bottom_right() = new QuadTreeNode(
          node->depth + 1, Box(mid_x, box.y1, box.x2, mid_y), node);
      node->leaf = false;

      Unit* p = node->head->next;
      while (p != node->tail) {
        Unit* temp = p->next;
        node->Delete(p);
        Insert(node, p);
        p = temp;
      }
    }
  }

  node->Foreach([this, unit](QuadTreeNode* child_node) {
    if (child_node->box.Contains(unit->x, unit->y)) {
      Insert(child_node, unit);
      return false;
    }
    return true;
  });
}

void QuadTreeAOI::QuadTree::Delete(Unit* unit) {
  QuadTreeNode* node = unit->quad_tree_node;
  node->Delete(unit);
  unit->quad_tree_node = nullptr;
}

void QuadTreeAOI::QuadTree::Search(const QuadTreeNode* node, const Box& box,
                                   AOI::UnitSet& unit_set) const {
  if (!node->box.Intersects(box)) {
    return;
  }

  if (!node->leaf) {
    const_cast<QuadTreeNode*>(node)->Foreach(
        [this, &box, &unit_set](QuadTreeNode* child_node) {
          Search(child_node, box, unit_set);
          return true;
        });
    return;
  }

  Unit* p = node->head->next;
  while (p != node->tail) {
    if (box.Contains(p->x, p->y)) {
      unit_set.insert(p);
    }
    p = p->next;
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
  ValidatetUnitID(id);
  ValidatePosition(x, y);

  Unit* unit = static_cast<Unit*>(NewUnit(id, x, y));
  quad_tree_->Insert(unit);

  OnAddUnit(unit);
}

void QuadTreeAOI::UpdateUnit(UnitID id, float x, float y) {
  ValidatePosition(x, y);

  Unit* unit = static_cast<Unit*>(get_unit(id));
  quad_tree_->Delete(unit);
  unit->x = x;
  unit->y = y;
  quad_tree_->Insert(unit);

  OnUpdateUnit(unit);
}

void QuadTreeAOI::RemoveUnit(UnitID id) {
  Unit* unit = static_cast<Unit*>(get_unit(id));
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
  delete static_cast<Unit*>(unit);
}
