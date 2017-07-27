//
// Copyright(c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#pragma once

#include <algorithm>
#include <shared_mutex>
#include <unordered_map>

template <typename Key, typename T>
class HashMap {
 public:
  HashMap() {}
  void Add(Key key, T value)
  {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (Find(key) != map_.end()) {
      return;
    }
    map_.push_back(std::make_pair(key, value));
  }

  void Remove(Key key)
  {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto it = Find(key);
    if (it != map_.end()) {
      map_.erase(it);
    }
  }

  bool Has(Key key) const
  {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = Find(key);
    return it != map_.end();
  }

  T Get(Key key) const
  {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = Find(key);
    if (it != map_.end()) {
      return it->second;
    }

    return T(0);
  }

  template <class UnaryPredicate>
  Key FindKey_if(UnaryPredicate pred)
  {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = std::find_if(map_.begin(), map_.end(), pred);
    if (it != map_.end()) {
      return it->first;
    }

    return Key(0);
  }

  template <class UnaryPredicate>
  void RemoveKeys_if(UnaryPredicate pred)
  {
      if (map_.size() == 0)
      {
          return;
      }

      std::unique_lock<std::shared_mutex> lock(mutex_);
      map_.erase(std::remove_if(map_.begin(), map_.end(), pred));
  }

  size_t GetSize() const
  {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return map_.size();
  }

 protected:
  auto Find(Key key) const
  {
    return std::find_if(map_.begin(), map_.end(),
                        [&key](const std::pair<Key, T>& v) { return v.first == key; });
  }

  mutable std::shared_mutex mutex_;
  std::vector<std::pair<Key, T>> map_;
};