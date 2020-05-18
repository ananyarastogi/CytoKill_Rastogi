#include "distribution.h"
#include <iostream>
#include <stdlib.h>
#include <math.h>
#include <sstream>
#include <fstream>
#include "random.h"
using namespace std;

string nameDistrib(int ID){
    if((ID < 0) || (ID >= NBDIstribs - 1)) return string("Unknown Distribution");

    static vector<string> names(NBDIstribs, string(""));
    static bool loaded = false;
    if(!loaded){
        names[Fixed] =          "Fixed";
        names[Normal] =         "Normal";
        names[LogNormal] =      "LogNormal";
        names[FromData] =       "FromData";
        names[BiModal] =        "BiModal";
        names[Exponential] =    "Exponential";
        loaded = true;
    }
    return names[ID];
}


double probaLawFromTable::IndependentRandRealGen(){
    return random::uniformDouble(0,1);
}

double probaLawFromTable::getRandValue(){
    int classTaken = getRandClass();
    if(uniformizeInsideClasses){
        return lowBoundsXs[classTaken] + IndependentRandRealGen() * (highBoundsXs[classTaken] - lowBoundsXs[classTaken]);
    }
    else{
        return Xs[classTaken];
    }
}

int probaLawFromTable::getRandClass()
{
    double rdreal = IndependentRandRealGen();
    //cerr << rdreal << "d" << endl;
    for(int i = 0; i < size; ++i){
        if(rdreal < cumulatedValuesHigh[i]){
            return i;
        }
    }
    return size-1;
}

probaLawFromTable::probaLawFromTable(string filename, bool _uniformizeInsideClasses){
    //cout << "Create probaLawFromTable" <<endl;
    vector <double> X;
    vector <double> Y;
    ifstream myfile(filename);
    if(!myfile){
        cerr << "ERR: " << filename << " not found!" << endl;
        return;
    }
    while (!myfile.eof())
    {
        double currentX;
        myfile>>currentX;
        X.push_back(currentX);
        double currentY;
        myfile>>currentY;
        Y.push_back(currentY);
        myfile.ignore(1000, '\n');
    }
    myfile.close();
    init(X, Y, _uniformizeInsideClasses);
}

probaLawFromTable::probaLawFromTable(vector<double> & _Xs, vector<double>&  densities, bool _uniformizeInsideClasses){
    init(_Xs, densities, _uniformizeInsideClasses);
}
void probaLawFromTable::init(vector<double> & _Xs, vector<double>&  densities, bool _uniformizeInsideClasses){
    uniformizeInsideClasses = _uniformizeInsideClasses;
    if(_Xs.size() != densities.size()){
        cerr << "ERR: probaLawFromTable, Xs and Densities vectors don't have the same size." << endl;
        return;
    }
    size = _Xs.size();
    Xs.resize(size, 0.0);
    NormalizedDensities.resize(size, 0.0);
    cumulatedValuesHigh.resize(size, 0.0);
    double sumDensities = 0.0;
    for(int i = 0; i < size; ++i){
        Xs[i] = _Xs[i];
        if((i > 0) && (Xs[i] < Xs[i-1]))
            cerr << "ERR : probaLawFromTable, the list of X values should be increasing only." << endl;
        sumDensities += densities[i];
    }

    normalizingCoeff = sumDensities;
    for(int i = 0; i < size; ++i){
        NormalizedDensities[i] = densities[i] / sumDensities;
    }

    cumulatedValuesHigh[0] = NormalizedDensities[0];
    for(int i = 1; i < size; ++i){
        cumulatedValuesHigh[i] = cumulatedValuesHigh[i-1] + NormalizedDensities[i];
    }

    if(fabs((cumulatedValuesHigh[size-1]) - 1.0) > 1e-6)
        cerr << "ERR : probaLawFromTable, sum of probabilities is not 1. Should not happen. Why ??" << endl;

    // In case one wants all the possible values inside each class,
    if(uniformizeInsideClasses){
        lowBoundsXs.resize(size);
        highBoundsXs.resize(size);
        for(int i = 0; i < size; ++i){
            if(i >0)
                lowBoundsXs[i] = (Xs[i] + Xs[i-1]) / 2.0;
            if(i < size-1)
                highBoundsXs[i] = (Xs[i+1] + Xs[i]) / 2.0;
        }

        lowBoundsXs[0] = Xs[0] - (highBoundsXs[0] - Xs[0]);
        highBoundsXs[size-1] = Xs[size-1] + (Xs[size-1] - lowBoundsXs[size-1]);
    }
}


