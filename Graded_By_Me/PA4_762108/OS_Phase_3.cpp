// header files
#include <iostream>
#include <fstream>
#include <thread> 
#include <iomanip>
#include <list>
#include <queue> 

using namespace std;

// global constants
int MAX_PROGRAM_INSTRUCTIONS = 100; // e.g I(monitor)5; is one instruction

// helper classes/structs

// struct used in PCB to hold values of meta-data e.g I(monitor)5;
struct MetaDataStruct 
        {
         char SAPIO;
         string descriptor;
         int runTime;
        };

// class to hold data of config file
class ConfigClass
        {
         public: 

         // vars
         string MetaDataFilePath;
         string Scheduling;
         int Quantum;
         int ProcessorCycleTime;
         int MonitorDisplayTime;
         int HardDriveCycleTime;
         int PrinterCycleTime;
         int KeyboardCycleTime;
         string Log;
         string LogPath;

         // functions
         bool loadConfigData(char*);
         };

// PCB class to hold each programs data
class PCB
        {
         public:
         // vars
         string state;                  // either Enter, Ready, Running, Blocked, or Exit
         int programCounter;            // where we are in the program execution (operation) offset
         int cyclesRanThisQuantum;      // how many cycles have been completed so far this quantom
         int cyclesRemaining;           // if operation was interrupted with cycles left
         MetaDataStruct* data;          // struct holding list of all program operations from meta data
         int numberOfOperations;        // length of whole program
         int id;                        // unique id for program

         // functions
         int loadProgram(string, int);  // load specified program from meta data file
         PCB();                         // constructor
         };


// probably the trickest part of this program, this is simply a custom compare class used to order the 
// priority queues (ready que) based on if scheduling type is fifo-p or rr or srtf-p
class Comparator_Selector
        {
         // vars
         string type;                                   // either fifo-p or rr or srtf-p

         // functions
         public:
         int calcRemainingCycles( PCB pcb );            // used for srtf-p
         bool operator() (PCB lhs, PCB rhs);            // to actually compare
         Comparator_Selector (string value);            // constructor         
        };



// main function prototypes

// run the programs associated with the passed in pcb array
void runPrograms( PCB*, int,  ConfigClass , clock_t   );

// simulates process doing operations by waiting a set amount of time, exits if interrupt triggered
int wait(int cycles, int msPerCycle, PCB* pcb, ConfigClass configData, queue<PCB>* interruptQueue,  bool* );

// runs the next instruction ( e.g I(monitor)5; is one instruction) in the program
string executeNextOperation(PCB* programPCB, ConfigClass configData, clock_t start, queue<PCB>*);

// used to execute each unique run opeation in its own unique thead (yes run operations are in their own thread too)
void uniqueThreadExecution( string print, clock_t startTime, ConfigClass configData, PCB* pcb, int, int, string*, queue<PCB>* interruptQueue);

// print/write/both data based on config input
void printOrWriteData ( double, string, ConfigClass);

// write a string to a file, helper for print or write function
void writeToFile( string, string);

// helper function to iterate over all pcbs and call approprate function to load
int loadPCBs( string, PCB* );

// for I/O operations, unique thread
void IOThreadExecution(PCB programPCB, queue<PCB>* interruptService, int waitTime );


