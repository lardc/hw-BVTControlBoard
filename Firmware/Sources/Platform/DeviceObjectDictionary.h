#ifndef __DEV_OBJ_DIC_H
#define __DEV_OBJ_DIC_H


// ACTIONS
//
#define ACT_ENABLE_POWER			1	// Enable charging cells
#define ACT_DISABLE_POWER			2	// Disable charging cells
#define ACT_CLR_FAULT				3	// Clear fault (try switch state from FAULT to NONE)
#define ACT_CLR_WARNING				4	// Clear warning flag
//
#define ACT_DBG_DIGI_PING			11	// Ping optical connection
#define ACT_DBG_DIGI_START_SMPL		12	// Digitizer start sampling
#define ACT_DBG_DIGI_STOP_SMPL		13	// Digitizer stop sampling
#define ACT_DBG_PULSE_SWITCH		14	// Pulse discharge switch
#define ACT_DBG_SWITCH_INDICATOR	15	// Checking the red indicator on the front panel (1 sec)
#define ACT_DBG_PULSE_SYNC			16	// Pulse sync output for 1 second
//
#define ACT_START_TEST				100	// Start test with defined parameters
#define ACT_STOP					101	// Stop test sequence
#define ACT_READ_FRAGMENT			110	// Read next data portion
#define ACT_READ_MOVE_BACK			111	// Move read pointer back
//
#define ACT_SAVE_TO_ROM				200	// Save parameters to EEPROM module
#define ACT_RESTORE_FROM_ROM		201	// Restore parameters from EEPROM module
#define ACT_RESET_TO_DEFAULT		202	// Reset parameters to default values (only in controller memory)
#define ACT_LOCK_NV_AREA			203	// Lock modifications of parameters area
#define ACT_UNLOCK_NV_AREA			204	// Unlock modifications of parameters area (password-protected)
//
#define ACT_BOOT_LOADER_REQUEST		320	// Request reboot to bootloader
#define ACT_WRITE_LABEL1			321	// Записать первую метку: BVTMainBoard v.2.0 [Manufacturing] + bridge rectifier
#define ACT_READ_SYMBOL				330	// Выполнить чтение символа из памяти
#define ACT_SELECT_MEM_LABEL		331	// Переместить указатель считывания в область метки


