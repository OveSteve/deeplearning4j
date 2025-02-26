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
// @author raver119@gmail.com, created on 19.01.18.
// @author Yurii Shyrma (iuriish@yahoo.com)
//

#include <ops/declarable/helpers/s_t_b.h>

namespace nd4j {
namespace ops {
namespace helpers {


//////////////////////////////////////////////////////////////////////////
template <typename T>
static void batchToSpace_(const NDArray& input, NDArray& output, const uint cropBottom, const uint cropTop, const uint cropLeft, const uint cropRight) {

    // input [bS, H * blockSize, W * blockSize, iC]
    // output [bS, H * blockSize - cropBottom - cropTop, W * blockSize - cropLeft - cropRight, iC]

    // if (cropTop = cropBottom = cropRight = cropLeft = 0) shapes are the same
    // else:
    // oH -> [cropBottom, iH - cropTop]
    // oW -> [cropLeft,   iH - cropRight]
    // xLen > zLen

    const T* x = input.bufferAsT<T>();
          T* z = output.bufferAsT<T>();

    const int rank = 4;

    const Nd4jLong* xShapeInfo = input.getShapeInfo();
    const Nd4jLong* zShapeInfo = output.getShapeInfo();

    const uint bS = xShapeInfo[1];
    const uint iH = xShapeInfo[2];
    const uint iW = xShapeInfo[3];
    const uint iC = xShapeInfo[4];

    // loop through output array
    PRAGMA_OMP_PARALLEL_FOR_SIMD_ARGS(collapse(4))
    for (uint b = 0; b < bS; ++b) {
        for (uint h = cropBottom; h < iH - cropTop; ++h) {
            for (uint w = cropLeft; w < iW - cropRight; ++w) {
                for (uint c = 0; c < iC; ++c) {

                    const Nd4jLong xOffset = b * xShapeInfo[5] + h * xShapeInfo[6] + w * xShapeInfo[7] + c * xShapeInfo[8];

                    const Nd4jLong zOffset = b * zShapeInfo[5] + (h - cropBottom) * zShapeInfo[6] + (w - cropLeft) * zShapeInfo[7] + c * zShapeInfo[8];

                    z[zOffset] = x[xOffset];
                }
            }
        }
    }
}

BUILD_SINGLE_TEMPLATE(template void batchToSpace_, (const NDArray& input, NDArray& output, const uint cropBottom, const uint cropTop, const uint cropLeft, const uint cropRight), LIBND4J_TYPES);

//////////////////////////////////////////////////////////////////////////
void batchToSpace(nd4j::LaunchContext* context, const NDArray& input, NDArray& output, const uint cropBottom, const uint cropTop, const uint cropLeft, const uint cropRight, const uint blockSize) {

    // [bS*blockSize*blockSize, H/blockSize, W/blockSize, iC] is rearranged/permuted to [bS, oH, oW, iC]
    // oH = H - cropTop  - cropBottom
    // oW = W - cropLeft - cropRight

    NDArray inputRearranged0 = input.reshape(input.ordering(), {blockSize, blockSize, output.sizeAt(0), input.sizeAt(1), input.sizeAt(2), input.sizeAt(3)});
    inputRearranged0.permutei({2, 3,0, 4,1, 5});

    if(input.lengthOf() == output.lengthOf())
        output.assign(inputRearranged0);
    else {
        NDArray inputRearranged1 = inputRearranged0.reshape(input.ordering(), {output.sizeAt(0), input.sizeAt(1) * blockSize, input.sizeAt(2) * blockSize, input.sizeAt(3)});
        BUILD_SINGLE_SELECTOR(input.dataType(), batchToSpace_, (inputRearranged1, output, cropBottom, cropTop, cropLeft, cropRight), LIBND4J_TYPES);
    }
}


//////////////////////////////////////////////////////////////////////////
template <typename T>
static void spaceToBatch_(const NDArray& input, NDArray& output, const uint padBottom, const uint padTop, const uint padLeft, const uint padRight) {

    // input [bS, H * blockSize - padBottom - padTop, W * blockSize - padLeft - padRight, iC]
    // output [bs, H * blockSize, W * blockSize, iC]

    // if (padTop = padBottom = padRight = padLeft = 0) shapes are the same
    // else:
    // iH -> [padBottom, oH - padTop]
    // iW -> [padLeft,   oW - padRight]
    // zLen > xLen

    const T* x = input.bufferAsT<T>();
          T* z = output.bufferAsT<T>();

    const int rank = 4;

    const Nd4jLong* xShapeInfo = input.getShapeInfo();
    const Nd4jLong* zShapeInfo = output.getShapeInfo();

    const uint bS = zShapeInfo[1];
    const uint oH = zShapeInfo[2];
    const uint oW = zShapeInfo[3];
    const uint iC = zShapeInfo[4];

    // loop through output array
    PRAGMA_OMP_PARALLEL_FOR_SIMD_ARGS(collapse(4))
    for (uint b = 0; b < bS; ++b) {
        for (uint h = 0; h < oH; ++h) {
            for (uint w = 0; w < oW; ++w) {
                for (uint c = 0; c < iC; ++c) {

                    const Nd4jLong zOffset = b * zShapeInfo[5] + h * zShapeInfo[6] + w * zShapeInfo[7] + c * zShapeInfo[8];

                    if(h >= padBottom && h < oH - padTop && w >= padLeft && w < oW - padRight) {
                        const Nd4jLong xOffset = b * xShapeInfo[5] + (h - padBottom) * xShapeInfo[6] + (w - padLeft) * xShapeInfo[7] + c * xShapeInfo[8];
                        z[zOffset] = x[xOffset];
                    }
                    else
                        z[zOffset] = 0.f;
                }
            }
        }
    }
}

BUILD_SINGLE_TEMPLATE(template void spaceToBatch_, (const NDArray& input, NDArray& output, const uint padBottom, const uint padTop, const uint padLeft, const uint padRight), LIBND4J_TYPES);

//////////////////////////////////////////////////////////////////////////
void spaceToBatch(nd4j::LaunchContext* context, const NDArray& input, NDArray& output, const uint padBottom, const uint padTop, const uint padLeft, const uint padRight, const uint blockSize) {

    // [bS, iH, iW, iC] is rearranged/permuted to [bS*blockSize*blockSize, (iH + padBottom + padTop)/blockSize, (iW + padLeft + padRight)/blockSize, iC]

    NDArray outputRearranged0 = output.reshape(output.ordering(), {blockSize, blockSize, input.sizeAt(0), output.sizeAt(1), output.sizeAt(2), input.sizeAt(3)});
    outputRearranged0.permutei({2, 3,0, 4,1, 5});

    if(input.lengthOf() == output.lengthOf()) {
        outputRearranged0.assign(input);
    }
    else {
        NDArray outputRearranged1 = outputRearranged0.reshape(output.ordering(), {input.sizeAt(0), output.sizeAt(1) * blockSize, output.sizeAt(2) * blockSize, input.sizeAt(3)});
        BUILD_SINGLE_SELECTOR(input.dataType(), spaceToBatch_, (input, outputRearranged1, padBottom, padTop, padLeft, padRight), LIBND4J_TYPES);

        if(output.getBuffer() != outputRearranged1.getBuffer())
            outputRearranged0.assign(outputRearranged1);
    }

}





/*
    template <int N, bool B2S>
    struct SpaceToBatchHelper {
        template <typename T>
        static void run(T *ptrSpace, const Nd4jLong *space_shape, const Nd4jLong *space_strides, const Nd4jLong *block_shape, const Nd4jLong *pad_start, const Nd4jLong *block_offsets, T *ptrBatch, const Nd4jLong *batch_shape, const Nd4jLong *batch_strides) {
            for (int batch_pos = 0; batch_pos < batch_shape[0]; ++batch_pos) {
                const int space_pos = batch_pos * block_shape[0] + block_offsets[0] - pad_start[0];
                if (space_pos >= 0 && space_pos < space_shape[0]) {
                    SpaceToBatchHelper<N - 1, B2S>::run(ptrSpace + space_pos * space_strides[0], space_shape + 1, space_strides + 1, block_shape + 1, pad_start + 1, block_offsets + 1, ptrBatch, batch_shape + 1, batch_strides + 1);
                } else {
                    if (!B2S)
                        for (int i = 0; i < batch_strides[0]; i++)
                            ptrBatch[i] = (T) 0.f;
                }

                ptrBatch += batch_strides[0];
            }
        }
    };

    template <bool B2S>
    struct SpaceToBatchHelper<0, B2S> {
        template <typename T>
        static void run(T *ptrSpace, const Nd4jLong *space_shape, const Nd4jLong *space_strides, const Nd4jLong *block_shape, const Nd4jLong *pad_start, const Nd4jLong *block_offsets, T *ptrBatch, const Nd4jLong *batch_shape, const Nd4jLong *batch_strides) {
            int str = batch_strides[-1];
            for (int i = 0; i < str; i++)
                if (B2S)
                    ptrSpace[i] = ptrBatch[i];
                else
                    ptrBatch[i] = ptrSpace[i];
        }
    };

    template <typename T, int NUM_BLOCK_DIMS, bool B2S>
    void _execute(nd4j::LaunchContext * context, void *vptrSpace, const Nd4jLong *space_shape, const Nd4jLong *space_strides, const Nd4jLong *block_shape, const Nd4jLong *pad_start, const Nd4jLong *block_offsets, void *vptrBatch, const Nd4jLong *batch_shape, const Nd4jLong *batch_strides) {
        auto ptrSpace = reinterpret_cast<T *>(vptrSpace);
        auto ptrBatch = reinterpret_cast<T *>(vptrBatch);
        SpaceToBatchHelper<NUM_BLOCK_DIMS, B2S>::run(ptrSpace, space_shape, space_strides, block_shape, pad_start, block_offsets, ptrBatch, batch_shape, batch_strides);
    };

    Nd4jStatus _spaceToBatch(nd4j::LaunchContext * context, int internal_block_dims, NDArray *input, NDArray *output, std::vector<Nd4jLong> &internal_input_shape, std::vector<Nd4jLong> &internal_output_shape, Nd4jLong *block_shape, Nd4jLong *paddings) {
        auto in = input->reshape('c', internal_input_shape);
        auto out = output->reshape('c', internal_output_shape);
        switch (internal_block_dims) {
            case 1:
                _prepare<1, false>(context, &in, &out, block_shape, paddings);
                break;
            case 2:
                _prepare<2, false>(context, &in, &out, block_shape, paddings);
                break;
            case 3:
                _prepare<3, false>(context, &in, &out, block_shape, paddings);
                break;
            case 4:
                _prepare<4, false>(context, &in, &out, block_shape, paddings);
                break;
            default: {
                return Status::THROW("SpaceToBatch: Wrong number of internal_block_dims");
            }
        }

        return Status::OK();
    }

    Nd4jStatus _batchToSpace(nd4j::LaunchContext * context, int internal_block_dims, NDArray *input, NDArray *output, std::vector<Nd4jLong> &internal_input_shape, std::vector<Nd4jLong> &internal_output_shape, Nd4jLong *block_shape, Nd4jLong *crops) {
        auto in = input->reshape('c', internal_input_shape);
        auto out = output->reshape('c', internal_output_shape);
        switch (internal_block_dims) {
            case 1:
                _prepare<1, true>(context, &in, &out, block_shape, crops);
                break;
            case 2:
                _prepare<2, true>(context, &in, &out, block_shape, crops);
                break;
            case 3:
                _prepare<3, true>(context, &in, &out, block_shape, crops);
                break;
            case 4:
                _prepare<4, true>(context, &in, &out, block_shape, crops);
                break;
            default: {
                return Status::THROW("BatchToSpace: Wrong number of internal_block_dims");
            }
        }

        return Status::OK();
    }

#define STB_DIM (0, 1),\
                (1, 2),\
                (2, 3),\
                (3, 4)

#define STB_BOOL (0, false),\
                 (1, true)

    BUILD_TRIPLE_TEMPLATE(template void _execute, (nd4j::LaunchContext * context, void *ptrSpace, const Nd4jLong *space_shape, const Nd4jLong *space_strides, const Nd4jLong *block_shape, const Nd4jLong *pad_start, const Nd4jLong *block_offsets, void *ptrBatch, const Nd4jLong *batch_shape, const Nd4jLong *batch_strides), LIBND4J_TYPES, STB_DIM, STB_BOOL);

#undef STB_BOOL
#undef STB_DIM
*/

}
}
}