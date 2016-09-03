#include <stdio.h>
#include <sys/types.h>
#include <wait.h>
#include <stdlib.h>
#include <unistd.h>

/// function for checking if married
int finder(char W, char familytree[]);

/// the beginning of the main program
int main(int argc, char* argv[]){

/// counter for levels of a tree
int tabs = 0;

/// array for all family data 
char fam[100];
/// clearing the array
int ndx = 0;
for (ndx = 0; ndx < 100; ndx++){
	fam[ndx] = '\0';
}

/// declaration for the forking pid saving
pid_t pid;

/// open the file and read in every character into array
FILE* fpointer;
fpointer = fopen( argv[1], "r");

/// index for going through array 
ndx = 0;
/// temp variable for reading in
char temp;

/// read file until the end of the file
while (!feof(fpointer)){
	/// get the current character
	temp = fgetc(fpointer);
	/// if the character is not an endline or space char save in the fam array
	if (temp!='\n' && temp!=' '){
		/// assign to array
		fam[ndx] = temp;
		/// move to next array element
		ndx++;
	}	
}

/// close the file
fclose(fpointer);

/// child counter & array counter
int famNdx = 0;
int childC = 0;

/// print out the OGs (the original parents)
//cout << fam[0] << "-" << fam[1] << endl;
printf("%c-%c", fam[0], fam[1]);
printf("\n");

/// update the index to the location of the # of children
famNdx = famNdx + 2;

/// get # of children
childC = fam[famNdx] - '0';

/// update the location index
famNdx++;

/// index for number of children of the eldest parents
int i = 0;
for (i = 0; i < childC; i++){
	/// create new process
	pid = fork();

	/// if child
	if (pid == 0){ 
		/// update the generation
		tabs++;
		/// find spouse (if any)
		int wifey = finder(fam[famNdx], fam);

		/// if no spouse end process by making children 0
		if (wifey == -1){
			int j = 0;
			/// print out the number of appropriate tabs
			while (j < tabs){
				printf("\t");
				j++;
			}
			/// print out the child
			printf("%c", fam[famNdx]);
			printf("\n");
			/// update ChildC to 0 to end process
			childC = 0;
		}
		/// if spouse, print out the spouse and update children
		else{
			/// print out the tabs
			int j = 0;
			while (j < tabs){
				printf("\t");
				j++;
			}
			
			/// print the current child and its spouse
			printf("%c-%c", fam[famNdx], fam[wifey]);
			printf("\n");

			///update the location to the number of child for current child
			famNdx = wifey+1;
			///store # of children in the ChildC for new loop 
			childC = fam[wifey+1] - '0';
			/// update the counter so child can also loop
			i = -1;
			/// move location to child
			famNdx++;
		}
	}
	else{
		/// parent waits for its child
		waitpid(pid,NULL,0);
		/// move to next child
		famNdx++;
	}	
}

return 0;
}

int finder(char W, char familytree[]){
	int counter = 0;

	// cycle through family tree array
	int ndx = 0;
	for (ndx = 0; ndx < 100; ndx++){
		/// if found once record
		if (W == familytree[ndx])
			counter++;
		/// if found twice return the index
		if (counter == 2){
			ndx++;
			return ndx;
		}
	}

/// no spouse found
return -1;
}
