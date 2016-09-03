//Start program macros
#ifndef __OS_PHASE_1_
#define __OS_PHASE_1_

//Header Files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <wait.h>
#include <pthread.h>
#include <time.h>
#define BUFFER_SIZE 200
#define LARGE_BUFFER_SIZE 1000
#define STD_STR_LEN 20

//Struct Variable Definition
struct process{
   char type;                //type of operation to perform
   char operation[STD_STR_LEN];       //where operation will happen
   int cycle_time;           //number of cycles needed
};

struct configuration{
   float version;             //number
   char file_path[BUFFER_SIZE];       //complete file path and name of file
   int processor_cycle_time;  //(msec/cycle) time
   int monitor_cycle_time;    //(msec/cycle) time
   int hd_cycle_time;         //(msec/cycle) time
   int printer_cycle_time;    //(msec/cycle) time
   int keyboard_cycle_time;   //(msec/cycle) time
   int where_to_log;          //Neg - Monitor
                              //0   - Both
                              //Pos - File
   char log_file_path[BUFFER_SIZE];   //complete file path and name of file
};

struct PCB{
   //type of state
   enum mystate{
      ENTER,
      RUNNING,
      EXIT
   }state;
};

//Global Variables
struct process totalProcess[LARGE_BUFFER_SIZE];
struct configuration mainConfiguration;
struct PCB mainPCB;
int sizeOfInputForSim = 0;
char currentRunningReport[LARGE_BUFFER_SIZE];
int mainIndex = 0;
char loggingData[LARGE_BUFFER_SIZE][BUFFER_SIZE];
int reportIndex = 0;

//Function Prototypes
void readMetaDataFile( const char * filename );
void readConfigFile( const char * filename );
void fixAllignment( char * fixingWord );
void outputToMonitor( char outgoingData[][BUFFER_SIZE], int size );
void outputToFile( char outgoingData[][BUFFER_SIZE], int size );
void getCurrentTime( struct timeval epoch, char * outgoingWord );

void * inputOperation( void * param );
void * outputOperation( void * param );