// main program
int main(int argCount, char** args)
        {
         // vars
         ConfigClass config;
         clock_t start;
         ofstream outfile;
         PCB pcbArray [ 100 ];
         
         char* configPath = args[1];    
         bool validConfigFile;                  // false if invalid config file name
         int numberOfPrograms = -1;             // -1 if invalid meta data file name
         double currentElapsedTime = 0;

         // break if not enough command line args
         if (argCount !=2 )
	        {
	         cout << "Please run with 1 command line paramater (to config file location aka ./simulator config.txt)" << endl;
	         return 0;
	        }

         // load our config object with the file path of the passed in argument
         validConfigFile = config.loadConfigData(configPath);

         // break if no config file found
         if (!validConfigFile )
	        {
	         cout << "Config file not found ABORT" << endl;
	         return 0;
	        }

         // delete any old log files
         remove( config.LogPath.c_str() );

         // start running the clock
         start = clock();

         // print that we are starting the simulation
         currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
         printOrWriteData ( currentElapsedTime, " Simulator program starting", config);

         // print that processes will now be prepared
         currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
         printOrWriteData ( currentElapsedTime, " OS: preparing all processes", config);

         // load all pcbs into an array of pcbs         
         numberOfPrograms = loadPCBs(config.MetaDataFilePath, pcbArray);

         // break if no meta-data file found
         if (numberOfPrograms == -1)
	        {
	         cout << "Meta-Data file not found ABORT" << endl;
	         return 0;
	        }

         // run through our program executing each meta data operation and create threads for i/o operations
         runPrograms(pcbArray, numberOfPrograms, config, start);

         // print that the simulation is now over
         currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
         printOrWriteData ( currentElapsedTime, " Simulator program ending", config);

         return 0;
        }

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////// MAIN PROGRAM FUNCTIONS //////////////////////////////
////////////////////////////////////////////////////////////////////////////
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv/
int loadPCBs( string fileName, PCB* array)
{
        // vars
        int result = 1;
        int counter = 0;
        
        // while there is another pcb to get continue
        while ( result == 1 )
                {
                 // update our result while loading the next pcb in our array of pbcs
                 result = array[counter].loadProgram(fileName, counter+1);
                 counter++;
                }

        // return if there was an error reading meta data file
        if ( result == -1 )
                {
                 return -1;
                }

        return counter-1;
}

