#include <iostream>
#include <tclap/CmdLine.h>

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <animaNLMeansPatientToGroupComparisonImageFilter.h>
#include <itkTimeProbe.h>

using namespace std;

int main(int argc, char **argv)
{
    TCLAP::CmdLine cmd("INRIA / IRISA - Visages Team", ' ',"1.0");
    
    TCLAP::ValueArg<std::string> refLTArg("i","input","Test Image",true,"","test image",cmd);
    TCLAP::ValueArg<std::string> dataLTArg("I","database","Database Image List",true,"","database image list",cmd);
    
	TCLAP::ValueArg<std::string> maskArg("m","maskname","Computation mask",true,"","computation mask",cmd);
    TCLAP::ValueArg<std::string> resArg("o","outputpval","P-values output image",true,"","P-values output image",cmd);

    TCLAP::ValueArg<std::string> dbMeanDistAveArg("d","dbmeanave","DB average mean distance image",true,"","DB average mean distance image",cmd);
    TCLAP::ValueArg<std::string> dbMeanDistStdArg("D","dbmeanstd","DB std mean distance image",true,"","DB std mean distance image",cmd);
    TCLAP::ValueArg<std::string> dbCovDistAveArg("","dbcovave","DB average covariance distance image",true,"","DB average covariance distance image",cmd);
    TCLAP::ValueArg<std::string> dbCovDistStdArg("","dbcovstd","DB std covariance distance image",true,"","DB std covariance distance image",cmd);
    
    TCLAP::ValueArg<std::string> resScoreArg("O","outputscore","Score output image",false,"","Score output image",cmd);
    TCLAP::ValueArg<std::string> resNumPatchesArg("","outputnpatches","Number of patches output image",false,"","Number of patches output image",cmd);
	
	TCLAP::ValueArg<double> weightThrArg("w","weightthr","NL weight threshold: patches around have to be similar enough (default: 0.0)",false,0.0,"NL weight threshold",cmd);
	TCLAP::ValueArg<double> meanThrArg("M","patchmeanthr","Proportion of patches kept after mean test (default: 10.0)",false,10.0,"NL mean patch proportion",cmd);
	TCLAP::ValueArg<double> varThrArg("c","patchcovthr","Proportion of patches kept after covariance test (default: 10.0)",false,10.0,"NL covariance patch proportion",cmd);

	TCLAP::ValueArg<double> betaArg("b","beta","Beta parameter for local noise estimation (default: 1)",false,1,"Beta for local noise estimation",cmd);
    
    TCLAP::ValueArg<unsigned int> nbpArg("p","numberofthreads","Number of threads to run on (default: all cores)",false,itk::MultiThreader::GetGlobalDefaultNumberOfThreads(),"number of threads",cmd);

    TCLAP::ValueArg<unsigned int> patchHSArg("","patchhalfsize","Patch half size in each direction (default: 1)",false,1,"patch half size",cmd);
    TCLAP::ValueArg<unsigned int> patchSSArg("","patchstepsize","Patch step size for searching (default: 2)",false,2,"patch search step size",cmd);
    TCLAP::ValueArg<unsigned int> patchNeighArg("n","patchneighborhood","Patch half neighborhood size (default: 4)",false,4,"patch search neighborhood size",cmd);
    
    try
    {
        cmd.parse(argc,argv);
    }
    catch (TCLAP::ArgException& e)
    {
        std::cerr << "Error: " << e.error() << "for argument " << e.argId() << std::endl;
        return(1);
    }
    
    string dataLTName, maskName;
    dataLTName = dataLTArg.getValue();
    maskName = maskArg.getValue();
    
    typedef itk::ImageFileWriter < itk::Image <double, 3> > itkOutputWriter;
    typedef itk::ImageFileReader < itk::Image <double, 3> > itkOutputReader;
	
    typedef itk::ImageFileReader < itk::Image <unsigned char, 3> > itkMaskReader;
    itkMaskReader::Pointer maskRead = itkMaskReader::New();
    maskRead->SetFileName(maskName.c_str());
    maskRead->Update();
    
    typedef itk::VectorImage<double,3> LogTensorImageType;
    typedef itk::ImageFileReader <LogTensorImageType> itkInputReader;
    
    typedef anima::NLMeansPatientToGroupComparisonImageFilter<double> NLComparisonImageFilterType;
    
    NLComparisonImageFilterType::Pointer mainFilter = NLComparisonImageFilterType::New();
    mainFilter->SetComputationMask(maskRead->GetOutput());
    mainFilter->SetNumberOfThreads(nbpArg.getValue());

	mainFilter->SetWeightThreshold(weightThrArg.getValue());
	mainFilter->SetMeanThreshold(meanThrArg.getValue());
	mainFilter->SetVarianceThreshold(varThrArg.getValue());

	mainFilter->SetPatchHalfSize(patchHSArg.getValue());
	mainFilter->SetSearchStepSize(patchSSArg.getValue());
	mainFilter->SetSearchNeighborhood(patchNeighArg.getValue());
    mainFilter->SetBetaParameter(betaArg.getValue());
	
	itkInputReader::Pointer testLTReader = itkInputReader::New();
	testLTReader->SetFileName(refLTArg.getValue());
	testLTReader->Update();
	mainFilter->SetInput(0,testLTReader->GetOutput());
	
    ifstream fileIn(dataLTName.c_str());
    
    itkInputReader::Pointer ltReader;
    
    while (!fileIn.eof())
    {
        char tmpStr[2048];
        fileIn.getline(tmpStr,2048);
        
        if (strcmp(tmpStr,"") == 0)
            continue;
        
        std::cout << "Loading database image " << tmpStr << "..." << std::endl;
        ltReader = itkInputReader::New();
        ltReader->SetFileName(tmpStr);
        ltReader->Update();
        
        mainFilter->AddDatabaseInput(ltReader->GetOutput());
    }
    fileIn.close();
    
    itkOutputReader::Pointer dbMeanAveRead = itkOutputReader::New();
    dbMeanAveRead->SetFileName(dbMeanDistAveArg.getValue());
    dbMeanAveRead->Update();
    mainFilter->SetDatabaseMeanDistanceAverage(dbMeanAveRead->GetOutput());
  	
    itkOutputReader::Pointer dbMeanStdRead = itkOutputReader::New();
    dbMeanStdRead->SetFileName(dbMeanDistStdArg.getValue());
    dbMeanStdRead->Update();
    mainFilter->SetDatabaseMeanDistanceStd(dbMeanStdRead->GetOutput());
  	
    itkOutputReader::Pointer dbCovAveRead = itkOutputReader::New();
    dbCovAveRead->SetFileName(dbCovDistAveArg.getValue());
    dbCovAveRead->Update();
    mainFilter->SetDatabaseCovarianceDistanceAverage(dbCovAveRead->GetOutput());
  	
    itkOutputReader::Pointer dbCovStdRead = itkOutputReader::New();
    dbCovStdRead->SetFileName(dbCovDistStdArg.getValue());
    dbCovStdRead->Update();
    mainFilter->SetDatabaseCovarianceDistanceStd(dbCovStdRead->GetOutput());
  	
    try
    {
        mainFilter->Update();
    }
    catch (itk::ExceptionObject &e)
    {
        std::cerr << e << std::endl;
        return 1;
    }
    
    itkOutputWriter::Pointer resultWriter = itkOutputWriter::New();
    resultWriter->SetFileName(resArg.getValue());
	resultWriter->SetUseCompression(true);
	resultWriter->SetInput(mainFilter->GetOutput(0));
    
    resultWriter->Update();
    
    if (resScoreArg.getValue() != "")
    {
        itkOutputWriter::Pointer resultScoreWriter = itkOutputWriter::New();
        resultScoreWriter->SetFileName(resScoreArg.getValue());
        resultScoreWriter->SetUseCompression(true);
        resultScoreWriter->SetInput(mainFilter->GetOutput(1));
        
        resultScoreWriter->Update();
    }
    
    if (resNumPatchesArg.getValue() != "")
    {
        itkOutputWriter::Pointer resultNPatchesWriter = itkOutputWriter::New();
        resultNPatchesWriter->SetFileName(resNumPatchesArg.getValue());
        resultNPatchesWriter->SetUseCompression(true);
        resultNPatchesWriter->SetInput(mainFilter->GetOutput(2));
        
        resultNPatchesWriter->Update();
    }
    
    return 0;
}
