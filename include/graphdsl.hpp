#ifndef GRAPHDSL_HPP
#define GRAPHDSL_HPP
#include <vector>
#include <tuple>
#include <string>
#include <exception>
#include <memory>
#include "debug.hpp"

/**************************************************************************************************************
 *                                          pair<> GraphSqlSentence
 *                       first) SelectSentence                                             second) ReturnSentence
 * vector<NodeType> nodes;              vector<EdgeType> edges;                          vector<string> returnName;
 * string id; Properties properties;    EdgeDirection direction; Properties properties;
 *            name:str Rel value:str    prev,next,bidir          name:str Rel value:str
 **************************************************************************************************************/
namespace netalgo
{
    enum Relationship { greater, smaller, equal };

    struct Property
    {
        std::string name;
        Relationship relationship;
        std::string value;
    };
    
    typedef std::vector<Property> Properties;

    struct NodeType
    {
        std::string id;
        Properties properties;
    };

    enum EdgeDirection { prev, next, bidirection };
    struct EdgeType
    {
        std::string id;
        EdgeDirection direction;
        Properties properties;
    };

    struct SelectSentence
    {
        std::vector<NodeType> nodes;
        std::vector<EdgeType> edges;
    };

    struct ReturnSentence
    {
        std::vector<std::string> returnName;
    };

    typedef std::pair<SelectSentence, ReturnSentence> GraphSqlSentence;

    GraphSqlSentence parseGraphSql(const char* s); 
    class GraphSqlParseException : public std::exception
    {
        protected:
            std::unique_ptr<char[]> info;
        public:
            GraphSqlParseException();
            GraphSqlParseException(const char* inf);
            GraphSqlParseException(const GraphSqlParseException&);
            virtual const char* what() const noexcept override;
            virtual ~GraphSqlParseException ();
    };
    
    class GraphSqlParseStateException: public GraphSqlParseException
    {
        protected:
            std::string nearbyChars;
        public:
            GraphSqlParseStateException(const char* inf, std::istream& state);
            GraphSqlParseStateException(const char* inf, const std::string& pNearbyChars);
            GraphSqlParseStateException(const GraphSqlParseStateException&) = default;
            virtual const char* what() const noexcept override;
            const std::string& getNearbyChars();
            virtual ~GraphSqlParseStateException() override;
    };
}


netalgo::GraphSqlSentence operator""_graphsql(const char*, size_t);

#endif

