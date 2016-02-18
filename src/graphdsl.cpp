#include "debug.hpp"
#include "graphdsl.hpp"
#include "utility.hpp"
#include <sstream>
#include <iostream>
#include <string>
#include <deque>
#include <cstring>
#include <cctype>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <cassert>

namespace HIDDEN
{
    using namespace netalgo;
    using namespace std;
    static const size_t BUFLEN=300;
    static const size_t CONTEXT_LEN=30;
    static const size_t LOOKBACKWARD_LEN = 10;

    static const char chareof = static_cast<char>(char_traits<char>::eof());

    class NodeNotFoundException : GraphSqlParseException
    {
        public:
            NodeNotFoundException(): GraphSqlParseException("Node not found") {}
            virtual ~NodeNotFoundException() override {}
    };

    class EdgeNotFoundException : GraphSqlParseException
    {
        public:
            EdgeNotFoundException(): GraphSqlParseException("Edge not found") {}
            virtual ~EdgeNotFoundException() override {}
    };

    GraphSqlSentence parseGraphSqlImpl(const char* s);

    class GraphSqlManager
    {
        private:
            static unordered_map<const char*, GraphSqlSentence> cache;
        public:
            static GraphSqlSentence& getParsedSentence(const char* p)
            {
                if (cache.find(p)==cache.end())
                  cache.insert(make_pair(p, parseGraphSqlImpl(p)));
                return cache[p];
            }
    };
    unordered_map<const char*, GraphSqlSentence> GraphSqlManager::cache;

    //Implementation of parser
    //in small letters
    const char* KEYWORD_TABLE[] = { "select", "return" };

    struct token
    {
        enum TokenType
        {
            identifier, //abc, :foo
            keyword, //select, return
            dash, //-
            leftbracket, //[
            rightbracket, //]
            smaller, // <
            greater, // >
            equal, //=
            number, //-123
            leftparen, //(
            rightparen, //)
            comma, //,
            eof,
            string //"dsfa"
        } type;
        std::string raw;
        bool operator==(const token& other) const
        {
            return type == other.type && raw == other.raw;
        }
        bool operator!=(const token& other) const
        {
            return !operator ==(other);
        }
        token() {}
        token(const TokenType& t, const std::string s): type(t), raw(std::move(s)) {}
        friend ostream& operator<<(ostream&, const token&);
    };
    ostream& operator<<(ostream& o, const token& t)
    {
        return o<<t.type<<" "<<t.raw;
    }

    //helpers of parser
    istream& eatWhite(istream& is)
    {
        while (!is.eof())
        {
            char c = is.get();
            if (!std::isspace(c))
            {
              is.unget();
              return is;
            }
        }
        return is;
    }

    //Support +1.21
    //Not supported: 1.7e10
    string getNumber(istream& is)
    {
        string result;
        bool hasdot = false;
        while(char c = is.get())
        {
            if (c!='-' && c!='+') { is.unget(); break; }
            result.push_back(c);
        }
        while (char c = is.get())
        {
            if (!isdigit(c) && c!='.') { is.unget(); break; }
            if (c=='.')
            {
                if (hasdot)
                  throw GraphSqlParseException("You have more than one dot in number");
                else
                  hasdot = true;
            }
            result.push_back(c);
        }
        return result;
    }

    string getIdentifier(istream& is)
    {
        char lookahead = is.get();
        if (!isalpha(lookahead) && lookahead!=':' && lookahead!='_')
          throw GraphSqlParseStateException("The first character of identifier should be a letter or : or _", is);
        string result; result.push_back(lookahead);
        while (char c = is.get())
        {
            if (!isalnum(c) && c!='_')
            {
                is.unget();
                break;
            }
            result.push_back(c);
        }
        return result;
    }

    bool isLogicalOp(const token& t)
    {
        return t.type == token::smaller || t.type == token::greater || t.type == token::equal;
    }

