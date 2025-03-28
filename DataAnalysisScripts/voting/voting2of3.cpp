#include <vector>
#include <fstream>
#include <iostream>
#include <cmath>
#include <string>
#include <string.h>
#include <sstream>
#include <pthread.h>
#include <unistd.h>
#include <chrono>
typedef std::chrono::high_resolution_clock Clock;

#define THRESHOLDSTART 1.5 //for evaluating thresholds I did 1.5 to 6
#define THRESHOLDEND 6.25

//from previous code for evaluations DO NOT REMOVE though
#define THRESHOLDSET false
#define THRESHOLDT2 228.9 //228.9 is from two channel real-time in vivo data
#define THRESHOLDT3 201.7 //201.7 is from two channel real-time in vivo data

#define BOOTSTRAPS 1000

#define GENERATE_CANONICAL_RIPPLES false

#define DATAINT2FILENAME "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/twoChanAnalysis/smoothed_envelope_simulatedT2.out"
#define DATAINT3FILENAME "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/twoChanAnalysis/smoothed_envelope_simulatedT3.out"
#define DATAINT4FILENAME "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/twoChanAnalysis/smoothed_envelope_simulatedT4.out"

#define RIPPLESTARTBOUNDT2 "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/twoChanAnalysis/rippleBoundsStartT2.out"
#define RIPPLESTARTBOUNDT3 "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/twoChanAnalysis/rippleBoundsStartT3.out"
#define RIPPLEENDBOUNDT2 "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/twoChanAnalysis/rippleBoundsEndT2.out"
#define RIPPLEENDBOUNDT3 "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/twoChanAnalysis/rippleBoundsEndT3.out"

#define RIPPLESTARTBOUND "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/twoChanAnalysis/rippleBoundsStartTwoChan.out"
#define RIPPLEENDBOUND "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/twoChanAnalysis/rippleBoundsEndTwoChan.out"

#define SIMDETECTIONFILENAME "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/offlineAnalysis/twoOfThree/simDetectionsTwoChan"
#define TPRATEFILENAME "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/offlineAnalysis/twoOfThree/tpRate"
#define FPRATEFILENAME "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/offlineAnalysis/twoOfThree/fpRate"
#define FPPERCENTAGEFILENAME "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/offlineAnalysis/twoOfThree/fpPercent"
#define DETECTIONLATENCYFILENAME "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/offlineAnalysis/twoOfThree/detectionLatency"
#define RELATIVEDETECTIONLATENCYFILENAME "/home/shayok/Documents/Code/RippleDetectionAnalysis/Cavaradossi/paperData/offlineAnalysis/twoOfThree/relativeDetectionLatency"

double calcMean(std::vector<double> arrrayForEst)
{
    double sums = 0;
    for(unsigned int x = 0; x<arrrayForEst.size();++x){
        sums += arrrayForEst[x];
    }
    double mean = sums/arrrayForEst.size();
    return mean;
}

double calcSTD(std::vector<double> arrrayForEst)
{
    double mean = calcMean(arrrayForEst);
    double sums = 0;
    for(unsigned int x = 0; x<arrrayForEst.size(); ++x)
        sums += (arrrayForEst[x]-mean) * (arrrayForEst[x]-mean);

    sums = sums/arrrayForEst.size();
    double standardDeviation = std::sqrt(sums);
    return standardDeviation;
}

//in case we want to multithread the bootsraps
struct filehandlers{
    std::ofstream tpRate;
    std::ofstream fpRate;
    std::ofstream detectionLatency;
    std::ofstream relativeDetectionLatency;
};

