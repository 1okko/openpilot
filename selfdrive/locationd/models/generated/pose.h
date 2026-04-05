#pragma once
#include "rednose/helpers/ekf.h"
extern "C" {
void pose_update_4(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void pose_update_10(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void pose_update_13(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void pose_update_14(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void pose_err_fun(double *nom_x, double *delta_x, double *out_8294194842749133858);
void pose_inv_err_fun(double *nom_x, double *true_x, double *out_1479878567395019915);
void pose_H_mod_fun(double *state, double *out_2565599195015806522);
void pose_f_fun(double *state, double dt, double *out_5219021910470307673);
void pose_F_fun(double *state, double dt, double *out_2816761085006576373);
void pose_h_4(double *state, double *unused, double *out_2523270913770393018);
void pose_H_4(double *state, double *unused, double *out_4953329560176816821);
void pose_h_10(double *state, double *unused, double *out_1639151185176902848);
void pose_H_10(double *state, double *unused, double *out_8201979873762599692);
void pose_h_13(double *state, double *unused, double *out_7609351363365188316);
void pose_H_13(double *state, double *unused, double *out_8787085023479340845);
void pose_h_14(double *state, double *unused, double *out_4420274961393380218);
void pose_H_14(double *state, double *unused, double *out_6012268698252994371);
void pose_predict(double *in_x, double *in_P, double *in_Q, double dt);
}