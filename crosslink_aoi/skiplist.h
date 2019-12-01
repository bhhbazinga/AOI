#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <random>

// It's almost guaranteed to be logn if the maximum number of nodes range in 0
// to 2^14
const int kMaxLevel = 14;

template <typename T, typename Comparator = std::less<T>>
class SkipList {
 private:
  class Node;

 public:
  class Iterator;

  SkipList() : head_(new Node(kMaxLevel)), compare_() {
    static_assert(std::is_default_constructible<T>::value,
                  "T requires default constructor");
    static_assert(std::is_copy_constructible<T>::value,
                  "T requires copy constructor");
    memset(&head_->nexts[0], 0, sizeof(head_->nexts[0]) * kMaxLevel);
  }

  ~SkipList() {
    Node* p = head_;
    Node* temp;
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

  // Find the first node whose data is greater than or equal to the given data,
  // then insert a new node before it.
  // The copy constructer will be used.
  void Insert(const T& data) {
    Node* new_node = new Node(RandomLevel(), data);
    InsertNode(new_node);
  }

  // Find the first node whose data is greater than the given data.
  // If success return the iterator, else return the end iterator.
  Iterator FindFirstGreater(const T& data) const {
    Node* find_node = FindNodeFirstGreater(data);
    return Iterator(find_node);
  }

  // Find the last node whose data is less than the given data.
  // If success return the iterator, else return the end iterator.
  Iterator FindLastLess(const T& data) const {
    Node* find_node = FindNodeLastLess(data);
    return Iterator(find_node);
  }

  // Delete the first node whose data is equals to give data
  // If success return true else return false
  bool Erase(const T& data);

  Iterator Begin() const { return Iterator(head_->nexts[0]); }
  Iterator End() const { return Iterator(nullptr); }

 private:
  int RandomLevel() const {
    int level = 1;
    while (level < kMaxLevel && (rand() % 2 == 0)) {
      ++level;
    }
    return level;
  }

  void InsertNode(Node* const new_node) {
    Node* prevs[kMaxLevel];
    FindNodeFirstGreaterOrEquals(new_node->data, prevs);

    int new_level = new_node->level;
    for (int l = 0; l < new_level; ++l) {
      // Update prevs' nexts and new_node's nexts
      new_node->nexts[l] = prevs[l]->nexts[l];
      prevs[l]->nexts[l] = new_node;
    }
  }

  Iterator Find(const T& data, Node** prevs) const {
    Node* find_node = FindNodeFirstGreaterOrEquals(data, prevs);
    return InternalFind(data, find_node);
  }

  Iterator InternalFind(const T& data, Node* find_node) const {
    if (nullptr != find_node && Equals(data, find_node->data)) {
      return Iterator(find_node);
    } else {
      return End();
    }
  }

  // Find the first node whose data is greater than or equal to the given data,
  // and save the prev nodes in prevs,
  Node* FindNodeFirstGreaterOrEquals(const T& data, Node** prevs) const;

  typedef std::function<bool(const T& data, const T& other)> Predicate;
  Node* Find(const T& data, Predicate&& pred) const {
    Node* p = head_;
    int level = head_->level - 1;
    while (level >= 0) {
      Node* next = p->nexts[level];
      if (nullptr != next && pred(data, next->data)) {
        // Move forward
        p = next;
        level = p->level;
      }
      --level;
    }
    return p;
  }

  // Find the first node whose data is greater than or equals to the given data
  Node* FindNodeFirstGreaterOrEquals(const T& data) const {
    Node* p = Find(data, [this](const T& data, const T& other) -> bool {
      return Greater(data, other);
    });
    return p->nexts[0];
  }

  // Find the first node whose data is greater than the given data
  Node* FindNodeFirstGreater(const T& data) const {
    Node* p = Find(data, [this](const T& data, const T& other) -> bool {
      return GreaterOrEquals(data, other);
    });
    return p->nexts[0];
  }

  // Find the last node whose data is less than the given data
  Node* FindNodeLastLess(const T& data) const {
    Node* p = Find(data, [this](const T& data, const T& other) -> bool {
      return Greater(data, other);
    });
    return p;
  }

  bool Equals(const T& data, const T& other) const {
    return !compare_(data, other) && !compare_(other, data);
  }

  bool Less(const T& data, const T& other) const {
    return compare_(data, other);
  }

  bool Greater(const T& data, const T& other) const {
    return !Less(data, other) && !Equals(data, other);
  }

  bool GreaterOrEquals(const T& data, const T& other) const {
    return Greater(data, other) || Equals(data, other);
  }

  Node* const head_;
  Comparator const compare_;
};

template <typename T, typename Comparator>
typename SkipList<T, Comparator>::Node*
SkipList<T, Comparator>::FindNodeFirstGreaterOrEquals(const T& data,
                                                      Node** prevs) const {
  Node* p = head_;
  int level = head_->level - 1;
  while (level >= 0) {
    Node* next = p->nexts[level];
    if (nullptr != next && Greater(data, next->data)) {
      // Move forward
      p = next;
      level = p->level;
    } else {
      // Move down
      prevs[level] = p;
    }
    --level;
  }
  return p->nexts[0];
}

template <typename T, typename Comparator>
bool SkipList<T, Comparator>::Erase(const T& data) {
  Node* prevs[kMaxLevel];

  Iterator it = Find(data, prevs);
  if (it == End()) {
    return false;
  }

  Node* erase_node = it.get_node();
  int erase_level = erase_node->level;
  for (int l = 0; l < erase_level; ++l) {
    // Update prevs' nexts
    prevs[l]->nexts[l] = erase_node->nexts[l];
  }

  delete erase_node;
  return true;
}

template <typename T, typename Comparator>
struct SkipList<T, Comparator>::Node {
  Node(const int level_) : level(level_), nexts(new Node*[level]) {}

  Node(const int level_, const T& data_)
      : data(data_), level(level_), nexts(new Node*[level]) {}

  Node(const int level_, T&& data_)
      : data(std::move(data_)), level(level_), nexts(new Node*[level]) {}
  ~Node() { delete[] nexts; }

  T data;
  const int level;
  Node** nexts;
};

template <typename T, typename Comparator>
class SkipList<T, Comparator>::Iterator {
 private:
  Iterator(Node* node) : node_(node) {}
  Node* get_node() const { return node_; };
  void AssertValid() { assert(nullptr != node_); }

  friend SkipList;

 public:
  ~Iterator() {}
  Iterator(const Iterator& other) { node_ = other.node_; }
  Iterator(Iterator&& other) { node_ = other.node_; }
  Iterator& operator=(const Iterator& other) {
    node_ = other.node_;
    return *this;
  }
  Iterator& operator=(Iterator&& other) { return *this = other; }

  // Compare nodes to determine if iterators are strictly equal
  bool operator==(const Iterator& other) const { return node_ == other.node_; }
  bool operator!=(const Iterator& other) const { return node_ != other.node_; }

  // Move the iterator to next
  // Prefix Increment Operator
  Iterator& operator++() {
    AssertValid();
    node_ = node_->nexts[0];
    return *this;
  }

  // Move the iterator to next
  // Postfix Increment Operator
  Iterator operator++(int) {
    AssertValid();
    Node* temp = node_;
    node_ = node_->nexts[0];
    return Iterator(temp);
  }

  // Get the reference of data
  const T& operator*() {
    AssertValid();
    return node_->data;
  }

  // Get the reference of data
  const T* operator->() {
    AssertValid();
    return &node_->data;
  }

 private:
  Node* node_;
};
#endif  // SKIPLIST_H