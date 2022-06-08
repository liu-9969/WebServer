#ifndef COURSESERVICE_H_
#define COURSESERVICE_H_
#include <string>
#include <list>
#include <memory>
#include <json/json.h>
using std::string;
using std::list;

namespace csk
{
//class MysqlPool;
class Takes
{
    public:
        Takes(string sem,string year,string couID,string stuN,string gra,string gpa,string teaN)
        :semester(sem),
        year(year),
        courseID(couID),
        studentName(stuN),
        grade(gra),
        gpa(gpa),
        teacherName(teaN){}

        string semester; 
        string year; 
        string courseID; 
        string studentName;
        string grade;
        string gpa;
        string teacherName;
};

class CourseService
{
    private:
        int courseNum;
        string courseList;
        std::shared_ptr<Takes> takes;

    private:
        void JsonParse(string course);
    public:
        CourseService(int num):courseNum(num){}
        
        void getCourseFromMysql();
        bool setCourseToMysql(string course);
        bool dropCourseToMysql(string course);

        string getCourseList(){return courseList;}
        int getCourseNum(){return courseNum;}
        void setCourseNum(int num){courseNum = num;}
};
}

#endif



/*


Json::Reader reader;
    Json::Value valuer;
    string semester;
    string year;
    string courseID;
    string studentName;
    string grade;
    string gpa;
    string teacherName;
    if (reader.parse(course, valuer))
    {
        semester = valuer["semester"].asString();
        year = valuer["year"].asString();
        courseID = valuer["courseID"].asString();
        studentName = valuer["studentName"].asString();
        grade = valuer["grade"].asString();
        gpa = valuer["gpa"].asString();
        teacherName = valuer["teacherName"].asString();

        //takes = std::make_shared<Takes>(semester,year,courseID,studentName,grade,gpa,teacherNmae);
    }










     
// class Course
// {
//     public:
//         Course(string id,string e_type,string name,string c_type,double cre)
//         :courseId(id),
//         electiveType(e_type),
//         courseName(name),
//         courseType(c_type),
//         credits(cre){}
//     private:
//         string courseId; //课程编号
//         string electiveType; //院系编号
//         string courseName; //课程名称
//         string courseType; //课程描述
//         double credits;

// };

*/