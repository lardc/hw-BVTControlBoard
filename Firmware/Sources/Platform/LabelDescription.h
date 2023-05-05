#ifndef __LABEL_DESCRIPTION_H
#define __LABEL_DESCRIPTION_H

// Include
#include "stdinc.h"
#include "FirmwareLabel.h"

// Definitions
#define LABEL_START_ADDRESS_SECTOR			0x3F0000	// Sector B
#define LABEL_START_ADDRESS_SHIFT			0x2000
#define LABEL_START_ADDRESS					(LABEL_START_ADDRESS_SECTOR + LABEL_START_ADDRESS_SHIFT)

// Variables
extern const Label BoardLabels[];
extern const Int16U BoardLabelsSize;

#endif // __LABEL_DESCRIPTION_H
