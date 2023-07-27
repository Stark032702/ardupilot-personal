#include "AC_AttitudeControl_Multi.h"
#include <AP_HAL/AP_HAL.h>
#include <AP_Math/AP_Math.h>
#include <AP_Logger/AP_Logger.h>

// table of user settable parameters
const AP_Param::GroupInfo AC_AttitudeControl_Multi::var_info[] = {
    // parameters from parent vehicle
    AP_NESTEDGROUPINFO(AC_AttitudeControl, 0),

    // @Param: RAT_RLL_P
    // @DisplayName: Roll axis rate controller P gain
    // @Description: Roll axis rate controller P gain.  Converts the difference between desired roll rate and actual roll rate into a motor speed output
    // @Range: 0.01 0.5
    // @Increment: 0.005
    // @User: Standard

    // @Param: RAT_RLL_I
    // @DisplayName: Roll axis rate controller I gain
    // @Description: Roll axis rate controller I gain.  Corrects long-term difference in desired roll rate vs actual roll rate
    // @Range: 0.01 2.0
    // @Increment: 0.01
    // @User: Standard

    // @Param: RAT_RLL_IMAX
    // @DisplayName: Roll axis rate controller I gain maximum
    // @Description: Roll axis rate controller I gain maximum.  Constrains the maximum motor output that the I gain will output
    // @Range: 0 1
    // @Increment: 0.01
    // @User: Standard

    // @Param: RAT_RLL_D
    // @DisplayName: Roll axis rate controller D gain
    // @Description: Roll axis rate controller D gain.  Compensates for short-term change in desired roll rate vs actual roll rate
    // @Range: 0.0 0.05
    // @Increment: 0.001
    // @User: Standard

    // @Param: RAT_RLL_FF
    // @DisplayName: Roll axis rate controller feed forward
    // @Description: Roll axis rate controller feed forward
    // @Range: 0 0.5
    // @Increment: 0.001
    // @User: Standard

    // @Param: RAT_RLL_FLTT
    // @DisplayName: Roll axis rate controller target frequency in Hz
    // @Description: Roll axis rate controller target frequency in Hz
    // @Range: 5 100
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: RAT_RLL_FLTE
    // @DisplayName: Roll axis rate controller error frequency in Hz
    // @Description: Roll axis rate controller error frequency in Hz
    // @Range: 0 100
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: RAT_RLL_FLTD
    // @DisplayName: Roll axis rate controller derivative frequency in Hz
    // @Description: Roll axis rate controller derivative frequency in Hz
    // @Range: 5 100
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: RAT_RLL_SMAX
    // @DisplayName: Roll slew rate limit
    // @Description: Sets an upper limit on the slew rate produced by the combined P and D gains. If the amplitude of the control action produced by the rate feedback exceeds this value, then the D+P gain is reduced to respect the limit. This limits the amplitude of high frequency oscillations caused by an excessive gain. The limit should be set to no more than 25% of the actuators maximum slew rate to allow for load effects. Note: The gain will not be reduced to less than 10% of the nominal value. A value of zero will disable this feature.
    // @Range: 0 200
    // @Increment: 0.5
    // @User: Advanced

    AP_SUBGROUPINFO(_pid_rate_roll, "RAT_RLL_", 1, AC_AttitudeControl_Multi, AC_PID),

    // @Param: RAT_PIT_P
    // @DisplayName: Pitch axis rate controller P gain
    // @Description: Pitch axis rate controller P gain.  Converts the difference between desired pitch rate and actual pitch rate into a motor speed output
    // @Range: 0.01 0.50
    // @Increment: 0.005
    // @User: Standard

    // @Param: RAT_PIT_I
    // @DisplayName: Pitch axis rate controller I gain
    // @Description: Pitch axis rate controller I gain.  Corrects long-term difference in desired pitch rate vs actual pitch rate
    // @Range: 0.01 2.0
    // @Increment: 0.01
    // @User: Standard

    // @Param: RAT_PIT_IMAX
    // @DisplayName: Pitch axis rate controller I gain maximum
    // @Description: Pitch axis rate controller I gain maximum.  Constrains the maximum motor output that the I gain will output
    // @Range: 0 1
    // @Increment: 0.01
    // @User: Standard

    // @Param: RAT_PIT_D
    // @DisplayName: Pitch axis rate controller D gain
    // @Description: Pitch axis rate controller D gain.  Compensates for short-term change in desired pitch rate vs actual pitch rate
    // @Range: 0.0 0.05
    // @Increment: 0.001
    // @User: Standard

    // @Param: RAT_PIT_FF
    // @DisplayName: Pitch axis rate controller feed forward
    // @Description: Pitch axis rate controller feed forward
    // @Range: 0 0.5
    // @Increment: 0.001
    // @User: Standard

    // @Param: RAT_PIT_FLTT
    // @DisplayName: Pitch axis rate controller target frequency in Hz
    // @Description: Pitch axis rate controller target frequency in Hz
    // @Range: 5 100
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: RAT_PIT_FLTE
    // @DisplayName: Pitch axis rate controller error frequency in Hz
    // @Description: Pitch axis rate controller error frequency in Hz
    // @Range: 0 100
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: RAT_PIT_FLTD
    // @DisplayName: Pitch axis rate controller derivative frequency in Hz
    // @Description: Pitch axis rate controller derivative frequency in Hz
    // @Range: 5 100
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: RAT_PIT_SMAX
    // @DisplayName: Pitch slew rate limit
    // @Description: Sets an upper limit on the slew rate produced by the combined P and D gains. If the amplitude of the control action produced by the rate feedback exceeds this value, then the D+P gain is reduced to respect the limit. This limits the amplitude of high frequency oscillations caused by an excessive gain. The limit should be set to no more than 25% of the actuators maximum slew rate to allow for load effects. Note: The gain will not be reduced to less than 10% of the nominal value. A value of zero will disable this feature.
    // @Range: 0 200
    // @Increment: 0.5
    // @User: Advanced

    AP_SUBGROUPINFO(_pid_rate_pitch, "RAT_PIT_", 2, AC_AttitudeControl_Multi, AC_PID),

    // @Param: RAT_YAW_P
    // @DisplayName: Yaw axis rate controller P gain
    // @Description: Yaw axis rate controller P gain.  Converts the difference between desired yaw rate and actual yaw rate into a motor speed output
    // @Range: 0.10 2.50
    // @Increment: 0.005
    // @User: Standard

    // @Param: RAT_YAW_I
    // @DisplayName: Yaw axis rate controller I gain
    // @Description: Yaw axis rate controller I gain.  Corrects long-term difference in desired yaw rate vs actual yaw rate
    // @Range: 0.010 1.0
    // @Increment: 0.01
    // @User: Standard

    // @Param: RAT_YAW_IMAX
    // @DisplayName: Yaw axis rate controller I gain maximum
    // @Description: Yaw axis rate controller I gain maximum.  Constrains the maximum motor output that the I gain will output
    // @Range: 0 1
    // @Increment: 0.01
    // @User: Standard

    // @Param: RAT_YAW_D
    // @DisplayName: Yaw axis rate controller D gain
    // @Description: Yaw axis rate controller D gain.  Compensates for short-term change in desired yaw rate vs actual yaw rate
    // @Range: 0.000 0.02
    // @Increment: 0.001
    // @User: Standard

    // @Param: RAT_YAW_FF
    // @DisplayName: Yaw axis rate controller feed forward
    // @Description: Yaw axis rate controller feed forward
    // @Range: 0 0.5
    // @Increment: 0.001
    // @User: Standard

    // @Param: RAT_YAW_FLTT
    // @DisplayName: Yaw axis rate controller target frequency in Hz
    // @Description: Yaw axis rate controller target frequency in Hz
    // @Range: 1 50
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: RAT_YAW_FLTE
    // @DisplayName: Yaw axis rate controller error frequency in Hz
    // @Description: Yaw axis rate controller error frequency in Hz
    // @Range: 0 20
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: RAT_YAW_FLTD
    // @DisplayName: Yaw axis rate controller derivative frequency in Hz
    // @Description: Yaw axis rate controller derivative frequency in Hz
    // @Range: 5 50
    // @Increment: 1
    // @Units: Hz
    // @User: Standard

    // @Param: RAT_YAW_SMAX
    // @DisplayName: Yaw slew rate limit
    // @Description: Sets an upper limit on the slew rate produced by the combined P and D gains. If the amplitude of the control action produced by the rate feedback exceeds this value, then the D+P gain is reduced to respect the limit. This limits the amplitude of high frequency oscillations caused by an excessive gain. The limit should be set to no more than 25% of the actuators maximum slew rate to allow for load effects. Note: The gain will not be reduced to less than 10% of the nominal value. A value of zero will disable this feature.
    // @Range: 0 200
    // @Increment: 0.5
    // @User: Advanced

    AP_SUBGROUPINFO(_pid_rate_yaw, "RAT_YAW_", 3, AC_AttitudeControl_Multi, AC_PID),

    // @Param: THR_MIX_MIN
    // @DisplayName: Throttle Mix Minimum
    // @Description: Throttle vs attitude control prioritisation used when landing (higher values mean we prioritise attitude control over throttle)
    // @Range: 0.1 0.25
    // @User: Advanced
    AP_GROUPINFO("THR_MIX_MIN", 4, AC_AttitudeControl_Multi, _thr_mix_min, AC_ATTITUDE_CONTROL_MIN_DEFAULT),

    // @Param: THR_MIX_MAX
    // @DisplayName: Throttle Mix Maximum
    // @Description: Throttle vs attitude control prioritisation used during active flight (higher values mean we prioritise attitude control over throttle)
    // @Range: 0.5 0.9
    // @User: Advanced
    AP_GROUPINFO("THR_MIX_MAX", 5, AC_AttitudeControl_Multi, _thr_mix_max, AC_ATTITUDE_CONTROL_MAX_DEFAULT),

    // @Param: THR_MIX_MAN
    // @DisplayName: Throttle Mix Manual
    // @Description: Throttle vs attitude control prioritisation used during manual flight (higher values mean we prioritise attitude control over throttle)
    // @Range: 0.1 0.9
    // @User: Advanced
    AP_GROUPINFO("THR_MIX_MAN", 6, AC_AttitudeControl_Multi, _thr_mix_man, AC_ATTITUDE_CONTROL_MAN_DEFAULT),

    // @Param: Take_off
    // @DisplayName: To know if you are taking off or not
    // @Description: To know if you are taking off or not
    // @Values: 1 if taking off, 0 otherwise
    AP_GROUPINFO("TAKE_OFF", 60, AC_AttitudeControl_Multi, _take_off, 1),

    // @Param: MRAC
    // @DisplayName: To choose if we use MRAC or not
    // @Description: To choose if we use MRAC or not
    // @Values: 1 if using MRAC, 0 if not
    AP_GROUPINFO("MRAC", 62, AC_AttitudeControl_Multi, _mrac, 1),

    AP_GROUPEND
};

