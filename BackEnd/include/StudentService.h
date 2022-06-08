#ifndef STUDENTSERVICE_H_
#define STUDENTSERVICE_H_
#include <string>
#include <json/json.h> 
using std::string;

namespace csk
{
class CourseService; 
class StudentService
{
    public:
    
        StudentService(int num);
        ~StudentService(){delete course;}
        int getCourseToSelectSize();
        void setCourseList();
        string getCourseToSelect();                           // 返回全部课程
        int selectCourse(string courseJson);                  // 提交选课
        int dropCourse(string courseJson);                    // 提交退课
        Json::Value getCourseList(string studentID);
        void getGpa();

    private:
        CourseService *course;
};
}
#endif




