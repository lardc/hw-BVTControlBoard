// Header
#include "StorageDescription.h"
#include "Global.h"
#include "MemoryBuffers.h"

// Variables
RecordDescription StorageDescription[] =
{
	{"EP 1. for I value sequence",										DT_Int16U, VALUES_x_SIZE},
	{"EP 2. for V value sequence",										DT_Int16U, VALUES_x_SIZE},
	{"EP 3. for PWM & other value sequence",							DT_Int16U, VALUES_x_SIZE},
	{"EP 4. for V following error",										DT_Int16U, VALUES_x_SIZE},
	{"EP 5. for peak I value sequence",									DT_Int16U, VALUES_x_SIZE},
	{"EP 6. for peak V value sequence",									DT_Int16U, VALUES_x_SIZE},

	{"IV Counter",														DT_Int16U, 1},
	{"Diag Counter",													DT_Int16U, 1},
	{"Err Counter",														DT_Int16U, 1},
	{"IV Peak Counter",													DT_Int16U, 1},

	{"REG 128. Measurement type",										DT_Int16U, 1},
	{"REG 129. Measurement mode (V-mode or I-mode)",					DT_Int16U, 1},
	{"REG 130. Threshold current (in mA * 10)",							DT_Int16U, 1},
	{"REG 131. Maximum or test voltage (in V)",							DT_Int16U, 1},
	{"REG 132. Voltage plate time in AC measurement mode (in ms)",		DT_Int16U, 1},
	{"REG 133. Rate of increasing AC voltage (in kV/s * 10)",			DT_Int16U, 1},
	{"REG 134. Start voltage for AC modes (in V)",						DT_Int16U, 1},
	{"REG 135. Voltage frequency (in Hz)",								DT_Int16U, 1},
	{"REG 136. Divisor for voltage pulses rate",						DT_Int16U, 1},
	{"REG 137. Scope rate divisor",										DT_Int16U, 1},

	{"Device state",													DT_Int16U, 1},
	{"Fault reason in the case DeviceState -> FAULT",					DT_Int16U, 1},
	{"Fault reason in the case DeviceState -> DISABLED",				DT_Int16U, 1},
	{"Warning if present",												DT_Int16U, 1},
	{"Problem reason",													DT_Int16U, 1},
	{"Indicates that test is done and there is result or fault",		DT_Int16U, 1},
	{"Test result (in V)",												DT_Int16U, 1},
	{"Test result (mA * 10 or uA)",										DT_Int16U, 1},
	{"Resistance result DC-mode or Test result mA fraction for AC mode",DT_Int16U, 1},

	{"Primary side capacitor voltage based on sensing (in V)",			DT_Int16U, 1},
	{"Primary side capacitor voltage used by control system (in V)",	DT_Int16U, 1},
};    
Int32U TablePointers[sizeof(StorageDescription) / sizeof(StorageDescription[0])] = {0};
const Int16U StorageSize = sizeof(StorageDescription) / sizeof(StorageDescription[0]);

