
#CC = gcc
CC  = arm-none-linux-gnueabi-gcc -w
CXX = arm-none-linux-gnueabi-g++ -w

CFLAGS        = -pipe -O2 -Wall -W -D_REENTRANT $(DEFINES)
CXXFLAGS      = -pipe -O2 -Wall -W -D_REENTRANT $(DEFINES)

INCPATH      =   -I ./common -I ./HttpServer/include  -I ./libiconv/include

LINK          = arm-none-linux-gnueabi-g++

LFLAGS        = -Wl,-O1 -Wl,-rpath,/home/ctools/arm-2011.03/arm-none-linux-gnueabi/lib

LIBS          =  $(SUBLIBS)  ./alib/CsshClient.a \
							 ./alib/comport.a \
							 ./alib/CabinetClient.a \
							 ./alib/CswitchClient.a \
							 ./alib/CfirewallClient.a \
							 ./alib/WalkClient.a \
							 ./alib/libnetsnmp.a \
							 ./alib/libnetsnmptrapd.a \
							 ./alib/ipcam.a \
							 ./alib/librsu.a \
							 ./alib/libsupervisionzte.a \
							 ./alib/libsupervisionxj.a \
							 ./alib/libxml.a \
							 ./alib/libhttp.a \
							 ./alib/CJsonObject.a \
							 ./solib/libssl.so \
							 ./solib/libcrypto.so \
							 ./solib/libssh2.so.1 \
							 ./solib/libcurl.so.4.5.0  \
							 ./solib/libevent-2.1.so.6.0.4 \
							 ./alib/SpdClient.a \
							 ./alib/canNode.a \
							 ./alib/canport.a \
							 ./alib/liblock.a \
							 ./alib/hw_locker.a \
							 ./alib/libtemhumi.a \
							 ./alib/libcamera.a \
							 ./alib/libaircondition.a \
							 ./alib/libiodev.a \
							 ./alib/libuart.a \
							 ./alib/libmoninterface.a \
							 ./alib/libtcp.a \
							 ./alib/libcsshdev.a \
							 ./alib/libmqtt.a \
							 ./libiconv/lib/*.a \
							 -L./goahead/bin -lgo -ldl  -lrt -lpthread 

TARGET        = tranter


OBJECTS       =  build/main.o     \
				 build/tsPanel.o 	\
                 build/initmodule.o   \
                 build/MyCritical.o		\
                 build/server.o     \
                 build/jsonPackage.o  \
                 build/base_64.o \
                 build/websocket.o \
                 build/webserver.o   \
                 build/lt_state_thread.o \
                 build/global.o \
                 build/tea.o   \
                 build/HttpServer.o \
                 build/config.o   



all: Makefile $(TARGET)

$(TARGET):  $(OBJECTS)  
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)



build/main.o: main.cpp 
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/main.o main.cpp

build/tsPanel.o: tsPanel.cpp \
    common/tsPanel.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/tsPanel.o tsPanel.cpp

build/config.o: config.cpp \
    common/config.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/config.o config.cpp

build/MyCritical.o: MyCritical.cpp \
    common/MyCritical.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/MyCritical.o MyCritical.cpp

build/server.o: server.cpp \
    common/server.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/server.o server.cpp

build/jsonPackage.o: jsonPackage.cpp \
    common/jsonPackage.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/jsonPackage.o jsonPackage.cpp

build/base_64.o: base_64.cpp \
    common/base_64.h
	$(CXX) -c  $(INCPATH) -o build/base_64.o base_64.cpp

build/websocket.o: websocket.cpp \
    common/websocket.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/websocket.o websocket.cpp

build/webserver.o: webserver.cpp \
    common/webserver.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/webserver.o webserver.cpp

build/lt_state_thread.o: lt_state_thread.cpp \
    common/lt_state_thread.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/lt_state_thread.o lt_state_thread.cpp

build/global.o: global.cpp \
    common/global.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/global.o global.cpp

build/initmodule.o: initmodule.cpp \
    common/initmodule.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/initmodule.o initmodule.cpp

build/tea.o: tea.cpp \
    common/tea.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/tea.o tea.cpp

build/HttpServer.o: HttpServer.cpp \
    common/HttpServer.h
	$(CXX) -c $(CXXFLAGS) $(INCPATH) $(LIBS) -o build/HttpServer.o HttpServer.cpp


	
clean:
	@rm -vf $(TARGET) build/*.o *~





