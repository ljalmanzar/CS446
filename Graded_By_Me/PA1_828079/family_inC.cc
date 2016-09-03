/**@file family.cpp
	@author 828079
	@bugs No Known Bugs

	UNR CSE Dept. CS 446
	Programming Assignment No. 1
*/
#ifndef __FAMILY_SIM_
#define __FAMILY_SIM_

//
//HEADER FILES
//

#include <iostream>			//console output
#include <cstdlib>			//misc. objects
#include <fstream>			//fin
#include <stdio.h>			//printf
#include <sys/types.h>		//system calls
#include <wait.h>				//waiting functions
#include <unistd.h>			//pid_t objects
#include <cstring>			//cstring

using namespace std;

//
//Global Variables
//
const int MAX_SIZE = 50;
const int BUFFER = 10;
char fam_tree_names[MAX_SIZE][BUFFER];
//
//Extra Function Implementations
//

/*
Function:	outputTabs
Purpose: 	Given an input integer, will output
			to the console that number of tabs to
			aid in indendentation and readability
			to the user
*/
void outputTabs( int numOfTabs ){
	//declare variables
	int i = 0;

	//loop through
	for( i = 0; i < numOfTabs; i++ ){
		printf("\t");
	}
}

/*
Function: 	fileReadIn
Purpose:	Given the name of the file and the string
			array to save them in, will read in information
			from a file with the names and number of children
			from each generation for this practice
Parameters:	fileName (char *)
*/
void fileReadIn( char * fileName ){
	//declare variales and open file
	int i = 0;
	char tempChar;
	FILE * rFile;
	rFile = fopen( fileName, "r" );

	//prime the loop
	tempChar = fgetc( rFile );

	//while file is not at end
	while( tempChar != EOF ){
		//ignore whitespace
		if( (tempChar != ' ') &&
				(tempChar != '\n' ) ){
			//keep reading and saving
			fam_tree_names[i][0] = tempChar;
			fam_tree_names[i][1] = '\0';
			i++;
		}

		tempChar = fgetc( rFile );		
	}

	//close file
	fclose( rFile );
}

/*
Function:	outputParents
Purpose: 	Given two names and the number of children
			between them, will output the result accordingly
			and will take in the case of no spouse or spouse
			with no children.
Parameters: father (const string&), mother (const string&)
			children (int), 
*/
void outputParents( const char * father,
					const char * mother, const int children, const int tabs ){
	//check to see if there is no father
	if( father != NULL ){
		//output the appropriate tabs
		outputTabs( tabs );
		//output father
		printf("%s", father );

		//if there is a spouse, output her
		if( mother != NULL ){
			printf("-%s", mother );
		}

		cout << endl;

	}else{
		//quit function
		return;
	}

}

/*
Function:	findRepeatOf
Purpose;	Given a specific name of a child and the location
			of where to start searching, will iterate through 
			the string list until another one that matches it
			is found.  If there does not exist one, will return
			a value of negative one, elseways, will return the 
			integer index location of the repeat.
Parameters:	wantedName(const string&), location(int)
Return:		foundIndex(int)
*/
int findRepeatOf( const char * wantedName, const int location ){
	//declare variables
	int foundIndex = location + 1;

	//iterate through the rest of the array
	while( foundIndex < MAX_SIZE ){
		//if match is found
		if( (strcmp( fam_tree_names[foundIndex], wantedName )) == 0 ){
			//return that value
			return foundIndex;
		}

		//increment index
		foundIndex++;
	}

	//iteration through array was done, no repeat was found
	return -1;
}

int main( int argc, char * argv[] )
	{
		//declare variables
			char fatherName[BUFFER],	//parent 1
			motherName[BUFFER], 			//parent 2 (placeholder)
			childName[BUFFER];			//childName
			int numOfChildren, 			//number of children for 
												//most current parents
			location = 0,					//counter for location in array 
			tabs = 0,						//counter for indentation
			i = 0;							//looping index
			pid_t pid;						//Process Identification Number

		//read in from file
		fileReadIn( argv[1] );

		//process first eldest parents
		strcpy( fatherName, fam_tree_names[location] );
		strcpy( motherName, fam_tree_names[++location] );

		//take in the preliminary number of children
		numOfChildren = fam_tree_names[++location][0] - '0';

		//output the OG's
		outputParents( fatherName, motherName, numOfChildren, tabs );

		//start for loop
		for( i = 0; i < numOfChildren; i++ ){	

			//fork for each children
			pid = fork();

			//Forking Error
			if( pid < 0 ){
				cout << "Error: Process not initalized correctly." << endl;
				return 0;
			}
			//Child Process
			else if( pid == 0 ){
				//increment the indendations
				tabs++;

				//assign new father name and mother name and new number of children
					//take in the child name
					strcpy( childName, fam_tree_names[++location] );

					//check to see where it's repeated again, if at all
					location = findRepeatOf( childName, location );

					//if not repeated, just output the child name and set
					//num of children to zero to break the loop
					if( location == -1 ){
						numOfChildren = 0;
						outputParents( childName, NULL, numOfChildren, tabs );
					}
					//if repeated,
					else{
						//take in the father name and mother name
						strcpy( fatherName, childName );
						strcpy( motherName, fam_tree_names[++location] );

						//update the number of children
						numOfChildren = fam_tree_names[++location][0] - '0';

						//reset the loop
						i = -1;

						//leave the index at right before the first child
						//and print out the child at this point in time
						outputParents( fatherName, motherName, numOfChildren, tabs );
					} 

			}
			//Parent Process
			else{
				//wait for child to die
				waitpid( pid, NULL, 0 );

				//increment count of loc
				location++;
			}

		}
		//end program
		return 0;
	}

#endif