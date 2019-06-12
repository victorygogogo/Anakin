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

#ifndef ANAKIN_OPERATOR_DENSE_DENSE_H
#define ANAKIN_OPERATOR_DENSE_DENSE_H

#include "framework/core/base.h"
#include "framework/core/data_types.h"
#include "framework/core/operator/operator.h"
#include "utils/logger/logger.h"
#include "saber/funcs/fc.h"

namespace anakin {

namespace ops {

template<typename Ttype, Precision Ptype>
class DenseDenseHelper;

/**
 * \brief DenseDense operation class
 * public inheritance Operator
 */
template<typename Ttype, Precision Ptype>
class DenseDense : public Operator<Ttype, Ptype> {
public:
    DenseDense() {}

    /// forward impl
    virtual void operator() (OpContext<Ttype> &ctx, 
                             const std::vector<Tensor4dPtr<Ttype> >& ins, 
                             std::vector<Tensor4dPtr<Ttype> >& outs) {
        LOG(ERROR) << "Not Impl Yet Operator  DenseDense<Ttype("
        << target_name<Ttype>::value << "), Precision(" << Ptype << ") >";	
    }

    friend class DenseDenseHelper<Ttype, Ptype>;
};

/**
 * \brief DenseDense helper class 
 * public inherit OperatorHelper
 * including init resource and shape size in densedense context
 */
template<typename Ttype, Precision Ptype>
class DenseDenseHelper : public OperatorHelper<Ttype, Ptype> {
public:
    DenseDenseHelper()=default;

    ~DenseDenseHelper() {}

    Status InitParam() override;

    /**
    * \brief initial all the resource needed by densedense
    * \param ctx stand for DenseDense operation context
    * \param ins stand for input tensor vector
    * \param outs stand for output tensor vector
    * \return status
    */
    Status Init(OpContext<Ttype> &ctx,
                const std::vector<Tensor4dPtr<Ttype> >& ins, 
                std::vector<Tensor4dPtr<Ttype> >& outs) override;

    /**
    * \brief infer the shape of output and input.
    * \param ins stand for input tensor vector
    * \param outs stand for output tensor vector
    * \return status
    */
    Status InferShape(const std::vector<Tensor4dPtr<Ttype> >& ins,
                      std::vector<Tensor4dPtr<Ttype> >& outs) override;

public:
    ///< _param_densedense stand for DenseDense parameter
    saber::FcParam<Ttype>  _param_dense_dense;
    ///< _funcs_densedense stand for DenseDense function
    saber::Fc<Ttype, PrecisionWrapper<Ptype>::saber_type> _funcs_dense_dense;

private:
    ///< _dims stand for DenseDense size
    PTuple<int> _dims; 
};

} /* namespace ops */

} /* namespace anakin */

#endif
