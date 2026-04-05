#pragma once
#include "rednose/helpers/ekf.h"
extern "C" {
void car_update_25(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_24(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_30(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_26(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_27(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_29(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_28(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_update_31(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void car_err_fun(double *nom_x, double *delta_x, double *out_4687057413608893266);
void car_inv_err_fun(double *nom_x, double *true_x, double *out_1123163458243898222);
void car_H_mod_fun(double *state, double *out_3478516407082423425);
void car_f_fun(double *state, double dt, double *out_4204343507556155546);
void car_F_fun(double *state, double dt, double *out_7422682666927534530);
void car_h_25(double *state, double *unused, double *out_2813253289340163271);
void car_H_25(double *state, double *unused, double *out_8288531546790823280);
void car_h_24(double *state, double *unused, double *out_3119050465121288677);
void car_H_24(double *state, double *unused, double *out_939533639278371945);
void car_h_30(double *state, double *unused, double *out_6911556587619535135);
void car_H_30(double *state, double *unused, double *out_8417870493934063350);
void car_h_26(double *state, double *unused, double *out_8416344714614372470);
void car_H_26(double *state, double *unused, double *out_6416709208044672112);
void car_h_27(double *state, double *unused, double *out_1831169196004865159);
void car_H_27(double *state, double *unused, double *out_7854110267975063355);
void car_h_29(double *state, double *unused, double *out_8602004422355216095);
void car_H_29(double *state, double *unused, double *out_6140747541105512322);
void car_h_28(double *state, double *unused, double *out_3170504071655980486);
void car_H_28(double *state, double *unused, double *out_1058348524035981748);
void car_h_31(double *state, double *unused, double *out_7752218133615339160);
void car_H_31(double *state, double *unused, double *out_8257885584913862852);
void car_predict(double *in_x, double *in_P, double *in_Q, double dt);
void car_set_mass(double x);
void car_set_rotational_inertia(double x);
void car_set_center_to_front(double x);
void car_set_center_to_rear(double x);
void car_set_stiffness_front(double x);
void car_set_stiffness_rear(double x);
}