// put program in enter state and wait for program to finish executing, interrupt, or quantum time reached
void runPrograms(PCB* arrayOfPBCs, int numberOfPCBs, ConfigClass configData, clock_t start )
        {
         // vars
         string processState = "Enter";
         double currentElapsedTime = 0;
         int currentProgram = 0;
         int numberBlockedPrograms = 0;
         PCB tmp;
         bool systemIsIdol = true;

         // normal queue for programs that need to be serviced ( I/O action complete )
         queue<PCB> interruptService;

         // load our custom comparator class for fifo-p or rr or srtf-p so we know how to order our priority queues, 
         // we are just initializing class here by loading the constructor with the scheduling type
         Comparator_Selector RR_or_FIFO_Comparator (configData.Scheduling);

         // create our readyQueue (which is a priorty queue) using the comparator class initiallized above
         // then passing the defined class to it in order to save the constructor data
         priority_queue<PCB, vector<PCB>, Comparator_Selector> readyQueue (RR_or_FIFO_Comparator);


         // init all programs
         while ( currentProgram < numberOfPCBs )
                {
                 // put program into enter state 
                 arrayOfPBCs[currentProgram].state = "Enter";
                 // push program into queue
                 readyQueue.push ( arrayOfPBCs[currentProgram] );
                 // udpate index
                 currentProgram++;
                }

         // reset counter
         currentProgram = 0;


         // while there is a program to run in ready queue or we are waiting for a process to return from I/O
         while ( readyQueue.size() > 0 || numberBlockedPrograms > 0)
                {

                 // if there is a process to run, so system not idle, continue to process it
                 if ( readyQueue.size() > 0 )
                        {
                         // reset idol flag
                         systemIsIdol = false;

                         // get the program that we should be running, ( will always be program at front of queue since we have a custom comparator )
                         tmp = readyQueue.top();

                         // prime loop
                         processState = tmp.state;

                         // if program valid continue to process
                         if ( processState != "Exit" )
                                {
                                 // print that starting the process
                                 currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
                                 printOrWriteData ( currentElapsedTime, " OS: selecting next process" , configData);

                                 // print that starting the process
                                 currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
                                 printOrWriteData ( currentElapsedTime, " OS: starting process "+to_string(tmp.id), configData);
                                }

                         // run each program while it reports back that everything is still good  
                         while (processState != "Exit" && processState != "Quantum Reached" && processState != "Blocked" && processState != "Interrupt")
                                {
                                 processState = executeNextOperation(&tmp, configData, start, &interruptService);
                                }

                         // if returning that the quantum time ended before finishing operation
                         if ( processState == "Quantum Reached" )
                                {

                                 currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
                                 printOrWriteData ( currentElapsedTime, " OS: Quantum time Interrupt ", configData);

                                 currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
                                 printOrWriteData ( currentElapsedTime, " OS: process "+to_string(tmp.id)+ " to ready Queue" , configData);

                                 // reset state to ready
                                 tmp.state = "Ready";

                                 // remove from front of queue
                                 readyQueue.pop();

                                 // put back in queue ( if RR comparator will place at end, if FIFO-P comparitor
                                 // will place at aproprate location, see comparitor class for details )
                                 readyQueue.push ( tmp );
                                }

                        // if program ran I/O and put it into blocked
                        else if ( processState == "Blocked" )
                                {
                                 currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
                                 printOrWriteData ( currentElapsedTime, " OS: process "+to_string(tmp.id)+" Blocked", configData);

                                 // remove from queue entirely
                                 readyQueue.pop();

                                 numberBlockedPrograms++;
                                }

                        // if process was intrupted by other I/O proceess ending, re-evaluate both 
                        else if ( processState == "Interrupt" )
                                {
                                 currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
                                 printOrWriteData ( currentElapsedTime, " OS: process "+to_string(tmp.id)+" Blocked from interrupt returning", configData);

                                 // put current process back into the ready queue, since it was interrupted
                                 tmp.state = "Ready";
                                 readyQueue.pop();
                                 readyQueue.push ( tmp );

                                 // put interruptong I/O process to queue
                                 tmp = interruptService.front();                               
                                 tmp.state = "Ready";
                                 readyQueue.push( tmp );

                                 // indicate I/O has been serviced
                                 interruptService.pop();
                                 numberBlockedPrograms--;
                                }

                         // if process completely done
                         else if ( processState == "Exit" )
                                {
                                 currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
                                 printOrWriteData ( currentElapsedTime, " OS: removing process "+to_string(tmp.id), configData);

                                 // remove from queue entirely
                                 readyQueue.pop();
                                }

                         // catch any errors
                         else
                                {
                                 cout<< "Invalid process state, ERROR" << endl;
                                }
                        }


                // if no processes to run, but waiting on I/O, continually check if int service queue has any data
                else 
                        {
                         // if I/O done put io into ready queue
                         if ( interruptService.size() > 0 )
                                {
                                 tmp = interruptService.front();
                               
                                 // if I/O was not the last operation put on ready queue
                                 if ( tmp.state != "Exit" )
                                        {
                                         tmp.state = "Ready";
                                         readyQueue.push( tmp );
                                        }
                                 // otherwise remove
                                 else
                                        {
                                         currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
                                         printOrWriteData ( currentElapsedTime, " OS: removing process "+to_string(tmp.id), configData);
                                        }

                                 numberBlockedPrograms--;
                                 interruptService.pop();
                                }

                         // print that system is idle so user knows what is going on
                         else if ( systemIsIdol == false )
                                {
                                 currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
                                 printOrWriteData ( currentElapsedTime, " OS: System Idol, waiting for I/O operation(s) to complete", configData);
                                 systemIsIdol = true; // only want to print once
                                }

                        }
               }


           // inform user everything done if saving to file only                
           if ( configData.Log == "Log to File")
                {
                 cout << "Output saved to file" << endl;
                }
        }



