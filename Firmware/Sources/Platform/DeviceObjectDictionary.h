#ifndef __DEV_OBJ_DIC_H
#define __DEV_OBJ_DIC_H


// ACTIONS
//
#define ACT_ENABLE_POWER			1	// Enable charging cells
#define ACT_DISABLE_POWER			2	// Disable charging cells
#define ACT_CLR_FAULT				3	// Clear fault (try switch state from FAULT to NONE)
#define ACT_CLR_WARNING				4	// Clear warning flag
//
#define ACT_DBG_DIGI_GET_PACKET		10	// Считать последний принятый пакет от HVDigitizer
#define ACT_DBG_DIGI_PING			11	// Ping optical connection
#define ACT_DBG_DIGI_SAMPLE			12	// Request ADC sample
//
#define ACT_START_TEST				100	// Start test with defined parameters
#define ACT_STOP					101	// Stop test sequence
#define ACT_READ_FRAGMENT			110	// Read next data portion
//
#define ACT_SAVE_TO_ROM				200	// Save parameters to EEPROM module
#define ACT_RESTORE_FROM_ROM		201	// Restore parameters from EEPROM module
#define ACT_RESET_TO_DEFAULT		202	// Reset parameters to default values (only in controller memory)
//
#define ACT_BOOT_LOADER_REQUEST		320	// Request reboot to bootloader


// REGISTERS
//
#define REG_RAW_ZERO_SVOLTAGE		0	// Значение нулевого уровня канала напряжения в тиках оцифровки
#define REG_RAW_ZERO_SCURRENT		1	// Значение нулевого уровня канала тока в тиках оцифровки

#define REG_COEFF_VOLTAGE_K			2	// Пропорциональный коэффициент пересёта напряжения
#define REG_COEFF_VOLTAGE_D			3	// Делитель пропорционального коэффициента x0.1
#define REG_COEFF_VOLTAGE_P2		4	// Коэффициент тонкой подстройки напряжения P2 x1e6
#define REG_COEFF_VOLTAGE_P1		5	// Коэффициент тонкой подстройки напряжения P1 x1000
#define REG_COEFF_VOLTAGE_P0		6	// Коэффициент тонкой подстройки напряжения P0 x10

#define REG_COEFF_CURRENT1_K		7	// Пропорциональный коэффициент пересёта напряжения х1000
#define REG_COEFF_CURRENT1_D		8	// Делитель пропорционального коэффициента x0.1
#define REG_COEFF_CURRENT1_P2		9	// Коэффициент тонкой подстройки тока 1 P2 x1e6
#define REG_COEFF_CURRENT1_P1		10	// Коэффициент тонкой подстройки тока 1 P1 x1000
#define REG_COEFF_CURRENT1_P0		11	// Коэффициент тонкой подстройки тока 1 P0 x10

#define REG_COEFF_CURRENT2_K		12	// Пропорциональный коэффициент пересёта напряжения х1000
#define REG_COEFF_CURRENT2_D		13	// Делитель пропорционального коэффициента x0.1
#define REG_COEFF_CURRENT2_P2		14	// Коэффициент тонкой подстройки тока 2 P2 x1e6
#define REG_COEFF_CURRENT2_P1		15	// Коэффициент тонкой подстройки тока 2 P1 x1000
#define REG_COEFF_CURRENT2_P0		16	// Коэффициент тонкой подстройки тока 2 P0 x10

#define REG_COEFF_CURRENT3_K		17	// Пропорциональный коэффициент пересёта напряжения х1000
#define REG_COEFF_CURRENT3_D		18	// Делитель пропорционального коэффициента x0.1
#define REG_COEFF_CURRENT3_P2		19	// Коэффициент тонкой подстройки тока 3 P2 x1e6
#define REG_COEFF_CURRENT3_P1		20	// Коэффициент тонкой подстройки тока 3 P1 x1000
#define REG_COEFF_CURRENT3_P0		21	// Коэффициент тонкой подстройки тока 3 P0 x10

#define REG_CAP_COEFF				22	// Коэффициент пересчёта напряжения первичной стороны х1000

#define REG_START_VOLTAGE			23	// Стартовое напряжение формирования (в В)
#define REG_VOLTAGE_RATE			24	// Скорость нарастания напряжения (в кВ/с х10)

#define REG_KP						25	// Пропорциональный коэффициент регулятора х100
#define REG_KI						26	// Интегральный коэффициент регулятора х100

#define REG_FE_ABSOLUTE				27	// Пороговая абсолютная ошибка регулирования действующего напряжения (в В)
#define REG_FE_RELATIVE				28	// Пороговая относительная ошибка регулирования действующего напряжения (в %)
#define REG_FE_COUNTER_MAX			29	// Максимальное количество срабатываний ошибки следования

