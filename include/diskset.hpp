#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include "cereal/archives/binary.hpp"
#include <leveldb/db.h>
#include <leveldb/comparator.h>

namespace netalgo
{
    namespace ds_impl
    {
        template<typename T>
            std::string serializeToString(const T& obj)
            {
                std::ostringstream os(std::ostringstream::binary | std::ostringstream::out);
                cereal::BinaryOutputArchive bo(os);
                bo << obj;
                return os.str();
            }
        template<typename T>
            T deserializeFromString(const std::string &s)
            {
                std::istringstream is(s, std::istringstream::binary | std::istringstream::in);
                cereal::BinaryInputArchive bi(is);
                T x;
                bi >> x;
                return x;
            }
    }

    template<typename T>
        class DiskSet
        {
            private:
                std::shared_ptr<leveldb::DB> db;

                class Iterator_c
                {
                    private:
                        leveldb::Iterator *it;
                        T obj;
                    public:
                        explicit Iterator_c(leveldb::Iterator *pi):
                            it(pi)
                    {
                        if (it->Valid())
                            obj = ds_impl::deserializeFromString<T>(it->key().ToString());
                    }

                        ~Iterator_c()
                        {
                            delete it;
                        }

                        typedef std::bidirectional_iterator_tag iterator_category;
                        typedef T value_type;
                        typedef size_t difference_type;
                        typedef const value_type& reference;
                        typedef const value_type *pointer;
                        
                        bool operator==(const Iterator_c &other) const
                        {
                            if (it->Valid() != other.it->Valid()) return false;
                            else if (it->Valid()) return it->key() == other.it->key();
                            else return true;
                        }

                        bool operator!=(const Iterator_c &other) const
                        {
                            return ! operator==(other);
                        }

                        Iterator_c& operator++()
                        {
                            it->Next();
                            if (it->Valid())
                                obj = ds_impl::deserializeFromString<T>(it->key().ToString());
                            return *this;
                        }

                        Iterator_c operator++(int)
                        {
                            Iterator_c x(*this);
                            operator++();
                            return x;
                        }

                        Iterator_c& operator--()
                        {
                            it->Prev();
                            if (it->Valid())
                                obj = ds_impl::deserializeFromString<T>(it->key().ToString());
                            return it;
                        }

                        Iterator_c operator--(int)
                        {
                            Iterator_c x(*this);
                            operator--();
                            return x;
                        }

                        reference operator*()
                        {
                            return obj;
                        }

                        pointer operator->()
                        {
                            return &obj;
                        }
                };


                class SmartComparator : public leveldb::Comparator {
                    public:
                        int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const {
                            T objA = 
                            ds_impl::deserializeFromString<T>(a.ToString());
                            T objB = 
                                ds_impl::deserializeFromString<T>
                                (b.ToString());
                            if (objA < objB) return -1;
                            if (objA == objB) return 0;
                            return 1;
                        }

                        // Ignore the following methods for now:
                        const char* Name() const { return "SmartComparator"; }
                        void FindShortestSeparator(std::string*, const leveldb::Slice&) const { }
                        void FindShortSuccessor(std::string*) const { }
                };

            static SmartComparator sc;

            public:
                explicit DiskSet(const std::string &filename)
                {
                    leveldb::DB *tmp_db = nullptr;
                    leveldb::Options option;
                    option.create_if_missing = true;
                    option.comparator = & DiskSet::sc;
                    leveldb::DB::Open(option, filename, &tmp_db);
                    try
                    {
                        db = std::shared_ptr<leveldb::DB>(tmp_db);
                    } catch(...)
                    {
                        delete tmp_db;
                        throw;
                    }
                }
                ~DiskSet() = default;
                DiskSet(const DiskSet&) = default;
                DiskSet(DiskSet&&) = default;
                DiskSet& operator=(const DiskSet&) = default;
                DiskSet& operator=(DiskSet&&) = default;

                typedef Iterator_c const_iterator;

                bool insert(const T& data)
                {
                    leveldb::Status status = db->Put(leveldb::WriteOptions(),
                                ds_impl::serializeToString(data),
                                ""
                                );
                    return status.ok();
                }

                const_iterator begin() const
                {
                    leveldb::Iterator *it =
                        db->NewIterator(leveldb::ReadOptions());
                    it->SeekToFirst();
                    return const_iterator(it);
                }

                const_iterator end() const
                {
                    leveldb::Iterator *it =
                        db->NewIterator(leveldb::ReadOptions());
                    it->SeekToLast();
                    while(it->Valid())
                        it->Next();
                    return const_iterator(it);
                }

                const_iterator find(const T& obj)
                {
                    leveldb::Iterator *it =
                        db->NewIterator(leveldb::ReadOptions());
                    it->Seek(ds_impl::serializeToString(obj));
                    if (it->Valid() && it->key() == ds_impl::serializeToString(obj))
                        return const_iterator(it);
                    else
                    {
                        delete it;
                        return end();
                    }
                }

                void clear()
                {
                    leveldb::Iterator *it = db->NewIterator(leveldb::ReadOptions());
                    it->SeekToFirst();
                    while (it->Valid())
                    {
                        leveldb::Iterator *itnext = it;
                        itnext->Next();
                        db->Delete(leveldb::WriteOptions(), it->key());
                        it = itnext;
                        delete itnext;
                    }
                    delete it;
                }

                bool empty() const
                {
                    leveldb::Iterator *it = db->NewIterator(leveldb::ReadOptions());
                    it->SeekToFirst();
                    if (it->Valid())
                    {
                        delete it;
                        return false;
                    } else
                    {
                        delete it;
                        return false;
                    }
                }

        };

    template<typename T>
        typename DiskSet<T>::SmartComparator DiskSet<T>::sc;

    namespace TempTest
    {
        inline void Test()
        {
            DiskSet<int> d("2324");
            d.insert(2);
            auto it = d.find(3);
        
        }
    }
}
