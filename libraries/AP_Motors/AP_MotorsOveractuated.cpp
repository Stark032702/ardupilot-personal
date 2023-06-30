#include <AP_HAL/AP_HAL.h>
#include <cmath>
#include <string>
#include "AP_MotorsOveractuated.h"
#include "AP_MotorsOveractuated.h"
#include "AP_Motors_Class.h"
#include <AP_Vehicle/AP_Vehicle.h>
#include <SRV_Channel/SRV_Channel.h>
#include <GCS_MAVLink/GCS.h>
extern const AP_HAL::HAL& hal;

void AP_MotorsOveractuated::output_to_motors()
{
    switch (_spool_state) {
        case SpoolState::SHUT_DOWN:
        case SpoolState::GROUND_IDLE:
            // no output, cant spin up for ground idle because we don't know which way motors should be spining
            for (uint8_t i = 0; i < AP_MOTORS_MAX_NUM_MOTORS; i++) {
                if (motor_enabled[i]) {
                    set_actuator_with_slew(_actuator[i], actuator_spin_up_to_ground_idle());
                }
            }

            rc_write(AP_MOTORS_1PITCH, 1500+AP_1PITCH_TRIM );
            rc_write(AP_MOTORS_1ROLL, 1500+AP_1ROLL_TRIM );
            rc_write(AP_MOTORS_2PITCH, 1500+AP_2PITCH_TRIM );
            rc_write(AP_MOTORS_2ROLL, 1500+AP_2ROLL_TRIM );
            rc_write(AP_MOTORS_3PITCH, 1500+AP_3PITCH_TRIM );
            rc_write(AP_MOTORS_3ROLL, 1500 +AP_3ROLL_TRIM);
            rc_write(AP_MOTORS_4PITCH, 1500+AP_4PITCH_TRIM);
            rc_write(AP_MOTORS_4ROLL, 1500+AP_4ROLL_TRIM);
            break;
        case SpoolState::SPOOLING_UP:
        case SpoolState::THROTTLE_UNLIMITED:
        case SpoolState::SPOOLING_DOWN:
            // set motor output based on thrust requests
            for (uint8_t i = 0; i < AP_MOTORS_MAX_NUM_MOTORS; i++) {
                if (motor_enabled[i]) {
                    if (_reversible[i]) {
                        // revesible motor can provide both positive and negative thrust, +- spin max, spin min does not apply
                        if (is_positive(_thrust_rpyt_out[i])) { 
                            _actuator[i] = apply_thrust_curve_and_volt_scaling(_thrust_rpyt_out[i]) *_spin_max;

                        } else if (is_negative(_thrust_rpyt_out[i])) {
                            _actuator[i] = apply_thrust_curve_and_volt_scaling(-_thrust_rpyt_out[i]) * _spin_max;

                        } else {
                            _actuator[i] = 0.0f;
                        }
                    } else {
                        // motor can only provide trust in a single direction, spin min to spin max as 'normal' copter
                         _actuator[i] = thrust_to_actuator(_thrust_rpyt_out[i]);
                    }
                }
            }
            // writes the outputs to the servos for translational movement
            rc_write(AP_MOTORS_1PITCH, 1000+AP_1PITCH_TRIM+((degrees(_servo_pitch1_angle)/180)*1000) -4500);
            rc_write(AP_MOTORS_1ROLL, 1000+AP_1ROLL_TRIM+((degrees(_servo_roll1_angle)/180)*1000 )-4500);
            rc_write(AP_MOTORS_2PITCH, 2000+AP_2PITCH_TRIM-((degrees(_servo_pitch2_angle)/180)*1000));
            rc_write(AP_MOTORS_2ROLL, 2000+AP_2ROLL_TRIM-((degrees(_servo_roll2_angle)/180)*1000));
            rc_write(AP_MOTORS_3PITCH, 2000+AP_3PITCH_TRIM-((degrees(_servo_pitch3_angle)/180)*1000)-4500);
            rc_write(AP_MOTORS_3ROLL,1000+ AP_3ROLL_TRIM+((degrees(_servo_roll3_angle)/180)*1000)-4500 );
            rc_write(AP_MOTORS_4PITCH,1000+ AP_4PITCH_TRIM+((degrees(_servo_pitch4_angle)/180)*1000 )-4500);
            rc_write(AP_MOTORS_4ROLL, 2000+ AP_4ROLL_TRIM-((degrees(_servo_roll4_angle)/180)*1000 )-4500);
            break;
    }

    // Send to each motor
    for (uint8_t i = 0; i < AP_MOTORS_MAX_NUM_MOTORS; i++) {
        if (motor_enabled[i]) {
            SRV_Channels::set_output_scaled(SRV_Channels::get_motor_function(i), _actuator[i] * 4500);
        }
    }
}

