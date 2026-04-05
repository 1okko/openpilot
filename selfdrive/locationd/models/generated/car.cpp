#include "car.h"

namespace {
#define DIM 9
#define EDIM 9
#define MEDIM 9
typedef void (*Hfun)(double *, double *, double *);

double mass;

void set_mass(double x){ mass = x;}

double rotational_inertia;

void set_rotational_inertia(double x){ rotational_inertia = x;}

double center_to_front;

void set_center_to_front(double x){ center_to_front = x;}

double center_to_rear;

void set_center_to_rear(double x){ center_to_rear = x;}

double stiffness_front;

void set_stiffness_front(double x){ stiffness_front = x;}

double stiffness_rear;

void set_stiffness_rear(double x){ stiffness_rear = x;}
const static double MAHA_THRESH_25 = 3.8414588206941227;
const static double MAHA_THRESH_24 = 5.991464547107981;
const static double MAHA_THRESH_30 = 3.8414588206941227;
const static double MAHA_THRESH_26 = 3.8414588206941227;
const static double MAHA_THRESH_27 = 3.8414588206941227;
const static double MAHA_THRESH_29 = 3.8414588206941227;
const static double MAHA_THRESH_28 = 3.8414588206941227;
const static double MAHA_THRESH_31 = 3.8414588206941227;

/******************************************************************************
 *                      Code generated with SymPy 1.14.0                      *
 *                                                                            *
 *              See http://www.sympy.org/ for more information.               *
 *                                                                            *
 *                         This file is part of 'ekf'                         *
 ******************************************************************************/
void err_fun(double *nom_x, double *delta_x, double *out_1278479234343698745) {
   out_1278479234343698745[0] = delta_x[0] + nom_x[0];
   out_1278479234343698745[1] = delta_x[1] + nom_x[1];
   out_1278479234343698745[2] = delta_x[2] + nom_x[2];
   out_1278479234343698745[3] = delta_x[3] + nom_x[3];
   out_1278479234343698745[4] = delta_x[4] + nom_x[4];
   out_1278479234343698745[5] = delta_x[5] + nom_x[5];
   out_1278479234343698745[6] = delta_x[6] + nom_x[6];
   out_1278479234343698745[7] = delta_x[7] + nom_x[7];
   out_1278479234343698745[8] = delta_x[8] + nom_x[8];
}
void inv_err_fun(double *nom_x, double *true_x, double *out_7478532589410690046) {
   out_7478532589410690046[0] = -nom_x[0] + true_x[0];
   out_7478532589410690046[1] = -nom_x[1] + true_x[1];
   out_7478532589410690046[2] = -nom_x[2] + true_x[2];
   out_7478532589410690046[3] = -nom_x[3] + true_x[3];
   out_7478532589410690046[4] = -nom_x[4] + true_x[4];
   out_7478532589410690046[5] = -nom_x[5] + true_x[5];
   out_7478532589410690046[6] = -nom_x[6] + true_x[6];
   out_7478532589410690046[7] = -nom_x[7] + true_x[7];
   out_7478532589410690046[8] = -nom_x[8] + true_x[8];
}
void H_mod_fun(double *state, double *out_7110864696211804484) {
   out_7110864696211804484[0] = 1.0;
   out_7110864696211804484[1] = 0.0;
   out_7110864696211804484[2] = 0.0;
   out_7110864696211804484[3] = 0.0;
   out_7110864696211804484[4] = 0.0;
   out_7110864696211804484[5] = 0.0;
   out_7110864696211804484[6] = 0.0;
   out_7110864696211804484[7] = 0.0;
   out_7110864696211804484[8] = 0.0;
   out_7110864696211804484[9] = 0.0;
   out_7110864696211804484[10] = 1.0;
   out_7110864696211804484[11] = 0.0;
   out_7110864696211804484[12] = 0.0;
   out_7110864696211804484[13] = 0.0;
   out_7110864696211804484[14] = 0.0;
   out_7110864696211804484[15] = 0.0;
   out_7110864696211804484[16] = 0.0;
   out_7110864696211804484[17] = 0.0;
   out_7110864696211804484[18] = 0.0;
   out_7110864696211804484[19] = 0.0;
   out_7110864696211804484[20] = 1.0;
   out_7110864696211804484[21] = 0.0;
   out_7110864696211804484[22] = 0.0;
   out_7110864696211804484[23] = 0.0;
   out_7110864696211804484[24] = 0.0;
   out_7110864696211804484[25] = 0.0;
   out_7110864696211804484[26] = 0.0;
   out_7110864696211804484[27] = 0.0;
   out_7110864696211804484[28] = 0.0;
   out_7110864696211804484[29] = 0.0;
   out_7110864696211804484[30] = 1.0;
   out_7110864696211804484[31] = 0.0;
   out_7110864696211804484[32] = 0.0;
   out_7110864696211804484[33] = 0.0;
   out_7110864696211804484[34] = 0.0;
   out_7110864696211804484[35] = 0.0;
   out_7110864696211804484[36] = 0.0;
   out_7110864696211804484[37] = 0.0;
   out_7110864696211804484[38] = 0.0;
   out_7110864696211804484[39] = 0.0;
   out_7110864696211804484[40] = 1.0;
   out_7110864696211804484[41] = 0.0;
   out_7110864696211804484[42] = 0.0;
   out_7110864696211804484[43] = 0.0;
   out_7110864696211804484[44] = 0.0;
   out_7110864696211804484[45] = 0.0;
   out_7110864696211804484[46] = 0.0;
   out_7110864696211804484[47] = 0.0;
   out_7110864696211804484[48] = 0.0;
   out_7110864696211804484[49] = 0.0;
   out_7110864696211804484[50] = 1.0;
   out_7110864696211804484[51] = 0.0;
   out_7110864696211804484[52] = 0.0;
   out_7110864696211804484[53] = 0.0;
   out_7110864696211804484[54] = 0.0;
   out_7110864696211804484[55] = 0.0;
   out_7110864696211804484[56] = 0.0;
   out_7110864696211804484[57] = 0.0;
   out_7110864696211804484[58] = 0.0;
   out_7110864696211804484[59] = 0.0;
   out_7110864696211804484[60] = 1.0;
   out_7110864696211804484[61] = 0.0;
   out_7110864696211804484[62] = 0.0;
   out_7110864696211804484[63] = 0.0;
   out_7110864696211804484[64] = 0.0;
   out_7110864696211804484[65] = 0.0;
   out_7110864696211804484[66] = 0.0;
   out_7110864696211804484[67] = 0.0;
   out_7110864696211804484[68] = 0.0;
   out_7110864696211804484[69] = 0.0;
   out_7110864696211804484[70] = 1.0;
   out_7110864696211804484[71] = 0.0;
   out_7110864696211804484[72] = 0.0;
   out_7110864696211804484[73] = 0.0;
   out_7110864696211804484[74] = 0.0;
   out_7110864696211804484[75] = 0.0;
   out_7110864696211804484[76] = 0.0;
   out_7110864696211804484[77] = 0.0;
   out_7110864696211804484[78] = 0.0;
   out_7110864696211804484[79] = 0.0;
   out_7110864696211804484[80] = 1.0;
}
void f_fun(double *state, double dt, double *out_6128765810932763886) {
   out_6128765810932763886[0] = state[0];
   out_6128765810932763886[1] = state[1];
   out_6128765810932763886[2] = state[2];
   out_6128765810932763886[3] = state[3];
   out_6128765810932763886[4] = state[4];
   out_6128765810932763886[5] = dt*((-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]))*state[6] - 9.8100000000000005*state[8] + stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*state[1]) + (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*state[4])) + state[5];
   out_6128765810932763886[6] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*state[4])) + state[6];
   out_6128765810932763886[7] = state[7];
   out_6128765810932763886[8] = state[8];
}
void F_fun(double *state, double dt, double *out_9219942795113063094) {
   out_9219942795113063094[0] = 1;
   out_9219942795113063094[1] = 0;
   out_9219942795113063094[2] = 0;
   out_9219942795113063094[3] = 0;
   out_9219942795113063094[4] = 0;
   out_9219942795113063094[5] = 0;
   out_9219942795113063094[6] = 0;
   out_9219942795113063094[7] = 0;
   out_9219942795113063094[8] = 0;
   out_9219942795113063094[9] = 0;
   out_9219942795113063094[10] = 1;
   out_9219942795113063094[11] = 0;
   out_9219942795113063094[12] = 0;
   out_9219942795113063094[13] = 0;
   out_9219942795113063094[14] = 0;
   out_9219942795113063094[15] = 0;
   out_9219942795113063094[16] = 0;
   out_9219942795113063094[17] = 0;
   out_9219942795113063094[18] = 0;
   out_9219942795113063094[19] = 0;
   out_9219942795113063094[20] = 1;
   out_9219942795113063094[21] = 0;
   out_9219942795113063094[22] = 0;
   out_9219942795113063094[23] = 0;
   out_9219942795113063094[24] = 0;
   out_9219942795113063094[25] = 0;
   out_9219942795113063094[26] = 0;
   out_9219942795113063094[27] = 0;
   out_9219942795113063094[28] = 0;
   out_9219942795113063094[29] = 0;
   out_9219942795113063094[30] = 1;
   out_9219942795113063094[31] = 0;
   out_9219942795113063094[32] = 0;
   out_9219942795113063094[33] = 0;
   out_9219942795113063094[34] = 0;
   out_9219942795113063094[35] = 0;
   out_9219942795113063094[36] = 0;
   out_9219942795113063094[37] = 0;
   out_9219942795113063094[38] = 0;
   out_9219942795113063094[39] = 0;
   out_9219942795113063094[40] = 1;
   out_9219942795113063094[41] = 0;
   out_9219942795113063094[42] = 0;
   out_9219942795113063094[43] = 0;
   out_9219942795113063094[44] = 0;
   out_9219942795113063094[45] = dt*(stiffness_front*(-state[2] - state[3] + state[7])/(mass*state[1]) + (-stiffness_front - stiffness_rear)*state[5]/(mass*state[4]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[6]/(mass*state[4]));
   out_9219942795113063094[46] = -dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*pow(state[1], 2));
   out_9219942795113063094[47] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_9219942795113063094[48] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_9219942795113063094[49] = dt*((-1 - (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*pow(state[4], 2)))*state[6] - (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*pow(state[4], 2)));
   out_9219942795113063094[50] = dt*(-stiffness_front*state[0] - stiffness_rear*state[0])/(mass*state[4]) + 1;
   out_9219942795113063094[51] = dt*(-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]));
   out_9219942795113063094[52] = dt*stiffness_front*state[0]/(mass*state[1]);
   out_9219942795113063094[53] = -9.8100000000000005*dt;
   out_9219942795113063094[54] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front - pow(center_to_rear, 2)*stiffness_rear)*state[6]/(rotational_inertia*state[4]));
   out_9219942795113063094[55] = -center_to_front*dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*pow(state[1], 2));
   out_9219942795113063094[56] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_9219942795113063094[57] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_9219942795113063094[58] = dt*(-(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*pow(state[4], 2)) - (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*pow(state[4], 2)));
   out_9219942795113063094[59] = dt*(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(rotational_inertia*state[4]);
   out_9219942795113063094[60] = dt*(-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])/(rotational_inertia*state[4]) + 1;
   out_9219942795113063094[61] = center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_9219942795113063094[62] = 0;
   out_9219942795113063094[63] = 0;
   out_9219942795113063094[64] = 0;
   out_9219942795113063094[65] = 0;
   out_9219942795113063094[66] = 0;
   out_9219942795113063094[67] = 0;
   out_9219942795113063094[68] = 0;
   out_9219942795113063094[69] = 0;
   out_9219942795113063094[70] = 1;
   out_9219942795113063094[71] = 0;
   out_9219942795113063094[72] = 0;
   out_9219942795113063094[73] = 0;
   out_9219942795113063094[74] = 0;
   out_9219942795113063094[75] = 0;
   out_9219942795113063094[76] = 0;
   out_9219942795113063094[77] = 0;
   out_9219942795113063094[78] = 0;
   out_9219942795113063094[79] = 0;
   out_9219942795113063094[80] = 1;
}
void h_25(double *state, double *unused, double *out_2330715477057474332) {
   out_2330715477057474332[0] = state[6];
}
void H_25(double *state, double *unused, double *out_2142691151585509895) {
   out_2142691151585509895[0] = 0;
   out_2142691151585509895[1] = 0;
   out_2142691151585509895[2] = 0;
   out_2142691151585509895[3] = 0;
   out_2142691151585509895[4] = 0;
   out_2142691151585509895[5] = 0;
   out_2142691151585509895[6] = 1;
   out_2142691151585509895[7] = 0;
   out_2142691151585509895[8] = 0;
}
void h_24(double *state, double *unused, double *out_7613782900241663253) {
   out_7613782900241663253[0] = state[4];
   out_7613782900241663253[1] = state[5];
}
void H_24(double *state, double *unused, double *out_8718262958177027996) {
   out_8718262958177027996[0] = 0;
   out_8718262958177027996[1] = 0;
   out_8718262958177027996[2] = 0;
   out_8718262958177027996[3] = 0;
   out_8718262958177027996[4] = 1;
   out_8718262958177027996[5] = 0;
   out_8718262958177027996[6] = 0;
   out_8718262958177027996[7] = 0;
   out_8718262958177027996[8] = 0;
   out_8718262958177027996[9] = 0;
   out_8718262958177027996[10] = 0;
   out_8718262958177027996[11] = 0;
   out_8718262958177027996[12] = 0;
   out_8718262958177027996[13] = 0;
   out_8718262958177027996[14] = 1;
   out_8718262958177027996[15] = 0;
   out_8718262958177027996[16] = 0;
   out_8718262958177027996[17] = 0;
}
void h_30(double *state, double *unused, double *out_7476151119933535400) {
   out_7476151119933535400[0] = state[4];
}
void H_30(double *state, double *unused, double *out_9059381493077126650) {
   out_9059381493077126650[0] = 0;
   out_9059381493077126650[1] = 0;
   out_9059381493077126650[2] = 0;
   out_9059381493077126650[3] = 0;
   out_9059381493077126650[4] = 1;
   out_9059381493077126650[5] = 0;
   out_9059381493077126650[6] = 0;
   out_9059381493077126650[7] = 0;
   out_9059381493077126650[8] = 0;
}
void h_26(double *state, double *unused, double *out_5640043182919525054) {
   out_5640043182919525054[0] = state[7];
}
void H_26(double *state, double *unused, double *out_1598812167288546329) {
   out_1598812167288546329[0] = 0;
   out_1598812167288546329[1] = 0;
   out_1598812167288546329[2] = 0;
   out_1598812167288546329[3] = 0;
   out_1598812167288546329[4] = 0;
   out_1598812167288546329[5] = 0;
   out_1598812167288546329[6] = 0;
   out_1598812167288546329[7] = 1;
   out_1598812167288546329[8] = 0;
}
void h_27(double *state, double *unused, double *out_1727399592380996815) {
   out_1727399592380996815[0] = state[3];
}
void H_27(double *state, double *unused, double *out_6884618181276701739) {
   out_6884618181276701739[0] = 0;
   out_6884618181276701739[1] = 0;
   out_6884618181276701739[2] = 0;
   out_6884618181276701739[3] = 1;
   out_6884618181276701739[4] = 0;
   out_6884618181276701739[5] = 0;
   out_6884618181276701739[6] = 0;
   out_6884618181276701739[7] = 0;
   out_6884618181276701739[8] = 0;
}
void h_29(double *state, double *unused, double *out_8826404986444031442) {
   out_8826404986444031442[0] = state[1];
}
void H_29(double *state, double *unused, double *out_8877131236318032782) {
   out_8877131236318032782[0] = 0;
   out_8877131236318032782[1] = 1;
   out_8877131236318032782[2] = 0;
   out_8877131236318032782[3] = 0;
   out_8877131236318032782[4] = 0;
   out_8877131236318032782[5] = 0;
   out_8877131236318032782[6] = 0;
   out_8877131236318032782[7] = 0;
   out_8877131236318032782[8] = 0;
}
void h_28(double *state, double *unused, double *out_4539951695650850198) {
   out_4539951695650850198[0] = state[0];
}
void H_28(double *state, double *unused, double *out_4487213820321988260) {
   out_4487213820321988260[0] = 1;
   out_4487213820321988260[1] = 0;
   out_4487213820321988260[2] = 0;
   out_4487213820321988260[3] = 0;
   out_4487213820321988260[4] = 0;
   out_4487213820321988260[5] = 0;
   out_4487213820321988260[6] = 0;
   out_4487213820321988260[7] = 0;
   out_4487213820321988260[8] = 0;
}
void h_31(double *state, double *unused, double *out_5968165005311496682) {
   out_5968165005311496682[0] = state[8];
}
void H_31(double *state, double *unused, double *out_2173337113462470323) {
   out_2173337113462470323[0] = 0;
   out_2173337113462470323[1] = 0;
   out_2173337113462470323[2] = 0;
   out_2173337113462470323[3] = 0;
   out_2173337113462470323[4] = 0;
   out_2173337113462470323[5] = 0;
   out_2173337113462470323[6] = 0;
   out_2173337113462470323[7] = 0;
   out_2173337113462470323[8] = 1;
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

void car_update_25(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_25, H_25, NULL, in_z, in_R, in_ea, MAHA_THRESH_25);
}
void car_update_24(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<2, 3, 0>(in_x, in_P, h_24, H_24, NULL, in_z, in_R, in_ea, MAHA_THRESH_24);
}
void car_update_30(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_30, H_30, NULL, in_z, in_R, in_ea, MAHA_THRESH_30);
}
void car_update_26(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_26, H_26, NULL, in_z, in_R, in_ea, MAHA_THRESH_26);
}
void car_update_27(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_27, H_27, NULL, in_z, in_R, in_ea, MAHA_THRESH_27);
}
void car_update_29(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_29, H_29, NULL, in_z, in_R, in_ea, MAHA_THRESH_29);
}
void car_update_28(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_28, H_28, NULL, in_z, in_R, in_ea, MAHA_THRESH_28);
}
void car_update_31(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea) {
  update<1, 3, 0>(in_x, in_P, h_31, H_31, NULL, in_z, in_R, in_ea, MAHA_THRESH_31);
}
void car_err_fun(double *nom_x, double *delta_x, double *out_1278479234343698745) {
  err_fun(nom_x, delta_x, out_1278479234343698745);
}
void car_inv_err_fun(double *nom_x, double *true_x, double *out_7478532589410690046) {
  inv_err_fun(nom_x, true_x, out_7478532589410690046);
}
void car_H_mod_fun(double *state, double *out_7110864696211804484) {
  H_mod_fun(state, out_7110864696211804484);
}
void car_f_fun(double *state, double dt, double *out_6128765810932763886) {
  f_fun(state,  dt, out_6128765810932763886);
}
void car_F_fun(double *state, double dt, double *out_9219942795113063094) {
  F_fun(state,  dt, out_9219942795113063094);
}
void car_h_25(double *state, double *unused, double *out_2330715477057474332) {
  h_25(state, unused, out_2330715477057474332);
}
void car_H_25(double *state, double *unused, double *out_2142691151585509895) {
  H_25(state, unused, out_2142691151585509895);
}
void car_h_24(double *state, double *unused, double *out_7613782900241663253) {
  h_24(state, unused, out_7613782900241663253);
}
void car_H_24(double *state, double *unused, double *out_8718262958177027996) {
  H_24(state, unused, out_8718262958177027996);
}
void car_h_30(double *state, double *unused, double *out_7476151119933535400) {
  h_30(state, unused, out_7476151119933535400);
}
void car_H_30(double *state, double *unused, double *out_9059381493077126650) {
  H_30(state, unused, out_9059381493077126650);
}
void car_h_26(double *state, double *unused, double *out_5640043182919525054) {
  h_26(state, unused, out_5640043182919525054);
}
void car_H_26(double *state, double *unused, double *out_1598812167288546329) {
  H_26(state, unused, out_1598812167288546329);
}
void car_h_27(double *state, double *unused, double *out_1727399592380996815) {
  h_27(state, unused, out_1727399592380996815);
}
void car_H_27(double *state, double *unused, double *out_6884618181276701739) {
  H_27(state, unused, out_6884618181276701739);
}
void car_h_29(double *state, double *unused, double *out_8826404986444031442) {
  h_29(state, unused, out_8826404986444031442);
}
void car_H_29(double *state, double *unused, double *out_8877131236318032782) {
  H_29(state, unused, out_8877131236318032782);
}
void car_h_28(double *state, double *unused, double *out_4539951695650850198) {
  h_28(state, unused, out_4539951695650850198);
}
void car_H_28(double *state, double *unused, double *out_4487213820321988260) {
  H_28(state, unused, out_4487213820321988260);
}
void car_h_31(double *state, double *unused, double *out_5968165005311496682) {
  h_31(state, unused, out_5968165005311496682);
}
void car_H_31(double *state, double *unused, double *out_2173337113462470323) {
  H_31(state, unused, out_2173337113462470323);
}
void car_predict(double *in_x, double *in_P, double *in_Q, double dt) {
  predict(in_x, in_P, in_Q, dt);
}
void car_set_mass(double x) {
  set_mass(x);
}
void car_set_rotational_inertia(double x) {
  set_rotational_inertia(x);
}
void car_set_center_to_front(double x) {
  set_center_to_front(x);
}
void car_set_center_to_rear(double x) {
  set_center_to_rear(x);
}
void car_set_stiffness_front(double x) {
  set_stiffness_front(x);
}
void car_set_stiffness_rear(double x) {
  set_stiffness_rear(x);
}
}

