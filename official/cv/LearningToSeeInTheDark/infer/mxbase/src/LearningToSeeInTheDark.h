/**
 * Copyright (c) 2021. Huawei Technologies Co., Ltd. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MXBASE_LEARNINGTOSEEINTHEDARK_H
#define MXBASE_LEARNINGTOSEEINTHEDARK_H

#include <memory>
#include <utility>
#include <vector>
#include <string>
#include <map>
#include <opencv2/opencv.hpp>
#include "MxBase/DvppWrapper/DvppWrapper.h"
#include "MxBase/ModelInfer/ModelInferenceProcessor.h"
#include "MxBase/Tensor/TensorContext/TensorContext.h"

struct InitParam {
    uint32_t deviceId;
    std::string modelPath;
};

class LearningToSeeInTheDark {
 public:
    APP_ERROR Init(const InitParam &initParam);
    APP_ERROR DeInit();
    APP_ERROR Inference(const std::vector<MxBase::TensorBase> &inputs, std::vector<MxBase::TensorBase> &outputs);
    APP_ERROR Process(const std::string &fileName);
    // get infer time
    double GetInferCostMilliSec() const { return inferCostTimeMilliSec; }
 protected:
    APP_ERROR ReadTensorFromFile(const std::string &file, uint32_t *data, uint32_t size);
    APP_ERROR ReadInputTensor(const std::string &fileName, std::vector<MxBase::TensorBase> *inputs);
    APP_ERROR WriteResult(const std::string &imageFile, std::vector<MxBase::TensorBase> &outputs);
 private:
    std::shared_ptr<MxBase::DvppWrapper> dvppWrapper_;
    std::shared_ptr<MxBase::ModelInferenceProcessor> model_;
    MxBase::ModelDesc modelDesc_;
    std::vector<std::string> labelMap_ = {};
    uint32_t deviceId_ = 0;
    // infer time
    double inferCostTimeMilliSec = 0.0;
};

#endif
