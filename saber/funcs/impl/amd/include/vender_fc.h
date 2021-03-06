/* Copyright (c) 2019 Anakin Authors, Inc. All Rights Reserved.

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

#ifndef ANAKIN_VENDER_FUNCS_IMPL_AMD_VENDER_FC_H
#define ANAKIN_VENDER_FUNCS_IMPL_AMD_VENDER_FC_H

#include "saber/funcs/base.h"
#include "saber/funcs/impl/impl_fc.h"
#include "saber/saber_types.h"
#include "saber/saber_funcs_param.h"
#include <miopengemm/miogemm.hpp>
#include <miopengemm/gemm.hpp>
#include <miopengemm/geometry.hpp>
#include "saber/core/impl/amd/utils/amd_kernel.h"

namespace anakin {
namespace saber {

template <DataType OpDtype>
class VenderFc<AMD, OpDtype> : public ImplBase<AMD, OpDtype, FcParam<AMD>> {
public:
    typedef typename DataTrait<AMD, OpDtype>::Dtype OpDataType;
    typedef AMD_API::TPtr PtrDtype;

    VenderFc() {
        _multikernel      = false;
        _pCounterForConv1x1FC = nullptr;
        _kernels_ptr.clear();
    }

    ~VenderFc() {
        if (_pCounterForConv1x1FC) {
            delete _pCounterForConv1x1FC;
        }

        _kernels_ptr.clear();
    }

    virtual SaberStatus
    init(const std::vector<Tensor<AMD>*>& inputs,
         std::vector<Tensor<AMD>*>& outputs,
         FcParam<AMD>& param,
         Context<AMD>& ctx) override;
    virtual SaberStatus
    create(const std::vector<Tensor<AMD>*>& inputs,
           std::vector<Tensor<AMD>*>& outputs,
           FcParam<AMD>& param,
           Context<AMD>& ctx) override;
    virtual SaberStatus dispatch(
        const std::vector<Tensor<AMD>*>& inputs,
        std::vector<Tensor<AMD>*>& outputs,
        FcParam<AMD>& param) override;

private:
    Tensor<AMD>* _pCounterForConv1x1FC {nullptr};
    std::vector<AMDKernelPtr> _kernels_ptr;
    int _branch = 1;
    int _usemacro = 0;
    bool _multikernel;
};

} // namespace saber
} // namespace anakin

#endif