//Main Program
int main(int argc, const char *argv[]){

   //report error if inadequate arguments passed
   if( argc < 2 ){
      perror("Didn't include name as second argument.");
      return 0;
   }

   //declare variables
   struct timeval pseudoEpoch;
   pthread_t thread_id;
   struct timespec timeToSleep = {0};
   int millisecondsToSleep;

   //put the PCB in ready state
   mainPCB.state = ENTER;
   
   //read in all files
      //accept configuration for this run
      readConfigFile( argv[1] );
      //accept meta data for one program
      readMetaDataFile( mainConfiguration.file_path );

   //get time of day to consider as beginning of time
   gettimeofday(&pseudoEpoch, NULL);

   //run it
   for(mainIndex = 0; mainIndex < sizeOfInputForSim; mainIndex++){

      //If this is a SYSTEM event
      if( totalProcess[mainIndex].type == 'S' ){
         //if system is starting
         if( !strcmp(totalProcess[mainIndex].operation, "start") ){

            //log event and save
            getCurrentTime( pseudoEpoch, currentRunningReport );
            strcat( currentRunningReport, "Simulator program starting" );

            //save over
            strcpy( loggingData[reportIndex++], currentRunningReport );
         }
         //If system is ending
         else{

            //log event and save
            getCurrentTime( pseudoEpoch, currentRunningReport );
            strcat( currentRunningReport, "Simulator program ending" );

            //save and log
            strcpy( loggingData[reportIndex++], currentRunningReport );
         }
      }

      //If this is an APPLICATION PROGRAM event
      else if( totalProcess[mainIndex].type == 'A' ){
         //if application is starting
         if( !strcmp(totalProcess[mainIndex].operation, "start") ){

            //log that the process is being prepared and save
            getCurrentTime( pseudoEpoch, currentRunningReport );
            strcat( currentRunningReport, "OS: preparing process 1" );
            strcpy( loggingData[reportIndex++], currentRunningReport );

            //set up the process control block
            mainPCB.state = RUNNING;

            //log that the process is starting and save
            getCurrentTime( pseudoEpoch, currentRunningReport );
            strcat( currentRunningReport, "OS: starting process 1" );
            strcpy( loggingData[reportIndex++], currentRunningReport );
         }

         //if application is ending
         else{

            //log that the process is ending and save
            getCurrentTime( pseudoEpoch, currentRunningReport );
            strcat( currentRunningReport, "OS: removing process 1" );
            strcpy( loggingData[reportIndex++], currentRunningReport );
         }
      }

      //If this is a PROCESS event
      else if( totalProcess[mainIndex].type == 'P' ){
         
         //log that the process is starting and save
         getCurrentTime( pseudoEpoch, currentRunningReport );
         strcat( currentRunningReport, "Process 1: start processing action");
         strcpy( loggingData[reportIndex++], currentRunningReport );

         //wait for however long you have to wait
         millisecondsToSleep = ( totalProcess[mainIndex].cycle_time * mainConfiguration.processor_cycle_time ) / 1000;
         timeToSleep.tv_sec = millisecondsToSleep / 1000;
         timeToSleep.tv_nsec = millisecondsToSleep * 1000000;
         nanosleep( &timeToSleep, NULL );

         //log that the process is ending and save
         getCurrentTime( pseudoEpoch, currentRunningReport );
         strcat( currentRunningReport, "Process 1: end processing action");
         strcpy( loggingData[reportIndex++], currentRunningReport );
      }

      //If this is an INPUT event
      else if( totalProcess[mainIndex].type == 'I' ){
         //create thread to do stuff
         pthread_create( &thread_id, NULL, inputOperation, &pseudoEpoch );

         //wait for the thread to end to save
         pthread_join( thread_id, NULL );
      }

      //If this is an OUTPUT event
      else if( totalProcess[mainIndex].type == 'O' ){
         //create thread to do stuff
         pthread_create( &thread_id, NULL, outputOperation, &pseudoEpoch );

         //wait for the thread to end to save
         pthread_join( thread_id, NULL );
      }

      //In the event that there was another option
      else{
         perror("Invalid Meta-Data operation is invalid.");
         exit(EXIT_FAILURE);
      }

   }

   //output the log to the monitor if possible
   if( mainConfiguration.where_to_log < 1 ){
      outputToMonitor( loggingData, reportIndex );
   }
   //output the log to the file if possible
   if( mainConfiguration.where_to_log > - 1 ){
      outputToFile( loggingData, reportIndex );
   }

   //end simulation
   mainPCB.state = EXIT;
   return EXIT_SUCCESS;
}

//Function Implementation
void readMetaDataFile( const char * filename ){
   //declare variables
   FILE * ifp;
   int mainArrayIndex = 0, index = 0, charIndex = 0;
   char buffer[LARGE_BUFFER_SIZE][STD_STR_LEN], tempword[STD_STR_LEN];
   char tempChar;

   //open file, report error and exit if err
   ifp = fopen( filename, "r");
   if( ifp == NULL ){
      printf("Can't open input file %s\n", filename );
      exit(1);
   }

   //check to see if its the right file
   for (index = 0; index < 4; index++){
      fscanf(ifp, "%s", tempword);
   }

   if( !strcmp(tempword, "Code:\n") ){
      printf("This file, '%s' is the wrong file.\n", filename );
      exit(1);
   }
   //take in the null
   fscanf(ifp, "%c", &tempword[0]);

   //read in from file
   index = 0; 
   while( !feof(ifp) && tempChar != '\n'){
      //reset local counter
      charIndex = 0;
      //while we haven't read the end of a statement
      do{
         //take in the next character
         fscanf(ifp, "%c", &tempChar);

         //save the character
         buffer[index][charIndex++] = tempChar;
      }while( tempChar != ';' && tempChar != '.' );

      //add null
      buffer[index][charIndex] = '\0';

      //if the first character is an endline, 
      //fix it, otherwise don't
      fixAllignment( buffer[index] );

      //take in the white space
      fscanf(ifp, "%c", &tempChar);
      index++;
   }

   sizeOfInputForSim = index - 1;

   //process each entry and save them into the main array
   for(mainArrayIndex = 0; mainArrayIndex < sizeOfInputForSim; mainArrayIndex++){
      //reset local counter
      charIndex = 0;

      //save the TYPE
      totalProcess[mainArrayIndex].type = buffer[mainArrayIndex][charIndex++];

      //extra increment for the first '(', set buffer index
      charIndex++;

      //save the OPERATION
         index = 0;
         tempChar = buffer[mainArrayIndex][charIndex++];
         while( tempChar != ')'){
            //take the temporary char
            tempword[index] = tempChar;
            tempChar = buffer[mainArrayIndex][charIndex++];
            index++;
         }
         //add the null
         tempword[index] = '\0';

         //actually save it over
         strcpy(totalProcess[mainArrayIndex].operation, tempword);

      //extra increment for the first '(', set buffer index
      charIndex++;

      //save the cycle time
         index = 0;
         while( tempChar != ';' && tempChar != '.' ){
            tempChar = buffer[mainArrayIndex][charIndex++];
            tempword[index] = tempChar;
            index++;
         }
         //add the null         
         tempword[index] = '\0';

         //actually save it over
         totalProcess[mainArrayIndex].cycle_time = atoi( tempword );
   }
   //close the file
   fclose( ifp );
}

