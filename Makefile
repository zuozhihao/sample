# Makefile for Tetris Game (Win32)
# 需要在Windows环境下使用MinGW或Visual Studio编译

CC = g++
CFLAGS = -std=c++11 -Wall -Wextra -O2
LIBS = -ld2d1 -ldwrite -lgdi32 -luser32 -lkernel32
TARGET = tetris.exe
SOURCE = main.cpp

# Windows编译目标
$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LIBS)

# 清理目标
clean:
	del $(TARGET)

# 运行程序
run: $(TARGET)
	./$(TARGET)

.PHONY: clean run