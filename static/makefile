EXE := server

# wildcard扫描源文件
sources1 := ${wildcard ./WebServer/*.cpp}
sources2 := ${wildcard ./log/*.cpp}
sources3 := ${wildcard ./Back_End/*.cpp}

headfile1 := ${wildcard ./WebServer/*.h}
headfile2 := ${wildcard ./log/*.h}
headfile3 := ${wildcard ./Back_End/*.h}

#objects1 := ${sources:.cpp=.o}
obj1 := ${patsubst %.cpp,%.o,$(sources1)} 
obj2 := ${patsubst %.cpp,%.o,$(sources2)}
obj3 := ${patsubst %.cpp,%.o,$(sources3)}


CC := g++ -std=c++11 -g 
LIB := -lpthread
MQ := `mysql_config --cflags --libs`
JSON := -ljsoncpp
RM := rm -rf

${EXE}: ${obj1} ${obj2} $(obj3)
	${CC} -o $@ $^ ${LIB} ${MQ} ${JSON}

${obj1} ${obj2} ${obj3}: %.o: %.cpp ${headfile1} ${headfile2} ${headfile3}
	${CC} -o $@ -c $<
#${obj2}: %.o: %.cpp ${headfile2}
#	${CC} -o $@ -c $<

# 伪目标，意味着clean不代表一个真正的文件名
.PHONY: clean cleanall
cleanall:
	${RM} ${EXE} ${obj1} ${obj2}
clean:
	${RM} ${obj1} ${obj2}

