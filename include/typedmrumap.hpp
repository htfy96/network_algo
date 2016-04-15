#ifndef GRAPH_TYPEDMRUMAP_HPP
#define GRAPH_TYPEDMRUMAP_HPP

#include <map>
#include <cstddef>
#include <cstdlib>
#include <unordered_map>
#include <iterator>
#include <utility>
#include <type_traits>

namespace netalgo
{
    namespace impl
    {
        template<typename T, typename U = decltype(std::declval<T>().capacity())>
            std::size_t getSize_impl(const T& value, char)
            // exact match(level 1)
            {
                return value.capacity();
            }

        template<typename T, typename U = decltype(std::declval<T>().size())>
            std::size_t getSize_impl(const T& value, int)
            // char to int is promotion(level 2)
            {
                return value.size();
            }

        template<typename T>
            std::size_t getSize_impl(const T& value, bool)
            // char to bool is conversion(level 3)
            {
                (void)(value); //eliminate warning
                return 1;
            }

        template<typename T>
            std::size_t getSize(const T& value)
            {
                return getSize_impl(value, char(0));
            }

        template<typename KeyT, typename ValueT>
            class TypedMRUMap
            {
                protected:
                    const std::size_t max_objectSize_;
                    typedef std::unordered_map<KeyT, ValueT> mapT;
                    typedef std::map<std::size_t, KeyT> lastUsedTimeRevT;
                    typedef std::map<KeyT, std::size_t> lastUsedTimeT;
                    mapT map_;
                    lastUsedTimeT lastUsedTime_;
                    lastUsedTimeRevT lastUsedTimeRev_;
                    std::size_t timestamp;
                    std::size_t objectSize;
                    static const std::size_t numberClearAtOnce = 10;
                public:
                    explicit TypedMRUMap(const size_t max_objectSize): max_objectSize_(max_objectSize), timestamp(0),
                    objectSize(0) {}
                    TypedMRUMap(const TypedMRUMap&) = default;
                    TypedMRUMap(TypedMRUMap&&) = default;
                    TypedMRUMap& operator=(const TypedMRUMap&) = default;
                    ~TypedMRUMap() = default;

                    typedef typename mapT::iterator iterator;
                    typedef typename mapT::const_iterator const_iterator;

                    typedef typename mapT::key_type key_type;
                    typedef typename mapT::mapped_type mapped_type;
                    typedef typename mapT::value_type value_type;
                    typedef typename mapT::size_type size_type;
                    typedef typename mapT::difference_type difference_type;
                    typedef typename mapT::reference reference;
                    typedef typename mapT::const_reference const_reference;
                    void clearNotUsedCache()
                    {
                        std::size_t count = 0;
                        for (auto it = lastUsedTimeRev_.cbegin(); it != lastUsedTimeRev_.cend();)
                        {
                            ++count;
                            if (count > numberClearAtOnce)
                                break;
                            auto nextit = std::next(it);
                            map_.erase(it->second);
                            lastUsedTime_.erase(it->second);
                            lastUsedTimeRev_.erase(it);
                            it = nextit;
                        }
                    }

                    std::pair<typename mapT::iterator, bool>
                        insert(typename mapT::value_type value) //should be exception safe
                        {
                            auto result = map_.insert(std::move(value));
                            ++timestamp;
                            try
                            {
                                if (result.second)
                                {
                                    lastUsedTimeRev_.insert(std::make_pair(timestamp, value.first));
                                    try
                                    {
                                        lastUsedTime_.insert(std::make_pair(value.first, timestamp));
                                    } catch(...)
                                    {
                                        lastUsedTimeRev_.erase(timestamp);
                                        throw;
                                    }
                                }
                            } catch (...)
                            {
                                map_.erase(result.first);
                                throw;
                            }
                            objectSize += getSize(value);
                            if (objectSize > max_objectSize_)
                                clearNotUsedCache();
                            return result;
                        }

                    iterator find(const KeyT& key)
                    {
                        iterator it = map_.find(key);
                        if (it != map_.end())
                        {
                            std::size_t ts;
                            auto result = lastUsedTime_.find(key);
                            assert(result!=lastUsedTime_.end());
                            ts = result->second;

                            ++timestamp;
                            result->second = timestamp;

                            lastUsedTimeRev_.erase(ts);
                            lastUsedTimeRev_.insert(std::make_pair(timestamp, key));
                        }
                        return it;
                    }

                    iterator begin()
                    {
                        return map_.begin();
                    }

                    iterator end()
                    {
                        return map_.end();
                    }

                    size_type size() const
                    {
                        return map_.size();
                    }

                    mapped_type& operator[](const KeyT& key)
                    {
                        auto it = map_.find(key);
                        if (it == map_.end())
                        {
                            auto insertResult = insert(std::make_pair(key, ValueT()));
                            assert(insertResult.second);
                            return insertResult.first->second;
                        }
                        else
                            return map_[key];
                    }
            };
    }
}
#endif
