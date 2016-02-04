#ifndef LAZYCACHE_HPP
#define LAZYCACHE_HPP
#include <map>
#include <string>
#include <cstdint>
#include <cstddef>
#include <sstream>
#include <iostream>
#include <functional>
#include "cereal/archives/binary.hpp"
#include "soci.h"
#include "sqlite3/soci-sqlite3.h"

namespace 
{
    const std::stringstream::openmode ssmode = std::stringstream::in | std::stringstream::out
        | std::stringstream::binary;
    std::stringstream blobToSS(soci::blob& blob)
    {
        std::stringstream ss(ssmode);
        size_t len = blob.blob::get_len();
        char* buf = new char[len];
        blob.blob::read(0, buf, len);
        ss.write(buf, len);
        delete[] buf;
        return ss;
    }

    soci::blob ssToBlob(soci::session& session, std::stringstream& ss)
    {
        ss.seekg(0, ss.end);
        size_t len = ss.tellg();
        ss.seekg(0, ss.beg);
        soci::blob blob(session);
        
        char* buf = new char[len];
        ss.read(buf, len);
        blob.write(0, buf, len);
        delete[] buf;
        return blob;
    }
}
namespace netalgo
{

    template<typename Index, typename Content> 
        std::function< std::vector<std::pair<Index, Content> >(const std::vector<Index>&)>
        singleToMulti(std::function< std::pair<Index, Content>(const Index&)> func)
        {
            return [func](const std::vector<Index>& idv)
            {
                std::vector< std::pair<Index, Content> > v;
                for(auto& id : idv)
                  v.push_back(func(id));
                return v;
            };
        }

    template<typename Index, typename Content> 
        class Lazymap
        {
            public:
                typedef std::function<std::pair<Index, Content>(const Index&)> singleHandlerType;
                typedef std::function< std::vector<std::pair<Index, Content> >
                    (const std::vector<Index>&)> multiHandlerType;

            protected:
                std::map<Index, Content> memmap;
                std::map<Index, size_t> hotmap;
                soci::session filemap;

                singleHandlerType singleHandler;
                multiHandlerType multiHandler;

            public:
                Lazymap(const std::string& filename,
                            const singleHandlerType& singleH, const multiHandlerType& multiH);

                bool insertToMem(const std::pair<Index, Content>& value, size_t hitcount);
                
                bool insertToFile(const std::pair<Index, Content>& value, size_t hitcount);
                size_t getHitCount(const Index& idx);
                

                Lazymap(const Lazymap&) = delete;
                Lazymap(Lazymap&&) = delete;
                ~Lazymap() = default;

        };

    template<typename Index, typename Content>
        Lazymap<Index,Content>::Lazymap(const std::string& filename,
                    const singleHandlerType& singleH, const multiHandlerType& multiH) :
            filemap(soci::sqlite3, filename), memmap(), hotmap(), singleHandler(singleH), multiHandler(multiH)
    {
        filemap << "CREATE TABLE IF NOT EXISTS CACHE (idx BLOB PRIMARY KEY UNIQUE, data BLOB, hitcount INT)";
        soci::blob blob1(filemap), blob2(filemap);
        int hitcount;
        soci::statement rs = (filemap.prepare << "SELECT idx,data,hitcount FROM CACHE ORDER BY hitcount DESC",
                    soci::into(blob1), soci::into(blob2), soci::into(hitcount));
        rs.execute();
        while (rs.fetch())
        {
            std::stringstream ss1 = blobToSS(blob1),
                ss2=blobToSS(blob2);
            cereal::BinaryInputArchive iarv1(ss1), iarv2(ss2);
            Index idx; Content cont;

            iarv1(idx); iarv2(cont);
            try
            {
                insertToMem(std::make_pair(idx, cont), hitcount);
            } catch(...)
            {
            }
        }
    }

    template<typename Index, typename Content>
        bool Lazymap<Index,Content>::insertToMem(const std::pair<Index, Content>& value, const size_t hitcount)
        {
            hotmap.insert(std::make_pair(value.first, hitcount));
            return memmap.insert(value).second;
        }

    template<typename Index, typename Content>
        bool Lazymap<Index,Content>::insertToFile(const std::pair<Index, Content>& value, const size_t hitcount)
        {
            hotmap.insert(std::make_pair(value.first, hitcount));

            const std::stringstream::openmode om = std::stringstream::in | 
                std::stringstream::out | std::stringstream::binary;
            std::stringstream ss1(om), ss2(om);
            cereal::BinaryOutputArchive oarv1(ss1), oarv2(ss2);
            oarv1(value.first); oarv2(value.second);
            filemap << "INSERT INTO CACHE (idx, data, hitcount) VALUES (:idx, :data, :hitcount)", soci::use(ssToBlob(filemap, ss1)),
                    soci::use(ssToBlob(filemap, ss2)), soci::use(hitcount);
        }

    template<typename Index, typename Content> 
        size_t Lazymap<Index, Content>::getHitCount(const Index& idx)
        {
            return hotmap[idx];
        }

}
#endif