void AP_MotorsOveractuated::output_armed_stabilizing()
{
    float   roll_thrust;                // roll thrust input value, +/- 1.0
    float   pitch_thrust;               // pitch thrust input value, +/- 1.0
    float   yaw_thrust;                 // yaw thrust input value, +/- 1.0
    float   throttle_thrust;            // throttle thrust input value, 0.0 - 1.0
    float   forward_thrust;             // forward thrust input value, +/- 1.0
    float   right_thrust;               // right thrust input value, +/- 1.0

    // note that the throttle, forwards and right inputs are not in bodyframe, they are in the frame of the 'normal' 4DoF copter were pretending to be

    // apply voltage and air pressure compensation
    const float compensation_gain = get_compensation_gain(); // compensation for battery voltage and altitude
    roll_thrust = (_roll_in + _roll_in_ff) * compensation_gain;
    pitch_thrust = (_pitch_in + _pitch_in_ff) * compensation_gain;
    yaw_thrust = (_yaw_in + _yaw_in_ff) * compensation_gain;
    throttle_thrust = get_throttle() * compensation_gain;

    // scale horizontal thrust with throttle, this mimics a normal copter
    // so we don't break the lean angle proportional acceleration assumption made by the position controller
    forward_thrust = get_forward() * throttle_thrust;
    right_thrust = get_lateral() * throttle_thrust;


    // set throttle limit flags
    if (throttle_thrust <= 0) {
        throttle_thrust = 0;
        // we cant thrust down, the vehicle can do it, but it would break a lot of assumptions further up the control stack
        // 1G decent probably plenty anyway....
        limit.throttle_lower = true;
    }
    if (throttle_thrust >= 1) {
        throttle_thrust = 1;
        limit.throttle_upper = true;
    }

    // rotate the thrust into bodyframe
    Matrix3f rot;
    Vector3f thrust_vec;
    rot.from_euler312(_roll_offset, _pitch_offset, 0.0f);


    /*
        upwards thrust, independent of orientation
    */
    thrust_vec.x = forward_thrust;
    thrust_vec.y = right_thrust;
    thrust_vec.z = throttle_thrust;
    thrust_vec = rot * thrust_vec;
    /*
        rotations: roll, pitch and yaw
    */
    float rpy_ratio = 1.0f;  // scale factor, output will be scaled by this ratio so it can all fit evenly

    // set limit flags if output is being scaled
    if (rpy_ratio < 1) {
        limit.roll = true;
        limit.pitch = true;
        limit.yaw = true;
    }


    
    float pseudo_inv[72] = {
        156250.0f,0.0f, 0.0f,-156250.0f,0.0f,0.0f,156250.0f,0.0f,0.0f,-156250.0f,0.0f,0.0f,
        0.0f, -156250.0f, 0.0f,0.0f,156250.0f,0.0f,0.0f,-156250.0f,0.0f,0.0f,156250.0f,0.0f,
        0.0f,0.0f,156250.0f,0.0f,0.0f,-156250.0f,0.0f,0.0f,156250.0f,0.0f,0.0f, -156250.0f,
        -189153.55f,0.0f,1327149.25f,  -189153.55f, 0.0f, -1381067.9f, -189153.55f, 0.0f,-1327149.25f,-189153.55f,0.0f,1381067.93f,
        189153.55f, 0.0f, -1327149.25f,189153.55f,0.0f,-1381067.93f, 189153.55f,0.0f,1327149.25f,189153.55f,0.0f,1381067.93f,
        -683590.82f,-683590.83f,-97429.62f,683590.83f, -683590.83f,-97429.611f,683590.83f,683590.83f,-97429.61f,-683590.83f,683590.83f,-97429.611f
    };
    float wrench[6] = {
        float(thrust_vec.x), 
        float(thrust_vec.y), 
        float(thrust_vec.z), 
        roll_thrust, 
        pitch_thrust, 
        yaw_thrust
    }; 
    matrix::Matrix<float, 6, 1> U(wrench);
        matrix::Matrix<float, 6,12> pseudo_inverse(pseudo_inv); 
        matrix::Matrix<float, 12, 1> outputs = pseudo_inverse.transpose() * U;
        float w_motor1 = sqrt(abs(outputs(2, 0)))/15000;
        float w_motor2 = sqrt(abs(outputs(5, 0)))/15000;
        float w_motor3 = sqrt(abs(outputs(8, 0)))/15000;
        float w_motor4 = sqrt(abs(outputs(11, 0)))/15000;
        _thrust_rpyt_out[AP_MOTORS_MOT_1] = constrain_float(w_motor1, 0.0f, 1.0f);
        _thrust_rpyt_out[AP_MOTORS_MOT_2] = constrain_float(w_motor2, 0.0f, 1.0f);
        _thrust_rpyt_out[AP_MOTORS_MOT_3] = constrain_float(w_motor3, 0.0f, 1.0f);
        _thrust_rpyt_out[AP_MOTORS_MOT_4] = constrain_float(w_motor4, 0.0f, 1.0f);
        if(isnan(_previous_pitch_angles[0])|| isnan(_previous_roll_angles[0])){
            float pitch1_angle = float(outputs(0,0)/outputs(2,0));
            float pitch2_angle = float(outputs(3,0)/outputs(2,0));
            float pitch3_angle = float(outputs(6,0)/outputs(2,0));
            float pitch4_angle = float(outputs(9,0)/outputs(2,0));
            _servo_pitch1_angle = M_PI_2 + remainder(pitch1_angle,M_PI);
            _servo_pitch3_angle = M_PI_2 +remainder(pitch2_angle,M_PI);
            _servo_pitch2_angle = M_PI_2 +remainder(pitch3_angle,M_PI);
            _servo_pitch4_angle = M_PI_2 +remainder(pitch4_angle,M_PI);
            _previous_pitch_angles[0] = _servo_pitch1_angle;
            _previous_pitch_angles[1] = _servo_pitch2_angle;
            _previous_pitch_angles[2] = _servo_pitch3_angle;
            _previous_pitch_angles[3] = _servo_pitch4_angle;

            float roll1_angle = float(outputs(1,0)/outputs(2,0));
            float roll2_angle = float(outputs(4,0)/outputs(2,0));
            float roll3_angle = float(outputs(7,0)/outputs(2,0));
            float roll4_angle = float(outputs(10,0)/outputs(2,0));
            _servo_roll1_angle = M_PI_2 +remainder(roll1_angle,M_PI);
            _servo_roll3_angle = M_PI_2 +remainder(roll2_angle,M_PI);
            _servo_roll2_angle = M_PI_2 +remainder(roll3_angle,M_PI);
            _servo_roll4_angle = M_PI_2 +remainder(roll4_angle,M_PI);
            _previous_roll_angles[0] = _servo_roll1_angle; 
            _previous_roll_angles[1] = _servo_roll2_angle; 
            _previous_roll_angles[2] = _servo_roll3_angle; 
            _previous_roll_angles[3] = _servo_roll4_angle; 
        
        }
        else{
            float pitch1_angle = float(outputs(0,0)/outputs(2,0));
            float pitch2_angle = float(outputs(3,0)/outputs(2,0));
            float pitch3_angle = float(outputs(6,0)/outputs(2,0));
            float pitch4_angle = float(outputs(9,0)/outputs(2,0));
            _servo_pitch1_angle = M_PI_2 + (remainder(pitch1_angle,M_PI) - _previous_pitch_angles[0]) * 0.1 + _previous_pitch_angles[0];
            _servo_pitch3_angle = M_PI_2 + (remainder(pitch2_angle,M_PI) - _previous_pitch_angles[1]) * 0.1 + _previous_pitch_angles[1];
            _servo_pitch2_angle = M_PI_2 + (remainder(pitch3_angle,M_PI) - _previous_pitch_angles[2]) * 0.1 + _previous_pitch_angles[2];
            _servo_pitch4_angle = M_PI_2 + (remainder(pitch4_angle,M_PI) - _previous_pitch_angles[3]) * 0.1 + _previous_pitch_angles[3];
            _previous_pitch_angles[0] = _servo_pitch1_angle;
            _previous_pitch_angles[1] = _servo_pitch2_angle;
            _previous_pitch_angles[2] = _servo_pitch3_angle;
            _previous_pitch_angles[3] = _servo_pitch4_angle;

            float roll1_angle = float(outputs(1,0)/outputs(2,0));
            float roll2_angle = float(outputs(4,0)/outputs(2,0));
            float roll3_angle = float(outputs(7,0)/outputs(2,0));
            float roll4_angle = float(outputs(10,0)/outputs(2,0));
            _servo_roll1_angle = M_PI_2 + (remainder(roll1_angle,M_PI) - _previous_roll_angles[0])*0.1 + _previous_roll_angles[0];
            _servo_roll3_angle = M_PI_2 + (remainder(roll2_angle,M_PI) - _previous_roll_angles[1])*0.1 + _previous_roll_angles[1];
            _servo_roll2_angle = M_PI_2 + (remainder(roll3_angle,M_PI) - _previous_roll_angles[2])*0.1 + _previous_roll_angles[2];
            _servo_roll4_angle = M_PI_2 + (remainder(roll4_angle,M_PI) - _previous_roll_angles[3])*0.1 + _previous_roll_angles[3];
            _previous_roll_angles[0] = _servo_roll1_angle; 
            _previous_roll_angles[1] = _servo_roll2_angle; 
            _previous_roll_angles[2] = _servo_roll3_angle; 
            _previous_roll_angles[3] = _servo_roll4_angle;
        }
    /*
    if ((thrust_vec.x < 0.005)||((thrust_vec.y <0.005))){
        _thrust_rpyt_out[AP_MOTORS_MOT_1] = 0.0f;
        _thrust_rpyt_out[AP_MOTORS_MOT_2] = 0.0f;
        _thrust_rpyt_out[AP_MOTORS_MOT_3] = 0.0f;
        _thrust_rpyt_out[AP_MOTORS_MOT_4] = 0.0f; 

        _servo_pitch1_angle = M_PI_2;
        _servo_pitch2_angle = M_PI_2;
        _servo_pitch3_angle = M_PI_2;
        _servo_pitch4_angle = M_PI_2;

        _servo_roll1_angle = M_PI_2 ;
        _servo_roll3_angle = M_PI_2 ;
        _servo_roll2_angle = M_PI_2 ;
        _servo_roll4_angle = M_PI_2 ;   
    }
    else{
    
        matrix::Matrix<float, 6, 1> U(wrench);
        matrix::Matrix<float, 6,12> pseudo_inverse(pseudo_inv); 
        matrix::Matrix<float, 12, 1> outputs = pseudo_inverse.transpose() * U;
        float w_motor1 = sqrt(abs(outputs(2, 0)))/800;
        float w_motor2 = sqrt(abs(outputs(5, 0)))/800;
        float w_motor3 = sqrt(abs(outputs(8, 0)))/800;
        float w_motor4 = sqrt(abs(outputs(11, 0)))/800;
        _thrust_rpyt_out[AP_MOTORS_MOT_1] = constrain_float(w_motor1, 0.0f, 1.0f);
        _thrust_rpyt_out[AP_MOTORS_MOT_2] = constrain_float(w_motor2, 0.0f, 1.0f);
        _thrust_rpyt_out[AP_MOTORS_MOT_3] = constrain_float(w_motor3, 0.0f, 1.0f);
        _thrust_rpyt_out[AP_MOTORS_MOT_4] = constrain_float(w_motor4, 0.0f, 1.0f);
        if(isnan(_previous_pitch_angles[0])|| isnan(_previous_roll_angles[0])){
            float pitch1_angle = float(outputs(0,0)/outputs(2,0));
            float pitch2_angle = float(outputs(3,0)/outputs(2,0));
            float pitch3_angle = float(outputs(6,0)/outputs(2,0));
            float pitch4_angle = float(outputs(9,0)/outputs(2,0));
            _servo_pitch1_angle = remainder(pitch1_angle,M_PI);
            _servo_pitch3_angle = remainder(pitch2_angle,M_PI);
            _servo_pitch2_angle = remainder(pitch3_angle,M_PI);
            _servo_pitch4_angle = remainder(pitch4_angle,M_PI);
            _previous_pitch_angles[0] = _servo_pitch1_angle;
            _previous_pitch_angles[1] = _servo_pitch2_angle;
            _previous_pitch_angles[2] = _servo_pitch3_angle;
            _previous_pitch_angles[3] = _servo_pitch4_angle;

            float roll1_angle = float(outputs(1,0)/outputs(2,0));
            float roll2_angle = float(outputs(4,0)/outputs(2,0));
            float roll3_angle = float(outputs(7,0)/outputs(2,0));
            float roll4_angle = float(outputs(10,0)/outputs(2,0));
            _servo_roll1_angle = remainder(roll1_angle,M_PI);
            _servo_roll3_angle = remainder(roll2_angle,M_PI);
            _servo_roll2_angle = remainder(roll3_angle,M_PI);
            _servo_roll4_angle = remainder(roll4_angle,M_PI);
            _previous_roll_angles[0] = _servo_roll1_angle; 
            _previous_roll_angles[1] = _servo_roll2_angle; 
            _previous_roll_angles[2] = _servo_roll3_angle; 
            _previous_roll_angles[3] = _servo_roll4_angle; 
        
        }
        else{
            float pitch1_angle = float(outputs(0,0)/outputs(2,0));
            float pitch2_angle = float(outputs(3,0)/outputs(2,0));
            float pitch3_angle = float(outputs(6,0)/outputs(2,0));
            float pitch4_angle = float(outputs(9,0)/outputs(2,0));
            _servo_pitch1_angle = (remainder(pitch1_angle,M_PI) - _previous_pitch_angles[0]) * 0.1 + _previous_pitch_angles[0];
            _servo_pitch3_angle = (remainder(pitch2_angle,M_PI) - _previous_pitch_angles[1]) * 0.1 + _previous_pitch_angles[1];
            _servo_pitch2_angle = (remainder(pitch3_angle,M_PI) - _previous_pitch_angles[2]) * 0.1 + _previous_pitch_angles[2];
            _servo_pitch4_angle = (remainder(pitch4_angle,M_PI) - _previous_pitch_angles[3]) * 0.1 + _previous_pitch_angles[3];
            _previous_pitch_angles[0] = _servo_pitch1_angle;
            _previous_pitch_angles[1] = _servo_pitch2_angle;
            _previous_pitch_angles[2] = _servo_pitch3_angle;
            _previous_pitch_angles[3] = _servo_pitch4_angle;

            float roll1_angle = float(outputs(1,0)/outputs(2,0));
            float roll2_angle = float(outputs(4,0)/outputs(2,0));
            float roll3_angle = float(outputs(7,0)/outputs(2,0));
            float roll4_angle = float(outputs(10,0)/outputs(2,0));
            _servo_roll1_angle = (remainder(roll1_angle,M_PI) - _previous_roll_angles[0])*0.1 + _previous_roll_angles[0];
            _servo_roll3_angle = (remainder(roll2_angle,M_PI) - _previous_roll_angles[1])*0.1 + _previous_roll_angles[1];
            _servo_roll2_angle = (remainder(roll3_angle,M_PI) - _previous_roll_angles[2])*0.1 + _previous_roll_angles[2];
            _servo_roll4_angle = (remainder(roll4_angle,M_PI) - _previous_roll_angles[3])*0.1 + _previous_roll_angles[3];
            _previous_roll_angles[0] = _servo_roll1_angle; 
            _previous_roll_angles[1] = _servo_roll2_angle; 
            _previous_roll_angles[2] = _servo_roll3_angle; 
            _previous_roll_angles[3] = _servo_roll4_angle;
        }

    
    }
    
    */

    
}


