
/////////////      HEADER     /////////////
///////////////////////////////////////////
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

////////////  Global Variables ////////////
///////////////////////////////////////////
int stringMax = 60;
int sizeOfData = 0;
const char ENTER = 'E';
const char READY = 'Y';
const char RUNNING = 'R';
const char EXIT = 'X';
const char PCBcomponent = 'I';
pthread_t tid[0];
int number = 1;

///////////     Structers    /////////////
//////////////////////////////////////////
struct configFile
{
	float version;
	char fPath[50];
	char approach[10];
	char schedule[50];
	int processCycle;
	int monitorCycle;
	int hardDriveCycle;
	int printerCycle;
	int keyboardCycle;
	char log[50];
	char logfPath[50];
};

struct metaData
{
	char component;
	char description[50];
	int cycles;
	int numOfProcesses;
};
struct processControl
{
	char state;
	struct metaData* progCounter;
	struct timeval start, stop;
	double totalTime;
	pthread_t thread;	
};
struct pointerQuantum
{
	int pointerInFile[10];
	int totalQuantum[20];
};


/////////Function Declaration/////////////
//////////////////////////////////////////

void decision( struct configFile config,struct metaData * meta );
void fifoSimulator( struct configFile config, struct metaData* meta );
void sjfSimulator( struct configFile config, struct metaData* meta );
void confileInput( FILE *file, struct configFile* first );
struct metaData* metaInput( struct configFile *config );
void logger( struct processControl block, char* logInfo,  FILE * file, struct configFile config );
void* sleeper( void* time );
void pcbState( struct processControl * setState, char state, struct metaData * meta );
void timer(struct processControl block, int processNum,char* Info);
void timer1(struct processControl block,char* Info);
struct pointerQuantum countQuantum(struct configFile config, struct metaData* meta);
void srtfSimulator( struct configFile config, struct metaData* meta );
void singlePSimu(struct configFile config, struct metaData* meta,int pointer,int pointer1,int proNum,int flag);
int minimum(int[],int);

/////////////       MAIN      /////////////
///////////////////////////////////////////
int main( int argc, char* argv[2])
{
 struct configFile config;
 struct metaData * metaDataInfo;
 FILE * file;
 if( (file = fopen( argv[1], "r" ))!= NULL)
	{
         confileInput( file, &config );
	 metaDataInfo = metaInput( &config );
	 fclose(file);
	 decision( config,metaDataInfo );	 
	}
 else
	{
         printf( "%s", "Error:File could not be found" );
	}
 return 0;
}


////////   Deciding which simulator to use  ///////
///////////////////////////////////////////////////
////////   As mentioned in config file   //////////
void decision( struct configFile config,struct metaData * meta )
{
	if(strcmp(config.approach,"FIFO") == 0)
	{
	printf("Using FIFO Algorithm\n");
	fifoSimulator( config, meta );
	}
	else if(strcmp(config.approach,"SJF") == 0)
	{
	printf("Using SJF Algorithm\n");
	sjfSimulator(config, meta );
	}
	else if(strcmp(config.approach,"SRTF") == 0)
	{
	printf("Using SRTF Algorithm\n");
	srtfSimulator(config, meta );
	}
}

