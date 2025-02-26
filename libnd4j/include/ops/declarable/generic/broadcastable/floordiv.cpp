/*******************************************************************************
 * Copyright (c) 2015-2018 Skymind, Inc.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License, Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 ******************************************************************************/

//
//  @author raver119@gmail.com
//

#include <op_boilerplate.h>
#if NOT_EXCLUDED(OP_floordiv)

#include <ops/declarable/generic/helpers/BroadcastHelper.h>
#include <ops/declarable/CustomOperations.h>

namespace nd4j {
    namespace ops {
        BROADCASTABLE_OP_IMPL(floordiv, 0, 0) {
            auto x = INPUT_VARIABLE(0);
            auto y = INPUT_VARIABLE(1);
            auto z = OUTPUT_VARIABLE(0);

            BROADCAST_CHECK_EMPTY(x,y,z);

            REQUIRE_TRUE(!y->isB(), 0, "FLOORDIV OP: you can't divide by bool array!");
            auto tZ = BroadcastHelper::broadcastApply(BroadcastOpsTuple::custom(scalar::FloorDiv, pairwise::FloorDiv, broadcast::FloorDiv), x, y, z);
            if (tZ == nullptr)
                return ND4J_STATUS_KERNEL_FAILURE;
            else if (tZ != z) {
                OVERWRITE_RESULT(tZ);
            }

            return Status::OK();
        }


        DECLARE_TYPES(floordiv) {
            getOpDescriptor()
                    ->setAllowedInputTypes(0, DataType::ANY)
                    ->setAllowedInputTypes(1, DataType::ANY)
                    ->setAllowedOutputTypes(0, DataType::INHERIT);
        }

        DECLARE_TYPES(floordiv_bp) {
            getOpDescriptor()
                    ->setAllowedInputTypes(DataType::ANY)
                    ->setAllowedOutputTypes({ALL_FLOATS});
        }

        CUSTOM_OP_IMPL(floordiv_bp, 3, 2, false, 0, 0) {
            // PLEASE NOTE: we're just passing eps down the line here
            auto x = INPUT_VARIABLE(0);
            auto y = INPUT_VARIABLE(1);
            auto epsNext = INPUT_VARIABLE(2);

            auto gradX = OUTPUT_VARIABLE(0);
            auto gradY = OUTPUT_VARIABLE(1);

            gradY->assign(x);
            std::unique_ptr<NDArray> ySq(y->dup());
            ySq->applyTransform(transform::Square, nullptr);
            gradY->applyPairwiseTransform(pairwise::FloorDiv, ySq.get(), gradY, nullptr);
            gradY->applyPairwiseTransform(pairwise::Multiply, epsNext, gradY, nullptr);
            gradY->applyTransform(transform::Neg, nullptr);
            gradX->assign(epsNext);
            //gradX->applyPairwiseTransform(pairwise::FloorDiv, y, gradX, nullptr);
            return Status::OK();
        }

        DECLARE_SHAPE_FN(floordiv_bp) {
            auto x = inputShape->at(0);
            auto y = inputShape->at(1);
            auto e = inputShape->at(2);

            // eps always has shape of x
            // grad always has shape of y

            Nd4jLong *shapeE;
            Nd4jLong *shapeG;

            COPY_SHAPE(x, shapeE);
            COPY_SHAPE(y, shapeG);

            return SHAPELIST(CONSTANT(shapeE), CONSTANT(shapeG));
        }
    }
}

#endif