// sets the roll and pitch offset, this rotates the thrust vector in body frame
// these are typically set such that the throttle thrust vector is earth frame up
void AP_MotorsOveractuated::set_roll_pitch(float roll_deg, float pitch_deg)
{
    _roll_offset = radians(roll_deg);
    _pitch_offset = radians(pitch_deg);
}

// add_motor, take roll, pitch, yaw, throttle(up), forward, right factors along with a bool if the motor is reversible and the testing order, called from scripting
void AP_MotorsOveractuated::add_motor(int8_t motor_num, float roll_factor, float pitch_factor, float yaw_factor, float throttle_factor, float forward_factor, float right_factor, bool reversible, uint8_t testing_order)
{
    if (initialised_ok()) {
        // don't allow matrix to be changed after init
        return;
    }

    // ensure valid motor number is provided
    if (motor_num >= 0 && motor_num < AP_MOTORS_MAX_NUM_MOTORS) {
        motor_enabled[motor_num] = true;

        _roll_factor[motor_num] = roll_factor;
        _pitch_factor[motor_num] = pitch_factor;
        _yaw_factor[motor_num] = yaw_factor;

        _throttle_factor[motor_num] = throttle_factor;
        _forward_factor[motor_num] = forward_factor;
        _right_factor[motor_num] = right_factor;

        // set order that motor appears in test
        _test_order[motor_num] = testing_order;

        // ensure valid motor number is provided
        SRV_Channel::Aux_servo_function_t function = SRV_Channels::get_motor_function(motor_num);
        SRV_Channels::set_aux_channel_default(function, motor_num);

        uint8_t chan;
        if (!SRV_Channels::find_channel(function, chan)) {
            gcs().send_text(MAV_SEVERITY_ERROR, "Motors: unable to setup motor %u", motor_num);
            return;
        }

        _reversible[motor_num] = reversible;
        if (_reversible[motor_num]) {
            // reversible, set to angle type hard code trim to 1500
            SRV_Channels::set_angle(function, 4500);
            SRV_Channels::set_trim_to_pwm_for(function, 1500);
        } else {
            SRV_Channels::set_range(function, 4500);
        }
        SRV_Channels::set_output_min_max(function, get_pwm_output_min(), get_pwm_output_max());
    }
}

