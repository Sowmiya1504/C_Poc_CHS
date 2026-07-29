# Makefile for Complaint Handling Management System (CHMS)
# Works with GCC on both Linux and Windows (MinGW).

CC      = gcc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2
SOURCES = main.c user.c complaint.c admin.c tracking.c report.c \
          notification.c feedback.c security.c utils.c
TARGET  = cms

ifeq ($(OS),Windows_NT)
    TARGET := cms.exe
    RM = del /Q
else
    RM = rm -f
endif

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	$(RM) $(TARGET) *.o