// REGISTERS
//
#define REG_TEST_PWM_AMPLITUDE		0	// Raw PWM amplitude
#define REG_TEST_FREQUENCY			1	// Sine frequency
#define REG_TEST_CONST_PWM_MODE		2	// Enable constant PWM mode
#define REG_TEST_CURRENT			3	// Threshold current integer part (in mA x10) (for sensing configuration)
#define REG_TEST_VOLTAGE			4	// Test voltage (for sensing configuration)
//
// 10 - 19
#define REG_CAP_V_COFF_N			20	// Capacitor voltage coefficient (N)
#define REG_CAP_V_COFF_D			21	// Capacitor voltage coefficient (D)
// 22 - 23
#define REG_SCURRENT1_COFF_N		24	// Secondary current 1 coefficient (N)
#define REG_SCURRENT1_COFF_D		25	// Secondary current 1 coefficient (D)
#define REG_SCURRENT2_COFF_N		26	// Secondary current 2 coefficient (N)
#define REG_SCURRENT2_COFF_D		27	// Secondary current 2 coefficient (D)
#define REG_SVOLTAGE1_COFF_N		28	// Secondary voltage 1 coefficient (N)
#define REG_SVOLTAGE1_COFF_D		29	// Secondary voltage 1 coefficient (D)
#define REG_SVOLTAGE2_COFF_N		30	// Secondary voltage 2 coefficient (N)
#define REG_SVOLTAGE2_COFF_D		31	// Secondary voltage 2 coefficient (D)
// 32
#define REG_KP_VAC_N				33	// AC voltage amplitude controller P coefficient (N)
#define REG_KP_VAC_D				34	// AC voltage amplitude controller P coefficient (D)
#define REG_KI_VAC_N				35	// AC voltage amplitude controller I coefficient (N)
#define REG_KI_VAC_D				36	// AC voltage amplitude controller I coefficient (D)
#define REG_SCURRENT3_COFF_N		37	// Secondary current 3 coefficient (N)
#define REG_SCURRENT3_COFF_D		38	// Secondary current 3 coefficient (D)
// 39 - 41
#define REG_BRAKE_TIME				42	// Brake time (in ms)
#define REG_TRANSFORMER_COFF		43	// Transformer V transfer ratio (secondary/primary)
#define REG_PRIM_V_LOW_RANGE		44	// Low range voltage on primary side (in V)
#define REG_PRIM_V_FULL_RANGE		45	// Full range voltage on primary side (in V)
#define REG_USE_CUSTOM_PRIM_V		46	// Switch to custom voltage value on primary side
#define REG_PRIM_V_CUSTOM			47	// Custom voltage on primary side (in V)
// 48
#define REG_OPTO_CONNECTION_MON		49	// Optical connection error when N packets lost (0 to disable)
#define REG_SKIP_LOGGING_VOIDS		50	// Don't log empty zones in case of using frequency divisor
// 51 - 79
#define REG_SAFE_MAX_PWM			80	// Maximum PWM (AC mode)
#define REG_USE_INST_METHOD			81	// Measurement method
#define REG_REPLACE_CURVES			82	// Replace output V/I curves by peak measurement
#define REG_PEAK_SEARCH_ZONE		83	// % of peak voltage to search max current (in %)
// 84 - 85
#define REG_MODIFY_SINE				86	// Enable sine modification at low currents
#define REG_SKIP_NEG_LOGGING		87	// Skip negative pulses logging
#define REG_MODIFY_SINE_SHIFT		88	// Modified sine sample point shift from pwm peak (in ticks)
// 89 - 95
#define REG_SCURRENT1_FINE_P2		96	// Secondary current 1 tune quadratic coefficient P2 x1e6
#define REG_SCURRENT1_FINE_P1		97	// Secondary current 1 tune quadratic coefficient P1 x1000
#define REG_SCURRENT2_FINE_P2		98	// Secondary current 2 tune quadratic coefficient P2 x1e6
#define REG_SCURRENT2_FINE_P1		99	// Secondary current 2 tune quadratic coefficient P1 x1000
// 100 - 103
#define REG_SVOLTAGE1_FINE_P2		104	// Secondary voltage 1 tune quadratic coefficient P2 x1e6
#define REG_SVOLTAGE1_FINE_P1		105	// Secondary voltage 1 tune quadratic coefficient P1 x1000
#define REG_SVOLTAGE2_FINE_P2		106	// Secondary voltage 2 tune quadratic coefficient P2 x1e6
#define REG_SVOLTAGE2_FINE_P1		107	// Secondary voltage 2 tune quadratic coefficient P1 x1000
// 108 - 111
#define REG_SCURRENT3_FINE_P2		112	// Secondary current 3 tune quadratic coefficient P2 x1e6
#define REG_SCURRENT3_FINE_P1		113	// Secondary current 3 tune quadratic coefficient P1 x1000
#define REG_SVOLTAGE1_FINE_P0		114	// Secondary voltage 1 tune quadratic coefficient P0 (in V x10)
#define REG_SVOLTAGE2_FINE_P0		115	// Secondary voltage 2 tune quadratic coefficient P0 (in V x10)
#define REG_SCURRENT1_FINE_P0		116	// Secondary current 1 tune quadratic coefficient P0 (in uA)
#define REG_SCURRENT2_FINE_P0		117	// Secondary current 2 tune quadratic coefficient P0 (in uA)
#define REG_SCURRENT3_FINE_P0		118	// Secondary current 3 tune quadratic coefficient P0 (in uA)
//
// ----------------------------------------
//
#define REG_MEASUREMENT_TYPE		128	// Measurement type
#define REG_MEASUREMENT_MODE		129	// Measurement mode (V-mode or I-mode)
#define REG_LIMIT_CURRENT			130	// Threshold current (in mA * 10)
#define REG_LIMIT_VOLTAGE			131	// Maximum or test voltage (in V)
#define REG_VOLTAGE_PLATE_TIME		132	// Voltage plate time in AC measurement mode (in ms)
#define REG_VOLTAGE_AC_RATE			133	// Rate of increasing AC voltage (in kV/s * 10)
#define REG_START_VOLTAGE_AC		134	// Start voltage for AC modes (in V)
#define REG_VOLTAGE_FREQUENCY		135	// Voltage frequency (in Hz)
#define REG_FREQUENCY_DIVISOR		136	// Divisor for voltage pulses rate
#define REG_SCOPE_RATE				137	// Scope rate divisor
//
#define REG_DBG_SRAM				170	// Write saw-shape debug sequence to SRAM
#define REG_DBG_MUTE_PWM			171	// Mute PWM output
#define REG_DBG_DUAL_POLARITY		172	// Use data points of both signs
#define REG_DBG_READ_XY_FRAGMENT	173	// Fragment length for XY plot
//
#define REG_PWD_1					180	// Unlock password location 1
#define REG_PWD_2					181	// Unlock password location 2
#define REG_PWD_3					182	// Unlock password location 3
#define REG_PWD_4					183	// Unlock password location 4
//
// ----------------------------------------
//
#define REG_DEV_STATE				192	// Device state
#define REG_FAULT_REASON			193	// Fault reason in the case DeviceState -> FAULT
#define REG_DISABLE_REASON			194	// Fault reason in the case DeviceState -> DISABLED
#define REG_WARNING					195	// Warning if present
#define REG_PROBLEM					196	// Problem reason
#define REG_FINISHED				197	// Indicates that test is done and there is result or fault
#define REG_RESULT_V				198	// Test result (in V)
#define REG_RESULT_I				199	// Test result (mA * 10 or uA)
#define REG_RESULT_I_UA_R			200	// Resistance result R for DC-mode (in MOhm * 10) or Test result mA fraction for AC mode (in uA)
#define REG_VOLTAGE_ON_PLATE		201	// Indicates voltage plate region
//
#define REG_ACTUAL_PRIM_VOLTAGE		210	// Primary side capacitor voltage based on sensing (in V)
#define REG_PRIM_VOLTAGE_CTRL		211	// Primary side capacitor voltage used by control system (in V)
//
#define REG_CAN_BUSOFF_COUNTER		220	// Counter of bus-off states
#define REG_CAN_STATUS_REG			221	// CAN status register (32 bit)
#define REG_CAN_STATUS_REG_32		222
#define REG_CAN_DIAG_TEC			223	// CAN TEC
#define REG_CAN_DIAG_REC			224	// CAN REC
#define REG_SPI_RX_RESETS			225	// Counter for SPI Rx resets
//
#define REG_DIAG_PING_RESULT		250	// Digitizer ping result
//
#define REG_QUADRATIC_CORR			254	// Use quadratic correction for block
//
// ----------------------------------------
//
#define REG_FWINFO_SLAVE_NID		256	// Device CAN slave node ID
#define REG_FWINFO_MASTER_NID		257	// Device CAN master node ID (if presented)
// 258 - 259
#define REG_FWINFO_STR_LEN			260	// Length of the information string record
#define REG_FWINFO_STR_BEGIN		261	// Begining of the information string record
//
#define REG_MEM_SYMBOL				299	// Считанный по адресу памяти символ