// function to run one operation (monitor, hard drive, etc), and update program counter and program state
string executeNextOperation(PCB* programPCB, ConfigClass configData, clock_t start, queue<PCB>* interruptQueue)
        {
         // vars
         int currentInstruction = programPCB->programCounter;
         int cycleOffset =  programPCB->cyclesRemaining;
         double currentElapsedTime = 0;
         int cycles;
         int msPerCycle;

         string outputText;
         string result;

         // break out if program not initialized in running/enter state
         if ( programPCB->state == "Exit" )
	        {
	         return "Exit";
	        }

         // if program just started ( in Entered state) setup program counter and update state
         if ( programPCB->state == "Enter" )
	        {
         	 programPCB->programCounter = 0; 
                 programPCB->cyclesRemaining = 0;
                 programPCB->cyclesRanThisQuantum = 0;
	         programPCB->state = "Running";
	        }

        // if in ready state put into running state 
         if ( programPCB->state == "Ready" )
	        {
	         programPCB->state = "Running";
                 programPCB->cyclesRanThisQuantum = 0;
	        }


         // break if at end of instructions and move to exit state
         if ( currentInstruction >= programPCB->numberOfOperations )
	        {
	         programPCB->state = "Exit";
                 // show we have finished our job (in this case just waiting)
                 currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
                 printOrWriteData ( currentElapsedTime, " Process "+ to_string(programPCB->id) +": complete ", configData);
	         return "Exit";
	        }

         // break if quantum reached
         else if ( programPCB->cyclesRanThisQuantum >= configData.Quantum )
	        {
                 programPCB->cyclesRemaining = 0;
	         return "Quantum Reached";
	        }

         // otherwise in running state so continue to execute operation
         else
	        {
                  // get number of cycles to run for
                 if ( cycleOffset == 0 )
                        {
                         cycles = programPCB->data[currentInstruction].runTime;
                        }
                 else
                        {
                         cycles = cycleOffset;
                        }

                  // if a run/prcoess command ( I am running 'run' operation in unique thread, still check every cycle dont worry)
                 if ( programPCB->data[currentInstruction].SAPIO == 'P')
                        {
                         msPerCycle = configData.ProcessorCycleTime;
                         outputText = "processing action";

                         // do the operation work in its unique thread
	                 thread runOperation(uniqueThreadExecution, outputText, start, configData, programPCB, cycles, msPerCycle, &result, interruptQueue);
	                 runOperation.join();
                        }

                // otherwise will be I/O command, run in unique thread 
                else
                        {       
                         // get times for each I/O operation
                         if ( programPCB->data[currentInstruction].descriptor == "monitor")
                                {
                                 msPerCycle = configData.MonitorDisplayTime;
                                }
                         else if ( programPCB->data[currentInstruction].descriptor == "hard drive")
                                {
                                 msPerCycle = configData.HardDriveCycleTime;
                                }
                         else if ( programPCB->data[currentInstruction].descriptor == "printer")
                                {
                                 msPerCycle = configData.PrinterCycleTime;
                                }
                         else if ( programPCB->data[currentInstruction].descriptor == "keyboard")
                                {
                                 msPerCycle = configData.KeyboardCycleTime;
                                }
                         // there was some error reading in the meta data, display this error to the user
                         else
                                {
                                 currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
                 		 printOrWriteData ( currentElapsedTime, " Error: Meta data format error or more than one program, ABORT", configData);
                                 return "Exit";
                                }

                         // update the text for i/o operations
                         outputText = programPCB->data[currentInstruction].descriptor;

                         // add if input or output operation to text
                         if ( programPCB->data[currentInstruction].SAPIO == 'I')
	                        {
	                         outputText+=" input";
	                        }
                         else
	                        {
	                          outputText+=" output";
	                        }

                         // show this process will now be waiting on I/O to complete
                         currentElapsedTime = (double)(clock() - start)/(CLOCKS_PER_SEC);
                         printOrWriteData ( currentElapsedTime, " Process "+ to_string(programPCB->id) +": start I/O operation " , configData);

                         // run io thread
                         thread runIO(IOThreadExecution, *programPCB, interruptQueue, msPerCycle*cycles*.001);
                         // detach so thread will still run even though this funtion is returning false
                         runIO.detach();

	                 return "Blocked";
                        }

                // return the value returned by the thread (passed by ref)
	         return result;
                }
        }

void IOThreadExecution(PCB programPCB, queue<PCB>* interruptService, int waitTime )
        {

         int start = clock();

         // wait set amount of time
         while (clock() - start < CLOCKS_PER_SEC * (double) (waitTime))
                {

                }

         // show we are done with this io operation
         programPCB.programCounter++;

         // indicate if done beforehand
         if ( programPCB.programCounter >= programPCB.numberOfOperations )
                {
                 programPCB.state = "Exit";
                }

        // push this pcb to the interrupt service queue indicating the io process is done and should be serivced
        (*interruptService).push(programPCB);

        }

