#find ../WebServer/ -name *.o -exec rm -rf {} ../include \;
#find ../AsynLogSystem/ -name *.h -exec cp {} ../AsynLogSystem/include/ \;
#find ../AsynLogSystem/ -name *.cpp -exec cp {} ../AsynLogSystem/src/ \;
find ../log/ -name *.h -exec mv {} ../AsynLogSystem/include \;
#find ../code/ -name *.h -exec mv {} ../include \;