EXE := server

# wildcard扫描源文件
sources1 := ${wildcard ./WebServer/src/*.cpp}
sources2 := ${wildcard ./AsynLogSystem/src/*.cpp}
sources3 := ${wildcard ./BackEnd/src/*.cpp}
sources4 := ${wildcard ./HttpServer/*.cpp}

headfile1 := ${wildcard ./WebServer/include/*.h}
headfile2 := ${wildcard ./AsynLogSystem/include/*.h}
headfile3 := ${wildcard ./BackEnd/include/*.h}
headfile4 := ${wildcard ./HttpServer/*.h}

#objects1 := ${sources:.cpp=.o}
obj1 := ${patsubst %.cpp,%.o,$(sources1)} 
obj2 := ${patsubst %.cpp,%.o,$(sources2)}
obj3 := ${patsubst %.cpp,%.o,$(sources3)}
obj4 := ${patsubst %.cpp,%.o,$(sources4)}


CC := g++ -std=c++11 -g 
LIB := -lpthread
MQ := `mysql_config --cflags --libs`
JSON := -ljsoncpp
RM := rm -rf

${EXE}: ${obj1} ${obj2} $(obj3) $(obj4)
	${CC} -o $@ $^ ${LIB} ${MQ} ${JSON}

${obj1} ${obj2} ${obj3} ${obj4}: %.o: %.cpp ${headfile1} ${headfile2} ${headfile3} ${headfile4}
	${CC} -o $@ -c $<
#${obj2}: %.o: %.cpp ${headfile2}
#	${CC} -o $@ -c $<

# 伪目标，意味着clean不代表一个真正的文件名
.PHONY: clean cleanall
cleanall:
	${RM} ${EXE} ${obj1} ${obj2}
clean:


	${RM} ${obj1} ${obj2}