/*double probaLawFromTable::IndependentRandRealGen()
{ // If later someone wants to separate generators for seeding.
    // needs C++11 and #include <random>
    static std::random_device *rd                       = new std::random_device();;
    static std::mt19937 *gen                            = new std::mt19937 ((*rd)());
    static std::uniform_real_distribution<> *RealDistrib= new std::uniform_real_distribution<> (0,1);
    return (*RealDistrib)(*gen);
    //return drandom();
}*/


string probaLawFromTable::print()
{
    stringstream res;
    res << "probaLawFromTable with following classes : " << endl;

    if(uniformizeInsideClasses)
        res << " --> Random values are taken uniformly inside each class\n";
    else
        res << " --> Random values are always the exact value of each class\n";
    res << "i\tX[i]\tproba\tcumulLow\toriginalDensity" << ((uniformizeInsideClasses) ? "\tLowBound\tHighBound" : "") << endl;

    for(int i = 0 ; i < size; ++i)
    {
        res << i << "\t" << Xs[i] << "\t" << ((i > 0) ? cumulatedValuesHigh[i] - cumulatedValuesHigh[i-1] : cumulatedValuesHigh[0]) << "\t" << cumulatedValuesHigh[i] << "\t" << (normalizingCoeff * ((i > 0) ? cumulatedValuesHigh[i] - cumulatedValuesHigh[i-1]: cumulatedValuesHigh[0]));
        if(uniformizeInsideClasses)
            res << "\t" << lowBoundsXs[i] << "\t" << highBoundsXs[i];
        res << endl;
    }

    res << "... The initial density was of total area " << normalizingCoeff << endl;
    return res.str();
}












probaLawStoringResults::probaLawStoringResults(vector<double> & _Xs, vector<double> & densities, bool _uniformizeInsideClasses) :
    probaLawFromTable(_Xs, densities,  _uniformizeInsideClasses)
{
    nbOfTimesPerClass.resize(size, 0);
    totalNbEvents = 0;
}

double probaLawStoringResults::getRandValue()
{
    double valueTaken = probaLawFromTable::getRandValue(); // the function from the mother class
    //cerr << valueTaken << "-\t" << endl;
    int correspondingClass = fitDoubleInAClass(valueTaken);
    if((correspondingClass < 0) || (correspondingClass >= size))
        cerr << "Err :  probaLawStoringResults, the getRandValue from probaLawFromTable was giving a value which is out of bounds : " << valueTaken << endl;
    else
    {
        nbOfTimesPerClass[correspondingClass] ++;
        totalNbEvents++;
    }
    return valueTaken;
}

double probaLawStoringResults::getFrequency(int classIndex)
{
    if((classIndex < 0) || (classIndex >= size))
        return -1;
    return ((double) nbOfTimesPerClass[classIndex]) / (totalNbEvents);
}

int probaLawStoringResults::fitDoubleInAClass(double val){
    if(uniformizeInsideClasses){
        for(int i = 0; i < size; ++i){
            if((val > lowBoundsXs[i]) && (val < highBoundsXs[i])){
                return i;
            }
        }
    }
    else{
        for(int i = 0; i < size; ++i){
            if(fabs(val - Xs[i]) < 1e-6){
                return i;
            }
        }
    }
    return -1;
}

void probaLawStoringResults::clearRecord()
{
    nbOfTimesPerClass.clear();
    nbOfTimesPerClass.resize(size, 0);
    totalNbEvents = 0;
}

