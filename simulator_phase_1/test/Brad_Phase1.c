// Include files
#include <stdio.h>     /* printf, perror */
#include <stdlib.h>    /* EXIT_FAILURE */
#include <string.h>    /* strcpy */
#include <unistd.h>  /* usleep */
#include <pthread.h>   /* pthread */
#include <sys/time.h>  /* gettimeofday */   
#include <ctype.h>    /* isspace */ 

// Struct declarations

//// struct to hold configuration file data ////
typedef struct{
  float version;
  char filePath[100];
  int processorCycleTime;
  int monitorDisplayTime;
  int hardDriveCycleTime;
  int printerCycleTime;
  int keyboardCycleTime;
  char logType[10];
  char logFilePath[100];
} configurationFile;


//// struct to hold meta data information ////
typedef struct{
  char componentLetter;
  char operation[20];
  int cycleTime;
} metaDataInfo;

//// struct to hold Process control block information ////
typedef struct{
  char processState[50];
  float timeUsed;
} processControlBlock;


// Function prototypes
void loadConfiguration( FILE* configFile, configurationFile *config, char* argv[] ); 

void loadMetaData( configurationFile config, metaDataInfo meta[], FILE* metaFile, int metaCounter );

void emulateProcess( metaDataInfo meta[], processControlBlock *pcb, int metaCounter, configurationFile config, FILE* logFile );

void logMetaData( char logMsg[], configurationFile config, float sleep, FILE* logFile );

void *emulateMetaData( void *addressPtr );


// Main program
int main(int argc, char* argv[]){
  // declare variables
  int metaDataCounter = 0;
  char temp;
  configurationFile config;
  processControlBlock pcb;

  // open config file and load struct
  FILE* configFileInput = fopen( argv[1], "r" );
  if(configFileInput == NULL) {
    perror("Error opening configuration file");
    return EXIT_FAILURE;
  }
  loadConfiguration( configFileInput, &config, argv );

  // open the file paths from the config
  FILE* metaDataInput = fopen( config.filePath, "r" );
  FILE* logFileOutput;

  // conditions for error handling and
  if( config.logType[0] == 'F' || config.logType[0] == 'B' ){
    if((metaDataInput == NULL) && !(logFileOutput = fopen( config.logFilePath, "w" ))){
      perror("Failed to open meta data file and log file. Check file paths in configuration file.");
      return EXIT_FAILURE;
    }

    if (!(logFileOutput = fopen( config.logFilePath, "w" ))) {
      perror("Failed to open log file. Check file path in the configuration file.");
      return EXIT_FAILURE;
    }
  }

  if(metaDataInput == NULL) {
    perror("Failed to open meta data file. Check file path in configuration file.");
    return EXIT_FAILURE;
  }

  // this loop is to find the total number of 
  // operations in the given meta data file   
  while( temp != EOF ){
    temp = fgetc( metaDataInput );

    if( temp == ';' || temp == '.' ){
        metaDataCounter++; 
      }
    }
  metaDataCounter -= 1;

  // allocate an array for the given size   
  // and rewind the file to prepare for load
  metaDataInfo metaData[metaDataCounter];
  rewind( metaDataInput );

  // load the meta data to the meta data struct
  loadMetaData( config, metaData, metaDataInput, metaDataCounter );

  // begin to execute the program 
  emulateProcess( metaData, &pcb, metaDataCounter, config, logFileOutput );

  if( config.logType[0] == 'F' ){
  printf( "Meta data logged to logfile.\n" );
  }
  
  return EXIT_SUCCESS;
}

