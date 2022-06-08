#include "../include/CourseService.h"
#include "../../AsynLogSystem/include/Logging.h"
#include "../../WebServer/include/MysqlPool.h"
#include <iostream>
using namespace csk;


void CourseService::getCourseFromMysql()
{
    std::shared_ptr<swings::Mysql_pool> ConPool = swings::Mysql_pool::GetInstance();
    MYSQL *con = ConPool->GetConnection();
    MYSQL_RES *res = NULL;

    const char* sql = "select * from course";
    if(!mysql_query(con,sql))
    {
        res = mysql_store_result(con);
        if(res)
        {
            int row_num, col_num;
            row_num = mysql_num_rows(res);
            col_num = mysql_num_fields(res);
            //std::cout<<"行"<<row_num<<"列"<<col_num<<std::endl;
            MYSQL_FIELD* field = NULL;
            MYSQL_ROW row;
            Json::Value jsonTable;
            row = mysql_fetch_row(res);
            //int tem = 0;

            //两三百多条数据，暂不考虑pageNum
            while(row != NULL)
            {
                //Json::StreamWriterBuilder builder;
                Json::Value jsonRow;
                //jsonRow["emitUTF8"] = true;
                //builder.setDefaults(&jsonRow);
                
                for (int i = 0; i < col_num; i++)
                {
                    //std::cout<<"第"<<i+1<<"个字段"<<std::endl;
                    field = mysql_fetch_field_direct( res, i );
                    switch(field->type)	
                    {
                    case MYSQL_TYPE_VAR_STRING:
                        jsonRow[field->name] =  Json::Value((row[i]));	
                        break;
                    case MYSQL_TYPE_DECIMAL:
                        jsonRow[field->name] =  Json::Value((row[i]));	
                        break;
                    }
                }
                row = mysql_fetch_row( res );
                jsonTable.append(jsonRow);
            }
            courseList = jsonTable.toStyledString();
            
        }   
    }
    else
        LOG<<CurrentThread::tid()<<mysql_errno(con);
    ConPool->ReleaseConn(con); 
    LOG<<"getFromMysql";  
}


void CourseService::JsonParse(string course)
{
    // 解析Json
    JSONCPP_STRING errs;
    Json::CharReaderBuilder readerBuilder;
    Json::CharReader *jsonReader = readerBuilder.newCharReader(); 
    Json::Value valuer;
    if (jsonReader->parse(course.c_str(), course.c_str()+course.length(),&valuer,&errs))
    {
        string semester = valuer["semester"].asString();
        string year = valuer["year"].asString();
        string courseID = valuer["courseID"].asString();
        string studentName = valuer["studentName"].asString();
        string grade = valuer["grade"].asString();
        string gpa = valuer["gpa"].asString();
        string teacherName = valuer["teacherName"].asString();

        takes = std::make_shared<Takes>(semester,year,courseID,studentName,grade,gpa,teacherName);
    }
}

bool CourseService::setCourseToMysql(string course)
{
    JsonParse(course);
    string sql = "insert into takes(semester, year, courseId, userName, grade, gpa, teacherUserName) values (" 
               + takes->semester + ","
               + takes->year + "," 
               + takes->courseID + "," 
               + takes->studentName + "," 
               + takes->grade + "," 
               + takes->gpa + "," 
               + takes->teacherName + ")";
    std::shared_ptr<swings::Mysql_pool> ConPool = swings::Mysql_pool::GetInstance();
    MYSQL *con = ConPool->GetConnection();
    if(! mysql_query(con,sql.c_str()))
        return false;
    return true;
}

bool CourseService::dropCourseToMysql(string course)
{
    JsonParse(course);
    string tem = "delete from takes where ";
    string sql = tem
               + "courseId = " +  takes->courseID + "and" 
               + "semester = " + takes->semester + "and"
               + "year = " + takes->year + "and"
               + "userName = " + takes->studentName +  "and"
               + "teacherUserName = " + takes->teacherName;
    std::shared_ptr<swings::Mysql_pool> ConPool = swings::Mysql_pool::GetInstance();
    MYSQL *con = ConPool->GetConnection();
    if(! mysql_query(con,sql.c_str()))
        return false;
    return true;
}
