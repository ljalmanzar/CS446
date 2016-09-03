#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

/// Structure Definition
struct PCB{

};

struct threadInfo{
   char component;
   char operation[25];
   int numOfCycles;
};

struct config{
   float version;
   char filePath[50];
   int processorCycleTime; 
   int monitorDisplayTime;
   int hardDriveCycleTime;
   int printerCycleTime;
   int keyboardCycleTime;
   int whereToLog; // 0 = monitor, 1 = file,  2 = both
   char loggingFilePath[50]; 
};

/// Global Variables
int timerSec = 0;
int timerMicro = 0; 
struct timeval tvalBefore, tvalAfter;
int processCounter = 1;
const int BUFFER_SIZE = 25;
struct config theConfig;
struct threadInfo theOperations[5000];
int metaSize = 0;

/// Function Prototypes
void readConfig (char* fileName);
int readMeta ();
char* appendChar(char* str, char c);
void startOperation(int location);
void appOperation(int location);
void running (int location);
void* inputOperation(void *arg);
void* outputOperation(void *arg);
void addTime();
void output();

/// Beginning of main program
int main(int argc, char* argv[]){
   /// error checking for arguments
   if (argc != 2){
      printf("Error with command line arguments. \n");
      return 0;
	}

   /// make "operation" all strings by adding null
   int counter = 0;
   for (counter = 0; counter < 5000; ++counter){
      theOperations[counter].operation[0] = '\0';
   }

   readConfig(argv[1]);

   metaSize = readMeta();

   /// do the work
   
   int ID;
   pthread_t tid[metaSize];
   int tidNdx = 0;
   counter = 0; 

   while (counter < metaSize){
      if ( theOperations[counter].component == 'S' ){
         startOperation(counter);
      }

      else if ( theOperations[counter].component == 'A' ){
         appOperation(counter);
      }

      else if ( theOperations[counter].component == 'P' ){
         running(counter);
      }

      else if ( theOperations[counter].component == 'I' ){
         ID = pthread_create(&(tid[tidNdx]), NULL, &inputOperation, &counter);

         if (ID != 0)
            printf("Error with thread creation.\n");

         pthread_join(tid[tidNdx],NULL);

         tidNdx++;
      }

      else if ( theOperations[counter].component == 'O' ){
         ID = pthread_create(&(tid[tidNdx]), NULL, &outputOperation, &counter);

         if (ID != 0)
            printf("Error with thread creation.\n");

         pthread_join(tid[tidNdx],NULL);

         tidNdx++;
      }

   counter++;
   }

   return 0;
}

void readConfig (char* fileName){
   /// garbage read in
   char temp1[BUFFER_SIZE];
   char temp2[BUFFER_SIZE];
   char temp3[BUFFER_SIZE];
   char temp4[BUFFER_SIZE];
   char temp5[BUFFER_SIZE];

   /// open file 
   FILE* fpointer;
	fpointer = fopen( fileName, "r" );

	/// take in first line of config file (garbage)
	fscanf(fpointer, "%s %s %s %s", temp1, temp2, temp3, temp4);

	/// take in version number
	fscanf(fpointer, "%s %f", temp1, &theConfig.version);

	/// metadata file path
	fscanf(fpointer, "%s %s %s", temp1, temp2, theConfig.filePath );

   /// Processpr cycle time
   fscanf(fpointer, "%s %s %s %s %i", temp1, temp2, temp3, temp4, &theConfig.processorCycleTime );

   /// Monitor display time
   fscanf(fpointer, "%s %s %s %s %i", temp1, temp2, temp3, temp4, &theConfig.monitorDisplayTime);

   /// Hard drive cycle time
   fscanf(fpointer, "%s %s %s %s %s %i", 
      temp1, temp2, temp3, temp4, temp5, &theConfig.hardDriveCycleTime);

   /// Printer cycle time
   fscanf(fpointer, "%s %s %s %s %i", temp1, temp2, temp3, temp4, &theConfig.printerCycleTime);

   /// Keyboard cycle time
   fscanf(fpointer, "%s %s %s %s %i", temp1, temp2, temp3, temp4, &theConfig.keyboardCycleTime);

   /// Where to log
   fscanf(fpointer, "%s %s %s %s", temp1, temp2, temp3, temp4);

   /// Check value of string
   if (strcmp(temp4, "Both") == 0){
      theConfig.whereToLog = 2;
   }
   else if (strcmp(temp4, "File") == 0){
      theConfig.whereToLog = 1;
   }
   else{
      theConfig.whereToLog = 0;
   }

   /// Log File Path
   fscanf(fpointer, "%s %s %s %s", temp1, temp2, temp3, theConfig.loggingFilePath);

   /// close the file
   fclose(fpointer);
}

int readMeta (){
   /// temp variables
   char temp1[BUFFER_SIZE];
   char temp2[BUFFER_SIZE];
   char temp3[BUFFER_SIZE];
   char temp4[BUFFER_SIZE];
   char temp;
   char semicolon = ';';

   FILE* fpointer;
   fpointer = fopen( theConfig.filePath, "r");

   /// read in heading
   fscanf (fpointer, "%s %s %s %s", temp1, temp2, temp3, temp4);

   int threadNdx = 0;
   while (semicolon == ';'){
      /// read in first char
      temp = fgetc(fpointer);
      if (temp == '\n'){
         temp = fgetc(fpointer);
         theOperations[threadNdx].component = temp;
      }
      else{
         theOperations[threadNdx].component = temp;
      }

      /// get parentheses
      temp = fgetc(fpointer);

      /// get operation
      temp = fgetc(fpointer);
      while (temp != ')'){
      	appendChar (theOperations[threadNdx].operation, temp);
      	temp = fgetc(fpointer);
      }

      /// get cycles for thread
      fscanf(fpointer, "%d",&theOperations[threadNdx].numOfCycles );

      /// read in semicolor and space
      semicolon = fgetc(fpointer);

      temp = fgetc(fpointer);

      threadNdx++;
   }

   printf("\n");

   fclose(fpointer);

   /// return size of metadata
   return threadNdx;
}

