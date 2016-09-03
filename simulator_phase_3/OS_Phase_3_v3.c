#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdbool.h>

/// Structure Definition
struct jobInfo{
   char component;
   char operation[25];
   int numOfCycles;
   int cyclesDone;
   int processThisBelongsTo;
   int cycleTime; 
};

struct PCB{
   /// running type
   enum {
      enter, ready, running, blocked, done
   } state;

   struct jobInfo * theJobs;
   int numOfJobs; 
   int jobsDone;

   bool interrupt; 
};

struct config{
   float version;
   char filePath[50];
   char scheduling[5];
   int quantum; 
   int processorCycleTime; 
   int monitorDisplayTime;
   int hardDriveCycleTime;
   int printerCycleTime;
   int keyboardCycleTime;
   enum {
      monitor, file, both
   } whereToLog; 
   char loggingFilePath[50]; 
};

/// Global var
struct timeval firstTime; 

/// Function Prototypes
void readConfig (char* fileName, struct config* theConfig);
int readMeta (char* fileName, struct PCB** theProcesses);
char* appendChar(char* str, char c);
void initializePCB(struct config* theConfig, struct PCB* theProcesses, int numOfProcesses);
/*

*/
void runProcess(struct config* theConfig, struct PCB* theProcess);
void startOp(struct PCB* theProcess);
void runningJob(struct config* theConfig, struct PCB* theProcess, int remainingTime);
void* inputOperation(void *arg);
void* outputOperation(void *arg);
void ioEndOp(struct PCB* theProcess);

void getTime(char* returnStr);

int main(int argc, char* argv[]){
   // error check fro main
   if (argc != 2){
      printf("Error with command line arguments. \n");
      return 0;
   }

   // Variables
   struct config configFile;
   struct PCB* processes; 
   int numOfProcesses = 0;

   readConfig(argv[1], &configFile);

   printf("Version: %f\n", configFile.version);
   printf("filePath: %s\n", configFile.filePath);
   printf("scheduling: %s\n", configFile.scheduling);
   printf("quantum: %i\n", configFile.quantum);
   printf("processorCycleTime: %i\n", configFile.processorCycleTime);
   printf("monitorDisplayTime: %i\n", configFile.monitorDisplayTime);
   printf("hardDriveCycleTime: %i\n", configFile.hardDriveCycleTime);
   printf("printerCycleTime: %i\n", configFile.printerCycleTime);
   printf("keyboardCycleTime: %i\n", configFile.keyboardCycleTime);
   printf("whereToLog: %i\n", configFile.whereToLog);
   printf("loggingFilePath: %s\n", configFile.loggingFilePath);


   numOfProcesses = readMeta(configFile.filePath, &processes);

   initializePCB(&configFile, processes, numOfProcesses);

   int i = 0;
   int j = 0;

   for (i = 0; i < numOfProcesses; ++i)
   {
      for (j = 0; j < processes[i].numOfJobs; ++j)
      {
         printf("%c %s Required Quantum: %d Time Per Quantum: %i Process: %i\n",
         processes[i].theJobs[j].component, processes[i].theJobs[j].operation,
               processes[i].theJobs[j].numOfCycles, processes[i].theJobs[j].cycleTime,
                processes[i].theJobs[j].processThisBelongsTo);
      }

      printf("\n");
   }

   gettimeofday(&firstTime,NULL);

   int processNdx = 0;
   int interruptNdx = 0;

   bool everythingDone = false;

   // check if ready

   while (!everythingDone){
      if(processes[processNdx].state == ready){
         runProcess(&configFile, &processes[processNdx]);
      }

      processNdx++;
      if (processNdx == numOfProcesses){
         processNdx = 0; 
      }

      for(interruptNdx=0; interruptNdx < numOfProcesses; interruptNdx++){
         if(processes[interruptNdx].interrupt == true){
            ioEndOp(&processes[interruptNdx]);
         }
      }

      everythingDone = true;
      for(interruptNdx = 0; interruptNdx < numOfProcesses; interruptNdx++){
         if(processes[interruptNdx].state != done){
            everythingDone = false;
         }
      }
   }

   return 0;
}

