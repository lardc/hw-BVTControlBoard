// -----------------------------------------
// Global definitions
// ----------------------------------------

// Header
#include "Constraints.h"
#include "DeviceObjectDictionary.h"
#include "PowerDriver.h"
#include "ZwPWM.h"

#define NO		0	// equal to FALSE
#define YES		1	// equal to TRUE

// Constants
//
const TableItemConstraint NVConstraint[DATA_TABLE_NV_SIZE] =
									  {
										   {0, ZW_PWM_DUTY_BASE, 0},												// 0
										   {1, 1000, VOLTAGE_FREQUENCY_DEF},										// 1
										   {NO, YES, NO},															// 2
										   {0, TEST_CURRENT_MAX, 0},												// 3
										   {0, LIMIT_VOLTAGE_MAX, 0},												// 4
										   {0, 0, 0},																// 5
										   {0, 0, 0},																// 6
										   {0, 0, 0},																// 7
										   {0, 0, 0},																// 8
										   {0, 0, 0},																// 9
										   {0, 0, 0},																// 10
										   {0, 0, 0},																// 11
										   {0, 0, 0},																// 12
										   {0, 0, 0},																// 13
										   {0, 0, 0},																// 14
										   {0, 0, 0},																// 15
										   {0, 0, 0},																// 16
										   {0, 0, 0},																// 17
										   {0, 0, 0},																// 18
										   {0, 0, 0},																// 19
										   {0, INT16U_MAX, CAP_V_COFF_N_DEF},										// 20
										   {1, X_D_DEF3, X_D_DEF2},													// 21
										   {0, 0, 0},																// 22
										   {0, 0, 0},																// 23
										   {0, INT16U_MAX, SCURRENT1_COFF_N_DEF},									// 24
										   {1, X_D_DEF3, X_D_DEF2},													// 25
										   {0, INT16U_MAX, SCURRENT2_COFF_N_DEF},									// 26
										   {1, X_D_DEF3, X_D_DEF3},													// 27
										   {0, INT16U_MAX, SVOLTAGE1_COFF_N_DEF},									// 28
										   {1, X_D_DEF3, X_D_DEF2},													// 29
										   {0, INT16U_MAX, SVOLTAGE2_COFF_N_DEF},									// 30
										   {1, X_D_DEF3, X_D_DEF2},													// 31
										   {0, 0, 0},																// 32
										   {0, INT16U_MAX, 0},														// 33
										   {1, X_D_DEF3, X_D_DEF1},													// 34
										   {0, INT16U_MAX, 0},														// 35
										   {1, X_D_DEF3, X_D_DEF1},													// 36
										   {0, INT16U_MAX, SCURRENT_DCL_COFF_N_DEF},								// 37
										   {1, X_D_DEF3, X_D_DEF2},													// 38
										   {0, INT16U_MAX, SCURRENT_DCM_COFF_N_DEF},								// 39
										   {1, X_D_DEF3, X_D_DEF2},													// 40
										   {0, 0, 0},																// 41
										   {BRAKE_TIME_MIN, BRAKE_TIME_MAX, BRAKE_TIME_DEF},						// 42
										   {TRANSFORMER_COFF_MIN, TRANSFORMER_COFF_MAX, TRANSFORMER_COFF_DEF}, 		// 43
										   {0, 0, 0},																// 44
										   {0, 0, 0},																// 45
										   {NO, YES, NO},															// 46
										   {NOMINAL_PRIMARY_V_MIN, NOMINAL_PRIMARY_V_MAX, NOMINAL_PRIMARY_V_DEF},	// 47
										   {0, 0, 0},																// 48
										   {OPTO_MON_MIN, OPTO_MON_MAX, OPTO_MON_DEF},								// 49
										   {NO, YES, NO},															// 50
										   {0, 0, 0},																// 51
										   {0, 0, 0},																// 52
										   {0, 0, 0},																// 53
										   {0, 0, 0},																// 54
										   {0, 0, 0},																// 55
										   {0, 0, 0},																// 56
										   {0, 0, 0},																// 57
										   {0, 0, 0},																// 58
										   {0, 0, 0},																// 59
										   {0, 0, 0},																// 60
										   {0, 0, 0},																// 61
										   {0, 0, 0},																// 62
										   {0, 0, 0},																// 63
										   {0, 0, 0},																// 64
										   {0, 0, 0},																// 65
										   {0, 0, 0},																// 66
										   {0, 0, 0},																// 67
										   {0, 0, 0},																// 68
										   {0, 0, 0},																// 69
										   {0, 0, 0},																// 70
										   {0, 0, 0},																// 71
										   {0, 0, 0},																// 72
										   {0, 0, 0},																// 73
										   {0, 0, 0},																// 74
										   {0, 0, 0},																// 75
										   {0, 0, 0},																// 76
										   {0, 0, 0},																// 77
										   {0, 0, 0},																// 78
										   {0, 0, 0},																// 79
										   {0, ZW_PWM_DUTY_BASE, ZW_PWM_DUTY_BASE},									// 80
										   {NO, YES, YES},															// 81
										   {NO, YES, NO},															// 82
										   {0, 0, 0},																// 83
										   {0, 0, 0},																// 84
										   {1, POWER_OPTIONS_MAXNUM, POWER_OPTIONS_MAXNUM},							// 85
										   {NO, YES, NO},															// 86
										   {0, 0, 0},																// 87
										   {0, 0, 0},																// 88
										   {0, 0, 0},																// 89
										   {0, 0, 0},																// 90
										   {0, 0, 0},																// 91
										   {0, 0, 0},																// 92
										   {0, 0, 0},																// 93
										   {0, 0, 0},																// 94
										   {0, 0, 0},																// 95
										   {0, INT16U_MAX, 0},														// 96
										   {0, INT16U_MAX, 1000},													// 97
										   {0, INT16U_MAX, 0},														// 98
										   {0, INT16U_MAX, 1000},													// 99
										   {0, 0, 0},																// 100
										   {0, 0, 0},																// 101
										   {0, 0, 0},																// 102
										   {0, 0, 0},																// 103
										   {0, INT16U_MAX, 0},														// 104
										   {0, INT16U_MAX, 1000},													// 105
										   {0, INT16U_MAX, 0},														// 106
										   {0, INT16U_MAX, 1000},													// 107
										   {0, INT16U_MAX, 0},														// 108
										   {0, INT16U_MAX, 1000},													// 109
										   {0, INT16U_MAX, 0},														// 110
										   {0, 0, 0},																// 111
										   {0, INT16U_MAX, 0},														// 112
										   {0, INT16U_MAX, 1000},													// 113
										   {0, INT16U_MAX, 0},														// 114
										   {0, INT16U_MAX, 0},														// 115
										   {0, INT16U_MAX, 0},														// 116
										   {0, INT16U_MAX, 0},														// 117
										   {0, INT16U_MAX, 0},														// 118
										   {0, INT16U_MAX, 0},														// 119
										   {0, 0, 0},																// 120
										   {0, 0, 0},																// 121
										   {0, 0, 0},																// 122
										   {0, 0, 0},																// 123
										   {0, 0, 0},																// 124
										   {0, 0, 0},																// 125
										   {0, 0, 0},																// 126
										   {0, 0, 0}																// 127
									  };