char* appendChar(char* str, char c){
   int length = strlen(str);

   str[length+1] = '\0';
   str[length] = c;

   return str;
}

void startOperation(int location){
	if ( !strcmp(theOperations[location].operation, "start" ) ){
         	gettimeofday (&tvalBefore, NULL);
            printf("%d.%06d - Simulator program starting\n", timerSec, timerMicro);
            gettimeofday (&tvalAfter, NULL);
            addTime();
         }
         else{
         	gettimeofday (&tvalBefore, NULL);
			addTime();
            printf("%d.%06d - Simulator program ending\n", timerSec, timerMicro);
         }
}

void appOperation(int location){
	if ( !strcmp(theOperations[location].operation, "start" ) ){
			gettimeofday (&tvalBefore, NULL);
			addTime();
            printf("%d.%06d - OS: Starting process %d\n", timerSec, timerMicro, processCounter);
            gettimeofday (&tvalAfter, NULL);
			addTime();
         }
         else{
         	gettimeofday (&tvalBefore, NULL);
			addTime();
            printf("%d.%06d - OS: Removing process %d\n",timerSec, timerMicro, processCounter);
            gettimeofday (&tvalAfter, NULL);
			addTime();
            processCounter++;
         }
}

void running (int location){
	int waitTime;

	/// get time 
	gettimeofday (&tvalBefore, NULL);
	addTime();

    printf("%d.%06d - Process %d : start processing action\n", timerSec, timerMicro, processCounter);
    waitTime = theConfig.processorCycleTime * theOperations[location].numOfCycles;

    /// make comp wait for time
    sleep((waitTime*.001));

    /// get new time
    gettimeofday (&tvalAfter, NULL);
	addTime();
    printf("%d.%06d - Process %d : end processing action \n\n", timerSec, timerMicro, processCounter);
}

void* inputOperation(void *arg){
   int* counter = arg;
   int waitTime;

   if ( !strcmp(theOperations[(*counter)].operation, "hard drive" ) ){
   			gettimeofday (&tvalBefore, NULL);
			addTime();
            printf("%d.%06d - Process %d : start hard drive input\n", timerSec, timerMicro, processCounter);
            waitTime = theConfig.hardDriveCycleTime * theOperations[(*counter)].numOfCycles;
         }
         else{
         	gettimeofday (&tvalBefore, NULL);
			addTime();
            printf("%d.%06d - Process %d : start keyboard input\n", timerSec, timerMicro, processCounter);
            waitTime = theConfig.keyboardCycleTime * theOperations[(*counter)].numOfCycles;
         }

    /// make comp wait for time
    sleep((waitTime*.001));

   if ( !strcmp(theOperations[(*counter)].operation, "hard drive" ) ){
   			gettimeofday (&tvalAfter, NULL);
			addTime();
            printf("%d.%06d - Process %d : end hard drive input\n", timerSec, timerMicro, processCounter);
         }
         else{
         	gettimeofday (&tvalAfter, NULL);
			addTime();
            printf("%d.%06d - Process %d : end keyboard input\n", timerSec, timerMicro, processCounter);
         }

   printf("\n");      

   return NULL;
}

void* outputOperation(void *arg){
   int* counter = arg;
   int waitTime;

   	if ( !strcmp(theOperations[(*counter)].operation, "hard drive" ) ){
   			gettimeofday (&tvalBefore, NULL);
			addTime();
            printf("%d.%06d - Process %d : start hard drive output\n",timerSec, timerMicro, processCounter);
            waitTime = theConfig.hardDriveCycleTime * theOperations[(*counter)].numOfCycles;
         }
         else{
         	gettimeofday (&tvalBefore, NULL);
			addTime();
            printf("%d.%06d - Process %d : start monitor output\n",timerSec, timerMicro, processCounter);
            waitTime = theConfig.monitorDisplayTime * theOperations[(*counter)].numOfCycles;
         }

   sleep((waitTime*.001));

   if ( !strcmp(theOperations[(*counter)].operation, "hard drive" ) ){
   			gettimeofday (&tvalAfter, NULL);
			addTime();
            printf("%d.%06d - Process %d : end hard drive output\n",timerSec, timerMicro, processCounter);
         }
         else{
         	gettimeofday (&tvalAfter, NULL);
			addTime();
            printf("%d.%06d - Process %d : end monitor output\n",timerSec, timerMicro, processCounter);
         }

   printf("\n");  

   return NULL;
}

void addTime(){
	if (tvalAfter.tv_usec > tvalBefore.tv_usec){
		double newSecs = tvalAfter.tv_sec - tvalBefore.tv_sec;
		double newUsecs = tvalAfter.tv_usec - tvalBefore.tv_usec;
		timerSec = timerSec + newSecs;
		timerMicro = timerMicro + newUsecs;
	}
	else {
		double newSecs = tvalBefore.tv_sec - tvalAfter.tv_sec;
		double newUsecs = tvalBefore.tv_usec - tvalAfter.tv_usec;
		timerSec = timerSec + newSecs;
		timerMicro = timerMicro + newUsecs;
	}
}

void output(){

}