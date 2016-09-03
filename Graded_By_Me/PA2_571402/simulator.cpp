#include <iostream>      // Input/Output for reporting
#include <fstream>       // File I/O handling formatted config and meta data files
#include <sys/time.h>    // Time reporting and clock ticks per second
#include <pthread.h>     // Threading library for program I/O simulation
#include <stdlib.h>      // atoi() function for converting string to integer
#include <sstream>       // String stream functions for converting float time values to reports
#include <unistd.h>      // sleep(microseconds) function for cycle time simulation
using namespace std;

// Create a struct to hold the values of the OS Configuration (Config)
struct OSConfig {
    // Version/Phase value
    float version;
    // filePaths
    string filePath, logFilePath, logOutputChoice;
    // Cycle times for suggested hardware
    int processorCycleTime, monitorCycleTime, hardDriveCycleTime,
        printerCycleTime, keyboardCycleTime;
};

// Create a struct to hold process meta-data commands
struct Command {
    // Value for type (System[S], Application[A], Process[P], Input[I], or Output[O])
    char commandType;
    // Value for command text (e.g. Monitor)
    string commandText;
    // Value for CPU bursts (cycles)
    int cycles;
};

// Create a struct to manage the Program Control Block (PCB) commands
struct PCB {
    // Values for the command count and current
    int commandCount, currentCommand;
    // ID for assignment 1 (only one process, but will be expanded in PA3)
    int processID;
    // List of collected commands
    Command* PCBCommands;
};

// Threading requires structure for multiple parameter values
struct IOData {
    // Values for the cycle times of IO thread actions
    int millisecondsPerCycle, commandCycles;
};

// Function declarations
bool loadConfiguration(ifstream&, OSConfig&);
bool loadMetaData(string, PCB&);
void runSAP(double&, int, Command, string&, string&);
void setupIO(double&, const struct OSConfig, Command, string&, string&);
void *runIO(void*);
void report(ofstream&, bool, bool, string, string);

int main(int argc, char* argv[]) {
    // main function local variables
    bool errorInLog = false;
    bool logToFile = false;
    bool logToMonitor = false;
    double calculatedTime, elapsedTime = 0.000000;
    string reportInitial, reportStart, reportEnd;
    ifstream fileIN;
    ofstream fileOUT;
    OSConfig osConfig;
    PCB pcb;
    pcb.processID = 1;
    Command holdCommand;
    struct timeval startTime, endTime;

    // Check the values of the input arguments to ensure configuration file is found
    if(argc == 2) {
        // Check that the file was opened before proceeding
        fileIN.open(argv[1]);
        if(fileIN.is_open()) {
            // Load the configuration data and close the config file
            if(loadConfiguration(fileIN, osConfig)){
                fileIN.close();

                // Check the configuration for log method and open the log file if to file
                if(osConfig.logOutputChoice == "Log to File") {
                    logToFile = true;
                }
                if(osConfig.logOutputChoice == "Log to Monitor") {
                    logToMonitor = true;
                }
                if(osConfig.logOutputChoice == "Log to Both") {
                    logToFile = true;
                    logToMonitor = true;
                }
                // Attempt to establish the log file if requested
                if(logToFile) {
                    fileOUT.open(osConfig.logFilePath.c_str());
                    if(!fileOUT.is_open()) { // Log file could not be opened - print error
                        logToFile = false;
                        errorInLog = true;
                        cout << "ERROR: Log File could not be obtained. Please check the log file path and try again." << endl;
                    }
                }

                // Prompt for the program meta-data to run and continue if valid file found
                if(loadMetaData(osConfig.filePath, pcb) && !errorInLog) {
                    // Initialize the reporting and starting Time stamp
                    reportInitial = "0.000000 - Simulator program starting";
                    reportEnd = "";
                    gettimeofday(&startTime, NULL);

                    // Print reporting initialization
                    report(fileOUT, logToFile, logToMonitor, reportInitial, reportEnd);

                    // Run the target commands
                    for(pcb.currentCommand = 0; pcb.currentCommand < pcb.commandCount; pcb.currentCommand++) {
                        // Take in next command and determine action, clear the report strings between commands
                        holdCommand = pcb.PCBCommands[pcb.currentCommand];
                        reportStart.clear();
                        reportEnd.clear();

                        if(holdCommand.commandType == 'I' || holdCommand.commandType == 'O') {
                            // IO commands are to be handled by setupIO which will call on runIO during threads
                            setupIO(elapsedTime, osConfig, holdCommand, reportStart, reportEnd);
                            // Report results
                            report(fileOUT, logToFile, logToMonitor, reportStart, reportEnd);
                        } else {
                            // SAP (System, Application, and Process) methods are similar, so runSAP and collect reports
                            runSAP(elapsedTime, osConfig.processorCycleTime, holdCommand, reportStart, reportEnd);
                            // Report results
                            report(fileOUT, logToFile, logToMonitor, reportStart, reportEnd);
                        }
                    }
                    // Report time elapsed and shut down simulator
                    gettimeofday(&startTime, NULL);
                    gettimeofday(&endTime, NULL);
                    calculatedTime = (double)(endTime.tv_sec - startTime.tv_sec)
                                                   + (double)(endTime.tv_usec - startTime.tv_usec) / CLOCKS_PER_SEC;
                    elapsedTime += calculatedTime;
                    reportInitial = " - Simulator program ending";
                    // Manual reporting of system shut down
                    if(logToFile) {
                        fileOUT << elapsedTime << reportInitial;
                    }
                    if(logToMonitor) {
                        cout << elapsedTime << reportInitial << endl;
                    }
                }
            }
        } else { // The file was not successfully opened above
            cout << "ERROR: OS Configuration File was not opened. Please check the file format and try again." << endl;
        }
    } else {
        // Print an error message and close the program if the config file is not found
        cout << "ERROR: OS Configuration file not found. Please check the input file name and resubmit." << endl;
    }

    return 0;
}

