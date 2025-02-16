CXX = g++
INCDIR = include
TARGET = main
CXXFLAGS = -Wall -std=c++20 -I$(INCDIR)
LDFLAGS = -lcurl

# Directories
SRCDIR = dbrex/cpp
BUILDDIR = build

SRCFILES = $(wildcard $(SRCDIR)/*.cpp)
OBJFILES = $(SRCFILES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

all: $(TARGET)

# Linking
$(TARGET): $(OBJFILES)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Compiling
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp | $(BUILDDIR)		# ensure BUILDDIR exists
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILDDIR):
	@mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR) $(TARGET)

.PHONY: all clean