    bool isConstant(const token& t)
    {
        return t.type == token::number || t.type==token::string;
    }

    token getToken(istream& is)
    {
        eatWhite(is);
        char lookahead = is.get();
        switch(lookahead)
        {
            case '-':
                {
                    char c = is.peek();
                    if (!isdigit(c) && c!='.' && c!='+')
                        return {token::dash, "-"};
                    else
                    {
                        is.unget();
                        return {token::number, getNumber(is)};
                    }
                    break;
                }
            case '+':
                return {token::number, getNumber(is)};
                break;
            case '[':
                return {token::leftbracket, "["};
                break;
            case ']':
                return {token::rightbracket, "]"};
                break;
            case '<':
                return {token::smaller, "<"};
                break;
            case '>':
                return {token::greater, ">"};
                break;
            case '=':
                return {token::equal, "="};
                break;
            case '(':
                return {token::leftparen, "("};
                break;
            case ')':
                return {token::rightparen, ")"};
                break;
            case ',':
                return {token::comma, ","};
                break;
            case chareof:
                return {token::eof, string(1, chareof)};
                break;
            case '"':
                {
                    string tmp("\"");
                    while (!is.eof())
                    {
                        char c = is.get();
                        if (c=='\\')
                            if (!is.eof())
                                tmp.push_back(is.get());
                            else
                                throw GraphSqlParseStateException("Non-terminate string", is);
                        else
                        {
                            tmp.push_back(c);
                            if (c=='"') break;
                        }
                    }
                    break;
                }
            default:
                is.unget();
                if (lookahead == '+' || lookahead == '-' || isdigit(lookahead))
                    return {token::number, getNumber(is)};
                string identifier = getIdentifier(is);
                for(size_t i = 0; i < arrayLen(KEYWORD_TABLE); ++i)
                    if (strLower(identifier) == KEYWORD_TABLE[i])
                        return {token::keyword, KEYWORD_TABLE[i]};
                return {token::identifier, identifier};

        };
        throw GraphSqlParseStateException("Cannot determine the type of token", is);
    }

    Property getProperty(deque<token> &tokenQueue)
    {
        if (tokenQueue.front().type != token::identifier)
          throw GraphSqlParseStateException("the property name must be an identifier", tokenQueue.front().raw);
        Property prop;
        prop.name = tokenQueue.front().raw;
        tokenQueue.pop_front();

        if (tokenQueue.empty() || !isLogicalOp(tokenQueue.front()))
          throw GraphSqlParseStateException("logical op(<,=,>) not found when parsing property", tokenQueue.empty() ? "eof" : tokenQueue.front().raw);
        switch(tokenQueue.front().type)
        {
            case token::smaller:
                prop.relationship = Relationship::smaller;
                break;
            case token::equal:
                prop.relationship = Relationship::equal;
                break;
            case token::greater:
                prop.relationship = Relationship::greater;
                break;
            default:
                throw GraphSqlParseStateException("Unexpected logical operator", tokenQueue.front().raw);
        }
        tokenQueue.pop_front();

        if (tokenQueue.empty() || !isConstant(tokenQueue.front()))
          throw GraphSqlParseStateException("constant not found after logical op",
                      tokenQueue.empty() ? "eof" : tokenQueue.front().raw);
        prop.value = tokenQueue.front().raw;
        tokenQueue.pop_front();
        return prop;
    }

    template<int Terminator>
        vector<Property> getPropertyList(deque<token>& tokenQueue)
        {
            vector<Property> result;
            for(;;)
            {
                if (tokenQueue.empty()) throw GraphSqlParseException("Unexpected eof when parsing node");
                if (tokenQueue.front().type == Terminator)
                {
                    tokenQueue.pop_front();
                    return result;
                }
                result.push_back(getProperty(tokenQueue));
            }
            return result;
        }

