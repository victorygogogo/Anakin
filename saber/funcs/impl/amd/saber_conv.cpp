/* Copyright (c) 2018 Anakin Authors, Inc. All Rights Reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "saber/funcs/impl/amd/include/saber_conv.h"
#include "saber/funcs/conv.h"
#include "third-party/miopen/include/miopen/solver.hpp"
#include "saber/core/impl/amd/utils/amd_file_utils.h"
#include "saber/funcs/impl/amd/include/amd_utils.h"

namespace anakin {
namespace saber {

typedef TargetWrapper<AMD> AMD_API;
typedef Env<AMD> AMD_ENV;
typedef Tensor<AMD> TensorDf4;

template <DataType OpDtype>
SaberStatus SaberConv2D<AMD, OpDtype>::init(
    const std::vector<Tensor<AMD>*>& inputs,
    std::vector<Tensor<AMD>*>& outputs,
    ConvParam<AMD>& param,
    Context<AMD>& ctx) {
    this->_ctx = &ctx;
    return create(inputs, outputs, param, ctx);
}

template <DataType OpDtype>
SaberStatus SaberConv2D<AMD, OpDtype>::create(
    const std::vector<Tensor<AMD>*>& inputs,
    std::vector<Tensor<AMD>*>& outputs,
    ConvParam<AMD>& param,
    Context<AMD>& ctx) {
    ALOGD("create");
    this->_ctx = &ctx;
    KernelInfo kernelInfo;
    AMDKernelPtr kptr;
    _kernels_ptr.clear();

    ALOGD("num=" << inputs[0]->num() << " channel=" << inputs[0]->channel()
          << " height=" << inputs[0]->height() << " width=" << inputs[0]->width());

    ALOGD("stride_h=" << param.stride_h << " stride_w=" << param.stride_w << " height="
          << param.weight()->height() << " width=" << param.weight()->width()
          << " group=" << param.group << " dilation_h=" << param.dilation_h
          << " dilation_w=" << param.dilation_w << " pad_h=" << param.pad_h << " pad_w"
          << param.pad_w << " alpha=" << param.alpha << " beta=" << param.beta);

    if (param.activation_param.has_active) {
        ALOGD("activation.active=" << param.activation_param.active);
    }

    if (param.bias()->valid_size() > 0) {
        ALOGD("bias size=" << param.bias()->size());
    }

    Device<AMD> dev = Env<AMD>::cur_env()[inputs[0]->device_id()]; // anakin device id to AMD device
    cl_device_id device = dev.get_device();
    cl_context context  = dev.get_context();
    bool isBias         = (param.bias()->size() > 0) ? true : false;

    if (param.group == inputs[0]->channel() && param.group == outputs[0]->channel()) {
        kernelInfo.comp_options = std::string(" -DMLO_CONV_BIAS=") + std::to_string(isBias);

        if (param.activation_param.has_active) {
            if (param.activation_param.active == Active_relu) {
                bool isActiveRelu = true;
                kernelInfo.comp_options =
                    std::string(" -DMLO_CONV_ACTIVE_RELU=") + std::to_string(isActiveRelu);
            }
        }

        if ((inputs[0]->channel() == 32) && (inputs[0]->num() == 32)
                && (inputs[0]->height() == 112)) {
            kernelInfo.wk_dim      = 3;
            kernelInfo.l_wk        = {256, 1, 1};
            kernelInfo.g_wk        = {3584, 8, 32};
            kernelInfo.kernel_file = "Depthwiseconv_n32.cl";
            kernelInfo.kernel_name = "DepthwiseconvDw21n32";
        } else if (
            (inputs[0]->channel() == 64) && (inputs[0]->num() == 32)
            && (inputs[0]->height() == 112)) {
            kernelInfo.wk_dim      = 3;
            kernelInfo.l_wk        = {256, 1, 1};
            kernelInfo.g_wk        = {1792, 8, 64};
            kernelInfo.kernel_file = "Depthwiseconv_n32.cl";
            kernelInfo.kernel_name = "DepthwiseconvDw22n32";
        } else if (
            (inputs[0]->channel() == 128) && (inputs[0]->num() == 32)
            && (inputs[0]->height() == 56) && (param.stride_h == 1)) {
            kernelInfo.wk_dim      = 3;
            kernelInfo.l_wk        = {256, 1, 1};
            kernelInfo.g_wk        = {1792, 4, 128};
            kernelInfo.kernel_file = "Depthwiseconv_n32.cl";
            kernelInfo.kernel_name = "DepthwiseconvDw31n32";
        } else if (
            (inputs[0]->channel() == 256) && (inputs[0]->num() == 32)
            && (inputs[0]->height() == 28) && (param.stride_h == 1)) {
            kernelInfo.wk_dim      = 3;
            kernelInfo.l_wk        = {256, 1, 1};
            kernelInfo.g_wk        = {512, 4, 256};
            kernelInfo.kernel_file = "Depthwiseconv_n32.cl";
            kernelInfo.kernel_name = "DepthwiseconvDw41n32";
        } else if (
            (inputs[0]->channel() == 96) && (inputs[0]->num() == 32)
            && (inputs[0]->height() == 112) && (param.stride_h == 2)) {
            kernelInfo.wk_dim      = 3;
            kernelInfo.l_wk        = {256, 1, 1};
            kernelInfo.g_wk        = {1792, 96, 8};
            kernelInfo.kernel_file = "Depthwiseconv_n32.cl";
            kernelInfo.kernel_name = "DepthwiseconvDw222n32";
        } else if (
            (inputs[0]->channel() == 144) && (inputs[0]->num() == 32)
            && (inputs[0]->height() == 56) && (param.stride_h == 1)) {
            kernelInfo.wk_dim      = 3;
            kernelInfo.l_wk        = {256, 1, 1};
            kernelInfo.g_wk        = {1792, 144, 4};
            kernelInfo.kernel_file = "Depthwiseconv_n32.cl";
            kernelInfo.kernel_name = "DepthwiseconvDw231n32";
        } else if (
            (inputs[0]->channel() == 384) && (inputs[0]->num() == 32)
            && (inputs[0]->height() == 28) && (param.stride_h == 1)) {
            kernelInfo.wk_dim      = 3;
            kernelInfo.l_wk        = {256, 1, 1};
            kernelInfo.g_wk        = {98304, 2, 4};
            kernelInfo.kernel_file = "Depthwiseconv_n32.cl";
            kernelInfo.kernel_name = "DepthwiseconvDw242n32";
        } else {
            kernelInfo.wk_dim = 1;
            kernelInfo.l_wk   = {64};
            kernelInfo.g_wk   = {(inputs[0]->num() * inputs[0]->channel() * outputs[0]->height()
                                  * outputs[0]->width()
                                  + 64 - 1)
                                 / 64 * 64
                                };
            kernelInfo.kernel_file = "Depthwiseconv.cl";
            kernelInfo.kernel_name = "Depthwiseconv";
        }

        kptr = CreateKernel(inputs[0]->device_id(), &kernelInfo);

        if (!kptr.get()->isInit()) {
            ALOGE("Failed to load program");
            return SaberInvalidValue;
        }

        _kernels_ptr.push_back(kptr);

    } else {
        miopen::ConvolutionContext convContext;
        convContext.direction.Set(1);
        convContext.general_compile_options += "";
        convContext.n_inputs           = inputs[0]->channel();
        convContext.in_height          = inputs[0]->height();
        convContext.in_width           = inputs[0]->width();
        convContext.kernel_size0       = param.weight()->width();
        convContext.kernel_size1       = param.weight()->height();
        convContext.n_outputs          = param.weight()->num();
        convContext.out_height         = outputs[0]->height();
        convContext.out_width          = outputs[0]->width();
        convContext.batch_sz           = inputs[0]->num();
        convContext.pad0               = param.pad_w;
        convContext.pad1               = param.pad_h;
        convContext.kernel_stride0     = param.stride_w;
        convContext.kernel_stride1     = param.stride_h;
        convContext.kernel_dilation0   = param.dilation_w;
        convContext.kernel_dilation1   = param.dilation_h;
        convContext.bias               = (param.bias()->size() > 0) ? 1 : 0;
        convContext.float_size         = 32;
        convContext.in_stride          = inputs[0]->get_stride()[2];
        convContext.out_stride         = outputs[0]->get_stride()[2];
        convContext.in_channel_stride  = convContext.in_stride * convContext.in_height;
        convContext.in_batch_stride    = convContext.in_channel_stride * convContext.n_inputs;
        convContext.out_channel_stride = convContext.out_stride * convContext.out_height;
        convContext.out_batch_stride   = convContext.out_channel_stride * convContext.n_outputs;
        convContext.has_active         = param.activation_param.has_active ? 1 : 0;
        convContext.negative_slope =
            param.activation_param.has_active ? param.activation_param.negative_slope : 0;
        convContext.rmv             = rocm_meta_version::AMDHSA_1_0;
        convContext.use_binaries    = true;
        convContext.use_asm_kernels = true;
        convContext.do_search       = true;
        convContext.save_srch_req   = true;
        convContext.in_layout       = "NCHW";
        convContext.out_layout      = "NCHW";
        convContext.in_data_type    = "FP32";
        convContext.out_data_type   = "FP32";
        int data_len                = convContext.in_data_type == "FP32" ? 4 : 2;
        convContext.bot_sz = convContext.batch_sz * convContext.n_inputs * convContext.in_height
                             * convContext.in_width * data_len;
        convContext.top_sz = convContext.batch_sz * convContext.n_outputs * convContext.out_height
                             * convContext.out_width * data_len;
        convContext.weights_sz = convContext.n_outputs * convContext.n_inputs
                                 * convContext.kernel_size0 * convContext.kernel_size1 * data_len;
        convContext.bias_sz = (param.bias()->size() > 0) ? convContext.n_outputs * data_len : 0;
        convContext.deconvolution           = 0;
        convContext.general_compile_options = " -DMIOPEN_USE_FP32=1 -DMIOPEN_USE_FP16=0";

        miopen::Db db = anakin::saber::GetDb(dev._info._device_name, dev._info._compute_core_num);
        miopen::Handle::setClEnv(context, device);
        miopen::Handle handle;
        convContext.SetStream(&handle);
        miopen::solver::ConvSolution solution = miopen::solver::SearchForSolution <
                                                miopen::solver::ConvBinWinograd3x3U,
                                                miopen::solver::ConvOclDirectFwd1x1AMD,
                                                /*
                                                            miopen::solver::ConvAsm3x3U,
                                                            miopen::solver::ConvAsm1x1U,
                                                            miopen::solver::ConvAsm7x7c3h224w224k64u2v2p3q3f1,
                                                */
                                                miopen::solver::ConvOclDirectFwdGen,
                                                miopen::solver::ConvOclDirectFwd3x3,
                                                miopen::solver::ConvOclDirectFwd1x1,
                                                miopen::solver::ConvOclDirectFwd > (convContext, db);
        miopen::Handle::clearClEnv();
        ALOGD("Solution wk size:" << solution.workspce_sz
              << " param size:" << solution.construction_params.size());

        if (solution.construction_params.size() > 0) {
            for (auto miKernelInfo : solution.construction_params) {
                kernelInfo = miKernelInfo;

                if (kernelInfo.kernel_name == "conv1x1_act"
                        || kernelInfo.kernel_name == "InnerProduct") {
                    kernelInfo.kernel_type = SABER;
                }

                kptr = CreateKernel(inputs[0]->device_id(), &kernelInfo);

                if (!kptr.get()->isInit()) {
                    ALOGE("Failed to create kernel");
                    return SaberInvalidValue;
                }

                _kernels_ptr.push_back(kptr);
            }
        } else {
            // todo: gemm
            ALOGE("No solution found!!!");
            _outGemmWorkspace = new Tensor<AMD>();
            _outGemmWorkspace->re_alloc(
                Shape({(inputs[0]->num()),
                       std::max((inputs[0]->channel()), (param.weight()->num())),
                       (inputs[0]->height()),
                       (inputs[0]->width())
                      }));

            int K = (inputs[0]->channel()) * (param.weight()->height()) * (param.weight()->width());
            int M = (param.weight()->num());
            int N = (outputs[0]->height()) * (outputs[0]->width());
            float alpha = 1.0;
            float beta  = 0.0;
            bool tA     = false;
            bool tB     = false;
            bool tC     = false;
            int lda     = K;
            int ldb     = N;
            int ldc     = N;

            MIOpenGEMM::Geometry tgg {};
            tgg = MIOpenGEMM::Geometry(true, tB, tA, tC, ldb, lda, ldc, N, M, K, 0, 'f');
            AMD_API::stream_t cm = this->_ctx->get_compute_stream();

            /////////////////////////////////////////////////////////////
            // gemm kernel
            // jn : print search results to terminal
            bool miopengemm_verbose = false;

            // jn : print warning messages when the returned kernel(s) might be sub-optimal
            bool miopengemm_warnings = false;

            // jn : find with no workspace
            MIOpenGEMM::Solution soln = MIOpenGEMM::find(
                                            0.003f,
                                            cm,
                                            (cl_mem)inputs[0]->data(),
                                            (cl_mem)param.weight()->data(),
                                            (cl_mem)_outGemmWorkspace->mutable_data(),
                                            false,
                                            tgg,
                                            miopengemm_verbose,
                                            miopengemm_warnings);

            std::string kernel_clstring;
            size_t local_work_size;
            size_t global_work_size;
            cl_int errCode;

            for (int j = 0; j < inputs[0]->num(); j++) {
                int i = 0;

                if (soln.v_tgks.size() == 2) {
                    _multikernel = true;

                    // jn : the main kernel is at the back of the solution vector
                    kernel_clstring = soln.v_tgks[i].kernstr;
                    tempfix::set_offsets_to_uint(kernel_clstring, 1);

                    kernelInfo.kernel_name = soln.v_tgks[i].fname;
                    local_work_size        = soln.v_tgks[i].local_work_size;
                    global_work_size       = soln.v_tgks[i].global_work_size;

                    kernelInfo.kernel_file = kernel_clstring;
                    kernelInfo.l_wk        = {local_work_size, 1, 1};
                    kernelInfo.g_wk        = {global_work_size, 1, 1};
                    kernelInfo.kernel_type = SOURCE;

                    kptr = CreateKernel(inputs[0]->device_id(), &kernelInfo);

                    if (!kptr.get()->isInit()) {
                        ALOGE("Failed to create kernel");
                        return SaberInvalidValue;
                    }

                    _kernels_ptr.push_back(kptr);

                    i++;
                }

                // jn : the main kernel is at the back of the solution vector
                kernel_clstring = soln.v_tgks[i].kernstr;
                tempfix::set_offsets_to_uint(kernel_clstring, 3);

                kernelInfo.kernel_name = soln.v_tgks[i].fname;
                local_work_size        = soln.v_tgks[i].local_work_size;
                global_work_size       = soln.v_tgks[i].global_work_size;

                kernelInfo.kernel_file = kernel_clstring;
                kernelInfo.l_wk        = {local_work_size, 1, 1};
                kernelInfo.g_wk        = {global_work_size, 1, 1};
                kernelInfo.kernel_type = SOURCE;

                // To create the program
                kptr = CreateKernel(inputs[0]->device_id(), &kernelInfo);

                if (!kptr.get()->isInit()) {
                    ALOGE("Failed to create kernel");
                    return SaberInvalidValue;
                }

                _kernels_ptr.push_back(kptr);
            }

            /////////////////////////////////////////////////////////////
            // Bias relu kernel
            if (isBias) {
                kernelInfo.kernel_file = "MIOpenBiasReLuUni.cl";

                if (param.activation_param.has_active) {
                    kernelInfo.kernel_name = "MIOpenBiasReluBoth";
                } else {
                    kernelInfo.kernel_name = "MIOpenBias";
                }

                kernelInfo.l_wk = {256, 1, 1};
                kernelInfo.g_wk = {(inputs[0]->num())* (param.weight()->num())
                                   * (outputs[0]->height())* (outputs[0]->width()),
                                   1,
                                   1
                                  };

                kernelInfo.kernel_type = SABER;

                // To create the program
                kptr = CreateKernel(inputs[0]->device_id(), &kernelInfo);

                if (!kptr.get()->isInit()) {
                    ALOGE("Failed to create kernel");
                    return SaberInvalidValue;
                }

                _kernels_ptr.push_back(kptr);
            } else {
                if (param.activation_param.has_active) {
                    kernelInfo.kernel_file = "ReluUni.cl";
                    kernelInfo.kernel_name = "ReluUni";

                    kernelInfo.l_wk        = {256, 1, 1};
                    kernelInfo.g_wk        = {(inputs[0]->num())* (param.weight()->num())
                                              * (outputs[0]->height())* (outputs[0]->width()),
                                              1,
                                              1
                                             };
                    kernelInfo.kernel_type = SABER;

                    // To create the program
                    kptr = CreateKernel(inputs[0]->device_id(), &kernelInfo);

                    if (!kptr.get()->isInit()) {
                        ALOGE("Failed to create kernel");
                        return SaberInvalidValue;
                    }

                    _kernels_ptr.push_back(kptr);
                }
            }
        }
    }

    ALOGD("COMPLETE CREATE KERNEL");

    return SaberSuccess;
}