#define REG_TRANSFORMER_COFF		30	// Коэффициент трансформации
#define REG_PRIM_VOLTAGE			31	// Напряжение первичной сторны (в В)
#define REG_PRIM_IGNORE_CHECK		32	// Не проверять уровень напряжения первичной стороны (в В)
//
// ----------------------------------------
//
#define REG_TARGET_VOLTAGE			128	// Действующее напряжение уставки (в В)
#define REG_LIMIT_CURRENT_mA		129	// Предельный ток (часть мА)
#define REG_LIMIT_CURRENT_uA		130	// Предельный ток (часть мкА)
#define REG_ACTIVE_MODE				131	// Режим измерения активной составляющей тока
#define REG_VOLTAGE_PLATE_TIME		132	// Длительность формирования полки (в сек)
#define REG_SCOPE_RATE				133	// Делитель логгирования данных
//
#define REG_DBG_SRAM				170	// Write saw-shape debug sequence to SRAM
#define REG_DBG_MUTE_PWM			171	// Mute PWM output
//
// ----------------------------------------
//
#define REG_DEV_STATE				192	// Device state
#define REG_FAULT_REASON			193	// Fault reason in the case DeviceState -> FAULT
#define REG_DISABLE_REASON			194	// Fault reason in the case DeviceState -> DISABLED
#define REG_WARNING					195	// Warning if present
#define REG_PROBLEM					196	// Problem reason
#define REG_FINISHED				197	// Indicates that test is done and there is result or fault
//
#define REG_RESULT_V				200	// Test result (in V)
#define REG_RESULT_I_mA				201	// Test result (mA part)
#define REG_RESULT_I_uA				202	// Test result (uA part)
//
#define REG_ACTUAL_PRIM_VOLTAGE		210	// Primary side capacitor voltage based on sensing (in V)
//
#define REG_CAN_BUSOFF_COUNTER		220	// Counter of bus-off states
#define REG_CAN_STATUS_REG			221	// CAN status register (32 bit)
#define REG_CAN_STATUS_REG_32		222
#define REG_CAN_DIAG_TEC			223	// CAN TEC
#define REG_CAN_DIAG_REC			224	// CAN REC
#define REG_SPI_RX_RESETS			225	// Counter for SPI Rx resets
//
#define REG_DIAG_DIGI_RESULT		230	// Результат выполнения команды по оптическому интерфейсу
#define REG_DIAG_DIGI_PACKET_B1		231	// Байт 1 пакета, полученного от HVDigitizer
#define REG_DIAG_DIGI_PACKET_B2		232	// Байт 2 пакета, полученного от HVDigitizer
#define REG_DIAG_DIGI_PACKET_B3		233	// Байт 3 пакета, полученного от HVDigitizer
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


// ENDPOINTS
//
#define EP16_V						1	// Endpoint for V value sequence
#define EP16_ImA					2	// Endpoint for I value sequence mA part
#define EP16_IuA					3	// Endpoint for I value sequence uA part
#define EP16_Vrms					4	// Endpoint for Vrms value sequence
#define EP16_Irms_mA				5	// Endpoint for Irms value sequence mA part
#define EP16_Irms_uA				6	// Endpoint for Irms value sequence uA part
#define EP16_PWM					7	// Endpoint for PWM value sequence
#define EP16_Error					8	// Endpoint for V following error

// OPERATION RESULTS
//
#define OPRESULT_NONE				0	// No information or not finished
#define OPRESULT_OK					1	// Operation was successful
#define OPRESULT_FAIL				2	// Operation failed

// PROBLEM CODES
//
#define PROBLEM_NONE				0	// No problem
#define PROBLEM_STOP				1	// Stop by user command
#define PROBLEM_FOLLOWING_ERROR		2	// Following error
#define PROBLEM_PWM_SATURATION		3	// PWM reached upper limit

// FAULT & DISABLE
//
#define DF_NONE						0	// No fault
#define DF_OPTICAL_INTERFACE		1	// Optical interface failure
#define DF_PRIMARY_VOLTAGE			2	// Primary voltage failure


// WARNING CODES
//
#define WARNING_NONE				0	// No warning
#define WARNING_WATCHDOG_RESET		1001	// System has been reseted by WD

// USER ERROR CODES
//
#define ERR_NONE					0	// No error
#define ERR_CONFIGURATION_LOCKED	1	// Device is locked for writing
#define ERR_OPERATION_BLOCKED		2	// Operation can't be done due to current device state
#define ERR_DEVICE_NOT_READY		3	// Device isn't ready to switch state


#endif	// __DEV_OBJ_DIC_H