void fixAllignment( char * fixingWord ){
   //declare variables
   int index = 0;

   //if the first char not endline, skip
   if( fixingWord[0] == '\n' ){

      //loop until the null char is found
      while( fixingWord[index] != '\0' ){

         //move all characters over
         fixingWord[index] = fixingWord[index+1];
         index++;
      }
   }
}

void readConfigFile( const char * filename ){
   //declare variables
   FILE * ifp;
   char tempword[100], tempChar;
   int index;

   //if unable to open, report
   ifp = fopen( filename, "r");

   if( ifp == NULL ){
      printf("Can't open input file %s\n", filename );
      exit(1);
   }

   //compare the first line, report if wrong
   for( index = 0; index < 4; index++ ){
      fscanf(ifp, "%s", tempword);
   }
   if( !strcmp(tempword, "File\n") ){
      printf("This file, '%s' is the wrong file.\n", filename );
      exit(1);
   }

   //start taking in from the file
      //take in version
      fscanf(ifp, "%s %f", tempword, &mainConfiguration.version );

      //take in file path
      index = 0;
      fscanf(ifp, "%s %s %c", tempword, tempword, &tempChar );
      tempword[index++] = tempChar;

      //keep reading while not at the end of the line
      while( tempChar != '\n' ){
         fscanf(ifp, "%c", &tempChar );
         tempword[index++] = tempChar;
      }

      //take care of extra endl
      tempword[index - 1] = '\0';

      //copy the word over to the correct spot
      strcpy( mainConfiguration.file_path, tempword );

      //take in cycle times
      fscanf(ifp, "%s %s %s %s %i",
         tempword, tempword, tempword, tempword,
         &mainConfiguration.processor_cycle_time);

      fscanf(ifp, "%s %s %s %s %i",
         tempword, tempword, tempword, tempword,
         &mainConfiguration.monitor_cycle_time);

      fscanf(ifp, "%s %s %s %s %s %i",
         tempword, tempword, tempword, tempword, tempword,
         &mainConfiguration.hd_cycle_time);

      fscanf(ifp, "%s %s %s %s %i",
         tempword, tempword, tempword, tempword,
         &mainConfiguration.printer_cycle_time);

      fscanf(ifp, "%s %s %s %s %i",
         tempword, tempword, tempword, tempword,
         &mainConfiguration.keyboard_cycle_time);

      //take in the log location
      fscanf(ifp, "%s %s %s %s",
         tempword, tempword, tempword, tempword);

      if( !strcmp( tempword, "Monitor" ) ){
         mainConfiguration.where_to_log = -1;
      }else if( !strcmp( tempword, "File" ) ){
         mainConfiguration.where_to_log = 1;
      }else{
         mainConfiguration.where_to_log = 0;
      }

      //iff we have to log to the file at some 
      //point, save the log file location
      if( mainConfiguration.where_to_log > -1 ){
         index = 0;
         fscanf(ifp, "%s %s %s %c", tempword, tempword, tempword, &tempChar );
         tempword[index++] = tempChar;

         //keep reading while not at the end of the line
         while( tempChar != '\n' ){
          fscanf(ifp, "%c", &tempChar );
          tempword[index++] = tempChar;
         }

         //take care of extra endl
         tempword[index - 1] = '\0';

         //copy the word over to the correct spot
         strcpy( mainConfiguration.log_file_path, tempword );
      }

   //close the file
   fclose(ifp);
}