AC_AttitudeControl_Multi::AC_AttitudeControl_Multi(AP_AHRS_View &ahrs, const AP_Vehicle::MultiCopter &aparm, AP_MotorsMulticopter& motors, float dt) :
    AC_AttitudeControl(ahrs, aparm, motors, dt),
    _motors_multi(motors),
    _pid_rate_roll(AC_ATC_MULTI_RATE_RP_P, AC_ATC_MULTI_RATE_RP_I, AC_ATC_MULTI_RATE_RP_D, 0.0f, AC_ATC_MULTI_RATE_RP_IMAX, AC_ATC_MULTI_RATE_RP_FILT_HZ, 0.0f, AC_ATC_MULTI_RATE_RP_FILT_HZ, dt),
    _pid_rate_pitch(AC_ATC_MULTI_RATE_RP_P, AC_ATC_MULTI_RATE_RP_I, AC_ATC_MULTI_RATE_RP_D, 0.0f, AC_ATC_MULTI_RATE_RP_IMAX, AC_ATC_MULTI_RATE_RP_FILT_HZ, 0.0f, AC_ATC_MULTI_RATE_RP_FILT_HZ, dt),
    _pid_rate_yaw(AC_ATC_MULTI_RATE_YAW_P, AC_ATC_MULTI_RATE_YAW_I, AC_ATC_MULTI_RATE_YAW_D, 0.0f, AC_ATC_MULTI_RATE_YAW_IMAX, AC_ATC_MULTI_RATE_RP_FILT_HZ, AC_ATC_MULTI_RATE_YAW_FILT_HZ, 0.0f, dt)
{
    AP_Param::setup_object_defaults(this, var_info);
}