// Function implementations
// loadConfiguration() collects data from the command-line given config file and parses according to PA2
bool loadConfiguration(ifstream& file, OSConfig& osconfig) {
    string holdString, targetString;
    int offset = 2;
    size_t startPoint;

    // Get the string until the endline character from the file and parse as designated for each value
    getline(file, holdString);
    getline(file, holdString);
    // Given version will be 1.0 for this assignment
    osconfig.version = 1.0;
    getline(file, holdString);
    // Formatted data begins after "File Path: ", so iterate from ": "
    startPoint = holdString.find(": ");
    // Assign substring to filePath - This method will be used for output choice and log file path as well
    osconfig.filePath = holdString.substr(startPoint + offset);
    getline(file, holdString);
    startPoint = holdString.find(": ");
    targetString = holdString.substr(startPoint + offset);
    // atoi method for casting to integer from substring - set Processor cycle time
    osconfig.processorCycleTime = atoi(targetString.c_str());
    // Repeat find and atoi for monitor cycle time
    getline(file, holdString);
    startPoint = holdString.find(": ");
    targetString = holdString.substr(startPoint + offset);
    osconfig.monitorCycleTime = atoi(targetString.c_str());
    // Repeat find and atoi for hard drive cycle time
    getline(file, holdString);
    startPoint = holdString.find(": ");
    targetString = holdString.substr(startPoint + offset);
    osconfig.hardDriveCycleTime = atoi(targetString.c_str());
    // Repeat find and atoi for printer cycle time
    getline(file, holdString);
    startPoint = holdString.find(": ");
    targetString = holdString.substr(startPoint + offset);
    osconfig.printerCycleTime = atoi(targetString.c_str());
    // Repeat find and atoi for keyboard cycle time
    getline(file, holdString);
    startPoint = holdString.find(": ");
    targetString = holdString.substr(startPoint + offset);
    osconfig.keyboardCycleTime = atoi(targetString.c_str());
    // Get the log output choice (Log to File, Log to Monitor, Log to Both)
    getline(file, holdString);
    startPoint = holdString.find(": ");
    osconfig.logOutputChoice = holdString.substr(startPoint + offset);
    // Finish with the log's file path which will be handled if log output choice is file or both
    getline(file, holdString);
    startPoint = holdString.find(": ");
    osconfig.logFilePath = holdString.substr(startPoint + offset);

    return true;
}

// loadMetaData() locates the meta-data file created by programgenerator.cpp and parses for commands
bool loadMetaData(string filepath, PCB& pcb) {
    string readString, editString;
    int countCommands, index, finalLineSize = 25, offset = 2, startpos = 1;
    ifstream fileIN;
    size_t startPoint, endPoint;

    fileIN.open(filepath.c_str());
    if(fileIN.is_open()) {
        getline(fileIN, readString);
        // Get the first line of command until the ';' character or endline intialize size counter
        getline(fileIN, readString, ';');
        countCommands = startpos;
        while(readString.length() < finalLineSize) {
            // First run through will gather the expected commands and tally how many to expect
            countCommands++;
            getline(fileIN, readString, ';');
        }
        // Close and reopen to set file back to beginning
        fileIN.close();
        fileIN.open(filepath.c_str());
        // Gather and discard first line and prepare PCB for command list
        getline(fileIN, readString);
        getline(fileIN, readString, ';');
        pcb.PCBCommands = new Command[countCommands];
        pcb.commandCount = countCommands;
        for(index = 0; index < countCommands; index++) {
            // Gather commands and parse for command field entries
            // Check for starting whitespace
            if(readString[0] == ' ' || readString[0] == '\n') {
                readString = readString.substr(startpos);
                if(readString[0] == ' ' || readString[0] == '\n') {
                    readString = readString.substr(startpos);
                }
            }
            // First character will be commandType - Simply pull in the first character
            pcb.PCBCommands[index].commandType = readString[0];
            // Find the ( and ) positions and collect commandText
            startPoint = readString.find("(") + startpos;
            endPoint = readString.find(")");
            editString = readString.substr(startPoint, endPoint - offset);
            pcb.PCBCommands[index].commandText = editString;
            // Detect the given cycles, atoi, and assign to command
            editString = readString.substr(endPoint + startpos);
            pcb.PCBCommands[index].cycles = atoi(editString.c_str());
            // get the next command
            getline(fileIN, readString, ';');
        }
        return true;
    } else {
        // If the meta-data file could not be opened above, print the error message and return failure
        cout << "ERROR: Meta data file path could not be found. Please check the file path and try again." << endl;
        return false;
    }
}

