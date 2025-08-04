#pragma once
#include <stdio.h>
#include <string>
#include <vector>
#include <stdlib.h>


class WeAI{
  protected:
    int n_trd = 4;
    int dimin = 321;
    int dimout = 78;
    int dimlen = 1;
    int64_t sizein = 321*80*sizeof(float);
    int64_t sizeout = 78*256*sizeof(float);
    int64_t sizelen = sizeof(int32_t);
    float* bufin = NULL;
    float* bufout = NULL;
    int32_t buflen[1]; 
    int64_t shapein[3]={1,321,80};
    int64_t shapelen[1]={1};
    int64_t shapeout[3]={1,78,256};
    const char* names_in[2]={"speech","speech_lengths"};
    const char* names_out[1]={"encoder_out"};

    virtual int dorun(float* mel,int melcnt,float* bnf,int bnfcnt);
  public:
    WeAI(int melcnt,int bnfcnt,int trd=4);
    int run(float* mel,int melcnt,float* bnf,int bnfcnt);
    int test();
    virtual ~WeAI();
};


#define WENETONNX  1
#ifdef WENETONNX
#include "onnxruntime_cxx_api.h"
class WeOnnx:public WeAI{
  protected:

    //Ort::Value tensorin{nullptr};
    //Ort::Value tensorlen{nullptr};
    //Ort::Value tensorout{nullptr};

    Ort::Env env{nullptr};
    Ort::SessionOptions sessionOptions{nullptr};
    Ort::RunOptions runOptions;
    Ort::Session session{nullptr};
    Ort::MemoryInfo memoryInfo{nullptr};
  protected:
    virtual int dorun(float* mel,int melcnt,float* bnf,int bnfcnt);
  public:
    WeOnnx(std::string modelfn,int mel,int bnf,int trd);
    virtual ~WeOnnx();
};
#endif

#ifdef WENETMNN
class WeMnn:public WeAI{
};
#endif


//#define WENETOPENV 
#ifdef WENETOPENV
#include "openvino/openvino.hpp"
class WeOpvn:public WeAI{
  private:
    ov::element::Type ainput_type = ov::element::f32;
    ov::element::Type binput_type = ov::element::i32;
    ov::element::Type aoutput_type = ov::element::f32;

    ov::Shape ainput_shape = {1, 321,80};
    ov::Shape binput_shape = {1};
    ov::Shape aoutput_shape = {1, 79,256};


    int32_t  binput_data[1];

    ov::Core core;
    ov::InferRequest infer_request ;
    std::string aname = "speech";
    std::string bname = "speech_lengths";
    std::string cname = "encoder_out";
  protected:
    virtual int dorun(float* mel,int melcnt,float* bnf,int bnfcnt);
  public:
    WeOpvn(std::string modelfn,std::string xmlfn,int mel,int bnf,int trd);
    virtual ~WeOpvn();
};
#endif

