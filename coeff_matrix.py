import numpy as np
import matplotlib.pyplot as plt
import matplotlib as mpb 

mu = (1.6*(10**-6))
Km = (2.58 *(10**-8))
L = 0.16
c = 0.707106781

coefficient_matrix = [
       [float(mu), 0.0,0.0, -float(mu), 0.0,0.0,float(mu), 0.0,0.0, -float(mu), 0.0, 0.0], 
       [ 0.0, -float(mu),0.0, 0.0, float(mu), 0.0, 0.0, -float(mu), 0.0, 0.0, float(mu), 0.0], 
       [ 0.0, 0.0, float(mu), 0.0,0.0, -float(mu), 0.0,0.0, float(mu), 0.0,0.0, -float(mu)], 
       [-float(Km),0.0,float(c * mu * L ), -float(Km),0.0, -float(c* L * mu), -float(Km),0.0, -float(c*L*mu), -float(Km),0.0, float(c* L * mu)], 
       [float(Km),0.0,-float(c * mu * L ),float(Km),0.0, -float(c* L * mu), float(Km),0.0, float(c*L*mu), float(Km),0.0, float(c * L * mu)], 
       [-float(c * L * mu),-float(c * L * mu), -float(Km), float(c * L * mu),-float(c * L * mu), -float(Km), float(c * L * mu),float(c * L * mu), -float(Km ), -float(c * L * mu),float(c * L * mu), -float(Km)]
    ]

coeff_array = np.array(coefficient_matrix)
points = 150
noise = np.random.normal(0.1,0.002,6*points).reshape((6,points))
noise[2][:] = -0.9
noise[3][:] = 0.2
noise[4][:] = 0.3
noise
outputs = np.linalg.pinv(coeff_array).dot(noise)
np.set_printoptions(suppress=True)
#print(np.linalg.pinv(coeff_array).T)

angular_velocity = np.sqrt(np.abs(outputs[2:outputs.shape[0]:3]))

alpha_angles = np.zeros(outputs.shape)
beta_angles = np.zeros(outputs.shape)
lowpass_filter = 1
previous_beta = [0,0,0,0] 
previous_alpha = [0,0,0,0] 

for j in range(0, outputs.shape[1]):
    counter = 0
    for i in range(0,outputs.shape[0],3):
        if previous_alpha[i//3] == 0 or previous_beta[i//3] == 0 or lowpass_filter == 0:
            alpha_angles2 = (outputs[i,j]/outputs[i+2,j]) 
            alpha_angles[i//3,j] = np.remainder(alpha_angles2, np.pi) * (180/np.pi)
            beta_angles2 = (outputs[i+1,j]/outputs[i+2,j]) 
            beta_angles[i//3,j] = np.remainder(beta_angles2, np.pi) * (180/np.pi)
            previous_alpha[counter] = alpha_angles[i,j]
            previous_beta[counter] = beta_angles[i,j]
            counter+=1
        else: 
            alpha_angles2 = (outputs[i,j]/outputs[i+2,j]) 
            alpha_angles[i,j] = 0.1 * (np.remainder(alpha_angles2, np.pi) * (180/np.pi) - previous_alpha[counter]) +(previous_alpha[counter])
            beta_angles2 = (outputs[i+1,j]/outputs[i+2,j]) 
            beta_angles[i,j] = 0.1 *(np.remainder(beta_angles2, np.pi) * (180/np.pi) - previous_beta[counter]) + (previous_beta[counter])
            previous_alpha[counter] = alpha_angles[i,j]
            previous_beta[counter] = beta_angles[i,j] 
            counter+=1   
time = np.linspace(0,points,num=points)
fig,axes = plt.subplots(ncols=4, nrows=2, figsize=(5,5))
counter = 1
for row in range(2): 
    for col in range(2): 
        axes[row, col].plot(time, alpha_angles[counter-1][:], color='red', label="pitch angle")
        axes[row, col].plot(time, beta_angles[counter-1][:], color='blue', label="roll angle")
        axes[row, col].legend()
        axes[row, col].set_title(f"Servo Angles for Motor {counter}")
        axes[row, col].set_xlabel("points(t)")
        axes[row, col].set_ylabel("Angle (radians)")
        counter+=1


axis = ["forward", "right", "throttle", "roll", "pitch", "yaw"]
counter = 0
for row in range(2): 
    for col in range(2,4): 
        axes[row, col].scatter(noise.min(0), alpha_angles[counter][:], color='red', label="minimum of thrust vector")
        axes[row, col].scatter(noise.max(0), alpha_angles[counter][:], color='blue', label="maximum of thrust vector")
        axes[row, col].legend()
        axes[row, col].set_title(f"Motor {counter +1} angle vs min/max of desired wrench")
        axes[row, col].set_xlabel(f"Min/Max of thrust")
        axes[row, col].set_ylabel("Angle (radians)")
        counter+=1

plt.show()

