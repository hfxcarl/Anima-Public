#pragma once

#include <itkSingleValuedCostFunction.h>
#include "AnimaRelaxometryExport.h"
#include <vnl_matrix.h>

#include <animaEPGSignalSimulator.h>
#include <animaCholeskyDecomposition.h>
#include <animaNNLSOptimizer.h>

#include <map>

namespace anima
{

/**
 * \class B1GMMDistributionIntegrand
 * @brief Integrand to compute the internal integral per distribution in B1GMMRelaxometryCostFunction
 */
class B1GMMDistributionIntegrand
{
public:
    using EPGVectorsMapType = std::map <double, anima::EPGSignalSimulator::RealVectorType>;
    B1GMMDistributionIntegrand(anima::EPGSignalSimulator &sigSim, EPGVectorsMapType &val)
        : m_EPGSimulator(sigSim), m_EPGVectors (val) {}

    void SetT1Value(double val) {m_T1Value = val;}
    void SetFlipAngle(double val) {m_FlipAngle = val;}
    void SetEchoNumber(unsigned int val) {m_EchoNumber = val;}

    void SetGaussianMean(double val) {m_GaussianMean = val;}
    void SetGaussianVariance(double val) {m_GaussianVariance = val;}

    double operator() (const double t);

private:
    //! EPG signal simulator reference: instantiated outside
    anima::EPGSignalSimulator &m_EPGSimulator;

    double m_T1Value;
    double m_FlipAngle;
    unsigned int m_EchoNumber;

    //! Since boost Gauss Legendre integration works on object copies, we need to keep a reference to EPG vectors, held externally
    EPGVectorsMapType &m_EPGVectors;

    double m_GaussianMean, m_GaussianVariance;
};

/**
 * \class B1GMMRelaxometryCostFunction
 * \brief Cost function for estimating B1 from T2 relaxometry acquisition, following a multi-T2 EPG decay model.
 * The cost function includes (via variable projection) estimation of compartment weights
 *
 */
class ANIMARELAXOMETRY_EXPORT B1GMMRelaxometryCostFunction :
        public itk::SingleValuedCostFunction
{
public:
    /** Standard class typedefs. */
    typedef B1GMMRelaxometryCostFunction Self;
    typedef itk::SingleValuedCostFunction Superclass;
    typedef itk::SmartPointer<Self> Pointer;
    typedef itk::SmartPointer<const Self> ConstPointer;

    itkNewMacro(Self)

    /** Run-time type information (and related methods). */
    itkTypeMacro(B1GMMRelaxometryCostFunction, Superclass)

    typedef Superclass::MeasureType MeasureType;
    typedef Superclass::DerivativeType DerivativeType;
    typedef Superclass::ParametersType ParametersType;
    typedef std::vector < std::complex <double> > ComplexVectorType;
    typedef std::vector <ComplexVectorType> MatrixType;

    virtual MeasureType GetValue(const ParametersType &parameters) const ITK_OVERRIDE;
    virtual void GetDerivative(const ParametersType &parameters, DerivativeType &derivative) const ITK_OVERRIDE;

    itkSetMacro(EchoSpacing, double)
    itkSetMacro(ExcitationFlipAngle, double)

    void SetT2RelaxometrySignals(ParametersType &relaxoSignals) {m_T2RelaxometrySignals = relaxoSignals;}

    itkSetMacro(T1Value, double)
    void SetGaussianMeans(std::vector <double> &val) {m_GaussianMeans = val;}
    void SetGaussianVariances(std::vector <double> &val) {m_GaussianVariances = val;}
    itkSetMacro(GaussianIntegralTolerance, double)

    unsigned int GetNumberOfParameters() const ITK_OVERRIDE
    {
        return 1;
    }

    itkGetMacro(SigmaSquare, double)
    ParametersType &GetOptimalT2Weights() {return m_OptimalT2Weights;}

protected:
    B1GMMRelaxometryCostFunction()
    {
        m_NNLSBordersOptimizer = anima::NNLSOptimizer::New();

        m_T1Value = 1;
        m_EchoSpacing = 1;
        m_GaussianIntegralTolerance = 1.0e-8;

        m_T2IntegrationStep = 1;
    }

    virtual ~B1GMMRelaxometryCostFunction() {}

    void PrepareDataForLLS() const;
    void PrepareDataForDerivative() const;

    //! Computes maximum likelihood estimates of weights
    void SolveLinearLeastSquares() const;

private:
    B1GMMRelaxometryCostFunction(const Self&); //purposely not implemented
    void operator=(const Self&); //purposely not implemented

    double m_EchoSpacing;
    ParametersType m_T2RelaxometrySignals;
    mutable ParametersType m_TestedParameters;

    double m_ExcitationFlipAngle;

    double m_T2IntegrationStep;

    mutable ParametersType m_OptimalT2Weights;
    double m_T1Value;
    std::vector <double> m_GaussianMeans, m_GaussianVariances;
    double m_GaussianIntegralTolerance;

    // Internal working variables, not thread safe but so much faster !
    mutable anima::EPGSignalSimulator m_T2SignalSimulator;
    mutable anima::EPGSignalSimulator::RealVectorType m_SimulatedSignalValues;

    mutable ParametersType m_FSignals;
    mutable ParametersType m_Residuals;
    mutable vnl_matrix <double> m_PredictedSignalAttenuations, m_CholeskyMatrix;
    mutable double m_SigmaSquare;

    mutable anima::NNLSOptimizer::Pointer m_NNLSBordersOptimizer;
    mutable anima::CholeskyDecomposition m_CholeskySolver;
};

} // end namespace anima
