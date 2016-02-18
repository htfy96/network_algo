#ifndef UTILITY_HPP
#define UTILITY_HPP
#include <cstddef>
#include <cctype>
#include <cassert>
#include <string>
#include <iostream>
#include <map>
#include <unordered_map>
#include <utility>
#include <iterator>
#include <memory>

namespace
{
    template<typename T, int C>
        size_t arrayLen(T (&)[C])
        {
            return C;
        }

    std::string strLower(std::string s)
    {
        for(char& c: s)
            c = std::tolower(c);
#ifdef MYDEBUG
        //std::cout << s << std::endl;
#endif
        return s;
    }

    template<typename KeyT, typename ValueT>
        class TypedLRUMap
        {
            protected:
                const std::size_t max_entry_;
                typedef std::unordered_map<KeyT, ValueT> mapT;
                typedef std::map<std::size_t, KeyT> lastUsedTimeRevT;
                typedef std::map<KeyT, std::size_t> lastUsedTimeT;
                mapT map_;
                lastUsedTimeT lastUsedTime_;
                lastUsedTimeRevT lastUsedTimeRev_;
                std::size_t timestamp;
                static const std::size_t numberClearAtOnce = 10;
            public:
                TypedLRUMap(const size_t max_entry): max_entry_(max_entry), timestamp(0) {}
                TypedLRUMap(const TypedLRUMap&) = default;
                TypedLRUMap(TypedLRUMap&&) = default;
                TypedLRUMap& operator=(const TypedLRUMap&) = default;
                ~TypedLRUMap() = default;

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
                    insert(typename mapT::value_type value)
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
                                    lastUsedTime_.insert(make_pair(value.first, timestamp));
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
                        if (map_.size() > max_entry_)
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
#endif
