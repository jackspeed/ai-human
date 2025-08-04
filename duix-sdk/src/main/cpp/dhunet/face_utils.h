#pragma once
#include <stdint.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
//#include <opencv2/dnn.hpp>
//#include <opencv2/imgproc.hpp>
//#include <opencv2/highgui.hpp>


void dumpchar(char* abuf,int len);
void dumphex(char* abuf,int len);
void dumpshort(short* abuf,int len);
void dumpfloat(float* abuf,int len);
void dumpdouble(double* abuf,int len);
int dumpfile(char* file,char** pbuf);
int diffbuf(char* abuf,char* bbuf,int size);

uint64_t aitimer_msstamp();

