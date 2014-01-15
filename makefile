BULLET = ./lib/bullet

BULLETFLAGS = -I $(BULLET)/src/
CXXFLAGS += -g -Wall -std=c++11 $(shell pkg-config --cflags sdl2 glew glu gl) $(BULLETFLAGS)

BULLETLIB = $(BULLET)/static_lib/src
BULLETLIBS = $(BULLETLIB)/BulletDynamics/libBulletDynamics.a $(BULLETLIB)/BulletCollision/libBulletCollision.a $(BULLETLIB)/LinearMath/libLinearMath.a
LDLIBS += -g $(shell pkg-config --libs sdl2 glew glu gl) $(BULLETLIBS)

ODIR := obj
BDIR := build

SRC = main.cpp glhelpers.cpp Standard.cpp \
	SELib/SEStdMath.cpp SELib/SEMatrix.cpp SELib/SETimer.cpp \
	SELib/SEVector.cpp SELib/SEQuaternion.cpp SELib/SEMesh.cpp \
	component/events/events.cpp \
	component/physics/physics.cpp component/physics/GLDebugDrawer.cpp 
OBJS = $(patsubst %.cpp,$(ODIR)/%.o, $(SRC))

$(BDIR)/kart: $(OBJS) $(BULLET)/built
	@mkdir -p $(@D)
	c++ -o $@ $(OBJS) $(LDLIBS) 

$(ODIR)/%.o: %.cpp
	@mkdir -p $(@D)
	c++ $(CXXFLAGS) -c $< -o $@

$(BULLET)/built:
	cd $(BULLET); \
		mkdir static_lib; \
		cd static_lib; \
		cmake ../ -G "Unix Makefiles" -DBUILD_DEMOS=off -DBUILD_EXTRAS=off -DINSTALL_LIBS=on; \
		make -j4; \
		touch ../built;

.PHONY: clean
clean:
	rm -rf $(BDIR) $(ODIR) $(BULLET)/built