void readConfig (char* fileName, struct config* theConfig){
   
   /// garbage read in
   char temp1[25];
   char temp2[25];
   char temp3[25];
   char temp4[25];
   char temp5[25];

   /// open file 
   FILE* fpointer;
   fpointer = fopen( fileName, "r" );

   /// take in first line of config file (garbage)
   fscanf(fpointer, "%s %s %s %s", temp1, temp2, temp3, temp4);

   /// take in version number
   fscanf(fpointer, "%s %f", temp1, &theConfig->version);

   /// metadata file path
   fscanf(fpointer, "%s %s %s", temp1, temp2, theConfig->filePath );

   ///  read scheduling type
   fscanf(fpointer, "%s %s %s", temp1, temp2, theConfig->scheduling );

   ///  read quantum
   fscanf(fpointer, "%s %s %s %i", temp1, temp2, temp3, &theConfig->quantum );

   /// Processpr cycle time
   fscanf(fpointer, "%s %s %s %s %i", temp1, temp2, temp3, temp4, &theConfig->processorCycleTime );

   /// Monitor display time
   fscanf(fpointer, "%s %s %s %s %i", temp1, temp2, temp3, temp4, &theConfig->monitorDisplayTime);

   /// Hard drive cycle time
   fscanf(fpointer, "%s %s %s %s %s %i", 
      temp1, temp2, temp3, temp4, temp5, &theConfig->hardDriveCycleTime);


   /// Printer cycle time
   fscanf(fpointer, "%s %s %s %s %i", temp1, temp2, temp3, temp4, &theConfig->printerCycleTime);

   /// Keyboard cycle time
   fscanf(fpointer, "%s %s %s %s %i", temp1, temp2, temp3, temp4, &theConfig->keyboardCycleTime);

   /// Where to log
   fscanf(fpointer, "%s %s %s %s", temp1, temp2, temp3, temp4);

   /// Check value of string
   if (strcmp(temp4, "Both") == 0){
      theConfig->whereToLog = both;
   }
   else if (strcmp(temp4, "File") == 0){
      theConfig->whereToLog = file;
   }
   else{
      theConfig->whereToLog = monitor;
   }

   /// Log File Path
   fscanf(fpointer, "%s %s %s %s", temp1, temp2, temp3, theConfig->loggingFilePath);

   /// close the file
   fclose(fpointer);
}

int readMeta (char* fileName, struct PCB** theProcesses){
   int processCounter = 0;
   int sizeOfMeta = 0;

   /// temp variables
   char temp1[25];
   char temp2[25];
   char temp3[25];
   char temp4[25];
   char temp5[25];
   char temp;
   char semicolon = ';';

   /// open file
   FILE* fpointer;
   fpointer = fopen( fileName, "r");

   if (fpointer == NULL){
      printf("Could not open metadata file. Check filepath and retry.\n");
      return 0;
   }

   /// read in heading
   fscanf (fpointer, "%s %s %s %s", temp1, temp2, temp3, temp4);

   while (temp != '.'){
      temp = fgetc(fpointer);
      if (temp == ';')
         sizeOfMeta++;
      if (temp == 'A')
         processCounter++;
   }
   sizeOfMeta++;
   processCounter = processCounter / 2;

   int counter = 0;

   /// allocated memory for all PCB
   (*theProcesses) = (struct PCB*) malloc(processCounter * sizeof(struct PCB));

   /// allocate memory for the jobs of the PCB
   for (counter = 0; counter < processCounter; counter++){
      (*theProcesses)[counter].theJobs = (struct jobInfo* )malloc(sizeOfMeta * sizeof(struct jobInfo));
   }

   /// add null chars to all strings 
   int inner = 0;
   for (counter = 0; counter < processCounter; counter++){
         for (inner = 0; inner < sizeOfMeta; inner++){
            (*theProcesses)[counter].theJobs[inner].operation[0] = '\0';
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

         (*theProcesses)[processNdx].theJobs[jobNdx].component = temp;
      }
      else{
         (*theProcesses)[processNdx].theJobs[jobNdx].component = temp;
      }

      /// get parentheses
      temp = fgetc(fpointer);

      /// get operation
      temp = fgetc(fpointer);
      while (temp != ')'){
         appendChar ((*theProcesses)[processNdx].theJobs[jobNdx].operation, temp);
         temp = fgetc(fpointer);
      }

      /// get # of cycles for task
      fscanf(fpointer, "%d",&(*theProcesses)[processNdx].theJobs[jobNdx].numOfCycles );

      /// read in semicolon and space
      semicolon = fgetc(fpointer);

      temp = fgetc(fpointer);

      /// if end of go to next process and first job
      if ( !strcmp((*theProcesses)[processNdx].theJobs[jobNdx].operation, "end" ) ){
         (*theProcesses)[processNdx].numOfJobs = jobNdx+1;
         //(*theProcesses)[processNdx].state = ready;
         (*theProcesses)[processNdx].theJobs[jobNdx].processThisBelongsTo = (processNdx+1);

         /// if last process then end loop
         if ((processNdx+1) == processCounter){
            semicolon = '.';
         }

         processNdx++;
         jobNdx = 0;
      } 
      else {
         (*theProcesses)[processNdx].theJobs[jobNdx].processThisBelongsTo = (processNdx+1);
         jobNdx++;
      }  
   }

   /// close file
   fclose(fpointer);

   return processCounter;
}

