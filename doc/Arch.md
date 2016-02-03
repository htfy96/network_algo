#Architecture

##netalgo::remoteGraph

```cpp
class MySQLReader
{
    public:
        MySQLReader(const string& host, const string& dbname, const string& user,
                    const string& passwd="", const string& port="3306");
        void configNode(const string& tableName, const string& IdColumnName);
        void configEdge(const string& tableName, const string& SourceIdColumnName,
                    const string& TargetIdColumnName);

        typedef vector<boost::any> AnyEdge;
        typedef vector<boost::any> AnyNode;

        AnyNode queryNode(const string& nodeId);
        vector<AnyEdge> queryEdgeFrom(const string& nodeId);
        vector<AnyEdge> queryEdgeTo(const string& nodeId);
};
```
