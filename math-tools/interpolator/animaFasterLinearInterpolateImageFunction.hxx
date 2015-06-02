#pragma once

#include <itkConfigure.h>
#include "animaFasterLinearInterpolateImageFunction.h"
#include <vnl/vnl_math.h>

namespace anima
{

    /**
     * Define the number of neighbors
     */
    template<class TInputImage, class TCoordRep>
    const unsigned long
    FasterLinearInterpolateImageFunction< TInputImage, TCoordRep >
    ::m_Neighbors = 1 << TInputImage::ImageDimension;


    /**
     * Constructor
     */
    template<class TInputImage, class TCoordRep>
    FasterLinearInterpolateImageFunction< TInputImage, TCoordRep >
    ::FasterLinearInterpolateImageFunction()
    {

    }


    /**
     * PrintSelf
     */
    template<class TInputImage, class TCoordRep>
    void
    FasterLinearInterpolateImageFunction< TInputImage, TCoordRep >
    ::PrintSelf(std::ostream& os, itk::Indent indent) const
    {
        this->Superclass::PrintSelf(os,indent);
    }


    /**
     * Evaluate at image index position
     */
    template<class TInputImage, class TCoordRep>
    typename FasterLinearInterpolateImageFunction< TInputImage, TCoordRep >
    ::OutputType
    FasterLinearInterpolateImageFunction< TInputImage, TCoordRep >
    ::EvaluateAtContinuousIndex(const ContinuousIndexType& index) const
    {
        /**
         * Compute base index = closet index below point
         * Compute distance from point to base index
         */

        IndexType baseIndex, closestIndex;
        double distance[ImageDimension], oppDistance[ImageDimension];

        bool useClosest = true;

        for( unsigned int dim = 0; dim < ImageDimension; dim++ )
        {
            baseIndex[dim] = itk::Math::Floor< IndexValueType >( index[dim] );
            distance[dim] = index[dim] - baseIndex[dim];
            oppDistance[dim] = 1.0 - distance[dim];

            if (useClosest)
            {
                if (distance[dim] < 0.5)
                    closestIndex[dim] = baseIndex[dim];
                else
                    closestIndex[dim] = baseIndex[dim] + 1;

                if ((distance[dim] > 1.0e-8)&&(oppDistance[dim] > 1.0e-8))
                    useClosest = false;
            }
        }

        if (useClosest)
        {
            return this->GetInputImage()->GetPixel(closestIndex);
        }

        /**
         * Interpolated value is the weighted sum of each of the surrounding
         * neighbors. The weight for each neighbor is the fraction overlap
         * of the neighbor pixel with respect to a pixel centered on point.
         */
        double value = 0.0;

        double totalOverlap = 0.0;
        IndexType neighIndex;

        for( unsigned int counter = 0; counter < m_Neighbors; ++counter )
        {
            double overlap = 1.0;          // fraction overlap
            unsigned int upper = counter;  // each bit indicates upper/lower neighbour

            // get neighbor index and overlap fraction
            for( unsigned int dim = 0; dim < ImageDimension; ++dim )
            {

                if ( upper & 1 )
                {
                    neighIndex[dim] = baseIndex[dim] + 1;
                    overlap *= distance[dim];
                }
                else
                {
                    neighIndex[dim] = baseIndex[dim];
                    overlap *= oppDistance[dim];
                }

                upper >>= 1;

            }

            // get neighbor value only if overlap is not zero
            if( overlap )
            {
                value += this->GetInputImage()->GetPixel( neighIndex ) * overlap;
                totalOverlap += overlap;
            }

        }

        return value / totalOverlap;
    }

} // end of namespace anima