    template<int C>
        token getNextWithType(deque<token>& tokenQueue)
        {
            if (tokenQueue.empty() || tokenQueue.front().type != C)
              throw GraphSqlParseStateException("assert failure in next char", string("Expected: ")+char(C)+" Got:"+( tokenQueue.empty() ? "eof" : tokenQueue.front().raw));
            token t = tokenQueue.front();
            tokenQueue.pop_front();
            return t;
        }

    std::string getId(deque<token> &tokenQueue)
    {
        if (tokenQueue.empty() || tokenQueue.front().type != token::identifier)
          throw GraphSqlParseStateException("id not found",
                      tokenQueue.empty() ? "EOF" : tokenQueue.front().raw);
        token first = tokenQueue.front();
        string& lookahead = first.raw;
        tokenQueue.pop_front();

        if (lookahead[0]!=':' && !isLogicalOp(tokenQueue.front()))
          return lookahead;
        else
          tokenQueue.push_front(first);
        return "";
    }


    NodeType getNode(deque<token> &tokenQueue)
    {
        NodeType result;
        if (tokenQueue.front().type != token::leftparen)
          throw NodeNotFoundException();
        tokenQueue.pop_front();

        if (tokenQueue.front().type == token::rightparen)
        {
            result.id = "";
            tokenQueue.pop_front();
        } else
        {
          result.id = getId(tokenQueue);
          result.properties = getPropertyList<token::rightparen>(tokenQueue);
        }
        return result;
    }

    bool edgeTrailingDir(deque<token>& tokenQueue) //true on ->, false on -
    {
        getNextWithType<token::dash>(tokenQueue);
        if (tokenQueue.empty() || tokenQueue.front().type != token::greater)
            return false;

        tokenQueue.pop_front();
        return true;
    }

    EdgeType getEdge(deque<token>& tokenQueue)
    {
        EdgeType result;
        bool leftDir = false, rightDir = false;
        if (tokenQueue.empty()) throw GraphSqlParseException("unexpected eof when parsing edge");

        if (tokenQueue.front().type == token::dash)
            leftDir = false;
        else if (tokenQueue.front().type == token::smaller)
        {
            tokenQueue.pop_front();
            leftDir = true;
        }
        else
            throw EdgeNotFoundException();

        //--[>] or -[props]-[>]

        getNextWithType<token::dash>(tokenQueue);

        //-[>] or [props]-[>]

        if (tokenQueue.empty()) throw GraphSqlParseException("unexpected eof when parsing edge");
        if (tokenQueue.front().type == token::dash)
        {
            rightDir = edgeTrailingDir(tokenQueue);
        } else if (tokenQueue.front().type == token::leftbracket)
        {
            tokenQueue.pop_front();
            if (tokenQueue.front().type == token::rightbracket)
            {
                result.id = "";
                tokenQueue.pop_front();
            } else
            {
                result.id = getId(tokenQueue);
                result.properties = getPropertyList<token::rightbracket>(tokenQueue);
            }
            rightDir = edgeTrailingDir(tokenQueue);

        }

        if (leftDir)
            if (rightDir)
                throw GraphSqlParseException("<--> is not allowed");
            else
                result.direction = EdgeDirection::prev;
        else if (rightDir)
            result.direction = EdgeDirection::next;
        else
            result.direction = EdgeDirection::bidirection;

        return result;
    }

    bool isValidIdentifier(const SelectSentence& ss, const string& identifier)
    {
        if (identifier == "") return false;
        for(auto& node : ss.nodes)
            if (node.id == identifier)
                return true;
        for(auto& edge : ss.edges)
            if (edge.id == identifier)
                return true;
        return false;
    }