bool AP_MotorsOveractuated::init(uint8_t expected_num_motors){ 
    uint8_t num_motors = 0;
    for (uint8_t i = 0; i < AP_MOTORS_MAX_NUM_MOTORS; i++) {
        if (motor_enabled[i]) {
            num_motors++;
        }
    }

    set_initialised_ok(expected_num_motors == num_motors);

    if (!initialised_ok()) {
        _mav_type = MAV_TYPE_GENERIC;
        return false;
    }

    switch (num_motors) {
        case 3:
            _mav_type = MAV_TYPE_TRICOPTER;
            break;
        case 4:
            _mav_type = MAV_TYPE_QUADROTOR;
            break;
        case 6:
            _mav_type = MAV_TYPE_HEXAROTOR;
            break;
        case 8:
            _mav_type = MAV_TYPE_OCTOROTOR;
            break;
        case 10:
            _mav_type = MAV_TYPE_DECAROTOR;
            break;
        case 12:
            _mav_type = MAV_TYPE_DODECAROTOR;
            break;
        default:
            _mav_type = MAV_TYPE_GENERIC;
    }

    return true;
}

void AP_MotorsOveractuated:: init(motor_frame_class frame_class, motor_frame_type frame_type){  
    _frame_class_string = "OVERACTUATED"; 
    _frame_type_string = "X"; 
    //adds the motors to the frame and enables ther calibration 
    motor_enabled[AP_MOTORS_MOT_1] = true;
    motor_enabled[AP_MOTORS_MOT_2] = true;
    motor_enabled[AP_MOTORS_MOT_3] = true;
    motor_enabled[AP_MOTORS_MOT_4] = true; 

    add_motor(AP_MOTORS_MOT_1,     -0.71f,              0.71f,              1.0f,            1.0f,             0,                0,               false,1); 
    add_motor(AP_MOTORS_MOT_2,     0.71f,              -0.71f,              1.0f,           1.0f,             0,                0,               false,3); 
    add_motor(AP_MOTORS_MOT_3,     0.71f,              0.71f,               -1.0f,            1.0f,             0,                0,                false,4); 
    add_motor(AP_MOTORS_MOT_4,     -0.71f,              -0.71f,              -1.0f,           1.0f,             0,                0,                false,2); 
    set_update_rate(400); 
    
    _mav_type = MAV_TYPE_DODECAROTOR; 
    

    
    // tilt servos setup 
    add_motor(AP_MOTORS_1PITCH, 0,0,0,0,0,0,false, 5); 
    SRV_Channels::set_angle(SRV_Channels::get_motor_function(AP_MOTORS_1PITCH), MAX_TILT_SERVO_ANGLE*100);
    add_motor(AP_MOTORS_1ROLL, 0,0,0,0,0,1.0f,false, 6); 
    SRV_Channels::set_angle(SRV_Channels::get_motor_function(AP_MOTORS_1ROLL), MAX_TILT_SERVO_ANGLE*100);
    
    add_motor(AP_MOTORS_2PITCH, 0,0,0,0,1.0f,0,true, 7); 
    SRV_Channels::set_angle(SRV_Channels::get_motor_function(AP_MOTORS_2PITCH), MAX_TILT_SERVO_ANGLE*100);
    add_motor(AP_MOTORS_2ROLL, 0,0,0,0,0,1.0f,true, 8); 
    SRV_Channels::set_angle(SRV_Channels::get_motor_function(AP_MOTORS_2ROLL), MAX_TILT_SERVO_ANGLE*100);
    
    add_motor(AP_MOTORS_3PITCH, 0,0,0,0,1.0f,0,true, 9); 
    SRV_Channels::set_angle(SRV_Channels::get_motor_function(AP_MOTORS_3PITCH), MAX_TILT_SERVO_ANGLE*100);
    add_motor(AP_MOTORS_3ROLL, 0,0,0,0,0,1.0f,true, 10); 
    SRV_Channels::set_angle(SRV_Channels::get_motor_function(AP_MOTORS_3ROLL), MAX_TILT_SERVO_ANGLE*100);
    
    add_motor(AP_MOTORS_4PITCH, 0,0,0,0,1.0f,0,true, 11); 
    SRV_Channels::set_angle(SRV_Channels::get_motor_function(AP_MOTORS_4PITCH), MAX_TILT_SERVO_ANGLE*100);
    add_motor(AP_MOTORS_4ROLL, 0,0,0,0,0,1.0f,false, 12); 
    SRV_Channels::set_angle(SRV_Channels::get_motor_function(AP_MOTORS_4ROLL), MAX_TILT_SERVO_ANGLE*100);
        
    set_initialised_ok(true); 

}

