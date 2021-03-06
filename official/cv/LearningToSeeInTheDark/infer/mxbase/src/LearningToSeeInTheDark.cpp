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

#include "LearningToSeeInTheDark.h"
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include "MxBase/DeviceManager/DeviceManager.h"
#include "MxBase/Log/Log.h"

const uint32_t EACH_LABEL_LENGTH = 4;
const uint32_t MAX_LENGTH = 12121088;

APP_ERROR LearningToSeeInTheDark::Init(const InitParam &initParam) {
    deviceId_ = initParam.deviceId;
    APP_ERROR ret = MxBase::DeviceManager::GetInstance()->InitDevices();
    if (ret != APP_ERR_OK) {
        LogError << "Init devices failed, ret=" << ret << ".";
        return ret;
    }
    ret = MxBase::TensorContext::GetInstance()->SetContext(initParam.deviceId);
    if (ret != APP_ERR_OK) {
        LogError << "Set context failed, ret=" << ret << ".";
        return ret;
    }
    dvppWrapper_ = std::make_shared<MxBase::DvppWrapper>();
    ret = dvppWrapper_->Init();
    if (ret != APP_ERR_OK) {
        LogError << "DvppWrapper init failed, ret=" << ret << ".";
        return ret;
    }
    model_ = std::make_shared<MxBase::ModelInferenceProcessor>();
    ret = model_->Init(initParam.modelPath, modelDesc_);
    if (ret != APP_ERR_OK) {
        LogError << "ModelInferenceProcessor init failed, ret=" << ret << ".";
        return ret;
    }

    return APP_ERR_OK;
}

APP_ERROR LearningToSeeInTheDark::DeInit() {
    dvppWrapper_->DeInit();
    model_->DeInit();
    MxBase::DeviceManager::GetInstance()->DestroyDevices();
    return APP_ERR_OK;
}

APP_ERROR LearningToSeeInTheDark::ReadTensorFromFile(const std::string &file, uint32_t *data, uint32_t size) {
    if (data == NULL || size < MAX_LENGTH) {
        LogError << "Input data is invalid.";
        return APP_ERR_COMM_INVALID_POINTER;
    }
    std::ifstream infile;
    // open label file
    infile.open(file, std::ios_base::in | std::ios_base::binary);
    // check label file validity
    if (infile.fail()) {
        LogError << "Failed to open label file: " << file << ".";
        return APP_ERR_COMM_OPEN_FAIL;
    }
    infile.read(reinterpret_cast<char*>(data), sizeof(uint32_t)* MAX_LENGTH);
    infile.close();
    return APP_ERR_OK;
}

APP_ERROR LearningToSeeInTheDark::ReadInputTensor(const std::string &fileName,
                                                  std::vector<MxBase::TensorBase> *inputs) {
    uint32_t *data = new uint32_t[MAX_LENGTH]();
    APP_ERROR ret = ReadTensorFromFile(fileName, data, MAX_LENGTH);
    if (ret != APP_ERR_OK) {
        LogError << "ReadTensorFromFile failed.";
        return ret;
    }

    const uint32_t dataSize = MAX_LENGTH*4;
    LogInfo << dataSize;
    MxBase::MemoryData memoryDataDst(dataSize, MxBase::MemoryData::MEMORY_DEVICE, deviceId_);
    MxBase::MemoryData memoryDataSrc(reinterpret_cast<void*>(data), dataSize, MxBase::MemoryData::MEMORY_HOST_MALLOC);
    ret = MxBase::MemoryHelper::MxbsMallocAndCopy(memoryDataDst, memoryDataSrc);
    if (ret != APP_ERR_OK) {
        LogError << GetError(ret) << "Memory malloc and copy failed.";
        return ret;
    }

    std::vector<uint32_t> shape = { 1, MAX_LENGTH };
    inputs->push_back(MxBase::TensorBase(memoryDataDst, false, shape, MxBase::TENSOR_DTYPE_UINT32));
    delete[] data;
    return APP_ERR_OK;
}


