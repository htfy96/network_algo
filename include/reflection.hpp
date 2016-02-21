#ifndef GRAPH_REFLECTION_HPP
#define GRAPH_REFLECTION_HPP

#include <type_traits>
#include <string>
#include <stdexcept>
#include <typeinfo>
#include <cassert>
#include <utility>
#include "graphdsl.hpp"

#include <google/protobuf/stubs/common.h>
#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>

namespace
{
        inline std::pair<const google::protobuf::FieldDescriptor*,
        const google::protobuf::Reflection*>
            getFDPandRP(google::protobuf::Message* obj, const std::string& fieldName)
            {
                using namespace google::protobuf;
                const Descriptor* dp = obj->GetDescriptor();
                const FieldDescriptor* fdp = dp->FindFieldByName(fieldName);
                assert(fdp->label() == FieldDescriptor::LABEL_REQUIRED);
                const Reflection* reflection = obj->GetReflection();
                return make_pair(std::move(fdp),
                            std::move(reflection));
            }

    template<typename From, typename To>
        To omni_cast_impl_detail(const From& value, const bool)
        {
            return static_cast<To>(value);
        }

    //special case for const string& to const char*

    template<typename From, typename To>
        To omni_cast_impl_detail(const From& value, const int,
                    typename std::enable_if<std::is_same<From, std::string>::value
                    &&
                    std::is_same< typename std::decay<To>::type, const char* >::value >::type = 0)
        {
            return value.c_str();
        }

    template<typename From, typename To>
        To omni_cast_impl(const From& value, const int,
                    typename std::enable_if<std::is_convertible<typename std::decay<From>::type, typename std::decay<To>::type>::value ||
                    (std::is_same<From, std::string>::value &&
                     std::is_same< typename std::decay<To>::type, const char*>::value),
                    int>::type = 0) 

        {
            return omni_cast_impl_detail<From, To>(value, 1);
        }

    template<typename From, typename To>
        To omni_cast_impl(const From& value, const bool)
        {
            throw std::runtime_error(std::string("Reflection Error: Cannot cast ")+
                        typeid(From).name() +" (which decays to " +
                typeid(typename std::decay<From>::type).name()+") to " + typeid(To).name());
            (void)(value); //eliminate warning
            return To();
        }

    template<typename From, typename To>
        To omni_cast(const From& value)
        {
            return omni_cast_impl<From, To>(value, 1);
        }

    template<typename A, typename B>
        bool compareWithRelationship(const A& v1, const B& v2, netalgo::Relationship relationship)
        {
            switch(relationship)
            {
                case netalgo::Relationship::equal:
                return v1 == v2;
                break;

                case netalgo::Relationship::smaller:
                return v1 < v2;
                break;

                case netalgo::Relationship::greater:
                return v1 > v2;
                break;

                default:
                throw std::runtime_error("Relationship unknown when comparing values");
            }
        }

    //Compare given field of a protobuf object with a value
    template<typename U>
        bool reflectedCompare(google::protobuf::Message *obj, const std::string& fieldName,
                    netalgo::Relationship relationship, const U& value)
        {
            using namespace google::protobuf;
            const FieldDescriptor* fdp;
            const Reflection* reflection;

            std::tie(fdp, reflection) = getFDPandRP(obj, fieldName);
            switch(fdp->cpp_type())
            {
                case FieldDescriptor::CPPTYPE_BOOL:
                return compareWithRelationship( reflection->GetBool(*obj, fdp),omni_cast<U, bool>(value),
                            relationship);
                break;

                case FieldDescriptor::CPPTYPE_DOUBLE:
                return compareWithRelationship(  reflection->GetDouble(*obj, fdp),omni_cast<U, double>(value) ,
                            relationship);
                break;

                //case FieldDescriptor::CPPTYPE_ENUM:
                //return omni_cast<U, int32>(value) == *reflection->GetEnum(*obj, fdp);
                //break;

                case FieldDescriptor::CPPTYPE_FLOAT:
                return compareWithRelationship(  reflection->GetFloat(*obj, fdp),omni_cast<U, float>(value) ,
                            relationship);
                break;

                case FieldDescriptor::CPPTYPE_INT32:
                return compareWithRelationship(  reflection->GetInt32(*obj, fdp),omni_cast<U, int32>(value) ,
                            relationship);
                break;

                case FieldDescriptor::CPPTYPE_INT64:
                return compareWithRelationship(  reflection->GetInt64(*obj, fdp),omni_cast<U, int64>(value) ,
                            relationship);
                break;

                case FieldDescriptor::CPPTYPE_MESSAGE:
                throw std::runtime_error(
                            std::string(
                                "Currently the library doesn't support reflection on message type"
                                )
                            );
                break;

                case FieldDescriptor::CPPTYPE_STRING:
                return compareWithRelationship(  reflection->GetString(*obj, fdp),omni_cast<U, std::string>(value) ,
                            relationship);
                break;

                case FieldDescriptor::CPPTYPE_UINT32:
                return compareWithRelationship(  reflection->GetUInt32(*obj, fdp),omni_cast<U, uint32>(value) ,
                            relationship);
                break;

                case FieldDescriptor::CPPTYPE_UINT64:
                return compareWithRelationship(  reflection->GetUInt64(*obj, fdp),omni_cast<U, uint64>(value) ,
                            relationship);
                break;

                default:
                throw std::runtime_error(
                            std::string(
                                "unknown message type in reflection"
                                )
                            );
            }
                

        }
}

#endif
