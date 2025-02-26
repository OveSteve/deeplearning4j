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
#include <loops/broadcasting.h>
#include <loops/legacy_ops.h>
#include <types/types.h>
#include <Environment.h>
#include <cuda.h>
#include <cuda_runtime.h>
#include <string>
#include <stdexcept>
#include <StringUtils.h>
#include <specials_cuda.h>

using namespace simdOps;

template<typename X, typename Y, typename Z, typename OpClass>
static __global__ void broadcastSimple(
        void *x,
        Nd4jLong *xShapeInfo,
        void *y,
        Nd4jLong *yShapeInfo,
        void *z,
        Nd4jLong *zShapeInfo,
        int *dimension,
        int dimensionLength, Nd4jLong *tadOnlyShapeInfo, Nd4jLong *tadOffsets, Nd4jLong *tadOnlyShapeInfoZ, Nd4jLong *tadOffsetsZ) {
    
    functions::broadcast::Broadcast<X,Y,Z>::template transformCuda<OpClass>(x,xShapeInfo,y,yShapeInfo,z,zShapeInfo,dimension,dimensionLength,tadOnlyShapeInfo,tadOffsets,tadOnlyShapeInfoZ,tadOffsetsZ);
}

template<typename X, typename Y, typename Z, typename OpClass>
static __global__ void broadcastInverseSimple(
        void *x,
        Nd4jLong *xShapeInfo,
        void *y,
        Nd4jLong *yShapeInfo,
        void *z,
        Nd4jLong *zShapeInfo,
        int *dimension,
        int dimensionLength, Nd4jLong *tadOnlyShapeInfo, Nd4jLong *tadOffsets, Nd4jLong *tadOnlyShapeInfoZ, Nd4jLong *tadOffsetsZ) {

    functions::broadcast::Broadcast<X,Y,Z>::template transformInverseCuda<OpClass>(x,xShapeInfo,y,yShapeInfo,z,zShapeInfo,dimension,dimensionLength,tadOnlyShapeInfo,tadOffsets,tadOnlyShapeInfoZ,tadOffsetsZ);
}

namespace functions {
    namespace broadcast {

        template<typename X, typename Y, typename Z>
        template <typename OpClass>
        __host__ void Broadcast<X,Y,Z>::intermediateBroadcast(dim3 launchDims, cudaStream_t *stream, void *x, Nd4jLong *xShapeInfo, void *y, Nd4jLong *yShapeInfo, void *z, Nd4jLong *zShapeInfo, int *dimension, int dimensionLength, Nd4jLong *tadOnlyShapeInfo, Nd4jLong *tadOffsets, Nd4jLong *tadOnlyShapeInfoZ, Nd4jLong *tadOffsetsZ) {
            broadcastSimple<X, Y, Z, OpClass><<<launchDims.x, launchDims.y, launchDims.z, *stream>>>(x, xShapeInfo, y, yShapeInfo, z, zShapeInfo, dimension, dimensionLength, tadOnlyShapeInfo, tadOffsets, tadOnlyShapeInfoZ, tadOffsetsZ);
        }

        template<typename X, typename Y, typename Z>
        __host__ void Broadcast<X,Y,Z>::execBroadcast(dim3 launchDims, cudaStream_t *stream, int opNum, void *x, Nd4jLong *xShapeInfo, void *y, Nd4jLong *yShapeInfo, void *z, Nd4jLong *zShapeInfo, int *dimension, int dimensionLength, Nd4jLong *tadOnlyShapeInfo, Nd4jLong *tadOffsets, Nd4jLong *tadOnlyShapeInfoZ, Nd4jLong *tadOffsetsZ) {
            DISPATCH_BY_OPNUM_TTT(intermediateBroadcast,  PARAMS(launchDims, stream, x, xShapeInfo, y, yShapeInfo, z, zShapeInfo, dimension, dimensionLength, tadOnlyShapeInfo, tadOffsets, tadOnlyShapeInfoZ, tadOffsetsZ), OPS_A(BROADCAST_OPS))

	        DEBUG_KERNEL(stream, opNum);
        }

