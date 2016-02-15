#include "debug.hpp"
#include "graphdsl.hpp"
#include "utility.hpp"
#include <sstream>
#include <string>
#include <cstring>
#include <cctype>
#include <functional>
#include <unordered_map>
#include <algorithm>

namespace HIDDEN
{
    using namespace netalgo;
    using namespace std;
    static const size_t BUFLEN=300;
    static const size_t CONTEXT_LEN=30;

    GraphSqlSentence parseGraphSqlImpl(const char* s, size_t size)
    {
        //TODO
    };

    class GraphSqlManager
    {
        private:
            static unordered_map<const char*, GraphSqlSentence> cache;
        public:
            static GraphSqlSentence& getParsedSentence(const char* p, size_t size)
            {
                if (cache.find(p)==cache.end())
                  cache.insert(make_pair(p, parseGraphSqlImpl(p, size)));
                return cache[p];
            }
    };
    unordered_map<const char*, GraphSqlSentence> GraphSqlManager::cache;

    //Implementation of parser
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
        } type;
        std::string raw;
        bool operator==(const token& other) const 
        {
            return type == other.type && raw == other.raw;
        }
        token(const TokenType& t, const string s): type(t), raw(std::move(s)) {}
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
            default:
                is.unget();
                if (lookahead == '+' || lookahead == '-' || isdigit(lookahead))
                  return {token::number, getNumber(is)};
                string identifier = getIdentifier(is);
                for(size_t i = 0; i < arrayLen(KEYWORD_TABLE); ++i)
                  if (identifier == KEYWORD_TABLE[i])
                    return {token::keyword, identifier};
                return {token::identifier, identifier};

        };
        throw GraphSqlParseStateException("Cannot determine the type of token", is);
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
            GraphSqlParseException(inf), state_(state)
    {}
    
    const char* GraphSqlParseStateException::what() const noexcept
    {
        return GraphSqlParseException::what();
    }

    std::string GraphSqlParseStateException::getNearbyChars()
    {
        std::istream::pos_type pos = state_.tellg();
        state_.seekg(-std::min(pos, std::streampos(10)), std::istream::cur);
        char buffer[HIDDEN::BUFLEN];
        state_.readsome(buffer, HIDDEN::CONTEXT_LEN);
        return std::string(buffer);
    }
    
    GraphSqlParseStateException::~GraphSqlParseStateException() {}


    //GraphSqlSentenceParse

    GraphSqlSentence operator""_graphsql(const char* s, size_t size)
    {
        return parseGraphSql(s,size);
    }


    GraphSqlSentence parseGraphSql(const char* s, size_t size)
    {
        return HIDDEN::GraphSqlManager::getParsedSentence(s, size);
    }

}