void* real_work_thread(void *arg)
{
    double x = *((double*) arg); //stores threshold
    std::cout<<"Voting Detection Threshold " <<x<<" thread running!"<<'\n';

    //Read in ripple bound files and determine ripple bounds
    std::string line;

    std::ifstream startBounds(RIPPLESTARTBOUND);
    std::vector <int> rippleBoundStart;
    if(startBounds.is_open()){
        while(std::getline(startBounds,line)){
            rippleBoundStart.push_back(std::stod(line));
        }
    }
    else{
        std::cout<< "Error opening file ripple bounds start"<<std::endl;
        pthread_exit(0);
    }

    std::ifstream endBounds(RIPPLEENDBOUND);
    std::vector <int> rippleBoundEnd;
    if(endBounds.is_open()){
        while(std::getline(endBounds,line)){
            rippleBoundEnd.push_back(std::stod(line));
        }
    }
    else{
        std::cout<< "Error opening file ripple bounds end"<<std::endl;
        pthread_exit(0);
    }

    startBounds.close(); endBounds.close();

    std::ifstream dataInT2(DATAINT2FILENAME);
    std::ifstream dataInT3(DATAINT3FILENAME);
    std::ifstream dataInT4(DATAINT4FILENAME);

    std::vector <int> detectionTimeIndexes;
    std::vector <double> smoothed_envelopeT2, smoothed_envelopeT3, smoothed_envelopeT4;
    double thresholdT2, thresholdT3, thresholdT4;

    if(dataInT2.is_open()){
        while(std::getline(dataInT2,line)){
            smoothed_envelopeT2.push_back(std::stod(line));
        }
        if(THRESHOLDSET)
            thresholdT2 = THRESHOLDT2;
        else
            thresholdT2 = calcMean(smoothed_envelopeT2)+(x*calcSTD(smoothed_envelopeT2));
    }
    else{
        std::cout << "Error opening file T2";
        pthread_exit(0);
    }

    if(dataInT3.is_open()){
        while(std::getline(dataInT3,line)){
            smoothed_envelopeT3.push_back(std::stod(line));
        }
        if(THRESHOLDSET)
            thresholdT3=THRESHOLDT3;
        else
            thresholdT3 = calcMean(smoothed_envelopeT3)+(x*calcSTD(smoothed_envelopeT3));
    }
    else{
        std::cout << "Error opening file T3";
        pthread_exit(0);
    }

    if(dataInT4.is_open()){
            while(std::getline(dataInT4,line)){
                smoothed_envelopeT4.push_back(std::stod(line));
            }
            //threshold t4...not gonna do a threshold set here since we didn't use this channel online
            thresholdT4 = calcMean(smoothed_envelopeT4) + (x*calcSTD(smoothed_envelopeT4));
    }
    else{
        std::cout << "Error opening file T4";
        pthread_exit(0);
    }

    //create simulated detections output file 
    std::ostringstream strs;
    strs << x*100;

    std::ofstream myfile;
    
    std::string fileNameee = SIMDETECTIONFILENAME;

    fileNameee += strs.str() + "out";
    myfile.open(fileNameee, std::ofstream::out | std::ofstream::trunc);

    //Hunt for ripples on three channels. Two out of three must detect ripples for us to "stimulate"
    int detectTimeReqIndexes = 45; //detect within 15ms of each other or 45 indexes at sampling rate 3kHz
    int blockLength = 600; //200ms block after detection or 600 indexes at sampling rate 3kHz
    bool firstRippleDetected = false;
    int firstDetectionIndex = -1;
    bool rippleDetectedT2 = false;
    bool rippleDetectedT3 = false;
    bool rippleDetectedT4 = false;
    int i=0;
    while (i<smoothed_envelopeT2.size()){ //loop through all elements of channel
        //if we exceed time requirements reset all flags
        if(firstRippleDetected && i>firstDetectionIndex+detectTimeReqIndexes){
            ++i;
            firstRippleDetected = false;
            rippleDetectedT2 = false;
            rippleDetectedT3 = false;
            rippleDetectedT4 = false;
        }
        else{
            if(!rippleDetectedT2 && smoothed_envelopeT2[i]>thresholdT2){
                if(firstRippleDetected){
                    firstRippleDetected = false;
                    rippleDetectedT2 = false;
                    rippleDetectedT3 = false;
                    rippleDetectedT4 = false;
                    myfile << i << " " << i+10 <<"\n";
                    myfile.flush();
                    detectionTimeIndexes.push_back(i);
                    i+=blockLength;
                }
                else{
                    rippleDetectedT2 = true;
                    firstRippleDetected = true;
                    firstDetectionIndex = i;
                    ++i;
                }
            }
            if(!rippleDetectedT3 && smoothed_envelopeT3[i]>thresholdT3){
                if(firstRippleDetected){
                    firstRippleDetected = false;
                    rippleDetectedT2 = false;
                    rippleDetectedT3 = false;
                    rippleDetectedT4 = false;
                    myfile << i << " " << i+10 <<"\n";
                    myfile.flush();
                    detectionTimeIndexes.push_back(i);
                    i+=blockLength;
                }
                else{
                    rippleDetectedT3 = true;
                    firstRippleDetected = true;
                    firstDetectionIndex = i;
                    ++i;
                }
            }
            if(!rippleDetectedT4 && smoothed_envelopeT4[i]>thresholdT4){
                if(firstRippleDetected){
                    firstRippleDetected = false;
                    rippleDetectedT2 = false;
                    rippleDetectedT3 = false;
                    rippleDetectedT4 = false;
                    myfile << i << " " << i+10 <<"\n";
                    myfile.flush();
                    detectionTimeIndexes.push_back(i);
                    i+=blockLength;
                }
                else{
                    rippleDetectedT4 = true;
                    firstRippleDetected = true;
                    firstDetectionIndex = i;
                    ++i;
                }
            }
            else
                ++i;
        }
    }
        
    //track which detections detect canonical ripples
    std::vector<std::string> rippleDetected;
    
    for(unsigned int xx = 0; xx<rippleBoundStart.size(); ++xx){
        rippleDetected.push_back("F");
    }
    unsigned int yyy = 0;
    unsigned int iiiii = 0;
    while(iiiii<detectionTimeIndexes.size() && yyy <rippleBoundStart.size()){
        //if pre detect!
        if(detectionTimeIndexes[iiiii]<rippleBoundStart[yyy]){
            //don't double penalize for both false detection and missed detection
            if(detectionTimeIndexes[iiiii]+blockLength > rippleBoundStart[yyy]){
                /**
                 * It's worth noting here that this accounts for the case that
                 * there are 3 ripples consecutively and the first one is the 
                 * one that is detected. The following one is missed because it 
                 * is within 200 ms of the first one (so practically it would've
                 * been disrupted as well). The second one would be missed and
                 * the third one would be detected but it may be a late detection
                 * thus penalizing the detection latency quantification. Skipping
                 * events like this (they do occur occassionally) provides a more
                 * representative view of boths system and algorithm performance.
                 */
               rippleDetected[yyy] = "skip"; ++yyy;
            }
            ++iiiii;
        }
        //if ripple detected!
        else if(detectionTimeIndexes[iiiii] >= rippleBoundStart[yyy] 
            && detectionTimeIndexes[iiiii] <= rippleBoundEnd[yyy]){
                rippleDetected[yyy]=std::to_string(iiiii);
                //make sure we don't penalize detection that would be disrupted
                if(yyy+1 < rippleDetected.size()){
                    if(rippleBoundStart[yyy+1]-rippleBoundEnd[yyy]<blockLength){
                        rippleDetected[yyy+1]="skip";
                        ++yyy;
                    }
                }
                else
                    break;
                ++iiiii;++yyy;
        }
        //if ripple was missed!
        else if(detectionTimeIndexes[iiiii]>rippleBoundEnd[yyy])
        {
            //std::cout<<"MISSED DETECTION RB INDEX: " <<yyy<<'\n';
            ++yyy;
        }
    }
    //open detection metric quantification files
    struct filehandlers fileHandlers;
    
    fileNameee = TPRATEFILENAME;
    fileNameee += strs.str() + ".out";
    fileHandlers.tpRate.open(fileNameee, std::ofstream::out | std::ofstream::trunc);


    fileNameee = FPRATEFILENAME;
    fileNameee += strs.str() + ".out";
    fileHandlers.fpRate.open(fileNameee, std::ofstream::out | std::ofstream::trunc);


    fileNameee =DETECTIONLATENCYFILENAME;
    fileNameee += strs.str() + ".out";
    fileHandlers.detectionLatency.open(fileNameee, std::ofstream::out | std::ofstream::trunc);


    fileNameee =RELATIVEDETECTIONLATENCYFILENAME;
    fileNameee += strs.str() + ".out";
    fileHandlers.relativeDetectionLatency.open(fileNameee, std::ofstream::out | std::ofstream::trunc);

    //Generate 1000 20 minute sample metric quantifications
    unsigned int myCounter = 0;
    auto t1 = Clock::now();
    while(myCounter < BOOTSTRAPS){
        ++myCounter;
        if(myCounter %10 == 0){
            std::cout<<"Voting Detection Thread: " << x << ": Last 10 iterations time: " <<
            std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - t1).count()
            <<" s" << ". Percent Complete: "<<(double)myCounter*100/BOOTSTRAPS<<"%"<<std::endl;
            t1 = Clock::now();
        }
        //Metric quantification algorithm
        //instantiate detection latency metrics vectors
        std::vector<double> detectionlatency, relativedetectionlatency;
        //instantiate canonical ripple counter
        int totalRipples = 0;
        //instantiate true/false positive counters
        int trueDetections = 0;
        int falseDetections = 0;
        //instantiate true negative counter
        int totalTrueNegatives = 0;
        // auto timeSection = Clock().now();

        //Generate 80k bootstrap samples 15 ms in length (80k * 15 ms = 20 min)
        for(unsigned int iii=0; iii<80000; ++iii){
            //generate random integer from 0 to size of data
            int bootstrapsample = std::rand() % (smoothed_envelopeT2.size()-45); //45 is length of 15 ms window in time indices

            /** Determine if sample is a canonical ripple
             * tail of 15 ms chunk has ripple bound start
             * 15 ms chunk is within ripple bound start and end
             * beginning of 15 ms chunk has ripple bound end
             *  this isn't a conditional b/c we're only looking at start times
             *  it's already accounted for by the  falling within the start and end
             * store associated ripple bound start and end if ripple
            */
            //Determine if sample is a canonical ripple
            int rippleBoundSample = -1;
            //loop through ripple bounds
            for(unsigned int xx=0; xx<rippleBoundStart.size(); ++xx){
                //if 15 ms chunk is within ripple start and end
                if(bootstrapsample > rippleBoundStart[xx] && bootstrapsample < rippleBoundEnd[xx]){
                    rippleBoundSample = xx;
                    break;
                }
                //if tail of 15 ms sample has ripple bound start
                else if(bootstrapsample+45 > rippleBoundStart[xx] && bootstrapsample+45 < rippleBoundEnd[xx]){
                    rippleBoundSample = xx;
                    break;
                }
            }
            //if sample is canonical ripple and we don't want to skip it because of block length
            if(rippleBoundSample != -1 && 
                rippleDetected[rippleBoundSample] != "skip"){
                //increment canonical ripple counter
                ++totalRipples;
                //Determine if canonical ripple was detected
                if(rippleDetected[rippleBoundSample].compare("F") != 0 &&
                    rippleDetected[rippleBoundSample].compare("skip") != 0){
                    int xx = 0;
                    std::stringstream streeem(rippleDetected[rippleBoundSample]);
                    streeem >> xx;
                    ++trueDetections;
                    //quantify detection latency in ms and store in vector (ms)
                    double detectionlat = (detectionTimeIndexes[xx]-rippleBoundStart[rippleBoundSample])/3;
                    detectionlatency.push_back(detectionlat);
                    //quantify relative detection latency and store in vector
                    relativedetectionlatency.push_back(detectionlat/((rippleBoundEnd[rippleBoundSample]-rippleBoundStart[rippleBoundSample])/3));
                }
            }
            //else determine if detection is present for false detection
            else if(rippleBoundSample == -1){
                ++totalTrueNegatives;
                //loop through all simulated detection time indices
                for(unsigned int xx=0; xx<detectionTimeIndexes.size(); ++xx){
                    //if detection is present within random sample bound
                    if(detectionTimeIndexes[xx] > bootstrapsample 
                        && detectionTimeIndexes[xx] < bootstrapsample + 45){
                            //increment false positive counter
                            ++falseDetections;
                            break;
                        }
                }
            }
        }
        /** Calculate metrics
         * tprate = tpcount/total canonical ripples
         * fprate = fpcount/20 min
         * average detection latencies
        */
        //write above metrics to appropriate files.
        fileHandlers.tpRate << (double)trueDetections/totalRipples << '\n';
        fileHandlers.fpRate << (double)falseDetections/(totalTrueNegatives*15/60000)<< '\n';
        fileHandlers.detectionLatency << calcMean(detectionlatency)<< '\n';
        fileHandlers.relativeDetectionLatency << calcMean(relativedetectionlatency)<< '\n';
        fileHandlers.tpRate.flush();fileHandlers.fpRate.flush();fileHandlers.detectionLatency.flush();fileHandlers.relativeDetectionLatency.flush();
        usleep(10);
    }
    std::cout<<"Threshold: " <<x<<" thread complete!!"<<'\n';
    fileHandlers.tpRate.close();fileHandlers.fpRate.close();fileHandlers.detectionLatency.close();fileHandlers.relativeDetectionLatency.close();
    pthread_exit(0);
}