const EKF car = {
  .name = "car",
  .kinds = { 25, 24, 30, 26, 27, 29, 28, 31 },
  .feature_kinds = {  },
  .f_fun = car_f_fun,
  .F_fun = car_F_fun,
  .err_fun = car_err_fun,
  .inv_err_fun = car_inv_err_fun,
  .H_mod_fun = car_H_mod_fun,
  .predict = car_predict,
  .hs = {
    { 25, car_h_25 },
    { 24, car_h_24 },
    { 30, car_h_30 },
    { 26, car_h_26 },
    { 27, car_h_27 },
    { 29, car_h_29 },
    { 28, car_h_28 },
    { 31, car_h_31 },
  },
  .Hs = {
    { 25, car_H_25 },
    { 24, car_H_24 },
    { 30, car_H_30 },
    { 26, car_H_26 },
    { 27, car_H_27 },
    { 29, car_H_29 },
    { 28, car_H_28 },
    { 31, car_H_31 },
  },
  .updates = {
    { 25, car_update_25 },
    { 24, car_update_24 },
    { 30, car_update_30 },
    { 26, car_update_26 },
    { 27, car_update_27 },
    { 29, car_update_29 },
    { 28, car_update_28 },
    { 31, car_update_31 },
  },
  .Hes = {
  },
  .sets = {
    { "mass", car_set_mass },
    { "rotational_inertia", car_set_rotational_inertia },
    { "center_to_front", car_set_center_to_front },
    { "center_to_rear", car_set_center_to_rear },
    { "stiffness_front", car_set_stiffness_front },
    { "stiffness_rear", car_set_stiffness_rear },
  },
  .extra_routines = {
  },
};

ekf_lib_init(car)
