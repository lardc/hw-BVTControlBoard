// Header
#include "FirmwareInfo.h"

// Definitions
#define COMMIT_LEN		4
#define DATE_LEN		19

// Includes
#include "git_info.h"

// Functions
//
Int16U FWINF_Compose(unsigned char *OutputString, Int16U MaxLength)
{
	Int16U BranchLen, i, counter = 0;
	
	// Compose commit info
	for(i = 0; i < COMMIT_LEN && counter < MaxLength; i++)
		OutputString[counter++] = git_commit[i];
	if(counter < MaxLength)
		OutputString[counter++] = ',';
	
	// Compose date info
	for(i = 0; i < DATE_LEN && counter < MaxLength; i++)
		OutputString[counter++] = git_date[i];
	if(counter < MaxLength)
		OutputString[counter++] = ',';
	
	// Compose date info
	BranchLen = sizeof(git_branch) / sizeof(char) - 1;
	for(i = 0; i < BranchLen && counter < MaxLength; i++)
		OutputString[counter++] = git_branch[i];
	
	// Align counter to even
	if(counter % 2 && counter < MaxLength)
		OutputString[counter++] = ' ';

	return counter;
}
// ----------------------------------------