int main(int argc, char *argv[])
{
    //Read in ripple bound files and determine ripple bounds
    std::string line;

    std::ifstream startBounds(RIPPLESTARTBOUNDT2);
    std::vector <int> rippleBoundStartT2;
    if(startBounds.is_open()){
        while(std::getline(startBounds,line)){
            rippleBoundStartT2.push_back(std::stod(line));
        }
    }
    else{
        std::cout<< "Error opening file ripple bounds start"<<std::endl;
        pthread_exit(0);
    }

    std::ifstream endBounds(RIPPLEENDBOUNDT2);
    std::vector <int> rippleBoundEndT2;
    if(endBounds.is_open()){
        while(std::getline(endBounds,line)){
            rippleBoundEndT2.push_back(std::stod(line));
        }
    }
    else{
        std::cout<< "Error opening file ripple bounds end"<<std::endl;
        pthread_exit(0);
    }

    startBounds.close(); endBounds.close();

    startBounds.open(RIPPLESTARTBOUNDT3);
    std::vector <int> rippleBoundStartT3;
    if(startBounds.is_open()){
        while(std::getline(startBounds,line)){
            rippleBoundStartT3.push_back(std::stod(line));
        }
    }
    else{
        std::cout<< "Error opening file ripple bounds start"<<std::endl;
        pthread_exit(0);
    }

    endBounds.open(RIPPLEENDBOUNDT3);
    std::vector <int> rippleBoundEndT3;
    if(endBounds.is_open()){
        while(std::getline(endBounds,line)){
            rippleBoundEndT3.push_back(std::stod(line));
        }
    }
    else{
        std::cout<< "Error opening file ripple bounds end"<<std::endl;
        pthread_exit(0);
    }
    startBounds.close();endBounds.close();

    //generate canonical ripple bounds...defined in jupyter notebook!
    if (GENERATE_CANONICAL_RIPPLES){
        std::ofstream canonicalStartBounds;
        std::ofstream canonicalEndBounds;

        canonicalStartBounds.open(RIPPLESTARTBOUND, std::ofstream::out | std::ofstream::trunc);
        canonicalEndBounds.open(RIPPLEENDBOUND, std::ofstream::out | std::ofstream::trunc);

        std::vector<int> rippleBoundStart, rippleBoundEnd;

        unsigned int xx = 0;
        for(unsigned int zz = 0; zz<rippleBoundStartT2.size(); ++zz){
            while(xx < rippleBoundStartT3.size()){
                if(rippleBoundStartT3[xx] <= rippleBoundStartT2[zz]){
                    //check if T2 detection is within T3
                    if(rippleBoundStartT2[zz] < rippleBoundEndT3[xx]){
                        rippleBoundStart.push_back(rippleBoundStartT3[xx]);
                        if(rippleBoundEndT2[zz] > rippleBoundEndT3[xx]){
                            rippleBoundEnd.push_back(rippleBoundEndT2[zz]);
                            canonicalEndBounds << rippleBoundEndT2[zz] << '\n';
                        }
                        else{
                            rippleBoundEnd.push_back(rippleBoundEndT3[xx]);
                            canonicalEndBounds << rippleBoundEndT3[xx] << '\n';
                        }
                        canonicalStartBounds << rippleBoundStartT3[xx] << '\n';
                        canonicalStartBounds.flush();canonicalEndBounds.flush();
                        break;
                    }//else let's iterate until T3 detections are caught up
                    else{
                        ++xx;
                    }
                }
                else{
                    //check if T3 detection is within T2
                    if(rippleBoundStartT3[xx] < rippleBoundEndT2[zz]){
                        rippleBoundStart.push_back(rippleBoundStartT2[zz]);
                        if(rippleBoundEndT2[zz] > rippleBoundEndT3[xx]){
                            rippleBoundEnd.push_back(rippleBoundEndT2[zz]);
                            canonicalEndBounds << rippleBoundEndT2[zz] << '\n';
                        }
                        else{
                            rippleBoundEnd.push_back(rippleBoundEndT3[xx]);
                            canonicalEndBounds << rippleBoundEndT3[xx] << '\n';
                        }
                        canonicalStartBounds << rippleBoundStartT2[zz] <<'\n';
                        canonicalStartBounds.flush();canonicalEndBounds.flush();
                        break;
                    }//else let the T2 detections catch up
                    else{
                        break;
                    }
                }
            }
        }
        canonicalStartBounds.close();canonicalEndBounds.close();
    }
    
    std::vector<pthread_t> tids;
    for(double x = THRESHOLDSTART; x<THRESHOLDEND; x+=0.25){ //loop through a bunch of thresholds, perform ripple detections, and evaluate
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_create(&tid,&attr,real_work_thread,&x);
        tids.push_back(tid);
        sleep(1);
    }
    for(unsigned int x = 0; x<tids.size(); ++x){
        int rc = pthread_join(tids[x],NULL);
        if (rc) { fprintf(stderr, "failed to join thread #%lf - %s\n",
                                (long)(x*0.25)+1.5, strerror(rc));
               exit(EXIT_FAILURE);}
    }
    return 0;
}