template <DataType OpDtype>
SaberStatus SaberConv2D<AMD, OpDtype>::dispatch(
    const std::vector<Tensor<AMD>*>& inputs,
    std::vector<Tensor<AMD>*>& outputs,
    ConvParam<AMD>& param) {

    ALOGD("dispatch");
    int err;
    amd_kernel_list list;
    bool isBias   = false;
    bool isActive = false;

    // To get the commpute command queue
    AMD_API::stream_t cm = this->_ctx->get_compute_stream();

    // To set the argument
    PtrDtype memObjects[4] = {0, 0, 0, 0};

    ALOGD(" num=" << inputs[0]->num() << " channel=" << inputs[0]->channel()
          << " height=" << inputs[0]->height() << " width=" << inputs[0]->width()
          << " param.weight()->num()=" << param.weight()->num()
          << " param.weight()->channel()=" << param.weight()->channel()
          << " param.weight()->width()=" << param.weight()->width()
          << " param.weight()->height()=" << param.weight()->height() << " param.group="
          << param.group << " param.pad_h=" << param.pad_h << " param.pad_w=" << param.pad_w
          << " param.stride_h=" << param.stride_h << " param.stride_w=" << param.stride_w
          << " param.dilation_h=" << param.dilation_h
          << " param.dilation_w=" << param.dilation_w << " param.alpha=" << param.alpha
          << " param.beta=" << param.beta);

    if (param.bias()->size() > 0) {
        isBias = true;
        ALOGD(" param.bias()->size()=" << param.bias()->size()
              << " param.bias()->channel()=" << param.bias()->channel()
              << " param.bias()->width()=" << param.bias()->width()
              << " param.bias()->height()=" << param.bias()->height());
    }

    if (param.activation_param.has_active) {
        isActive = true;
        ALOGD(" param.activation_param.has_active="
              << param.activation_param.has_active
              << " param.activation_param.negative_slope=" << param.activation_param.negative_slope
              << " param.activation_param.active=" << param.activation_param.active
              << " param.activation_param.coef=" << param.activation_param.coef);
    }

    if (_kernels_ptr[0] == NULL || _kernels_ptr[0].get() == NULL) {
        ALOGE("Kernel is not exist");
        return SaberInvalidValue;
    }

    ALOGD("kernel size:" << _kernels_ptr.size() << " name:" << _kernels_ptr[0].get()->GetName());

    if (param.group == inputs[0]->channel() && param.group == outputs[0]->channel()) {

        if (((inputs[0]->channel() == 32) && (inputs[0]->num() == 32)
                && (inputs[0]->height() == 112))
                || ((inputs[0]->channel() == 64) && (inputs[0]->num() == 32)
                    && (inputs[0]->height() == 112))
                || ((inputs[0]->channel() == 128) && (inputs[0]->num() == 32)
                    && (inputs[0]->height() == 56) && (param.stride_h == 1) && (param.stride_w == 1))
                || ((inputs[0]->channel() == 256) && (inputs[0]->num() == 32)
                    && (inputs[0]->height() == 28) && (param.stride_h == 1) && (param.stride_w == 1))
                || ((inputs[0]->channel() == 96) && (inputs[0]->num() == 32)
                    && (inputs[0]->height() == 112) && (param.stride_h == 2) && (param.stride_w == 2))
                || ((inputs[0]->channel() == 144) && (inputs[0]->num() == 32)
                    && (inputs[0]->height() == 56) && (param.stride_h == 1) && (param.stride_w == 1))
                || ((inputs[0]->channel() == 384) && (inputs[0]->num() == 32)
                    && (inputs[0]->height() == 28) && (param.stride_h == 1) && (param.stride_w == 1))) {
            if (isBias) {
                err = _kernels_ptr[0].get()->SetKernelArgs(
                          (PtrDtype)outputs[0]->mutable_data(),
                          (PtrDtype)inputs[0]->data(),
                          (PtrDtype)param.weight()->data(),
                          (PtrDtype)param.bias()->data());
            } else {
                err = _kernels_ptr[0].get()->SetKernelArgs(
                          (PtrDtype)outputs[0]->mutable_data(),
                          (PtrDtype)inputs[0]->data(),
                          (PtrDtype)param.weight()->data());
            }
        } else {
            if (isBias) {
                err = _kernels_ptr[0].get()->SetKernelArgs(
                          (PtrDtype)inputs[0]->data(),
                          (int)inputs[0]->num(),
                          (int)inputs[0]->channel(),
                          (int)inputs[0]->height(),
                          (int)inputs[0]->width(),
                          (int)outputs[0]->height(),
                          (int)outputs[0]->width(),
                          (int)param.weight()->height(),
                          (int)param.weight()->width(),
                          (int)param.stride_h,
                          (int)param.stride_w,
                          (int)param.pad_h,
                          (int)param.pad_w,
                          (PtrDtype)outputs[0]->mutable_data(),
                          (PtrDtype)param.weight()->data(),
                          (PtrDtype)param.bias()->data());
            } else {
                err = _kernels_ptr[0].get()->SetKernelArgs(
                          (PtrDtype)inputs[0]->data(),
                          (int)inputs[0]->num(),
                          (int)inputs[0]->channel(),
                          (int)inputs[0]->height(),
                          (int)inputs[0]->width(),
                          (int)outputs[0]->height(),
                          (int)outputs[0]->width(),
                          (int)param.weight()->height(),
                          (int)param.weight()->width(),
                          (int)param.stride_h,
                          (int)param.stride_w,
                          (int)param.pad_h,
                          (int)param.pad_w,
                          (PtrDtype)outputs[0]->mutable_data(),
                          (PtrDtype)param.weight()->data());
            }
        }

        if (!err) {
            ALOGE("Fail to set kernel args :" << err);
            return SaberInvalidValue;
        }

        list.push_back(_kernels_ptr[0]);
        err = LaunchKernel(cm, list);

        if (!err) {
            ALOGE("Fail to set execution :" << err);
            return SaberInvalidValue;
        }

    } else {
        memObjects[0] = (PtrDtype)inputs[0]->data();
        memObjects[1] = (PtrDtype)param.weight()->data();
        memObjects[2] = (isBias) ? (PtrDtype)param.bias()->data() : nullptr;
        memObjects[3] = (PtrDtype)outputs[0]->mutable_data();

        for (int i = 0; i < _kernels_ptr.size(); i++) {
            if ((_kernels_ptr[i].get()->GetName() == "MIOpenConvUni")
                    || (_kernels_ptr[i].get()->GetName() == "MIOpenConv1x1")
                    || (_kernels_ptr[i].get()->GetName() == "MIOpenConv1x1pquv")
                    || (_kernels_ptr[i].get()->GetName() == "MIOpenCvD3x3_WSR0")
                    || (_kernels_ptr[i].get()->GetName() == "MIOpenCDFGen")
                    || (_kernels_ptr[i].get()->GetName() == "MIOpenCDFGen4")) {

                if (isBias) {
                    if (isActive) {
                        err = _kernels_ptr[i].get()->SetKernelArgs(
                                  (PtrDtype)memObjects[0],
                                  (PtrDtype)memObjects[1],
                                  (PtrDtype)memObjects[2],
                                  (PtrDtype)memObjects[3],
                                  param.activation_param.negative_slope,
                                  0.0f);
                    } else {
                        err = _kernels_ptr[i].get()->SetKernelArgs(
                                  (PtrDtype)memObjects[0],
                                  (PtrDtype)memObjects[1],
                                  (PtrDtype)memObjects[2],
                                  (PtrDtype)memObjects[3],
                                  0.0f);
                    }
                } else {
                    if (isActive) {
                        err = _kernels_ptr[i].get()->SetKernelArgs(
                                  (PtrDtype)memObjects[0],
                                  (PtrDtype)memObjects[1],
                                  (PtrDtype)memObjects[3],
                                  param.activation_param.negative_slope,
                                  0.0f);
                    } else {
                        err = _kernels_ptr[i].get()->SetKernelArgs(
                                  (PtrDtype)memObjects[0],
                                  (PtrDtype)memObjects[1],
                                  (PtrDtype)memObjects[3],
                                  0.0f);
                    }
                }

                if (!err) {
                    ALOGE("Fail to set kernel args :" << err);
                    return SaberInvalidValue;
                }

                list.push_back(_kernels_ptr[i]);
            } else if (_kernels_ptr[i].get()->GetName() == "sp3AsmConv3x3F") {
                int d_n_groups = 64, d_flags = 0;

                err = _kernels_ptr[i].get()->SetKernelArgs(
                          (unsigned int)inputs[0]->num(),
                          (unsigned int)inputs[0]->channel(),
                          (unsigned int)inputs[0]->height(),
                          (unsigned int)inputs[0]->width(),
                          (unsigned int)param.weight()->num(),
                          (unsigned int)d_n_groups,
                          (unsigned int)d_flags,
                          param.activation_param.negative_slope,
                          (PtrDtype)memObjects[0],
                          (PtrDtype)memObjects[1],
                          (PtrDtype)memObjects[3],
                          (PtrDtype)memObjects[2]);

                if (!err) {
                    ALOGE("Fail to set kernel args :" << err);
                    return SaberInvalidValue;
                }

                list.push_back(_kernels_ptr[i]);
            } else if (_kernels_ptr[i].get()->GetName() == "conv1x1_act") {
                float negative_slope = 0.0f;

                if (isActive) {
                    negative_slope = param.activation_param.negative_slope;
                } else {
                    negative_slope = 1.0f;
                }

                if (isBias) {
                    err = _kernels_ptr[i].get()->SetKernelArgs(
                              (PtrDtype)memObjects[1],
                              (PtrDtype)memObjects[0],
                              (PtrDtype)memObjects[2],
                              (PtrDtype)memObjects[3],
                              negative_slope);
                } else {
                    err = _kernels_ptr[i].get()->SetKernelArgs(
                              (PtrDtype)memObjects[1],
                              (PtrDtype)memObjects[0],
                              (PtrDtype)memObjects[3],
                              negative_slope);
                }

                if (!err) {
                    ALOGE("Fail to set kernel args :" << err);
                    return SaberInvalidValue;
                }

                list.push_back(_kernels_ptr[i]);
            } else if (_kernels_ptr[i].get()->GetName() == "InnerProduct") {
                // todo: need review activation!!
                if (isBias) {
                    err = _kernels_ptr[i].get()->SetKernelArgs(
                              (PtrDtype)memObjects[0],
                              (PtrDtype)memObjects[1],
                              (PtrDtype)memObjects[2],
                              (PtrDtype)memObjects[3]);
                } else {
                    err = _kernels_ptr[i].get()->SetKernelArgs(
                              (PtrDtype)memObjects[0],
                              (PtrDtype)memObjects[1],
                              (PtrDtype)memObjects[3]);
                }

                if (!err) {
                    ALOGE("Fail to set kernel args :" << err);
                    return SaberInvalidValue;
                }

                list.push_back(_kernels_ptr[i]);
            } else {
                ALOGD("Not implement kernel name:" << _kernels_ptr[i].get()->GetName());

                if (_multikernel) {
                    ALOGD("kernel name :" << _kernels_ptr[i].get()->GetName());
                    err = _kernels_ptr[i].get()->SetKernelArgs(memObjects[3], 0, 0.0f);

                    if (!err) {
                        ALOGE("Fail to set kernel args :" << err);
                        return SaberInvalidValue;
                    }

                    list.push_back(_kernels_ptr[i++]);
                    CHECK_NOTNULL(_kernels_ptr[i]);
                    ALOGD("kernel name :" << _kernels_ptr[i].get()->GetName());
                    err = _kernels_ptr[i].get()->SetKernelArgs(
                              memObjects[0], 0, memObjects[1], 0, memObjects[3], 0, 1.0f);
                } else {
                    err = _kernels_ptr[i].get()->SetKernelArgs(
                              memObjects[0], 0, memObjects[1], 0, memObjects[3], 0, 1.0f, 0.0f);
                }

                if (!err) {
                    ALOGE("Fail to set kernel args :" << err);
                    return SaberInvalidValue;
                }

                list.push_back(_kernels_ptr[i]);

                if (isBias) {
                    if (isActive) {
                        err = _kernels_ptr[++i].get()->SetKernelArgs(
                                  memObjects[3],
                                  memObjects[3],
                                  memObjects[2],
                                  param.activation_param.negative_slope,
                                  (inputs[0]->num()),
                                  (outputs[0]->channel()),
                                  (outputs[0]->height()),
                                  (outputs[0]->width()),
                                  1,
                                  1);
                    } else {
                        err = _kernels_ptr[++i].get()->SetKernelArgs(
                                  memObjects[3],
                                  memObjects[3],
                                  memObjects[2],
                                  1.0f,
                                  (inputs[0]->num()),
                                  (outputs[0]->channel()),
                                  (outputs[0]->height()),
                                  (outputs[0]->width()));
                    }

                    if (!err) {
                        ALOGE("Fail to set kernel args :" << err);
                        return SaberInvalidValue;
                    }

                    list.push_back(_kernels_ptr[i]);
                } else {
                    if (isActive) {
                        err = _kernels_ptr[++i].get()->SetKernelArgs(
                                  memObjects[3],
                                  memObjects[3],
                                  param.activation_param.negative_slope);

                        if (!err) {
                            ALOGE("Fail to set kernel args :" << err);
                            return SaberInvalidValue;
                        }

                        list.push_back(_kernels_ptr[i]);
                    }
                }
            }
        }

        err = LaunchKernel(cm, list);

        if (!err) {
            ALOGE("Fail to set execution :" << err);
            return SaberInvalidValue;
        }
    }

    ALOGD("COMPLETE EXECUTION");

    return SaberSuccess;
}
template class SaberConv2D<AMD, AK_FLOAT>;
template class SaberConv2D<AMD, AK_HALF>;
template class SaberConv2D<AMD, AK_INT8>;
} // namespace saber
} // namespace anakin