////////// Reading Config File  //////////
//////////////////////////////////////////
void confileInput( FILE *file, struct configFile* theConfig )
{
  // String for garbage read
  char string[stringMax];

  // take in first line of config file (garbage)
  fscanf( file, "%s %s %s %s", string, string, string, string );

  // take in version number
  fscanf( file, "%s %f \n", string, &(*theConfig).version );

  // metadata file path	
  fscanf( file, "%s %s %s", string, string, (*theConfig).fPath );

  // take in approach to use
  fscanf( file, "%s %s %s", string, string, (*theConfig).approach );
  printf( "Approach Mentioned in config file:%s\n",(*theConfig).approach);

  // Process cycle time
  fscanf( file, "%s %s %s %s %d", string, string ,string ,string ,&(*theConfig).processCycle );

  //Monitor cycle time
  fscanf( file, "%s %s %s %s %d", string, string ,string ,string ,&(*theConfig).monitorCycle );

  //Hard drive cycle time
  fscanf( file, "%s %s %s %s %s %d", string, string , string,string ,string ,&(*theConfig).hardDriveCycle );

  //Printer cycle time;
  fscanf( file, "%s %s %s %s %d", string, string ,string ,string ,&(*theConfig).printerCycle );

  //Keyboard cycle time
  fscanf( file, "%s %s %s %s %d", string, string ,string ,string ,&(*theConfig).keyboardCycle );

  //where to log
  fscanf( file, "%s %s %s \n", string, string, (*theConfig).log );

  //log file path
  fscanf( file, "%s \n", (*theConfig).logfPath );	 							
}



///////////// Reading MetaFile  ////////////
////////////////////////////////////////////
struct metaData* metaInput( struct configFile *config )
{
  char string[stringMax];
  char temp;
  char temp1;
  int processess=0;
  int index = 0;
  int index1 = 0;
  FILE * file;
  if( (file = fopen( config->fPath, "r" ))!= NULL)
  {
     fgets( string, stringMax, file );
     while(temp!=EOF)
     {
        temp = fgetc( file );
        if( temp == ')' )
        {
	  sizeOfData++;//counting number of various actions
        }
     }
     rewind( file );
     struct metaData* mData = (struct metaData*)malloc(sizeof(int)*(sizeOfData+1));//Allocating memory	
     fgets( string, stringMax, file );
     while( index < sizeOfData)
     {	
	temp1=fgetc( file );
	if(temp1=='\n')//Escaping next line
	{
	temp1=fgetc( file );
	}
        mData[index].component = temp1; //Storing Component
        temp1 = fgetc( file );
        while( temp1 != ')' )
        {
	   if( temp1 != '(' )
	   {
	      mData[index].description[index1] = temp1;  //Storing Discription     
	      if(strcmp(mData[index].description,"start")== 0)//Counting number of processes
	      {
		processess++;
	      }
	      index1++;			
           }
	   temp1 = fgetc( file );
	}
	index1 = 0;
	fscanf( file, "%d %c", &(mData[index]).cycles, &temp1 );
        
	temp1 = fgetc( file );			
	index++;
     }
     mData[0].numOfProcesses=processess-1;  //Saving Number of processes/program in metafile
     return mData;		
  }
  else
  {
     printf( "%s", "Error:File could not be found" );
  }
  fclose(file);	
  return 0;
}