APP_ERROR LearningToSeeInTheDark::Inference(const std::vector<MxBase::TensorBase> &inputs,
                                            std::vector<MxBase::TensorBase> &outputs) {
    auto dtypes = model_->GetOutputDataType();
    for (size_t i = 0; i < modelDesc_.outputTensors.size(); ++i) {
        std::vector<uint32_t> shape = {};
        for (size_t j = 0; j < modelDesc_.outputTensors[i].tensorDims.size(); ++j) {
             shape.push_back((uint32_t)modelDesc_.outputTensors[i].tensorDims[j]);
        }
        MxBase::TensorBase tensor(shape, dtypes[i], MxBase::MemoryData::MemoryType::MEMORY_DEVICE, deviceId_);
        APP_ERROR ret = MxBase::TensorBase::TensorBaseMalloc(tensor);
        if (ret != APP_ERR_OK) {
            LogError << "TensorBaseMalloc failed, ret=" << ret << ".";
            return ret;
        }
        outputs.push_back(tensor);
    }
    MxBase::DynamicInfo dynamicInfo = {};
    dynamicInfo.dynamicType = MxBase::DynamicType::STATIC_BATCH;
    auto startTime = std::chrono::high_resolution_clock::now();
    APP_ERROR ret = model_->ModelInference(inputs, outputs, dynamicInfo);
    auto endTime = std::chrono::high_resolution_clock::now();
    double costMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    // save time
    inferCostTimeMilliSec += costMs;
    if (ret != APP_ERR_OK) {
        LogError << "ModelInference failed, ret=" << ret << ".";
        return ret;
    }
    LogInfo << "Inference success";
    return APP_ERR_OK;
}

APP_ERROR  LearningToSeeInTheDark::WriteResult(const std::string &imageFile,
                                               std::vector<MxBase::TensorBase> &outputs) {
    LogInfo << "imageFile:" << imageFile;
    std::string imgName = imageFile.substr(imageFile.find_last_of("/") + 1);
    LogInfo << "imgName:" << imgName;
    size_t dot = imgName.find_last_of(".");
    for (size_t i = 0; i < outputs.size(); ++i) {
        APP_ERROR ret = outputs[i].ToHost();
        if (ret != APP_ERR_OK) {
            LogError << GetError(ret) << "Tohost fail.";
            return ret;
        }
        void *netOutput = outputs[i].GetBuffer();
        std::vector<uint32_t> outshape = outputs[i].GetShape();
        // size_t outputSize = outputs[i].GetByteSize();
        std::string outFileName = "../result/" + imgName.substr(0, dot) + "1.bin";
        LogInfo << "outFileName:" << outFileName;
        FILE *outputFile_ = fopen(outFileName.c_str(), "wb");
        // if (outputFile_ == NULL) {
        //     std::cout << "read fail" << std::endl;
        // }
        fwrite(netOutput, outshape[0] * outshape[1] * outshape[2] * outshape[3], sizeof(float), outputFile_);
        fclose(outputFile_);
        outputFile_ = nullptr;
    }
    return APP_ERR_OK;
}

APP_ERROR LearningToSeeInTheDark::Process(const std::string &fileName) {
    std::vector<MxBase::TensorBase> inputs = {};
    std::vector<MxBase::TensorBase> outputs = {};
    APP_ERROR ret = ReadInputTensor(fileName, &inputs);
    if (ret != APP_ERR_OK) {
        LogError << "Read input failed, ret=" << ret << ".";
        return ret;
    }

    auto startTime = std::chrono::high_resolution_clock::now();
    ret = Inference(inputs, outputs);
    auto endTime = std::chrono::high_resolution_clock::now();
    double costMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    // save time
    inferCostTimeMilliSec += costMs;
    if (ret != APP_ERR_OK) {
        LogError << "Inference failed, ret=" << ret << ".";
        return ret;
    }

    ret = WriteResult(fileName, outputs);
    if (ret != APP_ERR_OK) {
        LogError << "Save infer results into file failed. ret = " << ret << ".";
        return ret;
    }

    return APP_ERR_OK;
}