char* appendChar(char* str, char c){
   int length = strlen(str);

   str[length+1] = '\0';
   str[length] = c;

   return str;
}

void initializePCB(struct config* theConfig, struct PCB* theProcesses, int numOfProcesses){
   
   int processNdx;
   int jobNdx;

   for (processNdx = 0; processNdx < numOfProcesses; processNdx++){
      for (jobNdx = 0; jobNdx < theProcesses[processNdx].numOfJobs; jobNdx++){
         if( !strcmp(theProcesses[processNdx].theJobs[jobNdx].operation, "run") ){
            theProcesses[processNdx].theJobs[jobNdx].cycleTime = theConfig->processorCycleTime;
         }
         else if( !strcmp(theProcesses[processNdx].theJobs[jobNdx].operation, "monitor") ){
            theProcesses[processNdx].theJobs[jobNdx].cycleTime = theConfig->monitorDisplayTime;
         }
         else if( !strcmp(theProcesses[processNdx].theJobs[jobNdx].operation, "hard drive") ){
            theProcesses[processNdx].theJobs[jobNdx].cycleTime = theConfig->hardDriveCycleTime;
         }
         else if( !strcmp(theProcesses[processNdx].theJobs[jobNdx].operation, "keyboard") ){
            theProcesses[processNdx].theJobs[jobNdx].cycleTime = theConfig->keyboardCycleTime;
         }
         else {
            theProcesses[processNdx].theJobs[jobNdx].cycleTime = theConfig->printerCycleTime;
         } 

         theProcesses[processNdx].theJobs[jobNdx].cyclesDone = 0;
      }

      theProcesses[processNdx].jobsDone = 0;
      theProcesses[processNdx].state = ready;
   }
}

void runProcess(struct config* theConfig, struct PCB* theProcess){
   if ( theProcess->theJobs[theProcess->jobsDone].component == 'A' ){
      startOp(theProcess);
   }

   else if ( theProcess->theJobs[theProcess->jobsDone].component == 'P' ){
      runningJob(theConfig, theProcess,theConfig->quantum);
   }

   else if ( theProcess->theJobs[theProcess->jobsDone].component == 'I' ){
      pthread_t tid;
      pthread_create(&tid, NULL, &inputOperation, theProcess);

      theProcess->state = blocked;
   }

   else if ( theProcess->theJobs[theProcess->jobsDone].component == 'O' ){
      pthread_t tid;
      pthread_create(&tid, NULL, &outputOperation, theProcess);

      theProcess->state = blocked;
   }

   if (theProcess->jobsDone == theProcess->numOfJobs)
      theProcess->state = done;

}

void startOp(struct PCB* theProcess){
   char timeStr[50];

   getTime(timeStr);

   if ( !strcmp(theProcess->theJobs[theProcess->jobsDone].operation, "start") ){
      printf("%s OS: Starting process %i \n", timeStr, 
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);
   }

   else{
      printf("%s OS: Ending process %i \n", timeStr,
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);
   }

   theProcess->jobsDone++;
}