void loadConfiguration( FILE* configFile, configurationFile *config, char* argv[] ){
  // declare variables
  char dummyString[100];

  // parse the config file and store necessary information
  fgets( dummyString , 100, configFile );
  fscanf( configFile, "%s %f \n" ,dummyString, &config->version ); 
  fscanf( configFile, "%s %s %s \n", dummyString, dummyString, config->filePath );
  fscanf( configFile, "%s %s %s %s %d \n" , dummyString, dummyString, dummyString, dummyString, &config->processorCycleTime );
  fscanf( configFile, "%s %s %s %s %d \n" , dummyString, dummyString, dummyString, dummyString, &config->monitorDisplayTime );
  fscanf( configFile, "%s %s %s %s %s %d \n" , dummyString, dummyString, dummyString, dummyString, dummyString, &config->hardDriveCycleTime );
  fscanf( configFile, "%s %s %s %s %d \n" , dummyString, dummyString, dummyString, dummyString, &config->printerCycleTime );
  fscanf( configFile, "%s %s %s %s %d \n" , dummyString, dummyString, dummyString, dummyString, &config->keyboardCycleTime );
  fscanf( configFile, "%s %s %s %s \n" , dummyString, dummyString, dummyString, config->logType );
  fscanf( configFile, "%s %s %s %s \n" , dummyString, dummyString, dummyString, config->logFilePath );
  fscanf( configFile, "%s %s %s %s \n" , dummyString, dummyString, dummyString, dummyString );

  // close config file since no longer needed
  fclose( configFile );
}

void loadMetaData( configurationFile config, metaDataInfo meta[], FILE* metaFile, int metaCounter ){
  // declare variables
  char dummy[100], temp;
  int index = 0, structIndex = 0;

  // get first line of input file
  fgets( dummy , 100, metaFile );

  // this loop parses the meta data file and stores necessary information
  for( structIndex = 0 ; structIndex < metaCounter ; structIndex++ ){
    // check for spaces at the end of a line of input
    temp = fgetc( metaFile );
    while( isspace( temp ) ){
      temp = fgetc( metaFile );
      }

    meta[structIndex].componentLetter = temp;
    fgetc( metaFile );
    dummy[index] = fgetc( metaFile );

    // this loop is to ignore parentheses on the operation/descriptor
    while( dummy[index] != ')' ){
      index++;
      dummy[index] = fgetc( metaFile );
      }
    dummy[index] = '\0';
    strcpy(meta[structIndex].operation, dummy);

    fscanf( metaFile, "%d", &meta[structIndex].cycleTime ); 
    fgetc( metaFile );

    index = 0;
  }

  // close meta data file since it is no longer needed
  fclose( metaFile );
}