///////////   Simulator for First in First Out ///////////
//////////////////////////////////////////////////////////
void fifoSimulator( struct configFile config, struct metaData* meta )
{
 int dataIndex=0;
 unsigned long quantum=0;
 char Info[100];
 int counter =0;
 struct processControl block;
 //Checking and processing each action
 for( dataIndex = 0; dataIndex < sizeOfData; dataIndex++ )
  {
   switch( meta[dataIndex].component )
   {
    case 'S':
    	if( strcmp( meta[dataIndex].description, "start" )==0 )
    	{
     	 strcpy( Info, "Simulator Program starting" );
     	 pcbState( &block, ENTER, meta ); 
     	 gettimeofday( &block.start, NULL );	
     	 timer1(block,Info);
    	}
    	else if( strcmp( meta[dataIndex].description, "end" )==0 )
    	{
     	 strcpy( Info, "Simulator Program Ending" );
     	 pcbState( &block, EXIT, meta );	
     	 timer1(block,Info);
    	}
 	break;

    case 'A':
    	if( strcmp( meta[dataIndex].description, "start" )==0 )
	{
	 strcpy( Info, "OS:Starting Process" );
	 timer1(block,Info);
	 pcbState( &block, RUNNING, meta );
	 block.progCounter = meta;
	}
	else if( strcmp( meta[dataIndex].description, "end" )==0 )
	{
	 strcpy( Info, "OS:Removing Process");
	 timer1(block,Info);
	 pcbState( &block, EXIT, meta );
	 counter++;
	}
	break;
    case 'P':
	if( strcmp( meta[dataIndex].description, "run" )==0 )
	{
	 quantum = ((meta[dataIndex].cycles )*(config.processCycle));
	 strcpy( Info, "Start process action" );
	 timer( block, counter,Info);
	 pthread_create( &block.thread, NULL, &sleeper, (void*) &quantum );//Creating thread for process action
	 pthread_join( block.thread, NULL );
	 strcpy( Info, "End process action");
	 timer( block, counter,Info);			
	}
	break;
    case 'I':
	if( strcmp( meta[dataIndex].description, "hard drive" )==0 )
	{
	 quantum = ((meta[dataIndex].cycles )*(config.hardDriveCycle));
	 strcpy( Info, "Start hard drive input" );
	 timer( block, counter,Info);
	 pthread_create( &block.thread, NULL, &sleeper, (void*) &quantum );//Creating thread for hard drive input
	 pthread_join( block.thread, NULL );
	 strcpy( Info, "End hard drive input" );
	 timer( block, counter,Info);				
	}
	else if( strcmp( meta[dataIndex].description, "keyboard" )==0 )
	{					
	 quantum = ((meta[dataIndex].cycles )*(config.keyboardCycle));
	 strcpy( Info, "Start keyboard input" );
	 timer( block, counter,Info);
	 pthread_create( &block.thread, NULL, &sleeper, (void*) &quantum );//Creating thread for keyboard input
	 pthread_join( block.thread, NULL );
	 strcpy( Info, "End keyboard input" );
	 timer( block, counter,Info);
	}
	break;
    case 'O':
	if( strcmp( meta[dataIndex].description, "hard drive" )==0 )
	{
	 quantum = ((meta[dataIndex].cycles )*(config.hardDriveCycle));
	 strcpy( Info, "Start hard drive output" );
	 timer( block, counter,Info);
	 pthread_create( &block.thread, NULL, &sleeper, (void*) &quantum );//Creating thread for hard drive output
	 pthread_join( block.thread, NULL );
	 strcpy( Info, "End hard drive output" );
	 timer( block, counter,Info);
	}
	else if( strcmp( meta[dataIndex].description, "monitor" )==0 )
	{
	 quantum = ((meta[dataIndex].cycles )*(config.monitorCycle));
	 strcpy( Info, "Start monitor output" );
	 timer( block, counter,Info);
	 pthread_create( &block.thread, NULL, &sleeper, (void*) &quantum );//Creating thread for monitor output
	 pthread_join( block.thread, NULL );
	 strcpy( Info, "End monitor output" );
	 timer( block, counter,Info);
	}
	else if( strcmp( meta[dataIndex].description, "printer" )==0 )
	{
	 quantum = ((meta[dataIndex].cycles)*config.printerCycle);
	 strcpy( Info, "Start printer output" );
	 timer( block, counter,Info);
	 pthread_create( &block.thread, NULL, &sleeper, (void*) &quantum );//Creating thread for printer 
	 pthread_join( block.thread, NULL );
	 strcpy( Info, "End printer output" );
	 timer( block, counter,Info);
	}
	break;
   }
  }
} 



/////////// Supporting Functions  ///////////
/////////////////////////////////////////////
//the following two function are almost same the difference is parameters as don't require process number with application start and end// 
void timer(struct processControl block, int processNum,char* Info)
{
   gettimeofday( &block.stop, NULL );
   block.totalTime = difftime( (block.stop).tv_sec, (block.start).tv_sec );
   block.totalTime += ((double)( (block.stop).tv_usec - (block.start).tv_usec )/1000000);
   printf("%.6f - Process:%d %s\n",block.totalTime,processNum,Info);//Printing time elapsed
}

