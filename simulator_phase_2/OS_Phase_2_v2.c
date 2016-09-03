#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>

/// Structure Definition
struct jobInfo{
   char component;
   char operation[25];
   int numOfCycles;

   int processThisBelongsTo;
};

struct PCB{
   /// running type
   enum {
      enter,
      ready,
      running,
      end 
   } state;

   struct jobInfo * theJobs;
   int numOfJobs; 
   int remainingTime; 
};

struct config{
   float version;
   char filePath[50];
   char scheduling[5];
   int processorCycleTime; 
   int monitorDisplayTime;
   int hardDriveCycleTime;
   int printerCycleTime;
   int keyboardCycleTime;
   enum {
      monitor,
      file,
      both
   } whereToLog; 
   char loggingFilePath[50]; 
};

/// Global Variables
const int BUFFER_SIZE = 25;
int timerSec = 0;
int timerMicro = 0; 
struct timeval tvalBefore, tvalAfter;
int processCounter = 1;
struct config theConfig;
struct PCB* theProcesses;
char finalData [5000][500];
int dataNdx = 0;
int numOfProcesses = 0; 

/// Function Prototypes
void readConfig (char* fileName);
int readMeta ();
char* appendChar(char* str, char c);

/* If parameter is a "1" then prints the beginning message, if paremeter 2 then prints ending
message */ 
void simulatorOperation(int startOrEnd);
void appOperation(int whichProcess, int whichJob);
void runningOperation (int whichProcess, int whichJob);
void* inputOperation(void *arg);
void* outputOperation(void *arg);
void runProcess (int whichProcess);
void addTime();
void output();

/// Beginning of main program
int main(int argc, char* argv[]){
   /// error checking for arguments
   if (argc != 2){
      printf("Error with command line arguments. \n");
      return 0;
	}

   //int metaSize = 0;

   readConfig(argv[1]);

   readMeta();

/*
   int i = 0;
   int j = 0;

   for (i = 0; i < numOfProcesses; ++i)
   {
      for (j = 0; j < theProcesses[i].numOfJobs; ++j)
      {
         printf("%c %s %d\n",
         theProcesses[i].theJobs[j].component, 
            theProcesses[i].theJobs[j].operation,
               theProcesses[i].theJobs[j].numOfCycles);
      }
   }
*/

   runProcess(0);
   runProcess(1);
   runProcess(2);

   output();

   int counter = 0;
   for (counter = 0; counter < numOfProcesses; counter++){
      free(theProcesses[counter].theJobs);
   }
   free(theProcesses);

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

	///  read scheduling type
	fscanf(fpointer, "%s %s %s", temp1, temp2, theConfig.scheduling );

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
   int sizeOfMeta = 0;

   /// temp variables
   char temp1[BUFFER_SIZE];
   char temp2[BUFFER_SIZE];
   char temp3[BUFFER_SIZE];
   char temp4[BUFFER_SIZE];
   char temp5[BUFFER_SIZE];
   char temp;
   char semicolon = ';';

   /// open file
   FILE* fpointer;
   fpointer = fopen( theConfig.filePath, "r");

   if (fpointer == NULL){
      printf("Could not open metadata file.\n");
      return 0;
   }

   /// read in heading
   fscanf (fpointer, "%s %s %s %s", temp1, temp2, temp3, temp4);

   while (temp != '.'){
      temp = fgetc(fpointer);
      if (temp == ';')
         sizeOfMeta++;
      if (temp == 'A')
         numOfProcesses++;
   }
   sizeOfMeta++;
   numOfProcesses = numOfProcesses / 2;

   int counter = 0;

   theProcesses = (struct PCB*) malloc(numOfProcesses * sizeof(struct PCB));

   for (counter = 0; counter < numOfProcesses; counter++){
      theProcesses[counter].theJobs = malloc(sizeOfMeta * sizeof(struct jobInfo));
   }

   int inner = 0;
   for (counter = 0; counter < numOfProcesses; counter++){
   		for (inner = 0; inner < sizeOfMeta; inner++){
   			theProcesses[counter].theJobs[inner].operation[0] = '\0';
   		}
   }
   
   /// go to beginning of file 
   rewind(fpointer);

   /// read in heading and beginning
   fscanf (fpointer, "%s %s %s %s %s", temp1, temp2, temp3, temp4, temp5);


   int processNdx = 0;
   int jobNdx = 0;
   while (semicolon == ';'){
      /// read in first char
      temp = fgetc(fpointer);

      /// checking for new line, if yes, reread
      if (temp == '\n' || temp == ' '){
         temp = fgetc(fpointer);

         if( temp == ' ' )
         	temp = fgetc(fpointer);
            theProcesses[processNdx].theJobs[jobNdx].component = temp;
      }
      else{
         theProcesses[processNdx].theJobs[jobNdx].component = temp;
      }

      /// get parentheses
      temp = fgetc(fpointer);

      /// get operation
      temp = fgetc(fpointer);
      while (temp != ')'){
      	appendChar (theProcesses[processNdx].theJobs[jobNdx].operation, temp);
      	temp = fgetc(fpointer);
      }

      /// get # of cycles for task
      fscanf(fpointer, "%d",&theProcesses[processNdx].theJobs[jobNdx].numOfCycles );

      /// read in semicolon and space
      semicolon = fgetc(fpointer);

      temp = fgetc(fpointer);

      if ( !strcmp(theProcesses[processNdx].theJobs[jobNdx].operation, "end" ) ){
         theProcesses[processNdx].numOfJobs = jobNdx;

         if ((processNdx+1) == numOfProcesses){
            semicolon = '.';
         }

         processNdx++;
         jobNdx = 0;
      } 
      else {
         theProcesses[processNdx].theJobs[jobNdx].processThisBelongsTo = (processNdx+1);
         jobNdx++;
      }  
   }

   fclose(fpointer);

   return sizeOfMeta;
}

