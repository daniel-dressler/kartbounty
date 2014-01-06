CXXFLAGS += -g $(shell pkg-config --cflags sdl2 glew glu gl)
LDLIBS += -g $(shell pkg-config --libs sdl2 glew glu gl)

ODIR := obj
BDIR := build

SRC = main.cpp glhelpers.cpp Standard.cpp SELib/SEStdMath.cpp SELib/SEMatrix.cpp \
	SELib/SEVector.cpp SELib/SEQuaternion.cpp
OBJS = $(patsubst %.cpp,$(ODIR)/%.o, $(SRC))

$(BDIR)/kart: $(OBJS)
	@mkdir -p $(@D)
	c++ -o $@ $(OBJS) $(LDLIBS) 

$(ODIR)/%.o: %.cpp
	@mkdir -p $(@D)
	c++ $(CXXFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BDIR) $(ODIR)
