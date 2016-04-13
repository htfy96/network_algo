#ifndef GRAPH_BACKEND_LEVELDBGRAPH_DB_UTILITY
#define GRAPH_BACKEND_LEVELDBGRAPH_DB_UTILITY

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/cache.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/string.hpp>

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/stubs/common.h>

#include <string>
#include <cassert>
#include <exception>
#include <iostream>
#include <type_traits>
#include <sstream>

namespace
{
	class stringStreamSlice
	{
		private:
			std::string str_;
		public:
			stringStreamSlice(const std::ostringstream& os):str_(os.str()) {}
			leveldb::Slice getSlice()
			{
				return leveldb::Slice(str_);
			}
	};

	class stringSlice
	{
		private:
			std::string str_;
		public:
			stringSlice(std::string s): str_(std::move(s)) {}
			leveldb::Slice getSlice()
			{
				return leveldb::Slice(str_);
			}
	};


	template<typename T>
		stringSlice dataToSliceByProtobuf(const T& data)
		{
			std::string result;
			data.SerializeToString(&result);
			return result;
		}

	template<typename T>
		stringStreamSlice dataToSliceByCereal(const T& data)
		{
			std::ostringstream os(std::ios::binary | std::ios::out);
			cereal::BinaryOutputArchive oa(os);
			oa << data;
			return os;
		}

	template<typename T>
		T sliceToDataByProtobuf(const leveldb::Slice& slice)
		{
			std::string str(slice.ToString());
			T result;
			result.ParseFromString(str);
			return result;
		}

	template<typename T>
		T strToDataByCereal(std::string s)
		{
			std::istringstream is(std::move(s), std::ios::binary | std::ios::in);
			cereal::BinaryInputArchive archive(is);
			T data;
			archive(data);
			return data;
		}

	template<typename T>
		T strToDataByProtobuf(const std::string s)
		{
			T result;
			result.ParseFromString(s);
			return result;
		}

	const char nodeDataIdSuffix[] = ":node:@data";
	const char edgeDataIdSuffix[] = ":edge:@data";
	const char outEdgeSuffix[] = ":@outedge";
	const char inEdgeSuffix[] = ":@inedge";

	template<typename T>
		std::string addSuffix(const T& originalId, const char* suffix)
		{
			leveldb::Slice idSlice(originalId);
			std::string dataId = idSlice.ToString();
            //std::cout << "Adding suffix " << dataId << "   " <<suffix << std::endl;
			dataId += suffix;
			return dataId;
		}

}
#endif