void timer1(struct processControl block,char* Info)
{
   gettimeofday( &block.stop, NULL );
   block.totalTime = difftime( (block.stop).tv_sec, (block.start).tv_sec );
   block.totalTime += ((double)( (block.stop).tv_usec - (block.start).tv_usec )/1000000);
   printf( "%.6f - %s \n", block.totalTime, Info );	//Printing time elapsed for various process and action
}

void* sleeper( void* time )
{
   unsigned long* micSecs = (unsigned long*)(time);
   usleep( *micSecs );
   return NULL;
}

void pcbState( struct processControl* setState, char state, struct metaData* info )
{
  if( state == 'E' )
  {
    setState->state = ENTER;
    setState->progCounter = NULL;
  }
  else if( state == 'Y' )
  {
    setState->state = READY;
    setState->progCounter = info;
  }
  else if( state == 'R' )
  {
    setState->state = RUNNING;
    setState->progCounter = info;
  }
  else if( state == 'X' )
  {
    setState->state = EXIT;
  }
}

////////////// Calculate total quantum of each process  //////////////
//////////////////////////////////////////////////////////////////////
struct pointerQuantum countQuantum(struct configFile config, struct metaData* meta)
{
  struct pointerQuantum indx;
  int index;
  int index1=0;
  int tempStore=0;
  int tempStore1=0;
  indx.pointerInFile[0]=0;
  for(index=0; index < meta[0].numOfProcesses;index++)
  { 
	
    for(index1=0;index1 < sizeOfData;index1++)
    {   
	if( meta[index1].component =='A' && strcmp(meta[index1].description, "end")==0 )
	{
	   indx.totalQuantum[index]=tempStore;//When end discription occurs we store the value which contain quantum of one process
           tempStore=0;
	   indx.pointerInFile[index+1]=index1+1;//Storing pointer value so as we can traverse each block in custom sequence if needed
	   index++;
	}
	else if( meta[index1].component =='I' && strcmp(meta[index1].description, "keyboard")==0 )
	{
           tempStore1=0;
           tempStore1=(meta[index1].cycles )*(config.keyboardCycle); //Calculating quantum of each action
           tempStore=tempStore+tempStore1;
	}
        else if( meta[index1].component =='O' && strcmp(meta[index1].description, "keyboard")==0 )
	{
           tempStore1=0;
           tempStore1=(meta[index1].cycles )*(config.keyboardCycle);//Calculating quantum of each action
           tempStore=tempStore+tempStore1;
	}
        else if( meta[index1].component =='I' && strcmp(meta[index1].description, "hard drive")==0 )
	{
           tempStore1=0;
           tempStore1=(meta[index1].cycles )*(config.hardDriveCycle);//Calculating quantum of each action
           tempStore=tempStore+tempStore1;
	}
        else if( meta[index1].component =='O' && strcmp(meta[index1].description, "hard drive")==0 )
	{
           tempStore1=0;
           tempStore1=(meta[index1].cycles )*(config.hardDriveCycle); //Calculating quantum of each action
           tempStore=tempStore+tempStore1;
	}
        else if( meta[index1].component =='O' && strcmp(meta[index1].description, "monitor")==0 )
	{
           tempStore1=0;
           tempStore1=(meta[index1].cycles )*(config.monitorCycle);//Calculating quantum of each action
           tempStore=tempStore+tempStore1;
	}
        else if( meta[index1].component =='P' && strcmp(meta[index1].description, "run")==0 )
	{
           tempStore1=0;
           tempStore1=(meta[index1].cycles )*(config.processCycle);//Calculating quantum of each action
           tempStore=tempStore+tempStore1;
	}    
    }
  }
  return indx;  //returning the indx of type processQuantum containing quantums of processes and pointer to each block
}