void outputToMonitor( char outgoingData[][BUFFER_SIZE], int size ){
   //declare variables
   int counter = 0;

   //iterate through array and output with printf
   for( counter = 0; counter < size; counter++ ){
      printf("%s\n", outgoingData[counter] );
   }
}

void outputToFile( char outgoingData[][BUFFER_SIZE], int size ){
   //declare variables
   int counter = 0;
   FILE * ofp;

   //open file for writing
   ofp = fopen( mainConfiguration.log_file_path, "w" );

   //if file can't be open say so
   if( ofp == NULL ){
      perror("Can't open file for writing, invalid file path or insufficient memory");
      exit(EXIT_FAILURE);
   }

   //iterate through array and output with fprintf
   for( counter = 0; counter < size; counter++ ){
      fprintf(ofp, "%s\n", outgoingData[counter] );
   }

   //close the file
   fclose(ofp);
}

void getCurrentTime( struct timeval epoch, char * outgoingWord ){
   //declare variables
   struct timeval current;
   int seconds, microseconds;
   char buffer[BUFFER_SIZE];

   //given the current time
   gettimeofday(&current, NULL);

   //subtract from epoch to get change in time
   seconds = current.tv_sec - epoch.tv_sec;
   microseconds = current.tv_usec - epoch.tv_usec;

   //get string that defines current time and add to word
   snprintf(buffer, sizeof(buffer), "%i.%6.6i", seconds, microseconds);
   strcpy( outgoingWord, buffer );

   //add the dash and keep going
   strcat( outgoingWord, " - " );
}

void * inputOperation( void * param ){
   //declare variables
   struct timeval * beginningOfTime = (struct timeval *) param;
   struct timespec timeToSleep = {0};
   int millisecondsToSleep;

   //get time and compute
   getCurrentTime( *beginningOfTime, currentRunningReport );
   strcat( currentRunningReport, "Process 1: start " );
   strcat( currentRunningReport, totalProcess[mainIndex].operation );
   strcat( currentRunningReport, " input" );
   strcpy( loggingData[reportIndex++], currentRunningReport );

   //wait for some times
   millisecondsToSleep = ( totalProcess[mainIndex].cycle_time * mainConfiguration.processor_cycle_time ) / 1000;
   timeToSleep.tv_sec = millisecondsToSleep / 1000;
   timeToSleep.tv_nsec = millisecondsToSleep * 1000000;
   nanosleep( &timeToSleep, NULL );

   //get time for end and compute
   getCurrentTime( *beginningOfTime, currentRunningReport );
   strcat( currentRunningReport, "Process 1: end " );
   strcat( currentRunningReport, totalProcess[mainIndex].operation );
   strcat( currentRunningReport, " input" );
   strcpy( loggingData[reportIndex++], currentRunningReport );

   //kill the thread
   return NULL;
}

void * outputOperation( void * param ){
   //declare variables
   struct timeval * beginningOfTime = (struct timeval *) param;
   struct timespec timeToSleep = {0};
   int millisecondsToSleep;

   //get time and compute
   getCurrentTime( *beginningOfTime, currentRunningReport );
   strcat( currentRunningReport, "Process 1: start " );
   strcat( currentRunningReport, totalProcess[mainIndex].operation );
   strcat( currentRunningReport, " output" );
   strcpy( loggingData[reportIndex++], currentRunningReport );

   //wait for some times
   millisecondsToSleep = ( totalProcess[mainIndex].cycle_time * mainConfiguration.processor_cycle_time ) / 1000;
   timeToSleep.tv_sec = millisecondsToSleep / 1000;
   timeToSleep.tv_nsec = millisecondsToSleep * 1000000;
   nanosleep( &timeToSleep, NULL );

   //get time for end and compute
   getCurrentTime( *beginningOfTime, currentRunningReport );
   strcat( currentRunningReport, "Process 1: end " );
   strcat( currentRunningReport, totalProcess[mainIndex].operation );
   strcat( currentRunningReport, " output" );
   strcpy( loggingData[reportIndex++], currentRunningReport );

   //kill the thread
   return NULL;
}

#endif