// thread function to display time, wait, and display end time
void uniqueThreadExecution( string print, clock_t startTime, ConfigClass configData, PCB* pcb, int cycles, int msToWait, string* returnVal, queue<PCB>* interruptQueue)
        {
         double currentElapsedTime = 0;
         int result = 0;
         bool wasStoppedByQuantum = true;

         // log we have started the instruction 
         currentElapsedTime = (double)(clock() - startTime)/(CLOCKS_PER_SEC);
         printOrWriteData ( currentElapsedTime, " Process " + to_string(pcb->id) + ": start " + print, configData);

         // waiting for set time, result is how many cycles we left off with before being interrupted
         result = wait( cycles, msToWait, pcb, configData, interruptQueue, &wasStoppedByQuantum );

        // if program didnt finish because of quantum return quantum reached
        if ( result > 0 && wasStoppedByQuantum)
                {
                 *returnVal = "Quantum Reached";
                  pcb->cyclesRemaining = result;                 
                }

        // if program didnt finish because of a differnt program I/O must be serviced
        else if ( result > 0 && !wasStoppedByQuantum)
                {
                 *returnVal = "Interrupt";
                  pcb->cyclesRemaining = result; 
                }

        // if have finished this operation successfully
        else if ( result == 0 )
                {
                 *returnVal = "Running";

                 // show we have completed this instruction and now moving to next one
                 pcb->programCounter++;
                 pcb->cyclesRemaining = 0;

                 // show we have finished our job (in this case just waiting)
                 currentElapsedTime = (double)(clock() - startTime)/(CLOCKS_PER_SEC);
                 printOrWriteData ( currentElapsedTime, " Process "+ to_string(pcb->id) +": end " + print, configData);
                }
        }


// wait for set amount of time based off clock cycles
int wait(int cycles, int msPerCycle, PCB* pcb, ConfigClass configData, queue<PCB>* interruptQueue, bool* quantum)
        {
         int start = clock();
         int currentCycle = 0;
         bool QuantumFlag = false;
         bool IOFlag = false;

        // prime flags

        // break out if have reached the quantum
         if ( pcb->cyclesRanThisQuantum > configData.Quantum-1 )
                {
                 QuantumFlag = true;
                 (*quantum) = true;
                }
        
        // break if I/O operaion must be serviced
        if ( (*interruptQueue).size() > 0 )
                {
                 IOFlag = true;
                 (*quantum) = false;
                }

       
         // loop over each cycle in operation if no interrupts
         for ( currentCycle = 0; currentCycle < cycles && QuantumFlag == false && IOFlag == false; currentCycle++ )
                {
             
                 // wait for defined time while in middle of cycle
                 while (clock() - start < CLOCKS_PER_SEC * (double) (msPerCycle*.001))
                        {
                         // do nothing, wait
                        }

                 // update/reset data
                 start = clock();
                 pcb->cyclesRanThisQuantum++;


                 // break out if have have reached the quantum
                 if ( pcb->cyclesRanThisQuantum > configData.Quantum-1 )
                        {
                         QuantumFlag = true;
                         (*quantum) = true;
                        }

                 // break if I/O operaion must be serviced
                 if ( (*interruptQueue).size() > 0 )
                        {
                         IOFlag = true;
                         (*quantum) = false;
                        }
               }
     
          // return how many cycles were left ( if interrupted by io or quantum )
          return cycles - currentCycle;
                
        }

// function called to print/write/both data based on config file values
void printOrWriteData ( double cycleTime, string operation, ConfigClass configData)
        {
         bool error = true;
         
         // if should print data to screen
         if ( configData.Log == "Log to Both" || configData.Log == "Log to Monitor")
	        {
	         // setup cout params for correct output look
	         std::cout << std::setprecision(6) << std::showpoint << std::fixed;	

	         cout << cycleTime << operation << endl;
                 error = false;
	        }

          // if should write data to file
         if ( configData.Log == "Log to Both" || configData.Log == "Log to File")
	        {
	         writeToFile( configData.LogPath, to_string(cycleTime) + operation + "\n");
                 error = false;
	        }

         // output if there was an error with config file format for log type
         if ( error )
                {
                 cout << "Config file format error, what is: " +configData.Log +"?" << endl;
                }
        }


// write data to our log file
void writeToFile( string filePath, string dataToWrite)
        {
         ofstream outfile;
         outfile.open(filePath, std::ios_base::app);
         outfile << dataToWrite; 
         outfile.close();
        }

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
///////////////////////// PCB CLASS FUNCTIONS //////////////////////////////
////////////////////////////////////////////////////////////////////////////
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv/