char* appendChar(char* str, char c){
   int length = strlen(str);

   str[length+1] = '\0';
   str[length] = c;

   return str;
}

void simulatorOperation(int startOrEnd){
   /// check for start and end
	if ( startOrEnd == 1 ){
         	gettimeofday (&tvalBefore, NULL);
            sprintf(finalData[dataNdx],"%d.%06d - Simulator program starting\n", timerSec, timerMicro);
            dataNdx++;
            gettimeofday (&tvalAfter, NULL);
            addTime();
   }
   else{
         	gettimeofday (&tvalBefore, NULL);
			   addTime();
            sprintf(finalData[dataNdx],"%d.%06d - Simulator program ending\n", timerSec, timerMicro);
            dataNdx++;
   }
}

void appOperation(int whichProcess, int whichJob){
	if ( !strcmp(theProcesses[whichProcess].theJobs[whichJob].operation, "start" ) ){
			gettimeofday (&tvalBefore, NULL);
			addTime();
         sprintf(finalData[dataNdx],"%d.%06d - OS: Starting process %d\n", timerSec, timerMicro, (whichProcess+1));
         //printf("%s\n", finalData[dataNdx]);
         dataNdx++;
         gettimeofday (&tvalAfter, NULL);
			addTime();
   }
   else{
       	gettimeofday (&tvalBefore, NULL);
			addTime();
         sprintf(finalData[dataNdx],"%d.%06d - OS: Removing process %d\n",timerSec, timerMicro, (whichProcess+1));
         //printf("%s\n", finalData[dataNdx]);
         dataNdx++;
         gettimeofday (&tvalAfter, NULL);
			addTime();
         }
}

void runningOperation (int whichProcess, int whichJob){
	int waitTime;

	/// get time & add, save data
	gettimeofday (&tvalBefore, NULL);
	addTime();

   /// save 
   sprintf(finalData[dataNdx],"%d.%06d - Process %d : start processing action\n", timerSec, timerMicro, (whichProcess+1));
   //printf("%s\n", finalData[dataNdx]);
   dataNdx++;
   waitTime = theConfig.processorCycleTime * theProcesses[whichProcess].theJobs[whichJob].numOfCycles;

   ///wait for time calulated time
   sleep((waitTime*.001));

   /// get new time and add
   gettimeofday (&tvalAfter, NULL);
	addTime();

   /// save
   sprintf(finalData[dataNdx],"%d.%06d - Process %d : end processing action \n\n", timerSec, timerMicro, (whichProcess+1));
   //printf("%s\n", finalData[dataNdx]);
   dataNdx++;
}

