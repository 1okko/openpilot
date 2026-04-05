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
void err_fun(double *nom_x, double *delta_x, double *out_4687057413608893266) {
   out_4687057413608893266[0] = delta_x[0] + nom_x[0];
   out_4687057413608893266[1] = delta_x[1] + nom_x[1];
   out_4687057413608893266[2] = delta_x[2] + nom_x[2];
   out_4687057413608893266[3] = delta_x[3] + nom_x[3];
   out_4687057413608893266[4] = delta_x[4] + nom_x[4];
   out_4687057413608893266[5] = delta_x[5] + nom_x[5];
   out_4687057413608893266[6] = delta_x[6] + nom_x[6];
   out_4687057413608893266[7] = delta_x[7] + nom_x[7];
   out_4687057413608893266[8] = delta_x[8] + nom_x[8];
}
void inv_err_fun(double *nom_x, double *true_x, double *out_1123163458243898222) {
   out_1123163458243898222[0] = -nom_x[0] + true_x[0];
   out_1123163458243898222[1] = -nom_x[1] + true_x[1];
   out_1123163458243898222[2] = -nom_x[2] + true_x[2];
   out_1123163458243898222[3] = -nom_x[3] + true_x[3];
   out_1123163458243898222[4] = -nom_x[4] + true_x[4];
   out_1123163458243898222[5] = -nom_x[5] + true_x[5];
   out_1123163458243898222[6] = -nom_x[6] + true_x[6];
   out_1123163458243898222[7] = -nom_x[7] + true_x[7];
   out_1123163458243898222[8] = -nom_x[8] + true_x[8];
}
void H_mod_fun(double *state, double *out_3478516407082423425) {
   out_3478516407082423425[0] = 1.0;
   out_3478516407082423425[1] = 0.0;
   out_3478516407082423425[2] = 0.0;
   out_3478516407082423425[3] = 0.0;
   out_3478516407082423425[4] = 0.0;
   out_3478516407082423425[5] = 0.0;
   out_3478516407082423425[6] = 0.0;
   out_3478516407082423425[7] = 0.0;
   out_3478516407082423425[8] = 0.0;
   out_3478516407082423425[9] = 0.0;
   out_3478516407082423425[10] = 1.0;
   out_3478516407082423425[11] = 0.0;
   out_3478516407082423425[12] = 0.0;
   out_3478516407082423425[13] = 0.0;
   out_3478516407082423425[14] = 0.0;
   out_3478516407082423425[15] = 0.0;
   out_3478516407082423425[16] = 0.0;
   out_3478516407082423425[17] = 0.0;
   out_3478516407082423425[18] = 0.0;
   out_3478516407082423425[19] = 0.0;
   out_3478516407082423425[20] = 1.0;
   out_3478516407082423425[21] = 0.0;
   out_3478516407082423425[22] = 0.0;
   out_3478516407082423425[23] = 0.0;
   out_3478516407082423425[24] = 0.0;
   out_3478516407082423425[25] = 0.0;
   out_3478516407082423425[26] = 0.0;
   out_3478516407082423425[27] = 0.0;
   out_3478516407082423425[28] = 0.0;
   out_3478516407082423425[29] = 0.0;
   out_3478516407082423425[30] = 1.0;
   out_3478516407082423425[31] = 0.0;
   out_3478516407082423425[32] = 0.0;
   out_3478516407082423425[33] = 0.0;
   out_3478516407082423425[34] = 0.0;
   out_3478516407082423425[35] = 0.0;
   out_3478516407082423425[36] = 0.0;
   out_3478516407082423425[37] = 0.0;
   out_3478516407082423425[38] = 0.0;
   out_3478516407082423425[39] = 0.0;
   out_3478516407082423425[40] = 1.0;
   out_3478516407082423425[41] = 0.0;
   out_3478516407082423425[42] = 0.0;
   out_3478516407082423425[43] = 0.0;
   out_3478516407082423425[44] = 0.0;
   out_3478516407082423425[45] = 0.0;
   out_3478516407082423425[46] = 0.0;
   out_3478516407082423425[47] = 0.0;
   out_3478516407082423425[48] = 0.0;
   out_3478516407082423425[49] = 0.0;
   out_3478516407082423425[50] = 1.0;
   out_3478516407082423425[51] = 0.0;
   out_3478516407082423425[52] = 0.0;
   out_3478516407082423425[53] = 0.0;
   out_3478516407082423425[54] = 0.0;
   out_3478516407082423425[55] = 0.0;
   out_3478516407082423425[56] = 0.0;
   out_3478516407082423425[57] = 0.0;
   out_3478516407082423425[58] = 0.0;
   out_3478516407082423425[59] = 0.0;
   out_3478516407082423425[60] = 1.0;
   out_3478516407082423425[61] = 0.0;
   out_3478516407082423425[62] = 0.0;
   out_3478516407082423425[63] = 0.0;
   out_3478516407082423425[64] = 0.0;
   out_3478516407082423425[65] = 0.0;
   out_3478516407082423425[66] = 0.0;
   out_3478516407082423425[67] = 0.0;
   out_3478516407082423425[68] = 0.0;
   out_3478516407082423425[69] = 0.0;
   out_3478516407082423425[70] = 1.0;
   out_3478516407082423425[71] = 0.0;
   out_3478516407082423425[72] = 0.0;
   out_3478516407082423425[73] = 0.0;
   out_3478516407082423425[74] = 0.0;
   out_3478516407082423425[75] = 0.0;
   out_3478516407082423425[76] = 0.0;
   out_3478516407082423425[77] = 0.0;
   out_3478516407082423425[78] = 0.0;
   out_3478516407082423425[79] = 0.0;
   out_3478516407082423425[80] = 1.0;
}
void f_fun(double *state, double dt, double *out_4204343507556155546) {
   out_4204343507556155546[0] = state[0];
   out_4204343507556155546[1] = state[1];
   out_4204343507556155546[2] = state[2];
   out_4204343507556155546[3] = state[3];
   out_4204343507556155546[4] = state[4];
   out_4204343507556155546[5] = dt*((-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]))*state[6] - 9.8100000000000005*state[8] + stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*state[1]) + (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*state[4])) + state[5];
   out_4204343507556155546[6] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*state[4])) + state[6];
   out_4204343507556155546[7] = state[7];
   out_4204343507556155546[8] = state[8];
}
void F_fun(double *state, double dt, double *out_7422682666927534530) {
   out_7422682666927534530[0] = 1;
   out_7422682666927534530[1] = 0;
   out_7422682666927534530[2] = 0;
   out_7422682666927534530[3] = 0;
   out_7422682666927534530[4] = 0;
   out_7422682666927534530[5] = 0;
   out_7422682666927534530[6] = 0;
   out_7422682666927534530[7] = 0;
   out_7422682666927534530[8] = 0;
   out_7422682666927534530[9] = 0;
   out_7422682666927534530[10] = 1;
   out_7422682666927534530[11] = 0;
   out_7422682666927534530[12] = 0;
   out_7422682666927534530[13] = 0;
   out_7422682666927534530[14] = 0;
   out_7422682666927534530[15] = 0;
   out_7422682666927534530[16] = 0;
   out_7422682666927534530[17] = 0;
   out_7422682666927534530[18] = 0;
   out_7422682666927534530[19] = 0;
   out_7422682666927534530[20] = 1;
   out_7422682666927534530[21] = 0;
   out_7422682666927534530[22] = 0;
   out_7422682666927534530[23] = 0;
   out_7422682666927534530[24] = 0;
   out_7422682666927534530[25] = 0;
   out_7422682666927534530[26] = 0;
   out_7422682666927534530[27] = 0;
   out_7422682666927534530[28] = 0;
   out_7422682666927534530[29] = 0;
   out_7422682666927534530[30] = 1;
   out_7422682666927534530[31] = 0;
   out_7422682666927534530[32] = 0;
   out_7422682666927534530[33] = 0;
   out_7422682666927534530[34] = 0;
   out_7422682666927534530[35] = 0;
   out_7422682666927534530[36] = 0;
   out_7422682666927534530[37] = 0;
   out_7422682666927534530[38] = 0;
   out_7422682666927534530[39] = 0;
   out_7422682666927534530[40] = 1;
   out_7422682666927534530[41] = 0;
   out_7422682666927534530[42] = 0;
   out_7422682666927534530[43] = 0;
   out_7422682666927534530[44] = 0;
   out_7422682666927534530[45] = dt*(stiffness_front*(-state[2] - state[3] + state[7])/(mass*state[1]) + (-stiffness_front - stiffness_rear)*state[5]/(mass*state[4]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[6]/(mass*state[4]));
   out_7422682666927534530[46] = -dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(mass*pow(state[1], 2));
   out_7422682666927534530[47] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_7422682666927534530[48] = -dt*stiffness_front*state[0]/(mass*state[1]);
   out_7422682666927534530[49] = dt*((-1 - (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*pow(state[4], 2)))*state[6] - (-stiffness_front*state[0] - stiffness_rear*state[0])*state[5]/(mass*pow(state[4], 2)));
   out_7422682666927534530[50] = dt*(-stiffness_front*state[0] - stiffness_rear*state[0])/(mass*state[4]) + 1;
   out_7422682666927534530[51] = dt*(-state[4] + (-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(mass*state[4]));
   out_7422682666927534530[52] = dt*stiffness_front*state[0]/(mass*state[1]);
   out_7422682666927534530[53] = -9.8100000000000005*dt;
   out_7422682666927534530[54] = dt*(center_to_front*stiffness_front*(-state[2] - state[3] + state[7])/(rotational_inertia*state[1]) + (-center_to_front*stiffness_front + center_to_rear*stiffness_rear)*state[5]/(rotational_inertia*state[4]) + (-pow(center_to_front, 2)*stiffness_front - pow(center_to_rear, 2)*stiffness_rear)*state[6]/(rotational_inertia*state[4]));
   out_7422682666927534530[55] = -center_to_front*dt*stiffness_front*(-state[2] - state[3] + state[7])*state[0]/(rotational_inertia*pow(state[1], 2));
   out_7422682666927534530[56] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_7422682666927534530[57] = -center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_7422682666927534530[58] = dt*(-(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])*state[5]/(rotational_inertia*pow(state[4], 2)) - (-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])*state[6]/(rotational_inertia*pow(state[4], 2)));
   out_7422682666927534530[59] = dt*(-center_to_front*stiffness_front*state[0] + center_to_rear*stiffness_rear*state[0])/(rotational_inertia*state[4]);
   out_7422682666927534530[60] = dt*(-pow(center_to_front, 2)*stiffness_front*state[0] - pow(center_to_rear, 2)*stiffness_rear*state[0])/(rotational_inertia*state[4]) + 1;
   out_7422682666927534530[61] = center_to_front*dt*stiffness_front*state[0]/(rotational_inertia*state[1]);
   out_7422682666927534530[62] = 0;
   out_7422682666927534530[63] = 0;
   out_7422682666927534530[64] = 0;
   out_7422682666927534530[65] = 0;
   out_7422682666927534530[66] = 0;
   out_7422682666927534530[67] = 0;
   out_7422682666927534530[68] = 0;
   out_7422682666927534530[69] = 0;
   out_7422682666927534530[70] = 1;
   out_7422682666927534530[71] = 0;
   out_7422682666927534530[72] = 0;
   out_7422682666927534530[73] = 0;
   out_7422682666927534530[74] = 0;
   out_7422682666927534530[75] = 0;
   out_7422682666927534530[76] = 0;
   out_7422682666927534530[77] = 0;
   out_7422682666927534530[78] = 0;
   out_7422682666927534530[79] = 0;
   out_7422682666927534530[80] = 1;
}
void h_25(double *state, double *unused, double *out_2813253289340163271) {
   out_2813253289340163271[0] = state[6];
}
void H_25(double *state, double *unused, double *out_8288531546790823280) {
   out_8288531546790823280[0] = 0;
   out_8288531546790823280[1] = 0;
   out_8288531546790823280[2] = 0;
   out_8288531546790823280[3] = 0;
   out_8288531546790823280[4] = 0;
   out_8288531546790823280[5] = 0;
   out_8288531546790823280[6] = 1;
   out_8288531546790823280[7] = 0;
   out_8288531546790823280[8] = 0;
}
void h_24(double *state, double *unused, double *out_3119050465121288677) {
   out_3119050465121288677[0] = state[4];
   out_3119050465121288677[1] = state[5];
}
void H_24(double *state, double *unused, double *out_939533639278371945) {
   out_939533639278371945[0] = 0;
   out_939533639278371945[1] = 0;
   out_939533639278371945[2] = 0;
   out_939533639278371945[3] = 0;
   out_939533639278371945[4] = 1;
   out_939533639278371945[5] = 0;
   out_939533639278371945[6] = 0;
   out_939533639278371945[7] = 0;
   out_939533639278371945[8] = 0;
   out_939533639278371945[9] = 0;
   out_939533639278371945[10] = 0;
   out_939533639278371945[11] = 0;
   out_939533639278371945[12] = 0;
   out_939533639278371945[13] = 0;
   out_939533639278371945[14] = 1;
   out_939533639278371945[15] = 0;
   out_939533639278371945[16] = 0;
   out_939533639278371945[17] = 0;
}
void h_30(double *state, double *unused, double *out_6911556587619535135) {
   out_6911556587619535135[0] = state[4];
}
void H_30(double *state, double *unused, double *out_8417870493934063350) {
   out_8417870493934063350[0] = 0;
   out_8417870493934063350[1] = 0;
   out_8417870493934063350[2] = 0;
   out_8417870493934063350[3] = 0;
   out_8417870493934063350[4] = 1;
   out_8417870493934063350[5] = 0;
   out_8417870493934063350[6] = 0;
   out_8417870493934063350[7] = 0;
   out_8417870493934063350[8] = 0;
}
void h_26(double *state, double *unused, double *out_8416344714614372470) {
   out_8416344714614372470[0] = state[7];
}
void H_26(double *state, double *unused, double *out_6416709208044672112) {
   out_6416709208044672112[0] = 0;
   out_6416709208044672112[1] = 0;
   out_6416709208044672112[2] = 0;
   out_6416709208044672112[3] = 0;
   out_6416709208044672112[4] = 0;
   out_6416709208044672112[5] = 0;
   out_6416709208044672112[6] = 0;
   out_6416709208044672112[7] = 1;
   out_6416709208044672112[8] = 0;
}
void h_27(double *state, double *unused, double *out_1831169196004865159) {
   out_1831169196004865159[0] = state[3];
}
void H_27(double *state, double *unused, double *out_7854110267975063355) {
   out_7854110267975063355[0] = 0;
   out_7854110267975063355[1] = 0;
   out_7854110267975063355[2] = 0;
   out_7854110267975063355[3] = 1;
   out_7854110267975063355[4] = 0;
   out_7854110267975063355[5] = 0;
   out_7854110267975063355[6] = 0;
   out_7854110267975063355[7] = 0;
   out_7854110267975063355[8] = 0;
}
void h_29(double *state, double *unused, double *out_8602004422355216095) {
   out_8602004422355216095[0] = state[1];
}
void H_29(double *state, double *unused, double *out_6140747541105512322) {
   out_6140747541105512322[0] = 0;
   out_6140747541105512322[1] = 1;
   out_6140747541105512322[2] = 0;
   out_6140747541105512322[3] = 0;
   out_6140747541105512322[4] = 0;
   out_6140747541105512322[5] = 0;
   out_6140747541105512322[6] = 0;
   out_6140747541105512322[7] = 0;
   out_6140747541105512322[8] = 0;
}
void h_28(double *state, double *unused, double *out_3170504071655980486) {
   out_3170504071655980486[0] = state[0];
}
void H_28(double *state, double *unused, double *out_1058348524035981748) {
   out_1058348524035981748[0] = 1;
   out_1058348524035981748[1] = 0;
   out_1058348524035981748[2] = 0;
   out_1058348524035981748[3] = 0;
   out_1058348524035981748[4] = 0;
   out_1058348524035981748[5] = 0;
   out_1058348524035981748[6] = 0;
   out_1058348524035981748[7] = 0;
   out_1058348524035981748[8] = 0;
}
void h_31(double *state, double *unused, double *out_7752218133615339160) {
   out_7752218133615339160[0] = state[8];
}
void H_31(double *state, double *unused, double *out_8257885584913862852) {
   out_8257885584913862852[0] = 0;
   out_8257885584913862852[1] = 0;
   out_8257885584913862852[2] = 0;
   out_8257885584913862852[3] = 0;
   out_8257885584913862852[4] = 0;
   out_8257885584913862852[5] = 0;
   out_8257885584913862852[6] = 0;
   out_8257885584913862852[7] = 0;
   out_8257885584913862852[8] = 1;
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
void car_err_fun(double *nom_x, double *delta_x, double *out_4687057413608893266) {
  err_fun(nom_x, delta_x, out_4687057413608893266);
}
void car_inv_err_fun(double *nom_x, double *true_x, double *out_1123163458243898222) {
  inv_err_fun(nom_x, true_x, out_1123163458243898222);
}
void car_H_mod_fun(double *state, double *out_3478516407082423425) {
  H_mod_fun(state, out_3478516407082423425);
}
void car_f_fun(double *state, double dt, double *out_4204343507556155546) {
  f_fun(state,  dt, out_4204343507556155546);
}
void car_F_fun(double *state, double dt, double *out_7422682666927534530) {
  F_fun(state,  dt, out_7422682666927534530);
}
void car_h_25(double *state, double *unused, double *out_2813253289340163271) {
  h_25(state, unused, out_2813253289340163271);
}
void car_H_25(double *state, double *unused, double *out_8288531546790823280) {
  H_25(state, unused, out_8288531546790823280);
}
void car_h_24(double *state, double *unused, double *out_3119050465121288677) {
  h_24(state, unused, out_3119050465121288677);
}
void car_H_24(double *state, double *unused, double *out_939533639278371945) {
  H_24(state, unused, out_939533639278371945);
}
void car_h_30(double *state, double *unused, double *out_6911556587619535135) {
  h_30(state, unused, out_6911556587619535135);
}
void car_H_30(double *state, double *unused, double *out_8417870493934063350) {
  H_30(state, unused, out_8417870493934063350);
}
void car_h_26(double *state, double *unused, double *out_8416344714614372470) {
  h_26(state, unused, out_8416344714614372470);
}
void car_H_26(double *state, double *unused, double *out_6416709208044672112) {
  H_26(state, unused, out_6416709208044672112);
}
void car_h_27(double *state, double *unused, double *out_1831169196004865159) {
  h_27(state, unused, out_1831169196004865159);
}
void car_H_27(double *state, double *unused, double *out_7854110267975063355) {
  H_27(state, unused, out_7854110267975063355);
}
void car_h_29(double *state, double *unused, double *out_8602004422355216095) {
  h_29(state, unused, out_8602004422355216095);
}
void car_H_29(double *state, double *unused, double *out_6140747541105512322) {
  H_29(state, unused, out_6140747541105512322);
}
void car_h_28(double *state, double *unused, double *out_3170504071655980486) {
  h_28(state, unused, out_3170504071655980486);
}
void car_H_28(double *state, double *unused, double *out_1058348524035981748) {
  H_28(state, unused, out_1058348524035981748);
}
void car_h_31(double *state, double *unused, double *out_7752218133615339160) {
  h_31(state, unused, out_7752218133615339160);
}
void car_H_31(double *state, double *unused, double *out_8257885584913862852) {
  H_31(state, unused, out_8257885584913862852);
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
