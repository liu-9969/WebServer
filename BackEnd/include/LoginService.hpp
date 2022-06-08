#include <string>
#include "../../WebServer/include/MysqlPool.h"
using std::string;
namespace swings
{
std::shared_ptr<Mysql_pool> ConPool = Mysql_pool::GetInstance();

class LoginService
{
    public:
        int login(string name ,string password);
        void getOAuthUser(string userName,string name,string type);
    private:
        bool checkUser(string name , string password);
};

bool LoginService::checkUser(string name , string password)
{
    MYSQL *con = ConPool->GetConnection();
    std::string sql = "select * from userauth where userName = " + name + "and password = " + password;
    if(mysql_query(con,sql.c_str()))
    {
    }
}
int LoginService::login(string name ,string password)
{
    if(checkUser)
        return 200;
    return 201;
}
}