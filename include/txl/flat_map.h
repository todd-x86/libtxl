#pragma once

#include <bitset>
#include <unordered_map>
#include <map>
#include <vector>
#include <tuple>
#include <limits>

namespace txl
{
	template<class K, class V>
	class flat_map
	{
	public:
	  struct bucket final
	  {
		char key_[sizeof(K)];
		char value_[sizeof(V)];

		bucket() = default;

		bucket(K && key, V && value)
		{
		  new(&key_[0])K(std::move(key));
		  new(&value_[0])V(std::move(value));
		}

		auto key() -> K & { return *reinterpret_cast<K *>(&key_[0]); }
		auto value() -> V & { return *reinterpret_cast<V *>(&value_[0]); }
		auto key() const -> K const & { return *reinterpret_cast<K const *>(&key_[0]); }
		auto value() const -> V const & { return *reinterpret_cast<V const *>(&value_[0]); }

		auto release_key() -> K && { return std::move(*reinterpret_cast<K *>(&key_[0])); }
		auto release_value() -> V && { return std::move(*reinterpret_cast<V *>(&value_[0])); }
	  };

	private:
      enum flags
      {
        VISITED = 0,
        EMPLACED = 1,
      };

      using bucket_flags = std::bitset<2>;
	  std::vector<bucket> buckets_;
	  std::vector<bucket_flags> emplaced_;
	  double threshold_;
	  size_t size_ = 0;
	  size_t max_size_ = 0;

	  auto resize(size_t new_size) -> void
	  {
		std::vector<bucket> new_buckets{};
		new_buckets.resize(new_size);
		std::vector<bucket_flags> new_emplaced{};
		new_emplaced.resize(new_size);

		for (size_t i = 0; i < buckets_.size(); ++i)
		{
		  auto & b = buckets_[i];
		  if (emplaced_[i][EMPLACED])
		  {
			emplace(new_buckets, new_emplaced, b.release_key(), b.release_value(), std::numeric_limits<int>::max());
		  }
		}

		buckets_ = std::move(new_buckets);
		emplaced_ = std::move(new_emplaced);
		max_size_ = buckets_.size() * threshold_;
	  }

	  static auto emplace(std::vector<bucket> & buckets, std::vector<bucket_flags> & emplaced, K && key, V && value, int max_tries) -> bool
	  {
		auto h = std::hash<K>{}(key);


		auto p = h % buckets.size();
		size_t num_tries = 0;
		while (emplaced[p][VISITED] && num_tries < max_tries)
		{
		  ++num_tries;
		  if (buckets[p].key() == key)
		  {
			buckets[p] = bucket{buckets[p].release_key(), std::move(value)};
			return false;
		  }
		  p++;
		  if (p == buckets.size())
		  {
			p = 0;
		  }
		}
		if (num_tries < max_tries)
        {
            buckets[p] = bucket{std::move(key), std::move(value)};
            emplaced[p].set();
            return true;
		}
		return -1;
	  }

	  template<class Key>
	  auto hash_find(Key const & key) -> bucket *
	  {
	  // TODO: hash should be Key not K (string_view can convert more easily than string)
		auto h = std::hash<Key>{}(key);
		auto p = h % buckets_.size();
		size_t num_tries = size_;
		while (emplaced_[p][VISITED] && num_tries != 0)
		{
		// TODO: convert safely
		  if (emplaced_[p][VISITED] && Key{buckets_[p].key()} == key)
		  {
			return &buckets_[p];
		  }
		  
		  --num_tries;
		  p++;
		  if (p == buckets_.size())
		  {
			p = 0;
		  }
		}

		return nullptr;
	  }

	  template<class Key>
	  auto linear_find(Key const & key) -> bucket *
	  {
		for (size_t i = 0; i < buckets_.size(); ++i)
		{
		  auto & b = buckets_[i];
		  // TODO: convert safely
		  if (emplaced_[i][VISITED] && Key{b.key()} == key)
		  {
			return &b;
		  }
		}

		return nullptr;
	  }
	public:
	  auto end() const -> bucket const *
	  {
		return nullptr;
	  }

	  template<class Key>
	  auto find(Key const & key) -> bucket *
	  {
		if (size_ >= 20)
		{
		  return hash_find(key);
		}

		return linear_find(key);
	  }

	  flat_map(size_t s = 17, double threshold = 0.45)
		: threshold_(threshold)
	  {
		buckets_.resize(s);
		emplaced_.resize(s);
		max_size_ = s * threshold_;
	  }

      auto erase(K const & key) -> bool
      {
        auto b = find(key);
        if (b == end())
        {
            return false;
        }
      }

	  auto emplace(K && key, V && value) -> void
	  {
		/*auto num_tries = */
		int s = 0;
		while ((s = emplace(buckets_, emplaced_, std::move(key), std::move(value), 16)) == -1)
		{
		  resize(buckets_.size() * (1/threshold_));
		}
		size_ += s;
	  }
	};
}