/*
Following two thread operation functions follow the same process as the "running" function above.
1) Get the time from the computer then add it to my overall counter before the operation
2) Save the data
3) Calculate the time the operation should take
4) Make the computer wait for the correct amount of milliseconds
5) Get the time from the computer then add it to my overall counter before the operation
6) Save the data
*/
void* inputOperation(void *arg){
   struct jobInfo* theJob = arg;
   int waitTime;

   if ( !strcmp(theJob->operation, "hard drive" ) ){
   			gettimeofday (&tvalBefore, NULL);
			   addTime();

            sprintf(finalData[dataNdx],"%d.%06d - Process %d : start hard drive input\n", timerSec, 
                     timerMicro, theJob->processThisBelongsTo);
            //printf("%s\n", finalData[dataNdx]);
            dataNdx++;

            waitTime = theConfig.hardDriveCycleTime * theJob->numOfCycles;
         }
         else{
         	gettimeofday (&tvalBefore, NULL);
			   addTime();

            sprintf(finalData[dataNdx],"%d.%06d - Process %d : start keyboard input\n", timerSec, 
                     timerMicro, theJob->processThisBelongsTo);
            //printf("%s\n", finalData[dataNdx]);
            dataNdx++;

            waitTime = theConfig.keyboardCycleTime * theJob->numOfCycles;
         }

    sleep((waitTime*.001));

   if ( !strcmp(theJob->operation, "hard drive" ) ){
   			gettimeofday (&tvalAfter, NULL);
			   addTime();

            sprintf(finalData[dataNdx],"%d.%06d - Process %d : end hard drive input\n\n", timerSec, 
                     timerMicro, theJob->processThisBelongsTo);
            //printf("%s\n", finalData[dataNdx]);
            dataNdx++;
         }
         else{
         	gettimeofday (&tvalAfter, NULL);
			   addTime();

            sprintf(finalData[dataNdx],"%d.%06d - Process %d : end keyboard input\n\n", timerSec, 
                     timerMicro, theJob->processThisBelongsTo);
            //printf("%s\n", finalData[dataNdx]);
            dataNdx++;
         }
  

   return NULL;
}

void* outputOperation(void *arg){
   struct jobInfo* theJob = arg;
   int waitTime;

   	if ( !strcmp(theJob->operation, "hard drive" ) ){
   			gettimeofday (&tvalBefore, NULL);
			   addTime();

            sprintf(finalData[dataNdx],"%d.%06d - Process %d : start hard drive output\n",timerSec, 
                     timerMicro, theJob->processThisBelongsTo);
            //printf("%s\n", finalData[dataNdx]);
            dataNdx++;

            waitTime = theConfig.hardDriveCycleTime * theJob->numOfCycles;
         }
         else{
         	gettimeofday (&tvalBefore, NULL);
			   addTime();

            sprintf(finalData[dataNdx],"%d.%06d - Process %d : start monitor output\n",timerSec, 
                     timerMicro, theJob->processThisBelongsTo);
            //printf("%s\n", finalData[dataNdx]);
            dataNdx++;

            waitTime = theConfig.monitorDisplayTime * theJob->numOfCycles;
         }

   sleep((waitTime*.001));

   if ( !strcmp(theJob->operation, "hard drive" ) ){
   			gettimeofday (&tvalAfter, NULL);
			   addTime();

            sprintf(finalData[dataNdx],"%d.%06d - Process %d : end hard drive output\n\n",timerSec, 
                     timerMicro, theJob->processThisBelongsTo);
            //printf("%s\n", finalData[dataNdx]);
            dataNdx++;
         }
         else{
         	gettimeofday (&tvalAfter, NULL);
			   addTime();

            sprintf(finalData[dataNdx],"%d.%06d - Process %d : end monitor output\n\n",timerSec, 
                     timerMicro, theJob->processThisBelongsTo);
            //printf("%s\n", finalData[dataNdx]);
            dataNdx++;
         }

   return NULL;
}

