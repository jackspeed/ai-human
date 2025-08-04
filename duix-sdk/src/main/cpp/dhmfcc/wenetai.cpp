#include "wenetai.h"
WeAI::WeAI(int melcnt,int bnfcnt,int trd){
  n_trd = trd;
  dimin = melcnt;
  dimout = bnfcnt;
  sizein = melcnt*80*sizeof(float);
  sizeout = bnfcnt*256*sizeof(float);
  shapein[1] = melcnt;
  shapeout[1] = bnfcnt;
  buflen[0] = melcnt;

  bufin = (float*)malloc(sizein+1024);
  bufout = (float*)malloc(sizeout+1024);
}

WeAI::~WeAI(){
  free(bufin);
  free(bufout);
}

int WeAI::dorun(float* mel,int melcnt,float* bnf,int bnfcnt){
  return 0;
}


int WeAI::run(float* mel,int melcnt,float* bnf,int bnfcnt){
  dimin = melcnt;
  dimout = bnfcnt;
  sizein = melcnt*80*sizeof(float);
  sizeout = bnfcnt*256*sizeof(float);
  shapein[1] = melcnt;
  shapeout[1] = bnfcnt;
  buflen[0] = melcnt;
  return dorun(mel,melcnt,bnf,bnfcnt);
}

int WeAI::test(){
  return dorun(bufin,dimin,bufout,dimout);
}

int WeOnnx::dorun(float* mel,int melcnt,float* bnf,int bnfcnt){
  //
  Ort::Value arrin[2] = {Ort::Value::CreateTensor( memoryInfo, mel ,sizein ,  shapein, 3 ,ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT),Ort::Value::CreateTensor( memoryInfo, buflen ,sizelen ,  shapelen, 1 ,ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32)};
  Ort::Value arrout[1] = {Ort::Value::CreateTensor( memoryInfo, bnf ,sizeout ,  shapeout, 3 ,ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT)};
  session.Run(runOptions, names_in, arrin, 2, names_out,arrout, 1);
  return 0;
}

WeOnnx::WeOnnx(std::string modelfn,int mel,int bnf,int trd):WeAI(mel,bnf,trd){
  //
  env = Ort::Env(OrtLoggingLevel::ORT_LOGGING_LEVEL_WARNING, "wenet");
  sessionOptions = Ort::SessionOptions();
//  sessionOptions.SetIntraOpNumThreads(n_trd);
    sessionOptions.SetIntraOpNumThreads(2);
// todo jth add
  //sessionOptions.SetIntraOpNumThreads(1);
  //sessionOptions.SetInterOpNumThreads(1);
  sessionOptions.AddConfigEntry("session.disable_prepacking", "1");
  sessionOptions.SetGraphOptimizationLevel( GraphOptimizationLevel::ORT_ENABLE_ALL);
  session = Ort::Session(env, modelfn.c_str(), sessionOptions);
  memoryInfo = Ort::MemoryInfo::CreateCpu( OrtAllocatorType::OrtDeviceAllocator, OrtMemType::OrtMemTypeCPU);
  //Ort::MemoryInfo::CreateCpu( OrtAllocatorType::OrtArenaAllocator, OrtMemType::OrtMemTypeDefault);
  //tensorin = Ort::Value::CreateTensor( memoryInfo, bufin ,sizein ,  shapein, 3 ,ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT);
  //tensorlen = Ort::Value::CreateTensor( memoryInfo, buflen ,sizelen ,  shapelen, 1 ,ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32);
  //tensorout = Ort::Value::CreateTensor( memoryInfo, bufout ,sizeout ,  shapeout, 3 ,ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT);
}

WeOnnx::~WeOnnx(){
}


#ifdef WENETOPENV
int WeOpvn::dorun(float* mel,int melcnt,float* bnf,int bnfcnt){
	printf("====opvn run %d \n",sizeout);
	std::cout<<ainput_shape<<std::endl;
	std::cout<<aoutput_shape<<std::endl;
  ov::Tensor ainput_tensor = ov::Tensor(ainput_type, ainput_shape, mel);
  ov::Tensor binput_tensor = ov::Tensor(binput_type, binput_shape, binput_data);
  ov::Tensor aoutput_tensor = ov::Tensor(aoutput_type, aoutput_shape, bnf);
  infer_request.set_input_tensor(0,ainput_tensor);
  infer_request.set_input_tensor(1,binput_tensor);
  infer_request.set_output_tensor(0,aoutput_tensor);
  infer_request.infer();
  //const ov::Tensor& output_tensor = infer_request.get_output_tensor();
  //const float* data = (float*)output_tensor.data();//<const float>();
  //memcpy(bnf,data,sizeout);
  return 0;
}

WeOpvn::WeOpvn(std::string modelfn,std::string xmlfn,int mel,int bnf,int trd):WeAI(mel,bnf,trd){
  std::shared_ptr<ov::Model>  model = core.read_model(xmlfn,modelfn);
  ov::preprocess::PrePostProcessor ppp(model);

  ov::preprocess::InputInfo& ainfo = ppp.input(aname);  
  ov::preprocess::InputInfo&  binfo = ppp.input(bname);  
  ainput_shape[1] = mel;
  aoutput_shape[1] = bnf;
  binput_data[0] = mel;
  ainfo.tensor().set_element_type(ainput_type).set_shape(ainput_shape);
  binfo.tensor().set_element_type(binput_type).set_shape(binput_shape);
  ainfo.preprocess();                                                                             //
  binfo.preprocess();                                                                             //
  ov::preprocess::OutputInfo&  aout = ppp.output(cname);  
  aout.tensor().set_element_type(aoutput_type);

  model = ppp.build();
  std::string device_name = "CPU";
  ov::CompiledModel  compiled_model = core.compile_model(model, device_name,
      ov::inference_num_threads(int(n_trd)) );

  infer_request = compiled_model.create_infer_request();
  //
  //model = nullptr;
}

WeOpvn::~WeOpvn(){

}
#endif