// Update Alt_Hold angle maximum
void AC_AttitudeControl_Multi::update_althold_lean_angle_max(float throttle_in)
{
    // calc maximum tilt angle based on throttle
    float thr_max = _motors_multi.get_throttle_thrust_max();

    // divide by zero check
    if (is_zero(thr_max)) {
        _althold_lean_angle_max = 0.0f;
        return;
    }

    float althold_lean_angle_max = acosf(constrain_float(throttle_in / (AC_ATTITUDE_CONTROL_ANGLE_LIMIT_THROTTLE_MAX * thr_max), 0.0f, 1.0f));
    _althold_lean_angle_max = _althold_lean_angle_max + (_dt / (_dt + _angle_limit_tc)) * (althold_lean_angle_max - _althold_lean_angle_max);
}

void AC_AttitudeControl_Multi::set_throttle_out(float throttle_in, bool apply_angle_boost, float filter_cutoff)
{
    _throttle_in = throttle_in;
    update_althold_lean_angle_max(throttle_in);
    _motors.set_throttle_filter_cutoff(filter_cutoff);
    if (apply_angle_boost) {
        // Apply angle boost
        throttle_in = get_throttle_boosted(throttle_in);
    } else {
        // Clear angle_boost for logging purposes
        _angle_boost = 0.0f;
    }
    _motors.set_throttle(throttle_in);
    _motors.set_throttle_avg_max(get_throttle_avg_max(MAX(throttle_in, _throttle_in)));
}