void runProcess (int whichProcess){
   int ID;
   pthread_t tid[theProcesses[whichProcess].numOfJobs];
   int tidNdx = 0;
   int counter = 0; 

   while ( counter < theProcesses[whichProcess].numOfJobs ){
      if ( theProcesses[whichProcess].theJobs[counter].component == 'A' ){
         /*printf("A with operation: %s and cycle time %d\n",
                  theProcesses[whichProcess].theJobs[counter].operation,
                     theProcesses[whichProcess].theJobs[counter].numOfCycles);*/
         appOperation(whichProcess, counter);
      }

      else if ( theProcesses[whichProcess].theJobs[counter].component == 'P' ){
         /*printf("P with operation: %s and cycle time %d\n",
                  theProcesses[whichProcess].theJobs[counter].operation,
                     theProcesses[whichProcess].theJobs[counter].numOfCycles);*/
         runningOperation(whichProcess, counter);
      }

      else if ( theProcesses[whichProcess].theJobs[counter].component == 'I' ){
         /*printf("I with operation: %s and cycle time %d\n",
                  theProcesses[whichProcess].theJobs[counter].operation,
                     theProcesses[whichProcess].theJobs[counter].numOfCycles);*/
         ID = pthread_create(&(tid[tidNdx]), NULL, &inputOperation, 
                              &theProcesses[whichProcess].theJobs[counter]);

         if (ID != 0)
            printf("Error with thread creation.\n");

         pthread_join(tid[tidNdx],NULL);

         tidNdx++;
      }

      else if ( theProcesses[whichProcess].theJobs[counter].component == 'O' ){
         /*printf("O with operation: %s and cycle time %d\n",
                  theProcesses[whichProcess].theJobs[counter].operation,
                     theProcesses[whichProcess].theJobs[counter].numOfCycles);*/
         ID = pthread_create(&(tid[tidNdx]), NULL, &outputOperation, 
                              &theProcesses[whichProcess].theJobs[counter]);

         if (ID != 0)
            printf("Error with thread creation.\n");

         pthread_join(tid[tidNdx],NULL);

         tidNdx++;
      }

   counter++;
   }
}

void addTime(){
   /// check if the before or after is greater
	if (tvalAfter.tv_usec > tvalBefore.tv_usec){
      /// calculate the new seconds and microseconds by taking differences 
		long newSecs = tvalAfter.tv_sec - tvalBefore.tv_sec;
		long newUsecs = tvalAfter.tv_usec - tvalBefore.tv_usec;
      /// update the overall timer
		timerSec = timerSec + newSecs;
		timerMicro = timerMicro + newUsecs;
	}
   /// same but numbers are inversed numbers
	else {
		double newSecs = tvalBefore.tv_sec - tvalAfter.tv_sec;
		double newUsecs = tvalBefore.tv_usec - tvalAfter.tv_usec;
		timerSec = timerSec + newSecs;
		timerMicro = timerMicro + newUsecs;
	}
}

void output(){
	int ndx = 0;
   /// to log only to monitor
	if(theConfig.whereToLog == monitor){
      /// loop through data array
		while(ndx < dataNdx){
		printf("%s", finalData[ndx]);
		ndx++;
		}
	}

   /// to log to file
	else if (theConfig.whereToLog == file){

      /// open file
		FILE* fpointer;
   	fpointer = fopen( theConfig.loggingFilePath, "w");

      /// if couldnt open file end
   	if (fpointer == NULL){
   		printf("Could not open output file.\n");
   		return;
   	}

      /// loop through data and save to file
   	ndx = 0;
   	while(ndx < dataNdx){
		fprintf(fpointer, "%s", finalData[ndx]);
		ndx++;
		}

		fclose(fpointer);
	}

   /// to log to file and print to monitor 
	else{
      /// open file
      FILE* fpointer;
   	fpointer = fopen( theConfig.loggingFilePath, "w");

      /// error 
   	if (fpointer == NULL){
   		printf("Could not open output file.\n");
   	}

      /// loop through data
   	ndx = 0;
   	while(ndx < dataNdx){
         /// if valid file save to file
   	  if (fpointer != NULL)
		       fprintf(fpointer, "%s", finalData[ndx]);

      /// print to screen
		printf("%s", finalData[ndx]);

		ndx++;
		}

		fclose(fpointer);
	}
}