// Copyright (C) 2018-2025 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
//

#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <sys/timeb.h>
#include <unistd.h>
#include <time.h>

// clang-format off
#include "openvino/openvino.hpp"
#include "openvino/core/preprocess/input_info.hpp"

uint64_t jtimer_msstamp(){
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec*1000l) + (ts.tv_nsec/CLOCKS_PER_SEC);
}

// clang-format on

/**
 * @brief Main with support Unicode paths, wide strings
 */
int main(int argc, char* argv[]) {

        const std::string amodel_path = "wenet.xml";
        const std::string bmodel_path = "wenet.bin";

        // -------- Step 1. Initialize OpenVINO Runtime Core --------
        ov::Core core;

        // -------- Step 2. Read a model --------
        printf("===aaa\n");
        std::shared_ptr<ov::Model> model = core.read_model(amodel_path,bmodel_path);
        printf("===bbb\n");
        //printInputAndOutputsInfo(*model);

        OPENVINO_ASSERT(model->inputs().size() == 2, "Sample supports models with 1 input only");
        OPENVINO_ASSERT(model->outputs().size() == 1, "Sample supports models with 1 output only");

        // -------- Step 3. Set up input

        // Read input image to a tensor and set it to an infer request
        // without resize and layout conversions

        ov::element::Type ainput_type = ov::element::f32;
        ov::Shape ainput_shape = {1, 321,80};
        float*  ainput_data = (float*)malloc(sizeof(float)*321*80);
        memset(ainput_data,0,sizeof(float)*321*80);
        ov::element::Type binput_type = ov::element::i32;
        ov::Shape binput_shape = {1};
        int32_t*  binput_data = (int32_t*)malloc(10);
        *binput_data = 321;

        // just wrap image data by ov::Tensor without allocating of new memory
        ov::Tensor ainput_tensor = ov::Tensor(ainput_type, ainput_shape, ainput_data);
        ov::Tensor binput_tensor = ov::Tensor(binput_type, binput_shape, binput_data);

        //const ov::Layout tensor_layout{"NHWC"};

        // -------- Step 4. Configure preprocessing --------

        ov::preprocess::PrePostProcessor ppp(model);

        // 1) Set input tensor information:
        // - input() provides information about a single model input
        // - reuse precision and shape from already available `input_tensor`
        // - layout of data is 'NHWC'
        std::string aname = "speech";
        ov::preprocess::InputInfo& ainfo = ppp.input(aname);  
        ainfo.tensor().set_shape(ainput_shape).set_element_type(ainput_type);//set_layout(tensor_layout);
        std::string bname = "speech_lengths";
        ov::preprocess::InputInfo& binfo = ppp.input(bname);  
        binfo.tensor().set_shape(binput_shape).set_element_type(binput_type);//set_layout(tensor_layout);
        ainfo.preprocess();                                                                             //
        binfo.preprocess();                                                                             //
        ppp.output().tensor().set_element_type(ov::element::f32);
                                                                                                                                    //
                                                                                                        //
                                                                                                        //
                                                                             //
        // 2) Adding explicit preprocessing steps:
        // - convert layout to 'NCHW' (from 'NHWC' specified above at tensor layout)
        // - apply linear resize from tensor spatial dims to model spatial dims
        //ppp.input().preprocess().resize(ov::preprocess::ResizeAlgorithm::RESIZE_LINEAR);
        // 4) Suppose model has 'NCHW' layout for input
        //ppp.input().model().set_layout("NCHW");
        // 5) Set output tensor information:
        // - precision of tensor is supposed to be 'f32'

        // 6) Apply preprocessing modifying the original 'model'
        model = ppp.build();

        std::string device_name = "CPU";
        // -------- Step 5. Loading a model to the device --------
        ov::CompiledModel compiled_model = core.compile_model(model, device_name,
          ov::inference_num_threads(int(4))
        );

        // -------- Step 6. Create an infer request --------
        ov::InferRequest infer_request = compiled_model.create_infer_request();
        // -----------------------------------------------------------------------------------------------------

        // -------- Step 7. Prepare input --------
        infer_request.set_input_tensor(0,ainput_tensor);
        infer_request.set_input_tensor(1,binput_tensor);

        // -------- Step 8. Do inference synchronously --------
        for(int k=0;k<10000;k++){
    uint64_t tick = jtimer_msstamp();
        infer_request.infer();
    int dist = jtimer_msstamp()-tick;
    printf("===dist %d\n",dist);
    usleep(1000);
        }

        // -------- Step 9. Process output
        const ov::Tensor& output_tensor = infer_request.get_output_tensor();
        const float* data = output_tensor.data<const float>();
        for(int k=0;k<10;k++){
          printf("===%f \n",data[k]);
        }
        //

        // Print classification results
        // -----------------------------------------------------------------------------------------------------

    return EXIT_SUCCESS;
}