void AC_AttitudeControl_Multi::set_throttle_mix_max(float ratio)
{
    ratio = constrain_float(ratio, 0.0f, 1.0f);
    _throttle_rpy_mix_desired = (1.0f - ratio) * _thr_mix_min + ratio * _thr_mix_max;
}

// returns a throttle including compensation for roll/pitch angle
// throttle value should be 0 ~ 1
float AC_AttitudeControl_Multi::get_throttle_boosted(float throttle_in)
{
    if (!_angle_boost_enabled) {
        _angle_boost = 0;
        return throttle_in;
    }
    // inverted_factor is 1 for tilt angles below 60 degrees
    // inverted_factor reduces from 1 to 0 for tilt angles between 60 and 90 degrees

    float cos_tilt = _ahrs.cos_pitch() * _ahrs.cos_roll();
    float inverted_factor = constrain_float(10.0f * cos_tilt, 0.0f, 1.0f);
    float cos_tilt_target = cosf(_thrust_angle);
    float boost_factor = 1.0f / constrain_float(cos_tilt_target, 0.1f, 1.0f);

    float throttle_out = throttle_in * inverted_factor * boost_factor;
    _angle_boost = constrain_float(throttle_out - throttle_in, -1.0f, 1.0f);
    return throttle_out;
}

// returns a throttle including compensation for roll/pitch angle
// throttle value should be 0 ~ 1
float AC_AttitudeControl_Multi::get_throttle_avg_max(float throttle_in)
{
    throttle_in = constrain_float(throttle_in, 0.0f, 1.0f);
    return MAX(throttle_in, throttle_in * MAX(0.0f, 1.0f - _throttle_rpy_mix) + _motors.get_throttle_hover() * _throttle_rpy_mix);
}