// dirty function to load pcb data from file
// 1 if another pcb available, 0 if last, -1 if error
int PCB::loadProgram(string metaDataPath, int programToLoad)
        {
         // vars
         string line;
         string rawOperation;
         char dummy = ' ';
         unsigned int index = 0;
         int currentProgram = 1;

         unsigned int firstParenthesisIndex = 0;
         unsigned int lastParenthesisIndex = 0;

         // data to save to pcb
         char SAPIO;
         string descriptor = "";
         string runTime = "";

         MetaDataStruct* pointer; // pointer to head
         pointer = data;

         // open stream to meta-data file
         ifstream file (metaDataPath.c_str());

         // if meta-data file exits do some dirty work to extract the needed values
         if (file.is_open())
	          {
                   id = programToLoad;
                   while (file.good() )
                        {
                           // get whole line in file
                           getline(file, line);

                           // do nothing if not on first line
                           if ( line == "Start Program Meta-Data Code:" )
                                {

                                }

                           // show down reading in if at end of file
                           else if (  line == "End Program Meta-Data Code." )
                                {
                                 // return that last pcb to get
                                 return 0;
                                }
                           
                           // continue to read in operations if not at end or beginnning
                           else
                                {

                                 // loop over each operation in line
                                 while ( index < line.length() )
                                        {
                                         // read in dummy value from string
                                         dummy = line.at(index);

                                         // if at end of meta data file 
                                         if ( dummy == '.')
                                                {
                                                 return 0;
                                                }

                                         // if not at end of operation continue to read in
                                         else if ( dummy != ';')
                                                {
                                                 rawOperation+=dummy;
                                                 index++;
                                                }

                                         // have read in all needed data, now analyze
                                         else
                                                {
                                                 // get index between Parenthesis
                                                 firstParenthesisIndex = rawOperation.find("(");
                                                 lastParenthesisIndex =  rawOperation.find(")");

                                                 // get sapio type ( before first parenthesis )
                                                 SAPIO = rawOperation.at(firstParenthesisIndex-1);
                                
                                                 // read in runtime data ( after last parenthesis )
                                                 lastParenthesisIndex++;
                                                 while ( lastParenthesisIndex < rawOperation.size())
                                                        {
                                                         runTime += rawOperation.at(lastParenthesisIndex);
                                                         lastParenthesisIndex++;
                                                        }

                                                  // update data since changed 
                                                 lastParenthesisIndex =  rawOperation.find(")");
                                          
                                                 // itereate between parenthesis to get descriptor data
                                                 firstParenthesisIndex++;
                                                 while ( firstParenthesisIndex < lastParenthesisIndex )
                                                        {
                                                         descriptor += rawOperation.at(firstParenthesisIndex);
                                                         firstParenthesisIndex++;
                                                        }

                                                 // at end of a program update program counter
                                                 if ( descriptor == "end" )
                                                        {
                                                         currentProgram++;
                                                         // if got all datay needed and still more return so
                                                         if (currentProgram > programToLoad)
                                                                {
                                                                 return 1;
                                                                }
                                                        }

                                                 // save all data if not looking at a system operation and at right data
                                                 else if ( SAPIO != 'A' && SAPIO != 'S' && currentProgram == programToLoad)
                                                        {
                                                         // update acutal data in pcb 
		                                         pointer->SAPIO = SAPIO;
		                                         pointer->descriptor = descriptor;
		                                         pointer->runTime = atoi(runTime.c_str());
		                                         numberOfOperations++;

	                                                 // increment meta data struct pointer 
	                                                 pointer++;
                                                        }
                                                
                                                 // update index and set values back to empty
                                                 index++;
                                                 rawOperation = "";
                                                 descriptor = "";
                                                 runTime = "";
                                                }
                                        }

                                 // reset params 
                                 index = 0;
                                 rawOperation = "";
                                 descriptor = "";
                                 SAPIO = ' ';
                                 runTime = "";
                                }
                        }
           file.close();    
           return 0;
	  }
         // otherwise there was some problem, fail
         else
	        {
	         return -1;
	        }
        }