//////////   Simulator for Shortest job first  ////////////
///////////////////////////////////////////////////////////
void sjfSimulator( struct configFile config, struct metaData* meta )
{	
   char Info[100];
   struct processControl block;
   struct pointerQuantum nindx;
   printf("Number of processes:%d\n",meta[0].numOfProcesses); //Printing number of processes
   nindx=countQuantum(config,meta);
   int index=0;
   int sortedArray[100];
   //storing quantum array and to another array which we will sort////
   for(index=0;index<meta[0].numOfProcesses;index++)
   {
	sortedArray[index]=nindx.totalQuantum[index];
   }
   //Sorting array
   int index1=0;
   int index2=0;
   int temp;
   for (index1 = 1; index1 < meta[0].numOfProcesses; index1++) 
	{
        temp = sortedArray[index1];
        for (index2 = index1; index2 > 0 && temp < sortedArray[index2 - 1]; index2--) 
	{
            sortedArray[index2] = sortedArray[index2 - 1];
        }
        sortedArray[index2] = temp;
	}
   

   //Compare sorted array with quatum array and CallSimulator for each block accordingly
   strcpy( Info, "Simulator Program starting" );
   pcbState( &block, ENTER, meta ); 
   gettimeofday( &block.start, NULL );	
   timer1(block,Info);
   for(index1=0;index1<meta[0].numOfProcesses; index1++)
   {	
	for(index2 = 0; index2 < meta[0].numOfProcesses; index2++)
	{
		if(sortedArray[index1]==nindx.totalQuantum[index2])
		{				
			singlePSimu(config,meta,nindx.pointerInFile[index2],nindx.pointerInFile[index2+1],index2, index1);//calling simulator with next shortest time
		}
	}
   }
   strcpy( Info, "Simulator Program Ending" );
   pcbState( &block, EXIT, meta );	
   timer1(block,Info);   
}



///////////Single program/block/process simulator/////////////
//////////////////////////////////////////////////////////////
void singlePSimu(struct configFile config, struct metaData* meta,int pointer,int pointer1,int proNum, int flag)
{
  int dataIndex;
  unsigned long quantum=0;
  char Info[100];
  int counter =proNum;
  struct processControl block;
  if(flag==0)//Flag is used to know if this is the first time simulator is processing a program so to start clock appropriately
  {
    pcbState( &block, ENTER, meta ); 
    gettimeofday( &block.start, NULL );
  }	
  //Checking and processing each action
  for( dataIndex = pointer; dataIndex < pointer1; dataIndex++ )
  {
    switch( meta[dataIndex].component )
    {
      case 'A':
    	if( strcmp( meta[dataIndex].description, "start" )==0 )
	{
	  strcpy( Info, "OS:Starting Process" );
	  timer1(block,Info);
	  pcbState( &block, RUNNING, meta );
	  block.progCounter = meta;
	}
	else if( strcmp( meta[dataIndex].description, "end" )==0 )
	{
	  strcpy( Info, "OS:Removing Process");
	  timer1(block,Info);
	  pcbState( &block, EXIT, meta );
	  counter++;
	}
	break;
      case 'P':
	if( strcmp( meta[dataIndex].description, "run" )==0 )
	{
	  quantum = ((meta[dataIndex].cycles )*(config.processCycle));
	  strcpy( Info, "Start process action" );
	  timer( block, counter,Info);
	  pthread_create( &block.thread, NULL, &sleeper, (void*) &quantum );
	  pthread_join( block.thread, NULL );
	  strcpy( Info, "End process action");
 	  timer( block, counter,Info);			
	}
	break;
      case 'I':
	if( strcmp( meta[dataIndex].description, "hard drive" )==0 )
	{
	  quantum = ((meta[dataIndex].cycles )*(config.hardDriveCycle));
	  strcpy( Info, "Start hard drive input" );
	  timer( block, counter,Info);
	  pthread_create( &block.thread, NULL, &sleeper, (void*) &quantum );
	  pthread_join( block.thread, NULL );
	  strcpy( Info, "End hard drive input" );
	  timer( block, counter,Info);				
	}
	else if( strcmp( meta[dataIndex].description, "keyboard" )==0 )
	{					
	  quantum = ((meta[dataIndex].cycles )*(config.keyboardCycle));
	  strcpy( Info, "Start keyboard input" );
	  timer( block, counter,Info);
	  pthread_create( &block.thread, NULL, &sleeper, (void*) &quantum );
	  pthread_join( block.thread, NULL );
	  strcpy( Info, "End keyboard input" );
	  timer( block, counter,Info);
	}
	break;
      case 'O':
	if( strcmp( meta[dataIndex].description, "hard drive" )==0 )
	{
	  quantum = ((meta[dataIndex].cycles )*(config.hardDriveCycle));
	  strcpy( Info, "Start hard drive output" );
	  timer( block, counter,Info);
	  pthread_create( &block.thread, NULL, &sleeper, (void*) &quantum );
	  pthread_join( block.thread, NULL );
	  strcpy( Info, "End hard drive output" );
	  timer( block, counter,Info);
	}
	else if( strcmp( meta[dataIndex].description, "monitor" )==0 )
	{
	  quantum = ((meta[dataIndex].cycles )*(config.monitorCycle));
	  strcpy( Info, "Start monitor output" );
	  timer( block, counter,Info);
	  pthread_create( &block.thread, NULL, &sleeper, (void*) &quantum );
          pthread_join( block.thread, NULL );
	  strcpy( Info, "End monitor output" );
	  timer( block, counter,Info);
	}
	else if( strcmp( meta[dataIndex].description, "printer" )==0 )
	{
	  quantum = ((meta[dataIndex].cycles)*config.printerCycle);
	  strcpy( Info, "Start printer output" );
	  timer( block, counter,Info);
	  pthread_create( &block.thread, NULL, &sleeper, (void*) &quantum );
	  pthread_join( block.thread, NULL );
	  strcpy( Info, "End printer output" );
	  timer( block, counter,Info);
	}
	break;
   }
  }	
}