void emulateProcess( metaDataInfo meta[], processControlBlock *pcb, int metaCounter, configurationFile config, FILE* logFile ){
  // declare variables
  int structIndex;
  char logDisplay[50];
  unsigned long sleepTime;
  pthread_t metaDataThread;
  struct timeval currentTime, startTime;

  // this loop emulates the operations given in the metadata file
  for( structIndex = 0 ; structIndex < metaCounter+1 ; structIndex++ ){
    // switch statement to check component letter
    switch( meta[structIndex].componentLetter ) {
      case 'S': 
        // check to see what the operation is
        if( meta[structIndex].operation[0] == 's' ){
          // set log display
          strcpy( logDisplay, "Simulator program starting" );

          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&startTime, NULL);
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );
          }
        // check to see what the operation is
        if( meta[structIndex].operation[0] == 'e' ){
          strcpy( logDisplay, "Simulator program ending" );

          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );
          }
      break;
   
      case 'A': 
        // check to see what the operation is
        if( meta[structIndex].operation[0] == 's' ){
          // set the state of the pcb
          strcpy( pcb->processState, "start" );
          strcpy( logDisplay, "OS: preparing process 1" );

          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );

          strcpy( logDisplay, "OS: starting process 1" );

          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );
          }

        // check to see what the operation is
        if( meta[structIndex].operation[0] == 'e' ){
          // set the state of the pcb
          strcpy( pcb->processState, "exit" );
          strcpy( logDisplay, "OS: removing process 1" );

          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );
          }
      break;

      case 'P': 
         // set the state of the pcb
         strcpy( pcb->processState, "running" );
         strcpy( logDisplay, "Process 1: start processing action" );

         // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
         gettimeofday(&currentTime, NULL);
         pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
         logMetaData( logDisplay, config, pcb->timeUsed, logFile );
 
         // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
         sleepTime = (config.processorCycleTime * meta[structIndex].cycleTime) * 1000;
         usleep( sleepTime );
     
         strcpy( logDisplay, "Process 1: end processing action" );
  
         // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
         gettimeofday(&currentTime, NULL);
         pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
         logMetaData( logDisplay, config, pcb->timeUsed, logFile );
      break;

      case 'I': 
      
        // check to see what the operation is
        if( meta[structIndex].operation[0] == 'h' ){
          strcpy( logDisplay, "Process 1: start hard drive input" );

          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );

          // calculate the time the operation takes and using a thread sleep for that long 
          sleepTime = config.hardDriveCycleTime * meta[structIndex].cycleTime;
          pthread_create( &metaDataThread, NULL, emulateMetaData, &sleepTime );
          pthread_join( metaDataThread, NULL ); 

          strcpy( logDisplay, "Process 1: end hard drive input" );

          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );
          }

        // check to see what the operation is
        if( meta[structIndex].operation[0] == 'k' ){
          strcpy( logDisplay, "Process 1: start keyboard input" );

          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );

          // calculate the time the operation takes and using a thread sleep for that long 
          sleepTime = config.keyboardCycleTime * meta[structIndex].cycleTime;
          pthread_create( &metaDataThread, NULL, emulateMetaData, &sleepTime );
          pthread_join( metaDataThread, NULL ); 

          strcpy( logDisplay, "Process 1: end keyboard input" );

          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );
          }
      break;

      case 'O': 

        // check to see what the operation is
        if( meta[structIndex].operation[0] == 'h' ){
          strcpy( logDisplay, "Process 1: start hard drive output" );

          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );

          // calculate the time the operation takes and use a thread to sleep for that long 
          sleepTime = config.hardDriveCycleTime * meta[structIndex].cycleTime;
          pthread_create( &metaDataThread, NULL, emulateMetaData, &sleepTime );
          pthread_join( metaDataThread, NULL ); 

          strcpy( logDisplay, "Process 1: end hard drive output" );
  
          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );
          }

        // check to see what the operation is
        if( meta[structIndex].operation[0] == 'm' ){
          strcpy( logDisplay, "Process 1: start monitor output" );

          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );

          // calculate the time the operation takes and use a thread to sleep for that long 
          sleepTime = config.monitorDisplayTime * meta[structIndex].cycleTime;
          pthread_create( &metaDataThread, NULL, emulateMetaData, &sleepTime );
          pthread_join( metaDataThread, NULL ); 

          strcpy( logDisplay, "Process 1: end monitor output" );

          // calculate the time the operation takes, store that in the pcb, and log the data according to the config file
          gettimeofday(&currentTime, NULL);
          pcb->timeUsed = (currentTime.tv_sec - startTime.tv_sec) + ((currentTime.tv_usec - startTime.tv_usec) * 0.000001);
          logMetaData( logDisplay, config, pcb->timeUsed, logFile );
          }
      break;
    }
  }

  // close log file since it is no longer needed
  if ((logFile = fopen( config.logFilePath, "w" ))) {
    fclose(logFile);
  }
}

void logMetaData( char logMsg[], configurationFile config, float sleep, FILE* logFile ){
  // check for the log type in the config file and output accordingly
  if( config.logType[0] == 'M' ){ 
    printf( "%f - %s\n" , sleep , logMsg );
    }

  else if( config.logType[0] == 'F' ){
    fprintf( logFile, "%f - %s\n" , sleep , logMsg ); 
    }

  else{
    printf( "%f - %s\n" , sleep , logMsg );
    fprintf( logFile, "%f - %s\n" , sleep , logMsg ); 
    }
}

// this is my entry point for my thread
void *emulateMetaData( void *addressPtr ){
 // declare variables
 unsigned long* time = (unsigned long*)addressPtr; 
 // delay for correct time
 *time *= 1000;
 usleep(*time);
 return NULL;
}