// constructor
PCB::PCB()
        {
	 state = "Enter";
	 programCounter = 0;
	 numberOfOperations = 0;
	 data = new MetaDataStruct [ MAX_PROGRAM_INSTRUCTIONS ];
         id = -1;
         cyclesRemaining = 0;
         cyclesRanThisQuantum = 0;
        }


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
/////////////////////// CONFIG CLASS FUNCTIONS /////////////////////////////
////////////////////////////////////////////////////////////////////////////
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv/

// function to load config data from file into class
bool ConfigClass::loadConfigData(char* configPath)
        {
         // vars
         string line;
         char dummy;
         string rawData [10]; 
         int lineNumber = 0;

         // create file object
         ifstream file (configPath);

        // open file
         if (file.is_open())
	          {
	           // ignore first 2 lines
	           getline (file,line);
	           getline (file,line);

	            // loop over full file
	            while (file.good())
		            {
		              // load next char
		              dummy = file.get();
		              // check if at data to load in
		              if (dummy == ':')
			         {

			          // move past white space
			          dummy = file.get();
			          dummy = file.get();
			          
			          // read in whole int
			          while (dummy!='\n')
				        {
				         rawData[lineNumber]+=dummy;
				         dummy = file.get();
				
				        }
			          lineNumber++;
			         }
		            }
	            file.close();

	            // now load data into correct params
	            MetaDataFilePath = rawData[0];
                    Scheduling = rawData[1];

	            // convert these strings to ints with the atoi call
                    Quantum = atoi(rawData[2].c_str());
	            ProcessorCycleTime = atoi(rawData[3].c_str());
	            MonitorDisplayTime = atoi(rawData[4].c_str());
	            HardDriveCycleTime = atoi(rawData[5].c_str());
	            PrinterCycleTime = atoi(rawData[6].c_str());
	            KeyboardCycleTime = atoi(rawData[7].c_str());
	            // back to strings
	            Log = rawData[8];
	            LogPath = rawData[9];

	            return true;
	          }

         // otherwise there was a problem opening the file, return error
         else 
	        {
	         return false;
	        }	 

        }


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
/////////////////// QUEUE COMPARATOR CLASS FUNCTIONS ///////////////////////
////////////////////////////////////////////////////////////////////////////
//vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv/

// constructor
Comparator_Selector::Comparator_Selector (string value)
        {
         type = value;
        }

// whole purpose of class, to order the queue properly
bool Comparator_Selector::operator() (PCB lhs, PCB rhs)
        {
         // custome compare for fifo, the ready queue for fifo is a priorty queue where the first process will have
         // the higest priority, first process determined by process id value
         if ( type == "FIFO-P" )
                {
                 // if process entering queue is older than item return true
                 if ( lhs.id > rhs.id )
                        {
                         return true;
                        }
                 return false;
                }

         // custom compare for round robin, the ready queue for RR is still a priority queue (like FIFO )
         // but the priority scheme here simply puts the new item on the end of the queue
         else if ( type == "RR" )
                {
                 return false;
                }

         // custom compare for srtf, when determining the order to push items to the queue srtf simply puts 
         // the object with the least remaining cycles in fromt
         else if ( type == "SRTF-P" )
                {
                 if ( calcRemainingCycles(lhs) > calcRemainingCycles(rhs) )
                        {
                         return true;
                        }
                 return false;
                }
         // catch some typo/error in the config data file and show to user
         else
                {
                 cout << "Scheduling type unknown, continuing as RR" << endl;
                 return false;
                }
        }

// used in SRTF-P scheduling type to calc remaining time
int Comparator_Selector::calcRemainingCycles( PCB pcb )
        {
         int counter = pcb.programCounter;      // how many operation left
         int cycles = pcb.cyclesRemaining;      // how many cycles left/per operation
         int index;                             // a tmp counter

         // if stopped in the middle of an operation
         if ( cycles > 0 )
                {
                 // move to next operation
                 counter++;
                }

         // loop over remaining operations
         for ( index = counter; index < pcb.numberOfOperations; index++)
                {
                 // update number of cycles
                 cycles+= pcb.data[counter].runTime;
                }

         return cycles;
        }

