#include "../include/StudentService.h"
#include "../include/CourseService.h"
using namespace csk;


//StudentService::StudentService(int num):course(std::make_shared<CourseService>(num)){}
StudentService::StudentService(int num)
:course(new CourseService(num)){}


void StudentService::setCourseList()
{
    course->getCourseFromMysql();
}
string StudentService::getCourseToSelect()
{  
    return course->getCourseList();
}  
int StudentService::getCourseToSelectSize()
{
    return course->getCourseList().size();
}
int StudentService::selectCourse(string courseJson)
{
    if(course->setCourseToMysql(courseJson))
        return 200;
    return 400;
} 

int StudentService::dropCourse(string courseJson)
{
    if(course->dropCourseToMysql(courseJson))
        return 200;
    return 400;
}