string probaLawStoringResults::print()
{
    stringstream res;
    res << "probaLawStoringResults, extending ";
    res << probaLawFromTable::print() << endl;
    res << totalNbEvents << " events were recorded\n";
    res << "i\tXs[i]\tFrequency\tRequestedProbability\n";

    for(int i = 0; i < size; ++i){
        res << i << "\t" << Xs[i] << "\t" << getFrequency(i) << "\t" << ((i > 0) ? cumulatedValuesHigh[i] - cumulatedValuesHigh[i-1]: cumulatedValuesHigh[0]) << endl;
    }
    return res.str();
}

string probaLawStoringResults::TestprobaLawFromTable()
{
    exit(0);
    //Data taken from paper- 1st column
    vector<double>Xs = {15,
                       30,
                       45,
                       60,
                       75,
                       90,
                       105,
                       120,
                       135,
                       150,
                       165,
                       180};
    vector <double> densities={0.072816385,
                               0.166221083333333,
                               0.186119766666667,
                               0.150314316666667,
                               0.121583811666667,
                               0.091202181666667,
                               0.056430823333333,
                               0.045969841666667,
                               0.034882416666667,
                               0.0322526,
                               0.0255909,
                               0.016615845833333};


    stringstream res;
    probaLawFromTable Law = probaLawFromTable(Xs, densities, true);
    probaLawStoringResults Law2 = probaLawStoringResults(Xs, densities, false);

    res << Law.print() << endl;
    res << Law2.print() << endl;
    for(int i = 0; i < 20; ++i){
        res<<Law.getRandValue()<<"  "<<Law2.getRandValue()<<endl;
    }

    return res.str();
}

















/*

float probaLawFromTableStoringResultsAnanya::TestprobaLawFromTable()
{

    //Data taken from paper- 1st column
    vector<double>Xs = {15,
                       30,
                       45,
                       60,
                       75,
                       90,
                        105,
                        120,
                        135,
                        150,
                        165,
                        180};
     vector <double> densities={0.072816385,
                                0.166221083333333,
                                0.186119766666667,
                                0.150314316666667,
                                0.121583811666667,
                                0.091202181666667,
                                0.056430823333333,
                                0.045969841666667,
                                0.034882416666667,
                                0.0322526,
                                0.0255909,
                                0.016615845833333};





    probaLawFromTableAnanya Law = probaLawFromTableAnanya(Xs, densities, true);

    return Law.getRandValue();
}


// Getting duration of interaction from data sent by S. Halle

float probaLawFromTableStoringResultsAnanya::InteractionTimeFromData()
{
    vector <double> IntTimeData;
    vector <double> FreqIntTimeData;
    std::string appAddress;         // Address of application (ending with "/")
    appAddress = currentDir();
    appAddress+=string("/../Code/");


    ifstream myfile(appAddress+string ("InteractionDuration.txt"));

    while (!myfile.eof())
    {
        double IntTime;
        myfile>>IntTime;
        IntTimeData.push_back(IntTime);
        double Freq;
        myfile>>Freq;
        FreqIntTimeData.push_back(Freq);
        myfile.ignore(1000, '\n');
    }

    myfile.close();

    probaLawFromTableAnanya Law = probaLawFromTableAnanya(IntTimeData, FreqIntTimeData, true);

    return Law.getRandValue();
}


*/



struct ministat
{

    ministat()
    {
        clear();
    }
    void clear()
    {
        N=0;
        sum=0;
        sumsq=0;
        values.clear();
    }
    void add(float v)
    {
        sum += v;
        sumsq += v*v;
        N++;
        values.push_back(v);
    }
    int N;
    vector<float> values;
    float sum;
    float sumsq;
    float average()
    {
        return sum / (float) N;
    }
    float stddev()
    {
        return (float) sqrt( (double) (sumsq / (float) N) - (sum / (float) N) * (sum / (float) N));
    }
    string print()
    {
        stringstream res;
        res << "[" << N << "]";
        for(int i = 0; i < N; ++i)
        {
            res << "\t" << values[i];
        }
        return res.str();
    }
};

void example()
{
    ministat a;
a.add(15);
a.add(20);
cout << a.average() << a.stddev() << endl;
a.clear();
}
