#pragma once
#include "rednose/helpers/ekf.h"
extern "C" {
void live_update_4(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_9(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_10(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_12(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_35(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_32(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_13(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_14(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_update_33(double *in_x, double *in_P, double *in_z, double *in_R, double *in_ea);
void live_H(double *in_vec, double *out_1722866598153986900);
void live_err_fun(double *nom_x, double *delta_x, double *out_8517780422191854213);
void live_inv_err_fun(double *nom_x, double *true_x, double *out_9136760954170533315);
void live_H_mod_fun(double *state, double *out_8244015398193583597);
void live_f_fun(double *state, double dt, double *out_2998811016785888472);
void live_F_fun(double *state, double dt, double *out_8151639541846347822);
void live_h_4(double *state, double *unused, double *out_4190412865012060092);
void live_H_4(double *state, double *unused, double *out_5578210320004808675);
void live_h_9(double *state, double *unused, double *out_3772942885354635444);
void live_H_9(double *state, double *unused, double *out_5819399966634399320);
void live_h_10(double *state, double *unused, double *out_5491052131473149100);
void live_H_10(double *state, double *unused, double *out_717831668309632232);
void live_h_12(double *state, double *unused, double *out_7011237489466514079);
void live_H_12(double *state, double *unused, double *out_7849077345672781146);
void live_h_35(double *state, double *unused, double *out_5555659049184463953);
void live_H_35(double *state, double *unused, double *out_8944872377377416051);
void live_h_32(double *state, double *unused, double *out_8623101657931442877);
void live_H_32(double *state, double *unused, double *out_4603867601644309163);
void live_h_13(double *state, double *unused, double *out_4438821551194389024);
void live_H_13(double *state, double *unused, double *out_5494507698112832507);
void live_h_14(double *state, double *unused, double *out_3772942885354635444);
void live_H_14(double *state, double *unused, double *out_5819399966634399320);
void live_h_33(double *state, double *unused, double *out_7034290426656834302);
void live_H_33(double *state, double *unused, double *out_6351314691693277961);
void live_predict(double *in_x, double *in_P, double *in_Q, double dt);
}