void runningJob(struct config* theConfig, struct PCB* theProcess, int remainingTime){
   char timeStr[50];

   // update state
   theProcess->state = running;

   getTime(timeStr);

   int waitTime; 

   printf("%s Process %i: starting processing action\n", timeStr, 
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);

   // check to see how many cycles the job has left 
   int cyclesLeft = theProcess->theJobs[theProcess->jobsDone].numOfCycles - theProcess->theJobs[theProcess->jobsDone].cyclesDone;

   //compare remainingtime (originally the quantum)

   if (remainingTime == cyclesLeft){
      waitTime = theConfig->processorCycleTime * cyclesLeft;
      sleep((waitTime*.001));

      getTime(timeStr);
      printf("%s Process %i: ending processing action\n", timeStr, 
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);

      theProcess->theJobs[theProcess->jobsDone].cyclesDone = theProcess->theJobs[theProcess->jobsDone].cyclesDone +
         cyclesLeft;

      theProcess->jobsDone++;

      theProcess->state = ready;
   }

   else if(remainingTime > cyclesLeft){
      waitTime = theConfig->processorCycleTime * cyclesLeft;
      sleep((waitTime*.001));

      getTime(timeStr);
      printf("%s Process %i: ending processing action\n", timeStr, 
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);

      theProcess->theJobs[theProcess->jobsDone].cyclesDone = theProcess->theJobs[theProcess->jobsDone].cyclesDone +
         cyclesLeft;

      theProcess->jobsDone++;

      // check next Job, if I/O, create thread

      if (theProcess->theJobs[theProcess->jobsDone].component == 'I'){
         pthread_t tid;
         pthread_create(&tid, NULL, &inputOperation, theProcess);

         theProcess->state = blocked;
      }
      else if (theProcess->theJobs[theProcess->jobsDone].component == 'O'){
         pthread_t tid;
         pthread_create(&tid, NULL, &outputOperation, theProcess);

         theProcess->state = blocked;
      }
      else{
         runningJob(theConfig, theProcess, (remainingTime-cyclesLeft));
      }      
   }

   else{
      waitTime = theConfig->processorCycleTime * cyclesLeft;
      sleep((waitTime*.001));

      getTime(timeStr);
      printf("%s Process %i: ending processing action\n", timeStr, 
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);

      theProcess->theJobs[theProcess->jobsDone].cyclesDone = theProcess->theJobs[theProcess->jobsDone].cyclesDone +
         cyclesLeft;

      theProcess->state = ready;
   }
}

void* inputOperation(void *arg){
   char timeStr[50];
   int waitTime; 

   struct PCB* theProcess = arg;

   getTime(timeStr);

   if ( !strcmp( theProcess->theJobs[theProcess->jobsDone].operation , "hard drive" ) ){
      printf("%s Process %i: starting hard drive input\n", timeStr, 
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);

      waitTime = theProcess->theJobs[theProcess->jobsDone].cycleTime 
                  * theProcess->theJobs[theProcess->jobsDone].numOfCycles;
   }
   else{
      printf("%s Process %i: starting keyboard input\n", timeStr, 
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);

      waitTime = theProcess->theJobs[theProcess->jobsDone].cycleTime 
                  * theProcess->theJobs[theProcess->jobsDone].numOfCycles;
   }

   sleep((waitTime * .001));

   theProcess->interrupt = true;

   return NULL;
}

void* outputOperation(void *arg){
   char timeStr[50];
   int waitTime; 

   struct PCB* theProcess = arg;

   getTime(timeStr);

   if ( !strcmp( theProcess->theJobs[theProcess->jobsDone].operation , "hard drive" ) ){
      printf("%s Process %i: starting hard drive output\n", timeStr, 
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);

      waitTime = theProcess->theJobs[theProcess->jobsDone].cycleTime 
                  * theProcess->theJobs[theProcess->jobsDone].numOfCycles;
   }
   else{
      printf("%s Process %i: starting monitor output\n", timeStr, 
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);

      waitTime = theProcess->theJobs[theProcess->jobsDone].cycleTime 
                  * theProcess->theJobs[theProcess->jobsDone].numOfCycles;
   }

   sleep((waitTime * .001));

   theProcess->interrupt = true;

   return NULL;
}

void ioEndOp(struct PCB* theProcess){
   char timeStr[50];

   getTime(timeStr);

   if (theProcess->theJobs[theProcess->jobsDone].component == 'I'){
      if ( !strcmp( theProcess->theJobs[theProcess->jobsDone].operation , "hard drive" ) ){
         printf("%s Process %i: ending hard drive input\n", timeStr, 
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);
      }
      else{
         printf("%s Process %i: ending keyboard input\n", timeStr, 
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);
      }
   }
   else{
      if ( !strcmp( theProcess->theJobs[theProcess->jobsDone].operation , "hard drive" ) ){
      printf("%s Process %i: ending hard drive output\n", timeStr, 
               theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);
      }
      else{
         printf("%s Process %i: ending monitor output\n", timeStr, 
                  theProcess->theJobs[theProcess->jobsDone].processThisBelongsTo);
      }

   }

   theProcess->interrupt = false;
   theProcess->state = ready;

   theProcess->jobsDone++;
}

void getTime(char* returnStr){
   int csec;
   int cusec;

   struct timeval currentTime;

   gettimeofday(&currentTime, NULL);

   csec = (currentTime.tv_sec - firstTime.tv_sec);
    cusec = (currentTime.tv_usec - firstTime.tv_usec);

    if (cusec < 0)
      cusec += 1000000;

    snprintf(returnStr, sizeof(returnStr)*50, "%i.%06i -", csec, cusec);
}