//////////////////////////   Shortest Remaining Time First    ///////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
void srtfSimulator( struct configFile config, struct metaData* meta )
{
   char Info[100];
   struct processControl block;
   struct pointerQuantum nindx;
   int proNum;
   printf("Number of processes:%d\n",meta[0].numOfProcesses);
   nindx=countQuantum(config,meta);
   int index=0;
   int array[100];
   //Storing quantum array and to another array on which we can perform various tasks without hurting original data////
   for(index=0;index<meta[0].numOfProcesses;index++)
   {
	array[index]=nindx.totalQuantum[index];
   }
   
   for(index = 0 ; index < meta[0].numOfProcesses ; index++)
   {
     printf("Quantum of Process:%d - %d\n",index,array[index]);
   }
   
   //Compare sorted array with quatum array and CallSimulator for each block accordingly
   strcpy( Info, "Simulator Program starting" );
   pcbState( &block, ENTER, meta ); 
   gettimeofday( &block.start, NULL );	
   timer1(block,Info);
   for(index=0;index<meta[0].numOfProcesses;index++)
   {
      proNum=minimum(array,meta[0].numOfProcesses);//finding process number which have minimum quantum, every time after one job is done
      singlePSimu(config,meta,nindx.pointerInFile[proNum],nindx.pointerInFile[proNum+1],proNum, index);//processing next minimum job
      int index1;
      for ( index1 = 0 ; index1 < meta[0].numOfProcesses ; index1++ ) 
      {
        if ( proNum==index1 ) 
        {
             array[proNum]=9999999;//changing the quantum size so that simulator will know this particular process is processed already
           	         
        }
      }   
   }
   strcpy( Info, "Simulator Program Ending" );
   pcbState( &block, EXIT, meta );	
   timer1(block,Info);   
}

///////////Function to calculate minimum and returns index for that minimum value in array//////////
int minimum(int array[],int totalProcess)
{
   int min=array[0];
   int proNum=0;
   int index;
   for ( index = 0 ; index < totalProcess ; index++ ) 
   {
        if ( array[index] < min ) 
        {
           min = array[index];
           proNum = index;
        }
   }
   return proNum; 	
}
