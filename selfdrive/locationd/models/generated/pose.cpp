#include "pose.h"

namespace {
#define DIM 18
#define EDIM 18
#define MEDIM 18
typedef void (*Hfun)(double *, double *, double *);
const static double MAHA_THRESH_4 = 7.814727903251177;
const static double MAHA_THRESH_10 = 7.814727903251177;
const static double MAHA_THRESH_13 = 7.814727903251177;
const static double MAHA_THRESH_14 = 7.814727903251177;

/******************************************************************************
 *                      Code generated with SymPy 1.14.0                      *
 *                                                                            *
 *              See http://www.sympy.org/ for more information.               *
 *                                                                            *
 *                         This file is part of 'ekf'                         *
 ******************************************************************************/
void err_fun(double *nom_x, double *delta_x, double *out_8294194842749133858) {
   out_8294194842749133858[0] = delta_x[0] + nom_x[0];
   out_8294194842749133858[1] = delta_x[1] + nom_x[1];
   out_8294194842749133858[2] = delta_x[2] + nom_x[2];
   out_8294194842749133858[3] = delta_x[3] + nom_x[3];
   out_8294194842749133858[4] = delta_x[4] + nom_x[4];
   out_8294194842749133858[5] = delta_x[5] + nom_x[5];
   out_8294194842749133858[6] = delta_x[6] + nom_x[6];
   out_8294194842749133858[7] = delta_x[7] + nom_x[7];
   out_8294194842749133858[8] = delta_x[8] + nom_x[8];
   out_8294194842749133858[9] = delta_x[9] + nom_x[9];
   out_8294194842749133858[10] = delta_x[10] + nom_x[10];
   out_8294194842749133858[11] = delta_x[11] + nom_x[11];
   out_8294194842749133858[12] = delta_x[12] + nom_x[12];
   out_8294194842749133858[13] = delta_x[13] + nom_x[13];
   out_8294194842749133858[14] = delta_x[14] + nom_x[14];
   out_8294194842749133858[15] = delta_x[15] + nom_x[15];
   out_8294194842749133858[16] = delta_x[16] + nom_x[16];
   out_8294194842749133858[17] = delta_x[17] + nom_x[17];
}
void inv_err_fun(double *nom_x, double *true_x, double *out_1479878567395019915) {
   out_1479878567395019915[0] = -nom_x[0] + true_x[0];
   out_1479878567395019915[1] = -nom_x[1] + true_x[1];
   out_1479878567395019915[2] = -nom_x[2] + true_x[2];
   out_1479878567395019915[3] = -nom_x[3] + true_x[3];
   out_1479878567395019915[4] = -nom_x[4] + true_x[4];
   out_1479878567395019915[5] = -nom_x[5] + true_x[5];
   out_1479878567395019915[6] = -nom_x[6] + true_x[6];
   out_1479878567395019915[7] = -nom_x[7] + true_x[7];
   out_1479878567395019915[8] = -nom_x[8] + true_x[8];
   out_1479878567395019915[9] = -nom_x[9] + true_x[9];
   out_1479878567395019915[10] = -nom_x[10] + true_x[10];
   out_1479878567395019915[11] = -nom_x[11] + true_x[11];
   out_1479878567395019915[12] = -nom_x[12] + true_x[12];
   out_1479878567395019915[13] = -nom_x[13] + true_x[13];
   out_1479878567395019915[14] = -nom_x[14] + true_x[14];
   out_1479878567395019915[15] = -nom_x[15] + true_x[15];
   out_1479878567395019915[16] = -nom_x[16] + true_x[16];
   out_1479878567395019915[17] = -nom_x[17] + true_x[17];
}
void H_mod_fun(double *state, double *out_2565599195015806522) {
   out_2565599195015806522[0] = 1.0;
   out_2565599195015806522[1] = 0.0;
   out_2565599195015806522[2] = 0.0;
   out_2565599195015806522[3] = 0.0;
   out_2565599195015806522[4] = 0.0;
   out_2565599195015806522[5] = 0.0;
   out_2565599195015806522[6] = 0.0;
   out_2565599195015806522[7] = 0.0;
   out_2565599195015806522[8] = 0.0;
   out_2565599195015806522[9] = 0.0;
   out_2565599195015806522[10] = 0.0;
   out_2565599195015806522[11] = 0.0;
   out_2565599195015806522[12] = 0.0;
   out_2565599195015806522[13] = 0.0;
   out_2565599195015806522[14] = 0.0;
   out_2565599195015806522[15] = 0.0;
   out_2565599195015806522[16] = 0.0;
   out_2565599195015806522[17] = 0.0;
   out_2565599195015806522[18] = 0.0;
   out_2565599195015806522[19] = 1.0;
   out_2565599195015806522[20] = 0.0;
   out_2565599195015806522[21] = 0.0;
   out_2565599195015806522[22] = 0.0;
   out_2565599195015806522[23] = 0.0;
   out_2565599195015806522[24] = 0.0;
   out_2565599195015806522[25] = 0.0;
   out_2565599195015806522[26] = 0.0;
   out_2565599195015806522[27] = 0.0;
   out_2565599195015806522[28] = 0.0;
   out_2565599195015806522[29] = 0.0;
   out_2565599195015806522[30] = 0.0;
   out_2565599195015806522[31] = 0.0;
   out_2565599195015806522[32] = 0.0;
   out_2565599195015806522[33] = 0.0;
   out_2565599195015806522[34] = 0.0;
   out_2565599195015806522[35] = 0.0;
   out_2565599195015806522[36] = 0.0;
   out_2565599195015806522[37] = 0.0;
   out_2565599195015806522[38] = 1.0;
   out_2565599195015806522[39] = 0.0;
   out_2565599195015806522[40] = 0.0;
   out_2565599195015806522[41] = 0.0;
   out_2565599195015806522[42] = 0.0;
   out_2565599195015806522[43] = 0.0;
   out_2565599195015806522[44] = 0.0;
   out_2565599195015806522[45] = 0.0;
   out_2565599195015806522[46] = 0.0;
   out_2565599195015806522[47] = 0.0;
   out_2565599195015806522[48] = 0.0;
   out_2565599195015806522[49] = 0.0;
   out_2565599195015806522[50] = 0.0;
   out_2565599195015806522[51] = 0.0;
   out_2565599195015806522[52] = 0.0;
   out_2565599195015806522[53] = 0.0;
   out_2565599195015806522[54] = 0.0;
   out_2565599195015806522[55] = 0.0;
   out_2565599195015806522[56] = 0.0;
   out_2565599195015806522[57] = 1.0;
   out_2565599195015806522[58] = 0.0;
   out_2565599195015806522[59] = 0.0;
   out_2565599195015806522[60] = 0.0;
   out_2565599195015806522[61] = 0.0;
   out_2565599195015806522[62] = 0.0;
   out_2565599195015806522[63] = 0.0;
   out_2565599195015806522[64] = 0.0;
   out_2565599195015806522[65] = 0.0;
   out_2565599195015806522[66] = 0.0;
   out_2565599195015806522[67] = 0.0;
   out_2565599195015806522[68] = 0.0;
   out_2565599195015806522[69] = 0.0;
   out_2565599195015806522[70] = 0.0;
   out_2565599195015806522[71] = 0.0;
   out_2565599195015806522[72] = 0.0;
   out_2565599195015806522[73] = 0.0;
   out_2565599195015806522[74] = 0.0;
   out_2565599195015806522[75] = 0.0;
   out_2565599195015806522[76] = 1.0;
   out_2565599195015806522[77] = 0.0;
   out_2565599195015806522[78] = 0.0;
   out_2565599195015806522[79] = 0.0;
   out_2565599195015806522[80] = 0.0;
   out_2565599195015806522[81] = 0.0;
   out_2565599195015806522[82] = 0.0;
   out_2565599195015806522[83] = 0.0;
   out_2565599195015806522[84] = 0.0;
   out_2565599195015806522[85] = 0.0;
   out_2565599195015806522[86] = 0.0;
   out_2565599195015806522[87] = 0.0;
   out_2565599195015806522[88] = 0.0;
   out_2565599195015806522[89] = 0.0;
   out_2565599195015806522[90] = 0.0;
   out_2565599195015806522[91] = 0.0;
   out_2565599195015806522[92] = 0.0;
   out_2565599195015806522[93] = 0.0;
   out_2565599195015806522[94] = 0.0;
   out_2565599195015806522[95] = 1.0;
   out_2565599195015806522[96] = 0.0;
   out_2565599195015806522[97] = 0.0;
   out_2565599195015806522[98] = 0.0;
   out_2565599195015806522[99] = 0.0;
   out_2565599195015806522[100] = 0.0;
   out_2565599195015806522[101] = 0.0;
   out_2565599195015806522[102] = 0.0;
   out_2565599195015806522[103] = 0.0;
   out_2565599195015806522[104] = 0.0;
   out_2565599195015806522[105] = 0.0;
   out_2565599195015806522[106] = 0.0;
   out_2565599195015806522[107] = 0.0;
   out_2565599195015806522[108] = 0.0;
   out_2565599195015806522[109] = 0.0;
   out_2565599195015806522[110] = 0.0;
   out_2565599195015806522[111] = 0.0;
   out_2565599195015806522[112] = 0.0;
   out_2565599195015806522[113] = 0.0;
   out_2565599195015806522[114] = 1.0;
   out_2565599195015806522[115] = 0.0;
   out_2565599195015806522[116] = 0.0;
   out_2565599195015806522[117] = 0.0;
   out_2565599195015806522[118] = 0.0;
   out_2565599195015806522[119] = 0.0;
   out_2565599195015806522[120] = 0.0;
   out_2565599195015806522[121] = 0.0;
   out_2565599195015806522[122] = 0.0;
   out_2565599195015806522[123] = 0.0;
   out_2565599195015806522[124] = 0.0;
   out_2565599195015806522[125] = 0.0;
   out_2565599195015806522[126] = 0.0;
   out_2565599195015806522[127] = 0.0;
   out_2565599195015806522[128] = 0.0;
   out_2565599195015806522[129] = 0.0;
   out_2565599195015806522[130] = 0.0;
   out_2565599195015806522[131] = 0.0;
   out_2565599195015806522[132] = 0.0;
   out_2565599195015806522[133] = 1.0;
   out_2565599195015806522[134] = 0.0;
   out_2565599195015806522[135] = 0.0;
   out_2565599195015806522[136] = 0.0;
   out_2565599195015806522[137] = 0.0;
   out_2565599195015806522[138] = 0.0;
   out_2565599195015806522[139] = 0.0;
   out_2565599195015806522[140] = 0.0;
   out_2565599195015806522[141] = 0.0;
   out_2565599195015806522[142] = 0.0;
   out_2565599195015806522[143] = 0.0;
   out_2565599195015806522[144] = 0.0;
   out_2565599195015806522[145] = 0.0;
   out_2565599195015806522[146] = 0.0;
   out_2565599195015806522[147] = 0.0;
   out_2565599195015806522[148] = 0.0;
   out_2565599195015806522[149] = 0.0;
   out_2565599195015806522[150] = 0.0;
   out_2565599195015806522[151] = 0.0;
   out_2565599195015806522[152] = 1.0;
   out_2565599195015806522[153] = 0.0;
   out_2565599195015806522[154] = 0.0;
   out_2565599195015806522[155] = 0.0;
   out_2565599195015806522[156] = 0.0;
   out_2565599195015806522[157] = 0.0;
   out_2565599195015806522[158] = 0.0;
   out_2565599195015806522[159] = 0.0;
   out_2565599195015806522[160] = 0.0;
   out_2565599195015806522[161] = 0.0;
   out_2565599195015806522[162] = 0.0;
   out_2565599195015806522[163] = 0.0;
   out_2565599195015806522[164] = 0.0;
   out_2565599195015806522[165] = 0.0;
   out_2565599195015806522[166] = 0.0;
   out_2565599195015806522[167] = 0.0;
   out_2565599195015806522[168] = 0.0;
   out_2565599195015806522[169] = 0.0;
   out_2565599195015806522[170] = 0.0;
   out_2565599195015806522[171] = 1.0;
   out_2565599195015806522[172] = 0.0;
   out_2565599195015806522[173] = 0.0;
   out_2565599195015806522[174] = 0.0;
   out_2565599195015806522[175] = 0.0;
   out_2565599195015806522[176] = 0.0;
   out_2565599195015806522[177] = 0.0;
   out_2565599195015806522[178] = 0.0;
   out_2565599195015806522[179] = 0.0;
   out_2565599195015806522[180] = 0.0;
   out_2565599195015806522[181] = 0.0;
   out_2565599195015806522[182] = 0.0;
   out_2565599195015806522[183] = 0.0;
   out_2565599195015806522[184] = 0.0;
   out_2565599195015806522[185] = 0.0;
   out_2565599195015806522[186] = 0.0;
   out_2565599195015806522[187] = 0.0;
   out_2565599195015806522[188] = 0.0;
   out_2565599195015806522[189] = 0.0;
   out_2565599195015806522[190] = 1.0;
   out_2565599195015806522[191] = 0.0;
   out_2565599195015806522[192] = 0.0;
   out_2565599195015806522[193] = 0.0;
   out_2565599195015806522[194] = 0.0;
   out_2565599195015806522[195] = 0.0;
   out_2565599195015806522[196] = 0.0;
   out_2565599195015806522[197] = 0.0;
   out_2565599195015806522[198] = 0.0;
   out_2565599195015806522[199] = 0.0;
   out_2565599195015806522[200] = 0.0;
   out_2565599195015806522[201] = 0.0;
   out_2565599195015806522[202] = 0.0;
   out_2565599195015806522[203] = 0.0;
   out_2565599195015806522[204] = 0.0;
   out_2565599195015806522[205] = 0.0;
   out_2565599195015806522[206] = 0.0;
   out_2565599195015806522[207] = 0.0;
   out_2565599195015806522[208] = 0.0;
   out_2565599195015806522[209] = 1.0;
   out_2565599195015806522[210] = 0.0;
   out_2565599195015806522[211] = 0.0;
   out_2565599195015806522[212] = 0.0;
   out_2565599195015806522[213] = 0.0;
   out_2565599195015806522[214] = 0.0;
   out_2565599195015806522[215] = 0.0;
   out_2565599195015806522[216] = 0.0;
   out_2565599195015806522[217] = 0.0;
   out_2565599195015806522[218] = 0.0;
   out_2565599195015806522[219] = 0.0;
   out_2565599195015806522[220] = 0.0;
   out_2565599195015806522[221] = 0.0;
   out_2565599195015806522[222] = 0.0;
   out_2565599195015806522[223] = 0.0;
   out_2565599195015806522[224] = 0.0;
   out_2565599195015806522[225] = 0.0;
   out_2565599195015806522[226] = 0.0;
   out_2565599195015806522[227] = 0.0;
   out_2565599195015806522[228] = 1.0;
   out_2565599195015806522[229] = 0.0;
   out_2565599195015806522[230] = 0.0;
   out_2565599195015806522[231] = 0.0;
   out_2565599195015806522[232] = 0.0;
   out_2565599195015806522[233] = 0.0;
   out_2565599195015806522[234] = 0.0;
   out_2565599195015806522[235] = 0.0;
   out_2565599195015806522[236] = 0.0;
   out_2565599195015806522[237] = 0.0;
   out_2565599195015806522[238] = 0.0;
   out_2565599195015806522[239] = 0.0;
   out_2565599195015806522[240] = 0.0;
   out_2565599195015806522[241] = 0.0;
   out_2565599195015806522[242] = 0.0;
   out_2565599195015806522[243] = 0.0;
   out_2565599195015806522[244] = 0.0;
   out_2565599195015806522[245] = 0.0;
   out_2565599195015806522[246] = 0.0;
   out_2565599195015806522[247] = 1.0;
   out_2565599195015806522[248] = 0.0;
   out_2565599195015806522[249] = 0.0;
   out_2565599195015806522[250] = 0.0;
   out_2565599195015806522[251] = 0.0;
   out_2565599195015806522[252] = 0.0;
   out_2565599195015806522[253] = 0.0;
   out_2565599195015806522[254] = 0.0;
   out_2565599195015806522[255] = 0.0;
   out_2565599195015806522[256] = 0.0;
   out_2565599195015806522[257] = 0.0;
   out_2565599195015806522[258] = 0.0;
   out_2565599195015806522[259] = 0.0;
   out_2565599195015806522[260] = 0.0;
   out_2565599195015806522[261] = 0.0;
   out_2565599195015806522[262] = 0.0;
   out_2565599195015806522[263] = 0.0;
   out_2565599195015806522[264] = 0.0;
   out_2565599195015806522[265] = 0.0;
   out_2565599195015806522[266] = 1.0;
   out_2565599195015806522[267] = 0.0;
   out_2565599195015806522[268] = 0.0;
   out_2565599195015806522[269] = 0.0;
   out_2565599195015806522[270] = 0.0;
   out_2565599195015806522[271] = 0.0;
   out_2565599195015806522[272] = 0.0;
   out_2565599195015806522[273] = 0.0;
   out_2565599195015806522[274] = 0.0;
   out_2565599195015806522[275] = 0.0;
   out_2565599195015806522[276] = 0.0;
   out_2565599195015806522[277] = 0.0;
   out_2565599195015806522[278] = 0.0;
   out_2565599195015806522[279] = 0.0;
   out_2565599195015806522[280] = 0.0;
   out_2565599195015806522[281] = 0.0;
   out_2565599195015806522[282] = 0.0;
   out_2565599195015806522[283] = 0.0;
   out_2565599195015806522[284] = 0.0;
   out_2565599195015806522[285] = 1.0;
   out_2565599195015806522[286] = 0.0;
   out_2565599195015806522[287] = 0.0;
   out_2565599195015806522[288] = 0.0;
   out_2565599195015806522[289] = 0.0;
   out_2565599195015806522[290] = 0.0;
   out_2565599195015806522[291] = 0.0;
   out_2565599195015806522[292] = 0.0;
   out_2565599195015806522[293] = 0.0;
   out_2565599195015806522[294] = 0.0;
   out_2565599195015806522[295] = 0.0;
   out_2565599195015806522[296] = 0.0;
   out_2565599195015806522[297] = 0.0;
   out_2565599195015806522[298] = 0.0;
   out_2565599195015806522[299] = 0.0;
   out_2565599195015806522[300] = 0.0;
   out_2565599195015806522[301] = 0.0;
   out_2565599195015806522[302] = 0.0;
   out_2565599195015806522[303] = 0.0;
   out_2565599195015806522[304] = 1.0;
   out_2565599195015806522[305] = 0.0;
   out_2565599195015806522[306] = 0.0;
   out_2565599195015806522[307] = 0.0;
   out_2565599195015806522[308] = 0.0;
   out_2565599195015806522[309] = 0.0;
   out_2565599195015806522[310] = 0.0;
   out_2565599195015806522[311] = 0.0;
   out_2565599195015806522[312] = 0.0;
   out_2565599195015806522[313] = 0.0;
   out_2565599195015806522[314] = 0.0;
   out_2565599195015806522[315] = 0.0;
   out_2565599195015806522[316] = 0.0;
   out_2565599195015806522[317] = 0.0;
   out_2565599195015806522[318] = 0.0;
   out_2565599195015806522[319] = 0.0;
   out_2565599195015806522[320] = 0.0;
   out_2565599195015806522[321] = 0.0;
   out_2565599195015806522[322] = 0.0;
   out_2565599195015806522[323] = 1.0;
}
void f_fun(double *state, double dt, double *out_5219021910470307673) {
   out_5219021910470307673[0] = atan2((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), -(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]));
   out_5219021910470307673[1] = asin(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]));
   out_5219021910470307673[2] = atan2(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), -(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]));
   out_5219021910470307673[3] = dt*state[12] + state[3];
   out_5219021910470307673[4] = dt*state[13] + state[4];
   out_5219021910470307673[5] = dt*state[14] + state[5];
   out_5219021910470307673[6] = state[6];
   out_5219021910470307673[7] = state[7];
   out_5219021910470307673[8] = state[8];
   out_5219021910470307673[9] = state[9];
   out_5219021910470307673[10] = state[10];
   out_5219021910470307673[11] = state[11];
   out_5219021910470307673[12] = state[12];
   out_5219021910470307673[13] = state[13];
   out_5219021910470307673[14] = state[14];
   out_5219021910470307673[15] = state[15];
   out_5219021910470307673[16] = state[16];
   out_5219021910470307673[17] = state[17];
}
void F_fun(double *state, double dt, double *out_2816761085006576373) {
   out_2816761085006576373[0] = ((-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*cos(state[0])*cos(state[1]) - sin(state[0])*cos(dt*state[6])*cos(dt*state[7])*cos(state[1]))*(-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + ((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*cos(state[0])*cos(state[1]) - sin(dt*state[6])*sin(state[0])*cos(dt*state[7])*cos(state[1]))*(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_2816761085006576373[1] = ((-sin(dt*state[6])*sin(dt*state[8]) - sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*cos(state[1]) - (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*sin(state[1]) - sin(state[1])*cos(dt*state[6])*cos(dt*state[7])*cos(state[0]))*(-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + (-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*sin(state[1]) + (-sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) + sin(dt*state[8])*cos(dt*state[6]))*cos(state[1]) - sin(dt*state[6])*sin(state[1])*cos(dt*state[7])*cos(state[0]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_2816761085006576373[2] = 0;
   out_2816761085006576373[3] = 0;
   out_2816761085006576373[4] = 0;
   out_2816761085006576373[5] = 0;
   out_2816761085006576373[6] = (-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(dt*cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]) + (-dt*sin(dt*state[6])*sin(dt*state[8]) - dt*sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-dt*sin(dt*state[6])*cos(dt*state[8]) + dt*sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + (-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(-dt*sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]) + (-dt*sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) - dt*cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (dt*sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - dt*sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_2816761085006576373[7] = (-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(-dt*sin(dt*state[6])*sin(dt*state[7])*cos(state[0])*cos(state[1]) + dt*sin(dt*state[6])*sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) - dt*sin(dt*state[6])*sin(state[1])*cos(dt*state[7])*cos(dt*state[8]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + (-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))*(-dt*sin(dt*state[7])*cos(dt*state[6])*cos(state[0])*cos(state[1]) + dt*sin(dt*state[8])*sin(state[0])*cos(dt*state[6])*cos(dt*state[7])*cos(state[1]) - dt*sin(state[1])*cos(dt*state[6])*cos(dt*state[7])*cos(dt*state[8]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_2816761085006576373[8] = ((dt*sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + dt*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (dt*sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - dt*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]))*(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2)) + ((dt*sin(dt*state[6])*sin(dt*state[8]) + dt*sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (-dt*sin(dt*state[6])*cos(dt*state[8]) + dt*sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]))*(-(sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) + (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) - sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/(pow(-(sin(dt*state[6])*sin(dt*state[8]) + sin(dt*state[7])*cos(dt*state[6])*cos(dt*state[8]))*sin(state[1]) + (-sin(dt*state[6])*cos(dt*state[8]) + sin(dt*state[7])*sin(dt*state[8])*cos(dt*state[6]))*sin(state[0])*cos(state[1]) + cos(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2) + pow((sin(dt*state[6])*sin(dt*state[7])*sin(dt*state[8]) + cos(dt*state[6])*cos(dt*state[8]))*sin(state[0])*cos(state[1]) - (sin(dt*state[6])*sin(dt*state[7])*cos(dt*state[8]) - sin(dt*state[8])*cos(dt*state[6]))*sin(state[1]) + sin(dt*state[6])*cos(dt*state[7])*cos(state[0])*cos(state[1]), 2));
   out_2816761085006576373[9] = 0;
   out_2816761085006576373[10] = 0;
   out_2816761085006576373[11] = 0;
   out_2816761085006576373[12] = 0;
   out_2816761085006576373[13] = 0;
   out_2816761085006576373[14] = 0;
   out_2816761085006576373[15] = 0;
   out_2816761085006576373[16] = 0;
   out_2816761085006576373[17] = 0;
   out_2816761085006576373[18] = (-sin(dt*state[7])*sin(state[0])*cos(state[1]) - sin(dt*state[8])*cos(dt*state[7])*cos(state[0])*cos(state[1]))/sqrt(1 - pow(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]), 2));
   out_2816761085006576373[19] = (-sin(dt*state[7])*sin(state[1])*cos(state[0]) + sin(dt*state[8])*sin(state[0])*sin(state[1])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/sqrt(1 - pow(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]), 2));
   out_2816761085006576373[20] = 0;
   out_2816761085006576373[21] = 0;
   out_2816761085006576373[22] = 0;
   out_2816761085006576373[23] = 0;
   out_2816761085006576373[24] = 0;
   out_2816761085006576373[25] = (dt*sin(dt*state[7])*sin(dt*state[8])*sin(state[0])*cos(state[1]) - dt*sin(dt*state[7])*sin(state[1])*cos(dt*state[8]) + dt*cos(dt*state[7])*cos(state[0])*cos(state[1]))/sqrt(1 - pow(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]), 2));
   out_2816761085006576373[26] = (-dt*sin(dt*state[8])*sin(state[1])*cos(dt*state[7]) - dt*sin(state[0])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/sqrt(1 - pow(sin(dt*state[7])*cos(state[0])*cos(state[1]) - sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1]) + sin(state[1])*cos(dt*state[7])*cos(dt*state[8]), 2));
   out_2816761085006576373[27] = 0;
   out_2816761085006576373[28] = 0;
   out_2816761085006576373[29] = 0;
   out_2816761085006576373[30] = 0;
   out_2816761085006576373[31] = 0;
   out_2816761085006576373[32] = 0;
   out_2816761085006576373[33] = 0;
   out_2816761085006576373[34] = 0;
   out_2816761085006576373[35] = 0;
   out_2816761085006576373[36] = ((sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[7]))*((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + ((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[7]))*(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_2816761085006576373[37] = (-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))*(-sin(dt*state[7])*sin(state[2])*cos(state[0])*cos(state[1]) + sin(dt*state[8])*sin(state[0])*sin(state[2])*cos(dt*state[7])*cos(state[1]) - sin(state[1])*sin(state[2])*cos(dt*state[7])*cos(dt*state[8]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + ((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))*(-sin(dt*state[7])*cos(state[0])*cos(state[1])*cos(state[2]) + sin(dt*state[8])*sin(state[0])*cos(dt*state[7])*cos(state[1])*cos(state[2]) - sin(state[1])*cos(dt*state[7])*cos(dt*state[8])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_2816761085006576373[38] = ((-sin(state[0])*sin(state[2]) - sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))*(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + ((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (-sin(state[0])*sin(state[1])*sin(state[2]) - cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))*((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_2816761085006576373[39] = 0;
   out_2816761085006576373[40] = 0;
   out_2816761085006576373[41] = 0;
   out_2816761085006576373[42] = 0;
   out_2816761085006576373[43] = (-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))*(dt*(sin(state[0])*cos(state[2]) - sin(state[1])*sin(state[2])*cos(state[0]))*cos(dt*state[7]) - dt*(sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[7])*sin(dt*state[8]) - dt*sin(dt*state[7])*sin(state[2])*cos(dt*state[8])*cos(state[1]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + ((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))*(dt*(-sin(state[0])*sin(state[2]) - sin(state[1])*cos(state[0])*cos(state[2]))*cos(dt*state[7]) - dt*(sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[7])*sin(dt*state[8]) - dt*sin(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_2816761085006576373[44] = (dt*(sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*cos(dt*state[7])*cos(dt*state[8]) - dt*sin(dt*state[8])*sin(state[2])*cos(dt*state[7])*cos(state[1]))*(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2)) + (dt*(sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*cos(dt*state[7])*cos(dt*state[8]) - dt*sin(dt*state[8])*cos(dt*state[7])*cos(state[1])*cos(state[2]))*((-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) - (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) - sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]))/(pow(-(sin(state[0])*sin(state[2]) + sin(state[1])*cos(state[0])*cos(state[2]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*cos(state[2]) - sin(state[2])*cos(state[0]))*sin(dt*state[8])*cos(dt*state[7]) + cos(dt*state[7])*cos(dt*state[8])*cos(state[1])*cos(state[2]), 2) + pow(-(-sin(state[0])*cos(state[2]) + sin(state[1])*sin(state[2])*cos(state[0]))*sin(dt*state[7]) + (sin(state[0])*sin(state[1])*sin(state[2]) + cos(state[0])*cos(state[2]))*sin(dt*state[8])*cos(dt*state[7]) + sin(state[2])*cos(dt*state[7])*cos(dt*state[8])*cos(state[1]), 2));
   out_2816761085006576373[45] = 0;
   out_2816761085006576373[46] = 0;
   out_2816761085006576373[47] = 0;
   out_2816761085006576373[48] = 0;
   out_2816761085006576373[49] = 0;
   out_2816761085006576373[50] = 0;
   out_2816761085006576373[51] = 0;
   out_2816761085006576373[52] = 0;
   out_2816761085006576373[53] = 0;
   out_2816761085006576373[54] = 0;
   out_2816761085006576373[55] = 0;
   out_2816761085006576373[56] = 0;
   out_2816761085006576373[57] = 1;
   out_2816761085006576373[58] = 0;
   out_2816761085006576373[59] = 0;
   out_2816761085006576373[60] = 0;
   out_2816761085006576373[61] = 0;
   out_2816761085006576373[62] = 0;
   out_2816761085006576373[63] = 0;
   out_2816761085006576373[64] = 0;
   out_2816761085006576373[65] = 0;
   out_2816761085006576373[66] = dt;
   out_2816761085006576373[67] = 0;
   out_2816761085006576373[68] = 0;
   out_2816761085006576373[69] = 0;
   out_2816761085006576373[70] = 0;
   out_2816761085006576373[71] = 0;
   out_2816761085006576373[72] = 0;
   out_2816761085006576373[73] = 0;
   out_2816761085006576373[74] = 0;
   out_2816761085006576373[75] = 0;
   out_2816761085006576373[76] = 1;
   out_2816761085006576373[77] = 0;
   out_2816761085006576373[78] = 0;
   out_2816761085006576373[79] = 0;
   out_2816761085006576373[80] = 0;
   out_2816761085006576373[81] = 0;
   out_2816761085006576373[82] = 0;
   out_2816761085006576373[83] = 0;
   out_2816761085006576373[84] = 0;
   out_2816761085006576373[85] = dt;
   out_2816761085006576373[86] = 0;
   out_2816761085006576373[87] = 0;
   out_2816761085006576373[88] = 0;
   out_2816761085006576373[89] = 0;
   out_2816761085006576373[90] = 0;
   out_2816761085006576373[91] = 0;
   out_2816761085006576373[92] = 0;
   out_2816761085006576373[93] = 0;
   out_2816761085006576373[94] = 0;
   out_2816761085006576373[95] = 1;
   out_2816761085006576373[96] = 0;
   out_2816761085006576373[97] = 0;
   out_2816761085006576373[98] = 0;
   out_2816761085006576373[99] = 0;
   out_2816761085006576373[100] = 0;
   out_2816761085006576373[101] = 0;
   out_2816761085006576373[102] = 0;
   out_2816761085006576373[103] = 0;
   out_2816761085006576373[104] = dt;
   out_2816761085006576373[105] = 0;
   out_2816761085006576373[106] = 0;
   out_2816761085006576373[107] = 0;
   out_2816761085006576373[108] = 0;
   out_2816761085006576373[109] = 0;
   out_2816761085006576373[110] = 0;
   out_2816761085006576373[111] = 0;
   out_2816761085006576373[112] = 0;
   out_2816761085006576373[113] = 0;
   out_2816761085006576373[114] = 1;
   out_2816761085006576373[115] = 0;
   out_2816761085006576373[116] = 0;
   out_2816761085006576373[117] = 0;
   out_2816761085006576373[118] = 0;
   out_2816761085006576373[119] = 0;
   out_2816761085006576373[120] = 0;
   out_2816761085006576373[121] = 0;
   out_2816761085006576373[122] = 0;
   out_2816761085006576373[123] = 0;
   out_2816761085006576373[124] = 0;
   out_2816761085006576373[125] = 0;
   out_2816761085006576373[126] = 0;
   out_2816761085006576373[127] = 0;
   out_2816761085006576373[128] = 0;
   out_2816761085006576373[129] = 0;
   out_2816761085006576373[130] = 0;
   out_2816761085006576373[131] = 0;
   out_2816761085006576373[132] = 0;
   out_2816761085006576373[133] = 1;
   out_2816761085006576373[134] = 0;
   out_2816761085006576373[135] = 0;
   out_2816761085006576373[136] = 0;
   out_2816761085006576373[137] = 0;
   out_2816761085006576373[138] = 0;
   out_2816761085006576373[139] = 0;
   out_2816761085006576373[140] = 0;
   out_2816761085006576373[141] = 0;
   out_2816761085006576373[142] = 0;
   out_2816761085006576373[143] = 0;
   out_2816761085006576373[144] = 0;
   out_2816761085006576373[145] = 0;
   out_2816761085006576373[146] = 0;
   out_2816761085006576373[147] = 0;
   out_2816761085006576373[148] = 0;
   out_2816761085006576373[149] = 0;
   out_2816761085006576373[150] = 0;
   out_2816761085006576373[151] = 0;
   out_2816761085006576373[152] = 1;
   out_2816761085006576373[153] = 0;
   out_2816761085006576373[154] = 0;
   out_2816761085006576373[155] = 0;
   out_2816761085006576373[156] = 0;
   out_2816761085006576373[157] = 0;
   out_2816761085006576373[158] = 0;
   out_2816761085006576373[159] = 0;
   out_2816761085006576373[160] = 0;
   out_2816761085006576373[161] = 0;
   out_2816761085006576373[162] = 0;
   out_2816761085006576373[163] = 0;
   out_2816761085006576373[164] = 0;
   out_2816761085006576373[165] = 0;
   out_2816761085006576373[166] = 0;
   out_2816761085006576373[167] = 0;
   out_2816761085006576373[168] = 0;
   out_2816761085006576373[169] = 0;
   out_2816761085006576373[170] = 0;
   out_2816761085006576373[171] = 1;
   out_2816761085006576373[172] = 0;
   out_2816761085006576373[173] = 0;
   out_2816761085006576373[174] = 0;
   out_2816761085006576373[175] = 0;
   out_2816761085006576373[176] = 0;
   out_2816761085006576373[177] = 0;
   out_2816761085006576373[178] = 0;
   out_2816761085006576373[179] = 0;
   out_2816761085006576373[180] = 0;
   out_2816761085006576373[181] = 0;
   out_2816761085006576373[182] = 0;
   out_2816761085006576373[183] = 0;
   out_2816761085006576373[184] = 0;
   out_2816761085006576373[185] = 0;
   out_2816761085006576373[186] = 0;
   out_2816761085006576373[187] = 0;
   out_2816761085006576373[188] = 0;
   out_2816761085006576373[189] = 0;
   out_2816761085006576373[190] = 1;
   out_2816761085006576373[191] = 0;
   out_2816761085006576373[192] = 0;
   out_2816761085006576373[193] = 0;
   out_2816761085006576373[194] = 0;
   out_2816761085006576373[195] = 0;
   out_2816761085006576373[196] = 0;
   out_2816761085006576373[197] = 0;
   out_2816761085006576373[198] = 0;
   out_2816761085006576373[199] = 0;
   out_2816761085006576373[200] = 0;
   out_2816761085006576373[201] = 0;
   out_2816761085006576373[202] = 0;
   out_2816761085006576373[203] = 0;
   out_2816761085006576373[204] = 0;
   out_2816761085006576373[205] = 0;
   out_2816761085006576373[206] = 0;
   out_2816761085006576373[207] = 0;
   out_2816761085006576373[208] = 0;
   out_2816761085006576373[209] = 1;
   out_2816761085006576373[210] = 0;
   out_2816761085006576373[211] = 0;
   out_2816761085006576373[212] = 0;
   out_2816761085006576373[213] = 0;
   out_2816761085006576373[214] = 0;
   out_2816761085006576373[215] = 0;
   out_2816761085006576373[216] = 0;
   out_2816761085006576373[217] = 0;
   out_2816761085006576373[218] = 0;
   out_2816761085006576373[219] = 0;
   out_2816761085006576373[220] = 0;
   out_2816761085006576373[221] = 0;
   out_2816761085006576373[222] = 0;
   out_2816761085006576373[223] = 0;
   out_2816761085006576373[224] = 0;
   out_2816761085006576373[225] = 0;
   out_2816761085006576373[226] = 0;
   out_2816761085006576373[227] = 0;
   out_2816761085006576373[228] = 1;
   out_2816761085006576373[229] = 0;
   out_2816761085006576373[230] = 0;
   out_2816761085006576373[231] = 0;
   out_2816761085006576373[232] = 0;
   out_2816761085006576373[233] = 0;
   out_2816761085006576373[234] = 0;
   out_2816761085006576373[235] = 0;
   out_2816761085006576373[236] = 0;
   out_2816761085006576373[237] = 0;
   out_2816761085006576373[238] = 0;
   out_2816761085006576373[239] = 0;
   out_2816761085006576373[240] = 0;
   out_2816761085006576373[241] = 0;
   out_2816761085006576373[242] = 0;
   out_2816761085006576373[243] = 0;
   out_2816761085006576373[244] = 0;
   out_2816761085006576373[245] = 0;
   out_2816761085006576373[246] = 0;
   out_2816761085006576373[247] = 1;
   out_2816761085006576373[248] = 0;
   out_2816761085006576373[249] = 0;
   out_2816761085006576373[250] = 0;
   out_2816761085006576373[251] = 0;
   out_2816761085006576373[252] = 0;
   out_2816761085006576373[253] = 0;
   out_2816761085006576373[254] = 0;
   out_2816761085006576373[255] = 0;
   out_2816761085006576373[256] = 0;
   out_2816761085006576373[257] = 0;
   out_2816761085006576373[258] = 0;
   out_2816761085006576373[259] = 0;
   out_2816761085006576373[260] = 0;
   out_2816761085006576373[261] = 0;
   out_2816761085006576373[262] = 0;
   out_2816761085006576373[263] = 0;
   out_2816761085006576373[264] = 0;
   out_2816761085006576373[265] = 0;
   out_2816761085006576373[266] = 1;
   out_2816761085006576373[267] = 0;
   out_2816761085006576373[268] = 0;
   out_2816761085006576373[269] = 0;
   out_2816761085006576373[270] = 0;
   out_2816761085006576373[271] = 0;
   out_2816761085006576373[272] = 0;
   out_2816761085006576373[273] = 0;
   out_2816761085006576373[274] = 0;
   out_2816761085006576373[275] = 0;
   out_2816761085006576373[276] = 0;
   out_2816761085006576373[277] = 0;
   out_2816761085006576373[278] = 0;
   out_2816761085006576373[279] = 0;
   out_2816761085006576373[280] = 0;
   out_2816761085006576373[281] = 0;
   out_2816761085006576373[282] = 0;
   out_2816761085006576373[283] = 0;
   out_2816761085006576373[284] = 0;
   out_2816761085006576373[285] = 1;
   out_2816761085006576373[286] = 0;
   out_2816761085006576373[287] = 0;
   out_2816761085006576373[288] = 0;
   out_2816761085006576373[289] = 0;
   out_2816761085006576373[290] = 0;
   out_2816761085006576373[291] = 0;
   out_2816761085006576373[292] = 0;
   out_2816761085006576373[293] = 0;
   out_2816761085006576373[294] = 0;
   out_2816761085006576373[295] = 0;
   out_2816761085006576373[296] = 0;
   out_2816761085006576373[297] = 0;
   out_2816761085006576373[298] = 0;
   out_2816761085006576373[299] = 0;
   out_2816761085006576373[300] = 0;
   out_2816761085006576373[301] = 0;
   out_2816761085006576373[302] = 0;
   out_2816761085006576373[303] = 0;
   out_2816761085006576373[304] = 1;
   out_2816761085006576373[305] = 0;
   out_2816761085006576373[306] = 0;
   out_2816761085006576373[307] = 0;
   out_2816761085006576373[308] = 0;
   out_2816761085006576373[309] = 0;
   out_2816761085006576373[310] = 0;
   out_2816761085006576373[311] = 0;
   out_2816761085006576373[312] = 0;
   out_2816761085006576373[313] = 0;
   out_2816761085006576373[314] = 0;
   out_2816761085006576373[315] = 0;
   out_2816761085006576373[316] = 0;
   out_2816761085006576373[317] = 0;
   out_2816761085006576373[318] = 0;
   out_2816761085006576373[319] = 0;
   out_2816761085006576373[320] = 0;
   out_2816761085006576373[321] = 0;
   out_2816761085006576373[322] = 0;
   out_2816761085006576373[323] = 1;
}
void h_4(double *state, double *unused, double *out_2523270913770393018) {
   out_2523270913770393018[0] = state[6] + state[9];
   out_2523270913770393018[1] = state[7] + state[10];
   out_2523270913770393018[2] = state[8] + state[11];
}
void H_4(double *state, double *unused, double *out_4953329560176816821) {
   out_4953329560176816821[0] = 0;
   out_4953329560176816821[1] = 0;
   out_4953329560176816821[2] = 0;
   out_4953329560176816821[3] = 0;
   out_4953329560176816821[4] = 0;
   out_4953329560176816821[5] = 0;
   out_4953329560176816821[6] = 1;
   out_4953329560176816821[7] = 0;
   out_4953329560176816821[8] = 0;
   out_4953329560176816821[9] = 1;
   out_4953329560176816821[10] = 0;
   out_4953329560176816821[11] = 0;
   out_4953329560176816821[12] = 0;
   out_4953329560176816821[13] = 0;
   out_4953329560176816821[14] = 0;
   out_4953329560176816821[15] = 0;
   out_4953329560176816821[16] = 0;
   out_4953329560176816821[17] = 0;
   out_4953329560176816821[18] = 0;
   out_4953329560176816821[19] = 0;
   out_4953329560176816821[20] = 0;
   out_4953329560176816821[21] = 0;
   out_4953329560176816821[22] = 0;
   out_4953329560176816821[23] = 0;
   out_4953329560176816821[24] = 0;
   out_4953329560176816821[25] = 1;
   out_4953329560176816821[26] = 0;
   out_4953329560176816821[27] = 0;
   out_4953329560176816821[28] = 1;
   out_4953329560176816821[29] = 0;
   out_4953329560176816821[30] = 0;
   out_4953329560176816821[31] = 0;
   out_4953329560176816821[32] = 0;
   out_4953329560176816821[33] = 0;
   out_4953329560176816821[34] = 0;
   out_4953329560176816821[35] = 0;
   out_4953329560176816821[36] = 0;
   out_4953329560176816821[37] = 0;
   out_4953329560176816821[38] = 0;
   out_4953329560176816821[39] = 0;
   out_4953329560176816821[40] = 0;
   out_4953329560176816821[41] = 0;
   out_4953329560176816821[42] = 0;
   out_4953329560176816821[43] = 0;
   out_4953329560176816821[44] = 1;
   out_4953329560176816821[45] = 0;
   out_4953329560176816821[46] = 0;
   out_4953329560176816821[47] = 1;
   out_4953329560176816821[48] = 0;
   out_4953329560176816821[49] = 0;
   out_4953329560176816821[50] = 0;
   out_4953329560176816821[51] = 0;
   out_4953329560176816821[52] = 0;
   out_4953329560176816821[53] = 0;
}
void h_10(double *state, double *unused, double *out_1639151185176902848) {
   out_1639151185176902848[0] = 9.8100000000000005*sin(state[1]) - state[4]*state[8] + state[5]*state[7] + state[12] + state[15];
   out_1639151185176902848[1] = -9.8100000000000005*sin(state[0])*cos(state[1]) + state[3]*state[8] - state[5]*state[6] + state[13] + state[16];
   out_1639151185176902848[2] = -9.8100000000000005*cos(state[0])*cos(state[1]) - state[3]*state[7] + state[4]*state[6] + state[14] + state[17];
}
void H_10(double *state, double *unused, double *out_8201979873762599692) {
   out_8201979873762599692[0] = 0;
   out_8201979873762599692[1] = 9.8100000000000005*cos(state[1]);
   out_8201979873762599692[2] = 0;
   out_8201979873762599692[3] = 0;
   out_8201979873762599692[4] = -state[8];
   out_8201979873762599692[5] = state[7];
   out_8201979873762599692[6] = 0;
   out_8201979873762599692[7] = state[5];
   out_8201979873762599692[8] = -state[4];
   out_8201979873762599692[9] = 0;
   out_8201979873762599692[10] = 0;
   out_8201979873762599692[11] = 0;
   out_8201979873762599692[12] = 1;
   out_8201979873762599692[13] = 0;
   out_8201979873762599692[14] = 0;
   out_8201979873762599692[15] = 1;
   out_8201979873762599692[16] = 0;
   out_8201979873762599692[17] = 0;
   out_8201979873762599692[18] = -9.8100000000000005*cos(state[0])*cos(state[1]);
   out_8201979873762599692[19] = 9.8100000000000005*sin(state[0])*sin(state[1]);
   out_8201979873762599692[20] = 0;
   out_8201979873762599692[21] = state[8];
   out_8201979873762599692[22] = 0;
   out_8201979873762599692[23] = -state[6];
   out_8201979873762599692[24] = -state[5];
   out_8201979873762599692[25] = 0;
   out_8201979873762599692[26] = state[3];
   out_8201979873762599692[27] = 0;
   out_8201979873762599692[28] = 0;
   out_8201979873762599692[29] = 0;
   out_8201979873762599692[30] = 0;
   out_8201979873762599692[31] = 1;
   out_8201979873762599692[32] = 0;
   out_8201979873762599692[33] = 0;
   out_8201979873762599692[34] = 1;
   out_8201979873762599692[35] = 0;
   out_8201979873762599692[36] = 9.8100000000000005*sin(state[0])*cos(state[1]);
   out_8201979873762599692[37] = 9.8100000000000005*sin(state[1])*cos(state[0]);
   out_8201979873762599692[38] = 0;
   out_8201979873762599692[39] = -state[7];
   out_8201979873762599692[40] = state[6];
   out_8201979873762599692[41] = 0;
   out_8201979873762599692[42] = state[4];
   out_8201979873762599692[43] = -state[3];
   out_8201979873762599692[44] = 0;
   out_8201979873762599692[45] = 0;
   out_8201979873762599692[46] = 0;
   out_8201979873762599692[47] = 0;
   out_8201979873762599692[48] = 0;
   out_8201979873762599692[49] = 0;
   out_8201979873762599692[50] = 1;
   out_8201979873762599692[51] = 0;
   out_8201979873762599692[52] = 0;
   out_8201979873762599692[53] = 1;
}
void h_13(double *state, double *unused, double *out_7609351363365188316) {
   out_7609351363365188316[0] = state[3];
   out_7609351363365188316[1] = state[4];
   out_7609351363365188316[2] = state[5];
}
void H_13(double *state, double *unused, double *out_8787085023479340845) {
   out_8787085023479340845[0] = 0;
   out_8787085023479340845[1] = 0;
   out_8787085023479340845[2] = 0;
   out_8787085023479340845[3] = 1;
   out_8787085023479340845[4] = 0;
   out_8787085023479340845[5] = 0;
   out_8787085023479340845[6] = 0;
   out_8787085023479340845[7] = 0;
   out_8787085023479340845[8] = 0;
   out_8787085023479340845[9] = 0;
   out_8787085023479340845[10] = 0;
   out_8787085023479340845[11] = 0;
   out_8787085023479340845[12] = 0;
   out_8787085023479340845[13] = 0;
   out_8787085023479340845[14] = 0;
   out_8787085023479340845[15] = 0;
   out_8787085023479340845[16] = 0;
   out_8787085023479340845[17] = 0;
   out_8787085023479340845[18] = 0;
   out_8787085023479340845[19] = 0;
   out_8787085023479340845[20] = 0;
   out_8787085023479340845[21] = 0;
   out_8787085023479340845[22] = 1;
   out_8787085023479340845[23] = 0;
   out_8787085023479340845[24] = 0;
   out_8787085023479340845[25] = 0;
   out_8787085023479340845[26] = 0;
   out_8787085023479340845[27] = 0;
   out_8787085023479340845[28] = 0;
   out_8787085023479340845[29] = 0;
   out_8787085023479340845[30] = 0;
   out_8787085023479340845[31] = 0;
   out_8787085023479340845[32] = 0;
   out_8787085023479340845[33] = 0;
   out_8787085023479340845[34] = 0;
   out_8787085023479340845[35] = 0;
   out_8787085023479340845[36] = 0;
   out_8787085023479340845[37] = 0;
   out_8787085023479340845[38] = 0;
   out_8787085023479340845[39] = 0;
   out_8787085023479340845[40] = 0;
   out_8787085023479340845[41] = 1;
   out_8787085023479340845[42] = 0;
   out_8787085023479340845[43] = 0;
   out_8787085023479340845[44] = 0;
   out_8787085023479340845[45] = 0;
   out_8787085023479340845[46] = 0;
   out_8787085023479340845[47] = 0;
   out_8787085023479340845[48] = 0;
   out_8787085023479340845[49] = 0;
   out_8787085023479340845[50] = 0;
   out_8787085023479340845[51] = 0;
   out_8787085023479340845[52] = 0;
   out_8787085023479340845[53] = 0;
}
void h_14(double *state, double *unused, double *out_4420274961393380218) {
   out_4420274961393380218[0] = state[6];
   out_4420274961393380218[1] = state[7];
   out_4420274961393380218[2] = state[8];
}
void H_14(double *state, double *unused, double *out_6012268698252994371) {
   out_6012268698252994371[0] = 0;
   out_6012268698252994371[1] = 0;
   out_6012268698252994371[2] = 0;
   out_6012268698252994371[3] = 0;
   out_6012268698252994371[4] = 0;
   out_6012268698252994371[5] = 0;
   out_6012268698252994371[6] = 1;
   out_6012268698252994371[7] = 0;
   out_6012268698252994371[8] = 0;
   out_6012268698252994371[9] = 0;
   out_6012268698252994371[10] = 0;
   out_6012268698252994371[11] = 0;
   out_6012268698252994371[12] = 0;
   out_6012268698252994371[13] = 0;
   out_6012268698252994371[14] = 0;
   out_6012268698252994371[15] = 0;
   out_6012268698252994371[16] = 0;
   out_6012268698252994371[17] = 0;
   out_6012268698252994371[18] = 0;
   out_6012268698252994371[19] = 0;
   out_6012268698252994371[20] = 0;
   out_6012268698252994371[21] = 0;
   out_6012268698252994371[22] = 0;
   out_6012268698252994371[23] = 0;
   out_6012268698252994371[24] = 0;
   out_6012268698252994371[25] = 1;
   out_6012268698252994371[26] = 0;
   out_6012268698252994371[27] = 0;
   out_6012268698252994371[28] = 0;
   out_6012268698252994371[29] = 0;
   out_6012268698252994371[30] = 0;
   out_6012268698252994371[31] = 0;
   out_6012268698252994371[32] = 0;
   out_6012268698252994371[33] = 0;
   out_6012268698252994371[34] = 0;
   out_6012268698252994371[35] = 0;
   out_6012268698252994371[36] = 0;
   out_6012268698252994371[37] = 0;
   out_6012268698252994371[38] = 0;
   out_6012268698252994371[39] = 0;
   out_6012268698252994371[40] = 0;
   out_6012268698252994371[41] = 0;
   out_6012268698252994371[42] = 0;
   out_6012268698252994371[43] = 0;
   out_6012268698252994371[44] = 1;
   out_6012268698252994371[45] = 0;
   out_6012268698252994371[46] = 0;
   out_6012268698252994371[47] = 0;
   out_6012268698252994371[48] = 0;
   out_6012268698252994371[49] = 0;
   out_6012268698252994371[50] = 0;
   out_6012268698252994371[51] = 0;
   out_6012268698252994371[52] = 0;
   out_6012268698252994371[53] = 0;
}
#include <eigen3/Eigen/Dense>
#include <iostream>

typedef Eigen::Matrix<double, DIM, DIM, Eigen::RowMajor> DDM;
typedef Eigen::Matrix<double, EDIM, EDIM, Eigen::RowMajor> EEM;
typedef Eigen::Matrix<double, DIM, EDIM, Eigen::RowMajor> DEM;

void predict(double *in_x, double *in_P, double *in_Q, double dt) {
  typedef Eigen::Matrix<double, MEDIM, MEDIM, Eigen::RowMajor> RRM;

  double nx[DIM] = {0};
  double in_F[EDIM*EDIM] = {0};

  // functions from sympy
  f_fun(in_x, dt, nx);
  F_fun(in_x, dt, in_F);


  EEM F(in_F);
  EEM P(in_P);
  EEM Q(in_Q);

  RRM F_main = F.topLeftCorner(MEDIM, MEDIM);
  P.topLeftCorner(MEDIM, MEDIM) = (F_main * P.topLeftCorner(MEDIM, MEDIM)) * F_main.transpose();
  P.topRightCorner(MEDIM, EDIM - MEDIM) = F_main * P.topRightCorner(MEDIM, EDIM - MEDIM);
  P.bottomLeftCorner(EDIM - MEDIM, MEDIM) = P.bottomLeftCorner(EDIM - MEDIM, MEDIM) * F_main.transpose();

  P = P + dt*Q;

  // copy out state
  memcpy(in_x, nx, DIM * sizeof(double));
  memcpy(in_P, P.data(), EDIM * EDIM * sizeof(double));
}

// note: extra_args dim only correct when null space projecting
// otherwise 1
template <int ZDIM, int EADIM, bool MAHA_TEST>
void update(double *in_x, double *in_P, Hfun h_fun, Hfun H_fun, Hfun Hea_fun, double *in_z, double *in_R, double *in_ea, double MAHA_THRESHOLD) {
  typedef Eigen::Matrix<double, ZDIM, ZDIM, Eigen::RowMajor> ZZM;
  typedef Eigen::Matrix<double, ZDIM, DIM, Eigen::RowMajor> ZDM;
  typedef Eigen::Matrix<double, Eigen::Dynamic, EDIM, Eigen::RowMajor> XEM;
  //typedef Eigen::Matrix<double, EDIM, ZDIM, Eigen::RowMajor> EZM;
  typedef Eigen::Matrix<double, Eigen::Dynamic, 1> X1M;
  typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> XXM;

  double in_hx[ZDIM] = {0};
  double in_H[ZDIM * DIM] = {0};
  double in_H_mod[EDIM * DIM] = {0};
  double delta_x[EDIM] = {0};
  double x_new[DIM] = {0};


  // state x, P
  Eigen::Matrix<double, ZDIM, 1> z(in_z);
  EEM P(in_P);
  ZZM pre_R(in_R);

  // functions from sympy
  h_fun(in_x, in_ea, in_hx);
  H_fun(in_x, in_ea, in_H);
  ZDM pre_H(in_H);

  // get y (y = z - hx)
  Eigen::Matrix<double, ZDIM, 1> pre_y(in_hx); pre_y = z - pre_y;
  X1M y; XXM H; XXM R;
  if (Hea_fun){
    typedef Eigen::Matrix<double, ZDIM, EADIM, Eigen::RowMajor> ZAM;
    double in_Hea[ZDIM * EADIM] = {0};
    Hea_fun(in_x, in_ea, in_Hea);
    ZAM Hea(in_Hea);
    XXM A = Hea.transpose().fullPivLu().kernel();


    y = A.transpose() * pre_y;
    H = A.transpose() * pre_H;
    R = A.transpose() * pre_R * A;
  } else {
    y = pre_y;
    H = pre_H;
    R = pre_R;
  }
  // get modified H
  H_mod_fun(in_x, in_H_mod);
  DEM H_mod(in_H_mod);
  XEM H_err = H * H_mod;

  // Do mahalobis distance test
  if (MAHA_TEST){
    XXM a = (H_err * P * H_err.transpose() + R).inverse();
    double maha_dist = y.transpose() * a * y;
    if (maha_dist > MAHA_THRESHOLD){
      R = 1.0e16 * R;
    }
  }

  // Outlier resilient weighting
  double weight = 1;//(1.5)/(1 + y.squaredNorm()/R.sum());

  // kalman gains and I_KH
  XXM S = ((H_err * P) * H_err.transpose()) + R/weight;
  XEM KT = S.fullPivLu().solve(H_err * P.transpose());
  //EZM K = KT.transpose(); TODO: WHY DOES THIS NOT COMPILE?
  //EZM K = S.fullPivLu().solve(H_err * P.transpose()).transpose();
  //std::cout << "Here is the matrix rot:\n" << K << std::endl;
  EEM I_KH = Eigen::Matrix<double, EDIM, EDIM>::Identity() - (KT.transpose() * H_err);

  // update state by injecting dx
  Eigen::Matrix<double, EDIM, 1> dx(delta_x);
  dx  = (KT.transpose() * y);
  memcpy(delta_x, dx.data(), EDIM * sizeof(double));
  err_fun(in_x, delta_x, x_new);
  Eigen::Matrix<double, DIM, 1> x(x_new);

  // update cov
  P = ((I_KH * P) * I_KH.transpose()) + ((KT.transpose() * R) * KT);

  // copy out state
  memcpy(in_x, x.data(), DIM * sizeof(double));
  memcpy(in_P, P.data(), EDIM * EDIM * sizeof(double));
  memcpy(in_z, y.data(), y.rows() * sizeof(double));
}




}
extern "C" {

void pose_update_4(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<3, 3, 0>(in_x, in_P, h_4, H_4, NULL, in_z, in_R, in_ea, MAHA_THRESH_4);
}
void pose_update_10(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<3, 3, 0>(in_x, in_P, h_10, H_10, NULL, in_z, in_R, in_ea, MAHA_THRESH_10);
}
void pose_update_13(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<3, 3, 0>(in_x, in_P, h_13, H_13, NULL, in_z, in_R, in_ea, MAHA_THRESH_13);
}
void pose_update_14(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<3, 3, 0>(in_x, in_P, h_14, H_14, NULL, in_z, in_R, in_ea, MAHA_THRESH_14);
}
void pose_err_fun(double *nom_x, double *delta_x, double *out_8294194842749133858) {
  err_fun(nom_x, delta_x, out_8294194842749133858);
}
void pose_inv_err_fun(double *nom_x, double *true_x, double *out_1479878567395019915) {
  inv_err_fun(nom_x, true_x, out_1479878567395019915);
}
void pose_H_mod_fun(double *state, double *out_2565599195015806522) {
  H_mod_fun(state, out_2565599195015806522);
}
void pose_f_fun(double *state, double dt, double *out_5219021910470307673) {
  f_fun(state,  dt, out_5219021910470307673);
}
void pose_F_fun(double *state, double dt, double *out_2816761085006576373) {
  F_fun(state,  dt, out_2816761085006576373);
}
void pose_h_4(double *state, double *unused, double *out_2523270913770393018) {
  h_4(state, unused, out_2523270913770393018);
}
void pose_H_4(double *state, double *unused, double *out_4953329560176816821) {
  H_4(state, unused, out_4953329560176816821);
}
void pose_h_10(double *state, double *unused, double *out_1639151185176902848) {
  h_10(state, unused, out_1639151185176902848);
}
void pose_H_10(double *state, double *unused, double *out_8201979873762599692) {
  H_10(state, unused, out_8201979873762599692);
}
void pose_h_13(double *state, double *unused, double *out_7609351363365188316) {
  h_13(state, unused, out_7609351363365188316);
}
void pose_H_13(double *state, double *unused, double *out_8787085023479340845) {
  H_13(state, unused, out_8787085023479340845);
}
void pose_h_14(double *state, double *unused, double *out_4420274961393380218) {
  h_14(state, unused, out_4420274961393380218);
}
void pose_H_14(double *state, double *unused, double *out_6012268698252994371) {
  H_14(state, unused, out_6012268698252994371);
}
void pose_predict(double *in_x, double *in_P, double *in_Q, double dt) {
  predict(in_x, in_P, in_Q, dt);
}
}

const EKF pose = {
  .name = "pose",
  .kinds = { 4, 10, 13, 14 },
  .feature_kinds = {  },
  .f_fun = pose_f_fun,
  .F_fun = pose_F_fun,
  .err_fun = pose_err_fun,
  .inv_err_fun = pose_inv_err_fun,
  .H_mod_fun = pose_H_mod_fun,
  .predict = pose_predict,
  .hs = {
    { 4, pose_h_4 },
    { 10, pose_h_10 },
    { 13, pose_h_13 },
    { 14, pose_h_14 },
  },
  .Hs = {
    { 4, pose_H_4 },
    { 10, pose_H_10 },
    { 13, pose_H_13 },
    { 14, pose_H_14 },
  },
  .updates = {
    { 4, pose_update_4 },
    { 10, pose_update_10 },
    { 13, pose_update_13 },
    { 14, pose_update_14 },
  },
  .Hes = {
  },
  .sets = {
  },
  .extra_routines = {
  },
};

ekf_lib_init(pose)