void AP_MotorsOveractuated::_output_test_seq(uint8_t motor_seq, int16_t pwm){ 
    
    switch(motor_seq){ 
        case 1: 
        rc_write(AP_MOTORS_MOT_1, pwm); 
        break; 
        case 2: 
        rc_write(AP_MOTORS_MOT_2, pwm); 
        break; 
        case 3: 
        rc_write(AP_MOTORS_MOT_3, pwm); 
        break; 
        case 4: 
        rc_write(AP_MOTORS_MOT_4, pwm); 
        break;
        case 5: 
        rc_write(AP_MOTORS_1PITCH, pwm); 
        break; 
        case 6: 
        rc_write(AP_MOTORS_1ROLL, pwm); 
        break; 
        case 7: 
        rc_write(AP_MOTORS_2PITCH, pwm); 
        break; 
        case 8: 
        rc_write(AP_MOTORS_2ROLL, pwm); 
        break; 
        case 9: 
        rc_write(AP_MOTORS_3PITCH, pwm); 
        break; 
        case 10: 
        rc_write(AP_MOTORS_3ROLL, pwm); 
        break; 
        case 11: 
        rc_write(AP_MOTORS_4PITCH, pwm); 
        break; 
        case 12: 
        rc_write(AP_MOTORS_4ROLL, pwm); 
        break; 
    }
}
// singleton instance
AP_MotorsOveractuated *AP_MotorsOveractuated::_singleton; 
