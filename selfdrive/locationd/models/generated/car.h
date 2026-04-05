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
void car_err_fun(double *nom_x, double *delta_x, double *out_1278479234343698745);
void car_inv_err_fun(double *nom_x, double *true_x, double *out_7478532589410690046);
void car_H_mod_fun(double *state, double *out_7110864696211804484);
void car_f_fun(double *state, double dt, double *out_6128765810932763886);
void car_F_fun(double *state, double dt, double *out_9219942795113063094);
void car_h_25(double *state, double *unused, double *out_2330715477057474332);
void car_H_25(double *state, double *unused, double *out_2142691151585509895);
void car_h_24(double *state, double *unused, double *out_7613782900241663253);
void car_H_24(double *state, double *unused, double *out_8718262958177027996);
void car_h_30(double *state, double *unused, double *out_7476151119933535400);
void car_H_30(double *state, double *unused, double *out_9059381493077126650);
void car_h_26(double *state, double *unused, double *out_5640043182919525054);
void car_H_26(double *state, double *unused, double *out_1598812167288546329);
void car_h_27(double *state, double *unused, double *out_1727399592380996815);
void car_H_27(double *state, double *unused, double *out_6884618181276701739);
void car_h_29(double *state, double *unused, double *out_8826404986444031442);
void car_H_29(double *state, double *unused, double *out_8877131236318032782);
void car_h_28(double *state, double *unused, double *out_4539951695650850198);
void car_H_28(double *state, double *unused, double *out_4487213820321988260);
void car_h_31(double *state, double *unused, double *out_5968165005311496682);
void car_H_31(double *state, double *unused, double *out_2173337113462470323);
void car_predict(double *in_x, double *in_P, double *in_Q, double dt);
void car_set_mass(double x);
void car_set_rotational_inertia(double x);
void car_set_center_to_front(double x);
void car_set_center_to_rear(double x);
void car_set_stiffness_front(double x);
void car_set_stiffness_rear(double x);
}