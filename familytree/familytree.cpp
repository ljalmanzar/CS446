#include <iostream>
#include <cstdlib>
#include <fstream>

#include <stdio.h>
#include <sys/types.h>
#include <wait.h>
#include <unistd.h>

using namespace std;

/// function for checking if married
int finder(char W, char familytree[]);

/// the beginning of the main program
int main(int argc, char* argv[]){

/// counter for levels of a tree
int tabs = 0;

/// array for all family data 
char fam[100];
/// clearing the array
for (int ndx = 0; ndx < 100; ndx++){
	fam[ndx] = '\0';
}

/// declaration for the forking
pid_t pid;

ifstream fin;
fin.clear();
fin.open(argv[1]);

/// prime loop and get all file inputs
int ndx = 0;
while(fin.good()){
	fin >> fam[ndx];
	ndx++;
} 

for (int ndx2 = 0; ndx2 < ndx; ndx2++){
	cout << fam[ndx2] << " ";
}
cout << endl << endl;
cout << endl<< fam[0] << fam[3] << fam[5] << fam[6];
/// child counter & array counter
int famNdx = 0;
int childC = 0;

/// print out the OGs
cout << fam[0] << "-" << fam[1] << endl;

famNdx = famNdx + 2;

/// get # of children
childC = fam[famNdx] - '0';
famNdx++;

for (int i = 0; i < childC; i++){
	pid = fork();

	/// if child
	if (pid == 0){ 
		tabs++;
		/// find spouse
		int wifey = finder(fam[famNdx], fam);

		/// if no spouse end process by making children 0
		if (wifey == -1){
			int j = 0;
			while (j < tabs){
				cout << '\t';
				j++;
			}
			cout << fam[famNdx] << endl;
			childC = 0;
		}
		/// if if spouse, print out the spouse and update children
		else{
			int j = 0;
			while (j < tabs){
				cout << '\t';
				j++;
			}
			cout << fam[famNdx] << '-' << fam[wifey] << endl;
			famNdx = wifey+1;
			childC = fam[wifey+1] - '0';
			i = -1;
			famNdx++;
		}
	}
	else{
		waitpid(pid,NULL,0);
		famNdx++;
	}
}

fin.close();

}

int finder(char W, char familytree[]){
	int counter = 0;

	for (int ndx = 0; ndx < 100; ndx++){
		if (W == familytree[ndx])
			counter++;
		if (counter == 2){
			ndx++;
			return ndx;
		}
	}

/// no spouse found
return -1;
}