// update_throttle_rpy_mix - slew set_throttle_rpy_mix to requested value
void AC_AttitudeControl_Multi::update_throttle_rpy_mix()
{
    // slew _throttle_rpy_mix to _throttle_rpy_mix_desired
    if (_throttle_rpy_mix < _throttle_rpy_mix_desired) {
        // increase quickly (i.e. from 0.1 to 0.9 in 0.4 seconds)
        _throttle_rpy_mix += MIN(2.0f * _dt, _throttle_rpy_mix_desired - _throttle_rpy_mix);
    } else if (_throttle_rpy_mix > _throttle_rpy_mix_desired) {
        // reduce more slowly (from 0.9 to 0.1 in 1.6 seconds)
        _throttle_rpy_mix -= MIN(0.5f * _dt, _throttle_rpy_mix - _throttle_rpy_mix_desired);
    }
    _throttle_rpy_mix = constrain_float(_throttle_rpy_mix, 0.1f, AC_ATTITUDE_CONTROL_MAX);
}

void AC_AttitudeControl_Multi::rate_controller_run()
{
    // move throttle vs attitude mixing towards desired (called from here because this is conveniently called on every iteration)
    update_throttle_rpy_mix();

    _ang_vel_body += _sysid_ang_vel_body;

    Vector3f gyro_latest = _ahrs.get_gyro_latest();


    un_roll = get_rate_roll_pid().update_all(_ang_vel_body.x, gyro_latest.x, 1, _motors.limit.roll) + _actuator_sysid.x;
    un_pitch = get_rate_pitch_pid().update_all(_ang_vel_body.y, gyro_latest.y, 2, _motors.limit.pitch) + _actuator_sysid.y;
    un_yaw = get_rate_yaw_pid().update_all(_ang_vel_body.z, gyro_latest.z, 3, _motors.limit.yaw) + _actuator_sysid.z;

    float err[6] = {gyro_latest.x - xref[0] ,  gyro_latest.y - xref[1] ,  gyro_latest.z - xref[2] ,  x_error_integral[0] - xref[3] ,  x_error_integral[1] - xref[4] ,  x_error_integral[2] - xref[5]};
    float w_array[10] = {gyro_latest.x, gyro_latest.y, gyro_latest.z, x_error_integral[0], x_error_integral[1], x_error_integral[2], _ang_vel_body.x, _ang_vel_body.y, _ang_vel_body.z, 1};
    // float err[3] = {gyro_latest.x - _ang_vel_body.x, gyro_latest.y - _ang_vel_body.y, gyro_latest.z - _ang_vel_body.z};
    // float w_array[7] = {gyro_latest.x, gyro_latest.y, gyro_latest.z, _ang_vel_body.x, _ang_vel_body.y, _ang_vel_body.z, 1};

    // un_roll = (- 5*gyro_latest.x - x_error_integral[0])/300;
    // un_pitch = (- 5*gyro_latest.y - x_error_integral[1])/300;
    // un_yaw = (- 5*gyro_latest.z - x_error_integral[2])/300;
    // un_roll = (_ang_vel_body.x - gyro_latest.x)/500;
    // un_pitch = (_ang_vel_body.y - gyro_latest.y)/500;
    // un_yaw = (_ang_vel_body.z - gyro_latest.z)/500;

    matrix::Matrix<float, 1, 6> e(err);
    matrix::Matrix<float, 10, 1> w(w_array);
    matrix::Matrix<float, 10, 3> MRAC_coef(MRAC_array);
    matrix::Matrix<float, 6, 6> P(P_array);
    matrix::Matrix<float, 6, 3> B(B_array);
    MRAC_coef += _dt*w*e*P*B;
    MRAC_array[0] = MRAC_coef(0, 0);
    MRAC_array[1] = MRAC_coef(0, 1);
    MRAC_array[2] = MRAC_coef(0, 2);
    MRAC_array[3] = MRAC_coef(1, 0);
    MRAC_array[4] = MRAC_coef(1, 1);
    MRAC_array[5] = MRAC_coef(1, 2);
    MRAC_array[6] = MRAC_coef(2, 0);
    MRAC_array[7] = MRAC_coef(2, 1);
    MRAC_array[8] = MRAC_coef(2, 2);
    MRAC_array[9] = MRAC_coef(3, 0);
    MRAC_array[10] = MRAC_coef(3, 1);
    MRAC_array[11] = MRAC_coef(3, 2);
    MRAC_array[12] = MRAC_coef(4, 0);
    MRAC_array[13] = MRAC_coef(4, 1);
    MRAC_array[14] = MRAC_coef(4, 2);
    MRAC_array[15] = MRAC_coef(5, 0);
    MRAC_array[16] = MRAC_coef(5, 1);
    MRAC_array[17] = MRAC_coef(5, 2);
    MRAC_array[18] = MRAC_coef(6, 0);
    MRAC_array[19] = MRAC_coef(6, 1);
    MRAC_array[20] = MRAC_coef(6, 2);
    MRAC_array[21] = MRAC_coef(7, 0);
    MRAC_array[22] = MRAC_coef(7, 1);
    MRAC_array[23] = MRAC_coef(7, 2);
    MRAC_array[24] = MRAC_coef(8, 0);
    MRAC_array[25] = MRAC_coef(8, 1);
    MRAC_array[26] = MRAC_coef(8, 2);
    MRAC_array[27] = MRAC_coef(9, 0);
    MRAC_array[28] = MRAC_coef(9, 1);
    MRAC_array[29] = MRAC_coef(9, 2);
    matrix::Matrix<float, 10, 3> MRAC_coef2(MRAC_array);
    float Gamma = 4;
    // matrix::Matrix<float, 1, 3> e(err);
    // matrix::Matrix<float, 7, 1> w(w_array);
    // matrix::Matrix<float, 7, 3> MRAC_coef(MRAC_array);
    // matrix::Matrix<float, 3, 3> P(P_array);
    // matrix::Matrix<float, 3, 3> B(B_array);
    // MRAC_coef += _dt*w*e*P*B;
    // MRAC_array[0] = MRAC_coef(0, 0);
    // MRAC_array[1] = MRAC_coef(0, 1);
    // MRAC_array[2] = MRAC_coef(0, 2);
    // MRAC_array[3] = MRAC_coef(1, 0);
    // MRAC_array[4] = MRAC_coef(1, 1);
    // MRAC_array[5] = MRAC_coef(1, 2);
    // MRAC_array[6] = MRAC_coef(2, 0);
    // MRAC_array[7] = MRAC_coef(2, 1);
    // MRAC_array[8] = MRAC_coef(2, 2);
    // MRAC_array[9] = MRAC_coef(3, 0);
    // MRAC_array[10] = MRAC_coef(3, 1);
    // MRAC_array[11] = MRAC_coef(3, 2);
    // MRAC_array[12] = MRAC_coef(4, 0);
    // MRAC_array[13] = MRAC_coef(4, 1);
    // MRAC_array[14] = MRAC_coef(4, 2);
    // MRAC_array[15] = MRAC_coef(5, 0);
    // MRAC_array[16] = MRAC_coef(5, 1);
    // MRAC_array[17] = MRAC_coef(5, 2);
    // MRAC_array[18] = MRAC_coef(6, 0);
    // MRAC_array[19] = MRAC_coef(6, 1);
    // MRAC_array[20] = MRAC_coef(6, 2);
    // matrix::Matrix<float, 7, 3> MRAC_coef2(MRAC_array);
    // float Gamma = 20;


    ua_roll = (-Gamma*MRAC_coef2.transpose()*w)(0, 0);
    ua_pitch = (-Gamma*MRAC_coef2.transpose()*w)(1, 0);
    ua_yaw = (-Gamma*MRAC_coef2.transpose()*w)(2, 0);


    if (_mrac == 0 || _take_off == 1)
    {    
        ua_roll = 0;
        ua_pitch = 0;
        ua_yaw = 0;
    }


    u_roll = un_roll + ua_roll;
    u_pitch = un_pitch + ua_pitch;
    u_yaw = un_yaw + ua_yaw;

    _motors.set_roll(u_roll);
    _motors.set_roll_ff(get_rate_roll_pid().get_ff());

    _motors.set_pitch(u_pitch);
    _motors.set_pitch_ff(get_rate_pitch_pid().get_ff());

    _motors.set_yaw(u_yaw);
    _motors.set_yaw_ff(get_rate_yaw_pid().get_ff()*_feedforward_scalar);


    // Propagation

    // Error_integral 
    x_error_integral[0] += _dt*(gyro_latest.x - _ang_vel_body.x);
    x_error_integral[1] += _dt*(gyro_latest.y - _ang_vel_body.y);
    x_error_integral[2] += _dt*(gyro_latest.z - _ang_vel_body.z);

    // Ref model
    // xref[0] = _dt*c_roll*un_roll;
    // xref[1] = _dt*c_pitch*un_pitch;
    // xref[2] = _dt*c_yaw*un_yaw;
    xref[3] += _dt*(xref[0] - _ang_vel_body.x);
    xref[4] += _dt*(xref[1] - _ang_vel_body.y);
    xref[5] += _dt*(xref[2] - _ang_vel_body.z);
    xref[0] = _ang_vel_body.x;
    xref[1] = _ang_vel_body.y;
    xref[2] = _ang_vel_body.z;



    _sysid_ang_vel_body.zero();
    _actuator_sysid.zero();

    // logging the values
    xref_roll = xref[0];
    xref_pitch = xref[1];
    xref_yaw = xref[2];
    e_roll = gyro_latest.x - target_roll;
    e_pitch = gyro_latest.y - target_pitch;
    e_yaw = gyro_latest.z - target_yaw;
    target_roll = _ang_vel_body.x;
    target_pitch = _ang_vel_body.y;
    target_yaw = _ang_vel_body.z;


    AP::logger().Write("MRA1", "TimeUS,un_roll,un_pitch,un_yaw,ua_roll,ua_pitch,ua_yaw",
                   "s------", // units: None
                   "F------", // mult: None
                   "Qffffff", // format: float
                   AP_HAL::micros64(),
                   un_roll,
                   un_pitch,
                   un_yaw,
                   ua_roll,
                   ua_pitch,
                   ua_yaw);

    AP::logger().Write("MRA2", "TimeUS,xref_r,xref_p,xref_y,targ_r,targ_p,targ_y",
                   "s------", // units: None
                   "F------", // mult: None
                   "Qffffff", // format: float
                   AP_HAL::micros64(),
                   xref_roll,
                   xref_pitch,
                   xref_yaw,
                   target_roll,
                   target_pitch,
                   target_yaw);

    AP::logger().Write("MRA3", "TimeUS,e_roll,e_pitch,e_yaw,u_roll,u_pitch,u_yaw",
                   "s------", // units: None
                   "F------", // mult: None
                   "Qffffff", // format: float
                   AP_HAL::micros64(),
                   e_roll,
                   e_pitch,
                   e_yaw,
                   u_roll,
                   u_pitch,
                   u_yaw);

    control_monitor_update();
}

