#include "soci.h"
#include "mysql/soci-mysql.h"
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <exception>

using namespace soci;
using namespace std;

bool get_name(string &name)
{
    cout << "Enter name: ";
    return cin >> name;
}

int main()
{
    try
    {
        session sql(mysql, "host=202.120.36.137 port=7891 db=paperieee user=acemap password='xwang8'");

        int count;
        sql << "select count(*) from paper_info", into(count);

        cout << "We have " << count << " entries in the phonebook.\n";

        string id;
        while (get_name(id))
        {
            string phone;
            indicator ind;
            sql << "select Title from paper_info where PaperID = :name",
                into(phone, ind), use(id);

            if (ind == i_ok)
            {
                cout << "The phone number is " << phone << '\n';
            }
            else
            {
                cout << "There is no phone for " << id << '\n';
            }
        }
    }
    catch (exception const &e)
    {
        cerr << "Error: " << e.what() << '\n';
    }
}
