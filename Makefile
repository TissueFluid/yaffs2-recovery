CXX = g++
CXXFLAGS = -Wall -Werror -O2
INCLUDE = .
YAFFS2_RECOVERY = yaffs2-recovery

all: $(YAFFS2_RECOVERY)

$(YAFFS2_RECOVERY): yaffs2_recovery.o yaffs2_struct.o
	$(CXX) -o $@ $^

yaffs2_recovery.o: yaffs2_recovery.cc
	$(CXX) -I$(INCLUDE) $(CXXFLAGS) -c $<

yaffs2_struct.o: yaffs2_struct.cc
	$(CXX) -I$(INCLUDE) $(CXXFLAGS) -c $<

clean:
	-rm -f *.o $(YAFFS2_RECOVERY)