const TableItemConstraint VConstraint[DATA_TABLE_WP_START - DATA_TABLE_WR_START] =
									 {
										   {MEASUREMENT_TYPE_NONE, MEASUREMENT_TYPE_TEST, MEASUREMENT_TYPE_AC_D},	// 128
										   {0, 1, 0},																// 129
										   {0, TEST_CURRENT_MAX, 0},												// 130
										   {LIMIT_VOLTAGE_MIN, LIMIT_VOLTAGE_MAX, LIMIT_VOLTAGE_DEF},				// 131
										   {0, VPLATE_TIME_MAX, 0},													// 132
										   {RATE_VAC_MIN, RATE_VAC_MAX, RATE_VAC_DEF},								// 133
										   {START_VAC_MIN, START_VAC_MAX, START_VAC_DEF},							// 134
										   {VOLTAGE_FREQUENCY_MIN, VOLTAGE_FREQUENCY_MAX, VOLTAGE_FREQUENCY_DEF},	// 135
										   {VOLTAGE_DIV_MIN, VOLTAGE_DIV_MAX, VOLTAGE_DIV_DEF},						// 136
										   {0, SCOPE_RATE_MAX, SCOPE_RATE_DEF},										// 137
										   {0, 0, 0},																// 138
										   {0, 0, 0},																// 139
										   {0, 0, 0},																// 140
										   {0, 0, 0},																// 141
										   {0, 0, 0},																// 142
										   {0, 0, 0},																// 143
										   {0, 0, 0},																// 144
										   {0, 0, 0},																// 145
										   {0, 0, 0},																// 146
										   {0, 0, 0},																// 147
										   {0, 0, 0},																// 148
										   {0, 0, 0},																// 149
										   {0, 0, 0},																// 150
										   {0, 0, 0},																// 151
										   {0, 0, 0},																// 152
										   {0, 0, 0},																// 153
										   {0, 0, 0},																// 154
										   {0, 0, 0},																// 155
										   {0, 0, 0},																// 156
										   {0, 0, 0},																// 157
										   {0, 0, 0},																// 158
										   {0, 0, 0},																// 159
										   {0, 0, 0},																// 160
										   {0, 0, 0},																// 161
										   {0, 0, 0},																// 162
										   {0, 0, 0},																// 163
										   {0, 0, 0},																// 164
										   {0, 0, 0},																// 165
										   {0, 0, 0},																// 166
										   {0, 0, 0},																// 167
										   {0, 0, 0},																// 168
										   {0, 0, 0},																// 169
										   {NO, YES, NO},															// 170
										   {NO, YES, NO},															// 171
										   {NO, YES, NO},															// 172
										   {0, INT16S_MAX, LAST_FRAG_SIZE_DEF},										// 173
										   {0, 0, 0},																// 174
										   {0, 0, 0},																// 175
										   {0, 0, 0},																// 176
										   {0, 0, 0},																// 177
										   {0, 0, 0},																// 178
										   {0, 0, 0},																// 179
										   {0, INT16U_MAX, 0},														// 180
										   {0, INT16U_MAX, 0},														// 181
										   {0, INT16U_MAX, 0},														// 182
										   {0, INT16U_MAX, 0},														// 183
										   {0, 0, 0},																// 184
										   {0, 0, 0},																// 185
										   {0, 0, 0},																// 186
										   {0, 0, 0},																// 187
										   {0, 0, 0},																// 188
										   {0, 0, 0},																// 189
										   {0, 0, 0},																// 190
										   {0, 0, 0},																// 191
									 };