        template<typename X, typename Y, typename Z>
        template <typename OpClass>
        __host__ void Broadcast<X,Y,Z>::intermediateInverseBroadcast(dim3 launchDims, cudaStream_t *stream, void *x, Nd4jLong *xShapeInfo, void *y, Nd4jLong *yShapeInfo, void *z, Nd4jLong *zShapeInfo, int *dimension, int dimensionLength, Nd4jLong *tadOnlyShapeInfo, Nd4jLong *tadOffsets, Nd4jLong *tadOnlyShapeInfoZ, Nd4jLong *tadOffsetsZ) {
            broadcastInverseSimple<X, Y, Z, OpClass><<<launchDims.x, launchDims.y, launchDims.z, *stream>>>(x, xShapeInfo, y, yShapeInfo, z, zShapeInfo, dimension, dimensionLength, tadOnlyShapeInfo, tadOffsets, tadOnlyShapeInfoZ, tadOffsetsZ);
        }

        template<typename X, typename Y, typename Z>
        __host__ void Broadcast<X,Y,Z>::execInverseBroadcast(dim3 launchDims, cudaStream_t *stream, int opNum, void *x, Nd4jLong *xShapeInfo, void *y, Nd4jLong *yShapeInfo, void *z, Nd4jLong *zShapeInfo, int *dimension, int dimensionLength, Nd4jLong *tadOnlyShapeInfo, Nd4jLong *tadOffsets, Nd4jLong *tadOnlyShapeInfoZ, Nd4jLong *tadOffsetsZ) {
            DISPATCH_BY_OPNUM_TTT(intermediateInverseBroadcast,  PARAMS(launchDims, stream, x, xShapeInfo, y, yShapeInfo, z, zShapeInfo, dimension, dimensionLength, tadOnlyShapeInfo, tadOffsets, tadOnlyShapeInfoZ, tadOffsetsZ), OPS_A(BROADCAST_OPS))

            DEBUG_KERNEL(stream, opNum);
        }

        template<typename X, typename Y, typename Z>
        template <typename OpType>
        __device__ void Broadcast<X,Y,Z>::transformInverseCuda(
                void *vx, Nd4jLong *xShapeInfo,
                void *vy, Nd4jLong *yShapeInfo,
                void *vz, Nd4jLong *zShapeInfo,
                int *dimension, int dimensionLength,
                Nd4jLong *tadOnlyShapeInfo, Nd4jLong *tadOffsets, Nd4jLong *tadOnlyShapeInfoZ, Nd4jLong *tadOffsetsZ) {

            if (tadOnlyShapeInfoZ == nullptr) {
                tadOnlyShapeInfoZ = tadOnlyShapeInfo;
                tadOffsetsZ = tadOffsets;
            }

            auto x = reinterpret_cast<X*>(vx);
            auto y = reinterpret_cast<Y*>(vy);
            auto z = reinterpret_cast<Z*>(vz);

            //decompose in to several sub tads after
            //moving all dimensions (in sorted order)
            //to the back.
            //permuted version of the x shape info for setting up the tad problem
            __shared__ Nd4jLong tadLength;
            __shared__ Nd4jLong tadEWS;
            __shared__ int numTads;
            __shared__ Nd4jLong xEWS;
            __shared__ Nd4jLong zEWS;


            if (threadIdx.x == 0) {

                tadLength = shape::length(tadOnlyShapeInfo);
                tadEWS = shape::elementWiseStride(tadOnlyShapeInfo);
                numTads = shape::length(yShapeInfo) / tadLength;
                xEWS = shape::elementWiseStride(xShapeInfo);
                zEWS = shape::elementWiseStride(tadOnlyShapeInfoZ);
            }
            __syncthreads();

            auto xOrder = shape::order(xShapeInfo);
            auto yOrder = shape::order(tadOnlyShapeInfo);
            auto zOrder = shape::order(tadOnlyShapeInfoZ);

            for (int r = blockIdx.x; r < numTads; r += gridDim.x) {


                auto rY = y + tadOffsets[r];
                auto rZ = z + tadOffsetsZ[r];


                if(tadEWS > 0 && zEWS > 0 && xEWS > 0 && dimensionLength == 1 && xOrder == yOrder && xOrder == zOrder) {
                    for (int i = threadIdx.x; i < tadLength; i+= blockDim.x)
                        rZ[i * zEWS] = OpType::op(x[i * xEWS], rY[i * tadEWS]);
                }
                else {
                    // it is expected that x and z tads and y array all have the same length
                    for (Nd4jLong i = threadIdx.x; i < tadLength; i+= blockDim.x) {
                        auto xOffset = shape::getIndexOffset(i, xShapeInfo,  tadLength);
                        auto yOffset = shape::getIndexOffset(i, tadOnlyShapeInfo, tadLength);
                        auto zOffset = shape::getIndexOffset(i, tadOnlyShapeInfoZ, tadLength);
                        rZ[zOffset] = OpType::op(x[xOffset], rY[yOffset]);
                    }
                }
            }
        }


