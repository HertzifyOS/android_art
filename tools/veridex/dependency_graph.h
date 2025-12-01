/*
 * Copyright (C) 2025 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_TOOLS_VERIDEX_DEPENDENCY_GRAPH_H_
#define ART_TOOLS_VERIDEX_DEPENDENCY_GRAPH_H_

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "dex/class_accessor.h"
#include "dex/dex_file_reference.h"
#include "resolver.h"

namespace art {

enum class NodeType { kMethod, kField };

// Custom comparator for std::map key (std::pair<NodeType, uint32_t>)
struct NodeKeyComparator {
  bool operator()(const std::pair<NodeType, uint32_t>& a,
                  const std::pair<NodeType, uint32_t>& b) const {
    if (a.first != b.first) {
      return a.first < b.first;
    }
    return a.second < b.second;
  }
};

class DependencyNode {
 public:
  DependencyNode(NodeType type, uint32_t id)
      : node_type_(type), id_(id), is_aconfig_flag_source_(false) {}

  void AddChild(DependencyNode* child) { children_.insert(child); }

  void PropagateAconfigFlag() {
    if (is_aconfig_flag_source_) {  // Already marked, stop propagation
      return;
    }
    is_aconfig_flag_source_ = true;  // Mark self
    for (DependencyNode* child : children_) {
      child->PropagateAconfigFlag();
    }
  }

  NodeType GetType() const { return node_type_; }
  uint32_t GetId() const { return id_; }
  bool IsAconfigSource() const { return is_aconfig_flag_source_; }
  const std::set<DependencyNode*>& GetChildren() const { return children_; }

 private:
  NodeType node_type_;
  uint32_t id_;
  bool is_aconfig_flag_source_;
  std::set<DependencyNode*> children_;
  std::set<DependencyNode*> parents_;
};

class DependencyGraph {
 public:
  DependencyNode* GetOrCreateNode(NodeType type, uint32_t id) {
    std::pair<NodeType, uint32_t> key = {type, id};
    if (nodes_.find(key) == nodes_.end()) {
      nodes_[key] = std::make_unique<DependencyNode>(type, id);
    }
    return nodes_[key].get();
  }

  DependencyNode* GetNode(NodeType type, uint32_t id) {
    std::pair<NodeType, uint32_t> key = {type, id};
    if (nodes_.find(key) == nodes_.end()) {
      return nullptr;
    }
    return nodes_[key].get();
  }

  void Build(const std::vector<std::unique_ptr<VeridexResolver>>& resolvers);
  void Dump(const std::vector<std::unique_ptr<VeridexResolver>>& resolvers) const;

  bool IsNodeAconfigSource(NodeType type, uint32_t id) const {
    auto it = nodes_.find({type, id});
    if (it != nodes_.end()) {
      return it->second->IsAconfigSource();
    }
    return false;
  }

 private:
  std::map<std::pair<NodeType, uint32_t>, std::unique_ptr<DependencyNode>, NodeKeyComparator>
      nodes_;
};

}  // namespace art

#endif  // ART_TOOLS_VERIDEX_DEPENDENCY_GRAPH_H_