// runSAP() takes System, Application, and Process commands and formats the reportStart and reportEnd strings
// after actions have been calculated and "run" by sleeping the program and collecting the wall clock times
void runSAP(double& elapsedTime, int cycleTime, Command command, string& reportStart, string& reportEnd) {
    unsigned long calcCycleTime;
    int millisecondBase = 1000;
    double calculatedTime;
    struct timeval endTime, startTime;
    string editString;
    ostringstream osString;

    // Run [S]ystem actions 'start' and 'end'
    if(command.commandType == 'S') {
        if(command.commandText == "start") {
            // Collect wall clock times for the system to make a brief comparison of process time
            gettimeofday(&startTime, NULL);
            gettimeofday(&endTime, NULL);
            calculatedTime = (double)(endTime.tv_sec - startTime.tv_sec)
                                        + (double)(endTime.tv_usec - startTime.tv_usec) / CLOCKS_PER_SEC;
            elapsedTime += calculatedTime;
            // osstringstream takes float and converts to string when str() is called like cout streams
            osString << elapsedTime;
            editString = osString.str();
            reportStart = editString + " - OS: preparing process 1";
        } else {
            gettimeofday(&startTime, NULL);
            gettimeofday(&endTime, NULL);
            calculatedTime = (double)(endTime.tv_sec - startTime.tv_sec)
                                        + (double)(endTime.tv_usec - startTime.tv_usec) / CLOCKS_PER_SEC;
            elapsedTime += calculatedTime;
            osString << elapsedTime;
            editString = osString.str();
            reportStart = editString + " - OS: removing process 1";
        }
    }

    // Run [A]pplication actions 'start' and 'end'
    if(command.commandType == 'A') {
        if(command.commandText == "start") {
            // Again, applications have only start and end and do not have cycles other than 0
            gettimeofday(&startTime, NULL);
            gettimeofday(&endTime, NULL);
            calculatedTime = (double)(endTime.tv_sec - startTime.tv_sec)
                                        + (double)(endTime.tv_usec - startTime.tv_usec) / CLOCKS_PER_SEC;
            elapsedTime += calculatedTime;
            osString << elapsedTime;
            editString = osString.str();
            reportStart = editString + " - OS: starting process 1";
        } else {
            gettimeofday(&startTime, NULL);
            gettimeofday(&endTime, NULL);
            calculatedTime = (double)(endTime.tv_sec - startTime.tv_sec)
                                        + (double)(endTime.tv_usec - startTime.tv_usec) / CLOCKS_PER_SEC;
            elapsedTime += calculatedTime;
            osString << elapsedTime;
            editString = osString.str();
            reportStart = editString + " - OS: removing process 1";
        }
    }

    // Run [P]rocess action 'run'
    if(command.commandType == 'P') {
        // calculate the target time based off cpu cycles in config and command cycles in milliseconds to microseconds
        calcCycleTime = cycleTime * command.cycles * millisecondBase;

        // Create and format the starting process action string
        gettimeofday(&startTime, NULL);
        gettimeofday(&endTime, NULL);
        calculatedTime = (double)(endTime.tv_sec - startTime.tv_sec)
                                    + (double)(endTime.tv_usec - startTime.tv_usec) / CLOCKS_PER_SEC;
        elapsedTime += calculatedTime;
        osString << elapsedTime;
        editString = osString.str();
        reportStart = editString + " - Process 1: start processing action";

        // Clean the osString buffer between reports
        osString.clear();
        osString.str("");

        // call usleep(microseconds from calculated cycle time) to hold the program and collect the time after
        usleep(calcCycleTime);
        gettimeofday(&endTime, NULL);
        calculatedTime = (double)(endTime.tv_sec - startTime.tv_sec)
                                    + (double)(endTime.tv_usec - startTime.tv_usec) / CLOCKS_PER_SEC;
        elapsedTime += calculatedTime;

        // format report for ending action
        osString << elapsedTime;
        editString = osString.str();
        reportEnd = editString + " - Process 1: end processing action";
    }
}