        template<typename X, typename Y, typename Z>
        template <typename OpType>
		__device__ void Broadcast<X,Y,Z>::transformCuda(
		                              void *vx, Nd4jLong *xShapeInfo,
		                              void *vy, Nd4jLong *yShapeInfo,
		                              void *vz, Nd4jLong *zShapeInfo,
		                              int *dimension, int dimensionLength, 
                                      Nd4jLong *tadOnlyShapeInfo, Nd4jLong *tadOffsets, Nd4jLong *tadOnlyShapeInfoZ, Nd4jLong *tadOffsetsZ) {

            if (tadOnlyShapeInfoZ == nullptr) {
                tadOnlyShapeInfoZ = tadOnlyShapeInfo;
                tadOffsetsZ = tadOffsets;
            }
            
            auto x = reinterpret_cast<X*>(vx);
            auto y = reinterpret_cast<Y*>(vy);
            auto z = reinterpret_cast<Z*>(vz);

            //decompose in to several sub tads after
            //moving all dimensions (in sorted order)
            //to the back.
            //permuted version of the x shape info for setting up the tad problem
	        __shared__ Nd4jLong tadLength;
            __shared__ Nd4jLong tadEWS;
            __shared__ int numTads;
            __shared__ Nd4jLong yEWS;
            __shared__ Nd4jLong zEWS;

            if (threadIdx.x == 0) {
                                
   	            tadLength = shape::length(tadOnlyShapeInfo);
                tadEWS = shape::elementWiseStride(tadOnlyShapeInfo);
                numTads = shape::length(xShapeInfo) / tadLength;
                yEWS = shape::elementWiseStride(yShapeInfo);
                zEWS = shape::elementWiseStride(tadOnlyShapeInfoZ);
            }
            __syncthreads();

            auto xOrder = shape::order(tadOnlyShapeInfo);
            auto yOrder = shape::order(yShapeInfo);
            auto zOrder = shape::order(tadOnlyShapeInfoZ);

            for (int r = blockIdx.x; r < numTads; r += gridDim.x) {

                auto rX = x + tadOffsets[r];
                auto rZ = z + tadOffsetsZ[r];


                if(tadEWS > 0 && zEWS > 0 && yEWS > 0 && xOrder == yOrder && xOrder == zOrder) {
                    for (int i = threadIdx.x; i < tadLength; i+= blockDim.x) 
                        rZ[i * zEWS] = OpType::op(rX[i * tadEWS], y[i * yEWS]); 
                }
                else {
                    // it is expected that x and z tads and y array all have the same length
                    for (Nd4jLong i = threadIdx.x; i < tadLength; i+= blockDim.x) {
                        
                        auto xOffset = shape::getIndexOffset(i, tadOnlyShapeInfo,  tadLength);
                        auto yOffset = shape::getIndexOffset(i, yShapeInfo, tadLength);
                        auto zOffset = shape::getIndexOffset(i, tadOnlyShapeInfoZ, tadLength);
                        rZ[zOffset] = OpType::op(rX[xOffset], y[yOffset]);
                    }
                }
            }
        }

        BUILD_PAIRWISE_TEMPLATE(template class ND4J_EXPORT Broadcast, , PAIRWISE_TYPES_0);
        BUILD_PAIRWISE_TEMPLATE(template class ND4J_EXPORT Broadcast, , PAIRWISE_TYPES_1);
        BUILD_PAIRWISE_TEMPLATE(template class ND4J_EXPORT Broadcast, , PAIRWISE_TYPES_2);
        BUILD_PAIRWISE_TEMPLATE(template class ND4J_EXPORT Broadcast, , PAIRWISE_TYPES_3);
        BUILD_PAIRWISE_TEMPLATE(template class ND4J_EXPORT Broadcast, , PAIRWISE_TYPES_4);
        BUILD_PAIRWISE_TEMPLATE(template class ND4J_EXPORT Broadcast, , PAIRWISE_TYPES_5);
        BUILD_PAIRWISE_TEMPLATE(template class ND4J_EXPORT Broadcast, , PAIRWISE_TYPES_6);
        BUILD_PAIRWISE_TEMPLATE(template class ND4J_EXPORT Broadcast, , PAIRWISE_TYPES_7);
        BUILD_PAIRWISE_TEMPLATE(template class ND4J_EXPORT Broadcast, , PAIRWISE_TYPES_8);
        BUILD_PAIRWISE_TEMPLATE(template class ND4J_EXPORT Broadcast, , PAIRWISE_TYPES_9);
    }
}