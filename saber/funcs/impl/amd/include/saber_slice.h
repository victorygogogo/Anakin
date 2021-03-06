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

#ifndef ANAKIN_SABER_FUNCS_AMD_SABER_SLICE_H
#define ANAKIN_SABER_FUNCS_AMD_SABER_SLICE_H

#include "saber/funcs/impl/impl_slice.h"
#include "saber/funcs/base.h"
#include "saber/core/impl/amd/utils/amd_base.h"
#include "saber/saber_types.h"
#include "saber/funcs/impl/impl_base.h"
#include "saber/saber_funcs_param.h"
#include "saber/core/impl/amd/utils/amd_kernel.h"

namespace anakin {

namespace saber {

template <DataType OpDtype>
class SaberSlice<AMD, OpDtype> : public ImplBase<AMD, OpDtype, SliceParam<AMD>> {

public:
    typedef TargetWrapper<AMD> API;
    typedef typename DataTrait<AMD, OpDtype>::Dtype OpDataType;
    typedef AMD_API::TPtr PtrDtype;

    SaberSlice()
        : _slice_num(0)
        , _slice_size(0)
    {
        _kernels_ptr.clear();
    }
    ~SaberSlice() {
        _kernels_ptr.clear();
    }

    virtual SaberStatus
    init(const std::vector<Tensor<AMD>*>& inputs,
         std::vector<Tensor<AMD>*>& outputs,
         SliceParam<AMD>& param,
         Context<AMD>& ctx) override;

    virtual SaberStatus
    create(const std::vector<Tensor<AMD>*>& inputs,
           std::vector<Tensor<AMD>*>& outputs,
           SliceParam<AMD>& param,
           Context<AMD>& ctx) override;

    virtual SaberStatus dispatch(
            const std::vector<Tensor<AMD>*>& inputs,
            std::vector<Tensor<AMD>*>& outputs,
            SliceParam<AMD>& param) override;

private:
    int _slice_num;
    int _slice_size;
    AMDKernelPtr _kernel_Slice_normal;
    std::vector<AMDKernelPtr> _kernels_ptr;
};
template class SaberSlice<AMD, AK_FLOAT>;
} // namespace saber

} // namespace anakin

#endif // ANAKIN_SABER_FUNCS_AMD_SABER_SLICE_H