// sanity check parameters.  should be called once before takeoff
void AC_AttitudeControl_Multi::parameter_sanity_check()
{
    // sanity check throttle mix parameters
    if (_thr_mix_man < 0.1f || _thr_mix_man > AC_ATTITUDE_CONTROL_MAN_LIMIT) {
        // parameter description recommends thr-mix-man be no higher than 0.9 but we allow up to 4.0
        // which can be useful for very high powered copters with very low hover throttle
        _thr_mix_man.set_and_save(constrain_float(_thr_mix_man, 0.1, AC_ATTITUDE_CONTROL_MAN_LIMIT));
    }
    if (_thr_mix_min < 0.1f || _thr_mix_min > AC_ATTITUDE_CONTROL_MIN_LIMIT) {
        _thr_mix_min.set_and_save(constrain_float(_thr_mix_min, 0.1, AC_ATTITUDE_CONTROL_MIN_LIMIT));
    }
    if (_thr_mix_max < 0.5f || _thr_mix_max > AC_ATTITUDE_CONTROL_MAX) {
        // parameter description recommends thr-mix-max be no higher than 0.9 but we allow up to 5.0
        // which can be useful for very high powered copters with very low hover throttle
        _thr_mix_max.set_and_save(constrain_float(_thr_mix_max, 0.5, AC_ATTITUDE_CONTROL_MAX));
    }
    if (_thr_mix_min > _thr_mix_max) {
        _thr_mix_min.set_and_save(AC_ATTITUDE_CONTROL_MIN_DEFAULT);
        _thr_mix_max.set_and_save(AC_ATTITUDE_CONTROL_MAX_DEFAULT);
    }
}