void setupIO(double& elapsedTime, const struct OSConfig osconfig,
                                        Command command, string& reportStart, string& reportEnd) {
    // Unlike runSAP, IO creates pthreads (POSIX threads) to run runIO() and calculates when
    // threads join so that one process is run at a time. Future iterations will have multiple threads running concurrently.
    unsigned long calcCycleTime;
    int millisecondBase = 1000;
    double calculatedTime;
    struct timeval endTime, startTime;
    string floatString, ioString;
    ostringstream osString;
    struct IOData iodata;
    pthread_t iothread;
    pthread_attr_t threadAttribute;
    void* status;

    // Determine thread cycle count and arguments for IOData structure
    if(command.commandText == "hard drive") {
        iodata.millisecondsPerCycle = osconfig.hardDriveCycleTime;
        iodata.commandCycles = command.cycles;
    }
    if(command.commandText == "monitor") {
        iodata.millisecondsPerCycle = osconfig.monitorCycleTime;
        iodata.commandCycles = command.cycles;
    }
    if(command.commandText == "keyboard") {
        iodata.millisecondsPerCycle = osconfig.keyboardCycleTime;
        iodata.commandCycles = command.cycles;
    }
    if(command.commandText == "printer") {
        iodata.millisecondsPerCycle = osconfig.printerCycleTime;
        iodata.commandCycles = command.cycles;
    }

    // Start process initialization and reporting
    gettimeofday(&startTime, NULL);
    gettimeofday(&endTime, NULL);
    calculatedTime = (double)(endTime.tv_sec - startTime.tv_sec)
                                    + (double)(endTime.tv_usec - startTime.tv_usec) / CLOCKS_PER_SEC;
    elapsedTime += calculatedTime;
    osString << elapsedTime;
    floatString = osString.str();
    // Determine if action is 'input' or 'output' and format to string
    if(command.commandType == 'I') {
        ioString = " input";
    } else {
        ioString = " output";
    }
    reportStart = floatString + " - Process 1: start " + command.commandText + ioString;

    // Clean the osString between reports
    osString.clear();
    osString.str("");

    // Create the thread data and run it as joinable
    pthread_attr_init(&threadAttribute);
    pthread_attr_setdetachstate(&threadAttribute, PTHREAD_CREATE_JOINABLE);
    // Ensure that the thread was created and IOData was passed for cycle counts
    if(pthread_create(&iothread, NULL, runIO, (void *)&iodata)){
        // Print an error message for this action if the thread could not be created
        cout << "ERROR: Unable to create thread. Please check the IO definitions and try again." << endl;
    }
    // Free the thread attribute (used when concurrent threads are introduced)
    pthread_attr_destroy(&threadAttribute);
    // Wait for the thread to finish and join - print an error if join did not occur
    if(pthread_join(iothread, &status)) {
        cout << "ERROR: Unable to join thread. Please Check the IO definitions and try again." << endl;
    }
    gettimeofday(&endTime, NULL);
    calculatedTime = (double)(endTime.tv_sec - startTime.tv_sec)
                                    + (double)(endTime.tv_usec - startTime.tv_usec) / CLOCKS_PER_SEC;
    elapsedTime += calculatedTime;

    // format report for ending action
    osString << elapsedTime;
    floatString = osString.str();
    if(command.commandType == 'I') {
        ioString = " input";
    } else {
        ioString = " output";
    }
    reportEnd = floatString + " - Process 1: end " + command.commandText + ioString;
}

// IO thread function to spin the usleep timer and return to the calling function when finished
void *runIO(void* iodata) {
    struct IOData* givenIO;
    unsigned long calcCycleTime;
    int millisecondBase = 1000;

    // Collect the passed in thread argument for IOData
    givenIO = (struct IOData*) iodata;

    // Calculate the cycle time to be run
    calcCycleTime = givenIO->commandCycles * givenIO->millisecondsPerCycle * millisecondBase;

    // usleep(calculated cycle time in microseconds) to pause, then exit
    usleep(calcCycleTime);
    pthread_exit(NULL);
}

// report() collects the report strings and file and monitor flags to determine where to distribute reports
void report(ofstream& fileOUT, bool reportToFile, bool reportToMonitor, string reportStart, string reportEnd) {
    // Function checks that the strings are not empty and reports to relevent logs
    if(reportStart != "") {
        if(reportToFile) {
            fileOUT << reportStart << endl;
        }
        if(reportToMonitor) {
            cout << reportStart << endl;
        }
    }
    if(reportEnd != "") {
        if(reportToFile) {
            fileOUT << reportEnd << endl;
        }
        if(reportToMonitor) {
            cout << reportEnd << endl;
        }
    }
}