// ENDPOINTS
//
#define EP16_I						1	// Endpoint for I value sequence
#define EP16_V						2	// Endpoint for V value sequence
#define EP16_DIAG					3	// Endpoint for PWM & other value sequence
#define EP16_ERR					4	// Endpoint for V following error
#define EP16_PEAK_I					5	// Endpoint for peak I value sequence
#define EP16_PEAK_V					6	// Endpoint for peak V value sequence

// MEASUREMENT TYPE
//
#define MEASUREMENT_TYPE_NONE		0	// None
#define MEASUREMENT_TYPE_AC			1	// AC full wave
#define MEASUREMENT_TYPE_AC_D		2	// AC positive polarity
#define MEASUREMENT_TYPE_AC_R		3	// AC negative polarity
#define MEASUREMENT_TYPE_TEST		6	// Test sine wave

// OPERATION RESULTS
//
#define OPRESULT_NONE				0	// No information or not finished
#define OPRESULT_OK					1	// Operation was successful
#define OPRESULT_FAIL				2	// Operation failed

// PROBLEM CODES
//
#define PROBLEM_NONE				0	// No problem
#define PROBLEM_STOP				100	// Stop by user command
#define PROBLEM_OUTPUT_SHORT		101	// Output short circuit

// FAULT & DISABLE
//
#define DF_NONE						0	// No faults
#define DF_BRIDGE_SHORT				200	// Bridge current overload
#define DF_TEMP_MON					201	// Bridge temperature overload
#define DF_OPTO_CON_ERROR			202	// Optical connection error
#define DF_LOW_SIDE_PS				203	// Low-side power supply fault
#define DF_FOLLOWING_ERROR			204	// Voltage following error
#define DF_PWM_SATURATION			205	// Detected PWM output saturation
#define DF_INTERNAL					0xFFFF

// Start-up check
//
#define DISABLE_NO_SHORT_SIGNAL		300	// Short signal low or disconnected on startup
#define DISABLE_NO_TEMP_SIGNAL		301	// Temperature signal low or disconnected on startup

// WARNING CODES
//
#define WARNING_NONE				0	// No warning
#define WARNING_CURRENT_NOT_REACHED 401	// No trip condition detected in I-mode
#define WARNING_VOLTAGE_NOT_REACHED 402	// Trip condition detected in V-mode
#define WARNING_RES_OUT_OF_RANGE	403	// Resistance is too low or too high
#define WARNING_OUTPUT_OVERLOAD		404	// Output overload
//
#define WARNING_WATCHDOG_RESET		1001	// System has been reseted by WD

// USER ERROR CODES
//
#define ERR_NONE					0	// No error
#define ERR_CONFIGURATION_LOCKED	1	// Device is locked for writing
#define ERR_OPERATION_BLOCKED		2	// Operation can't be done due to current device state
#define ERR_DEVICE_NOT_READY		3	// Device isn't ready to switch state
#define ERR_WRONG_PWD				4	// Wrong password - unlock failed


#endif	// __DEV_OBJ_DIC_H
