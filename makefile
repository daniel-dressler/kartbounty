BULLET = ./lib/bullet

BULLETFLAGS = -I $(BULLET)/src/
CXXFLAGS += -g -Wall -std=c++11 $(shell pkg-config --cflags sdl2 glew glu gl) $(BULLETFLAGS)

BULLETLIB = $(BULLET)/static_lib/src
BULLETLIBS = $(BULLETLIB)/BulletDynamics/libBulletDynamics.a $(BULLETLIB)/BulletCollision/libBulletCollision.a $(BULLETLIB)/LinearMath/libLinearMath.a
LDLIBS += -g -lm $(shell pkg-config --libs sdl2 glew glu gl) $(BULLETLIBS) \
		  -lXv -lXext -lX11 -lXxf86vm -ldl

ODIR := obj
BDIR := build

SRC = main.cpp Standard.cpp \
	component/events/events.cpp \
	component/input/input.cpp \
	component/rendering/rendering.cpp \
	component/rendering/glhelpers.cpp \
	component/rendering/Outsource/lodepng.cpp \
	component/rendering/SELib/SEStdMath.cpp \
	component/rendering/SELib/SEMatrix.cpp \
	component/rendering/SELib/SETimer.cpp \
	component/rendering/SELib/SEVector.cpp \
	component/rendering/SELib/SEQuaternion.cpp \
	component/rendering/SELib/SEMesh.cpp \
	component/state/state.cpp \
	component/physics/physics.cpp component/physics/GLDebugDrawer.cpp \
	component/entities/entities.cpp
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