    GraphSqlSentence parseGraphSqlImpl(const char* c)
    {
        istringstream is(c);
        deque<token> tokenQueue;
        for(token t = getToken(is); t.type!=token::eof; t=getToken(is))
            tokenQueue.push_back(t);
#ifdef MYDEBUG
        //int cnt = 0;
        //for (auto &s:tokenQueue)
        //{
            //cout << cnt << "."<<s.raw<<" "<<s.type << endl;
            //++cnt;
        //}
#endif

        GraphSqlSentence result;

        if (tokenQueue.front() != token( token::keyword, "select" ))
            throw GraphSqlParseStateException(
                        "the first token must be select",
                        tokenQueue.front().raw);
        tokenQueue.pop_front();

        result.first.nodes.push_back(getNode(tokenQueue));
        for(;;)
        {
            try
            {
                EdgeType e = getEdge(tokenQueue);
                result.first.edges.push_back(e);
            } catch(EdgeNotFoundException&)
            {
                break;
            }
            result.first.nodes.push_back(getNode(tokenQueue));
        }

        if (tokenQueue.front() != token(token::keyword, "return"))
            throw GraphSqlParseStateException(
                        "return doesn't appear in place",
                        tokenQueue.front().raw);
        tokenQueue.pop_front();

        token first = tokenQueue.front();
        if (first.type != token::identifier)
            throw GraphSqlParseStateException(
                        "only identifier could appear in return sentence",
                        first.raw);
        result.second.returnName.push_back(first.raw);
        tokenQueue.pop_front();

        while(!tokenQueue.empty())
        {
            getNextWithType<token::comma>(tokenQueue);
            result.second.returnName.push_back(getNextWithType<token::identifier>(tokenQueue).raw);
            if (!isValidIdentifier(result.first,
                            *result.second.returnName.rbegin()))
                throw GraphSqlParseStateException(
                            "Only node or edge id could appear in return sentence",
                            *result.second.returnName.rbegin());
        }
        return result;
    }

}

namespace netalgo
{
    //GraphSqlParseException
    GraphSqlParseException::GraphSqlParseException():GraphSqlParseException("") {}

    GraphSqlParseException::GraphSqlParseException(const char* inf):
        info(new char[HIDDEN::BUFLEN])
    {
        std::strcpy(info.get(), inf);
    }

    GraphSqlParseException::GraphSqlParseException(const GraphSqlParseException& other):
        info(new char[HIDDEN::BUFLEN])
    {
        std::strcpy(info.get(), other.info.get());
    }

    const char* GraphSqlParseException::what() const noexcept
    {
        return info.get();
    }
    GraphSqlParseException::~GraphSqlParseException()
    {}


    //GraphSqlParseStateException
    GraphSqlParseStateException::GraphSqlParseStateException(const char* inf, std::istream& state) :
            GraphSqlParseException(inf), nearbyChars()
    {
        std::istream::pos_type pos = state.tellg();
        state.seekg(-std::min(pos, std::streampos(HIDDEN::LOOKBACKWARD_LEN)), std::istream::cur);
        char buffer[HIDDEN::BUFLEN];
        state.read(buffer, HIDDEN::CONTEXT_LEN);
        buffer[state.gcount()] = '\0';
        nearbyChars = buffer;
        state.seekg(pos, std::istream::beg);
    }

    GraphSqlParseStateException::GraphSqlParseStateException(const char* inf, const std::string& pNearbyChars):
        GraphSqlParseException(inf), nearbyChars(pNearbyChars)
    {}

    const char* GraphSqlParseStateException::what() const noexcept
    {
        return GraphSqlParseException::what();
    }

    const std::string& GraphSqlParseStateException::getNearbyChars()
    {
        return nearbyChars;
    }

    GraphSqlParseStateException::~GraphSqlParseStateException() {}

    //GraphSqlSentenceParse

    const GraphSqlSentence& parseGraphSql(const char* s)
    {
        return HIDDEN::GraphSqlManager::getParsedSentence(s);
    }

}

const netalgo::GraphSqlSentence& operator""_graphsql(const char* s, size_t size)
{
    assert(size == std::strlen(s));
    return netalgo::parseGraphSql(s);
}
