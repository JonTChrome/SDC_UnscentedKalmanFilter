#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 * This is scaffolding, do not modify
 */
UKF::UKF() {
    
  
  /// Open NIS Files
  NIS_file_radar_.open( "../NIS/NIS_radar.txt", ios::out );
  NIS_file_laser_.open( "../NIS/NIS_laser.txt", ios::out );
    
  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 0.5;

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = 0.5;
  
  //DO NOT MODIFY measurement noise values below these are provided by the sensor manufacturer.
  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;
  //DO NOT MODIFY measurement noise values above these are provided by the sensor manufacturer.
  
  /**
  TODO:

  Complete the initialization. See ukf.h for other member properties.

  Hint: one or more values initialized above might be wildly off...
  */
    
    n_x_ = 5;
    
    // initial state vector
    x_ = VectorXd(n_x_);
    
    // initial covariance matrix
    P_ = MatrixXd(n_x_, n_x_);
    
    P_ << 1, 0, 0, 0, 0,
    0, 1, 0, 0, 0,
    0, 0, 1, 0, 0,
    0, 0, 0, 1, 0,
    0, 0, 0, 0, 1;
    

    lambda_ = 3 - n_x_;
    
    n_aug_ = n_x_ + 2;
    
    //Sigma
    n_sig_ = 2 * n_aug_ + 1;
    
    weights_ = VectorXd(n_sig_);
    
    weights_.fill(0.5 / (n_aug_ + lambda_));
    
    weights_(0) = lambda_ / (lambda_ + n_aug_);
    
    //Covariance
    R_radar_ = MatrixXd(3, 3);
    
    R_radar_ << pow(std_radr_, 2), 0, 0,
                0, pow(std_radphi_, 2), 0,
                0, 0, pow(std_radrd_, 2);
    
    R_lidar_ = MatrixXd(2, 2);
    
    R_lidar_ << pow(std_laspx_, 2), 0,
              0, pow(std_laspy_, 2);
    
    NIS_lidar_ = 0.0;
    NIS_radar_ = 0.0;
    
}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */
    
    if(!is_initialized_) {
        if(meas_package.sensor_type_ == MeasurementPackage::RADAR) {
            double rho = meas_package.raw_measurements_[0]; // range
            double phi = meas_package.raw_measurements_[1]; // bearing
            double rhoD = meas_package.raw_measurements_[2]; // velocity of rh
            double x = rho * cos(phi);
            double y = rho * sin(phi);
            double vx = rhoD * cos(phi);
            double vy = rhoD * sin(phi);
            double v = sqrt(vx * vx + vy * vy);
            x_ << x, y, v, 0, 0;
        } else if(meas_package.sensor_type_ == MeasurementPackage::LASER) {
            x_ << meas_package.raw_measurements_[0], meas_package.raw_measurements_[1], 0, 0, 0;
        }
        
        time_us_ = meas_package.timestamp_;
        
        is_initialized_ = true;
        return;
    }
    
    double dt = (meas_package.timestamp_ - time_us_) / 1000000.0;
    
    time_us_ = meas_package.timestamp_;
    // Prediction step
    Prediction(dt);
    
    if (meas_package.sensor_type_ == MeasurementPackage::RADAR && use_radar_) {
        UpdateRadar(meas_package);
    }
    if (meas_package.sensor_type_ == MeasurementPackage::LASER && use_laser_) {
        UpdateLidar(meas_package);
    }
}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
  /**
  TODO:

  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */
    
    //Sigma points
    //augmented mean vector
    VectorXd x_aug = VectorXd(n_aug_);
    x_aug.fill(0.0);
    x_aug.head(n_x_) = x_;
    
    //augmented state covariance
    MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);
    P_aug.fill(0.0);
    P_aug.topLeftCorner(n_x_, n_x_) = P_;
    P_aug(5,5) = pow(std_a_, 2);
    P_aug(6, 6) = pow(std_yawdd_, 2);
    
    //create sigma points
    
    MatrixXd Xsig_aug = MatrixXd(x_aug.size(), n_sig_);
    MatrixXd Sq = P_aug.llt().matrixL();
    
    Xsig_aug.col(0) = x_aug;
    
    double lambda_n_sqrt = sqrt(lambda_ + n_aug_);
    for(int i = 0; i < x_aug.size(); i++) {
        Xsig_aug.col(i + 1) = x_aug + lambda_n_sqrt * Sq.col(i);
        Xsig_aug.col(i + 1 + n_aug_) = x_aug - lambda_n_sqrt * Sq.col(i);
    }

    // predict sigma points
    //PREDICT SIGMA
    
    Xsig_pred_ = MatrixXd(n_x_, n_sig_);
    for(int i = 0; i < n_sig_; i++) {
        double p_x = Xsig_aug(0,i);
        double p_y = Xsig_aug(1,i);
        double v = Xsig_aug(2,i);
        double yaw = Xsig_aug(3,i);
        double yawd = Xsig_aug(4,i);
        double nu_a = Xsig_aug(5,i);
        double nu_yawdd = Xsig_aug(6,i);
        
        //predicted state values
        double px_p, py_p;
        
        //divide by 0
        if (fabs(yawd) > 0.0001) {
            px_p = p_x + v/yawd * ( sin (yaw + yawd*delta_t) - sin(yaw));
            py_p = p_y + v/yawd * ( cos(yaw) - cos(yaw+yawd*delta_t) );
        }
        else {
            px_p = p_x + v*delta_t*cos(yaw);
            py_p = p_y + v*delta_t*sin(yaw);
        }
        
        double v_p = v;
        double yaw_p = yaw + yawd*delta_t;
        double yawd_p = yawd;
        
        //add noise
        px_p = px_p + 0.5*nu_a*delta_t*delta_t * cos(yaw);
        py_p = py_p + 0.5*nu_a*delta_t*delta_t * sin(yaw);
        v_p = v_p + nu_a*delta_t;
        
        yaw_p = yaw_p + 0.5*nu_yawdd*delta_t*delta_t;
        yawd_p = yawd_p + nu_yawdd*delta_t;
        
        //write predicted sigma point into right column
        Xsig_pred_(0,i) = px_p;
        Xsig_pred_(1,i) = py_p;
        Xsig_pred_(2,i) = v_p;
        Xsig_pred_(3,i) = yaw_p;
        Xsig_pred_(4,i) = yawd_p;
    }
    
    // predict meand and covariance
    x_ = Xsig_pred_ * weights_;
    
    //predict state covariance
    P_.fill(0.0);
    for(int i = 0; i < n_sig_; i++) {
        VectorXd x_diff = Xsig_pred_.col(i) - x_;
        NormalizeAngleOnComponent(x_diff, 3);
        P_ = P_ + weights_(i) * x_diff * x_diff.transpose();
    }
}

/**
 * Updates the state and the state covariance matrix using a laser measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateLidar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use lidar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the lidar NIS.
  */
    
    //Mean predicted measurement
    int n_z = 2;
    MatrixXd Zsig = Xsig_pred_.block(0, 0, n_z, n_sig_);
    VectorXd Zpred = VectorXd(n_z);
    Zpred.fill(0.0);
    for(int i = 0; i < n_sig_; i++) {
        Zpred = Zpred + weights_(i) * Zsig.col(i);
    }
    
    //covariance matrix
    MatrixXd S = MatrixXd(n_z, n_z);
    S.fill(0.0);
    for(int i = 0; i < n_sig_; i ++) {
        VectorXd Zdif = Zsig.col(i) - Zpred;
        S = S + weights_(i) * Zdif * Zdif.transpose();
    }
    
    S = S + R_lidar_;
    //update state
    VectorXd z = meas_package.raw_measurements_;
    
    MatrixXd Tc = MatrixXd(n_x_, n_z);
    
    Tc.fill(0.0);
    for(int i = 0; i < n_sig_; i ++) {
        VectorXd Zdif = Zsig.col(i) - Zpred;
        VectorXd Xdif = Xsig_pred_.col(i) - x_;
        Tc = Tc + weights_(i) * Xdif * Zdif.transpose();
    }
    
    //Kalman Gain
    MatrixXd K = Tc * S.inverse();
    
    VectorXd Zdif = z - Zpred;
    
    //update state mean
    x_ = x_ + K * Zdif;
    
    P_ = P_ - K * S * K.transpose();
    
    NIS_lidar_ = Zdif.transpose() * S.inverse() * Zdif;
    NIS_file_laser_ << NIS_lidar_ << endl;
}

/**
 * Updates the state and the state covariance matrix using a radar measurement.
 * @param {MeasurementPackage} meas_package
 */
void UKF::UpdateRadar(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Use radar data to update the belief about the object's
  position. Modify the state vector, x_, and covariance, P_.

  You'll also need to calculate the radar NIS.
  */
    int n_z = 3;
    MatrixXd Zsig = MatrixXd(n_z, n_sig_);
    for(int i = 0; i < n_sig_; i++) {
        double p_x = Xsig_pred_(0,i);
        double p_y = Xsig_pred_(1,i);
        double v  = Xsig_pred_(2,i);
        double yaw = Xsig_pred_(3,i);
        
        double v1 = cos(yaw)*v;
        double v2 = sin(yaw)*v;
        
        // measurement model
        Zsig(0,i) = sqrt(p_x*p_x + p_y*p_y);
        Zsig(1,i) = atan2(p_y,p_x);
        Zsig(2,i) = (p_x*v1 + p_y*v2 ) / sqrt(p_x*p_x + p_y*p_y);
    }
    
    //mean prediction
    VectorXd Zpred = VectorXd(n_z);
    Zpred.fill(0.0);
    for (int i = 0; i < n_sig_; i++) {
        Zpred = Zpred + weights_(i) * Zsig.col(i);
    }
    
    //covariance
    MatrixXd S = MatrixXd(n_z, n_z);
    S.fill(0.0);
    for (int i = 0; i < n_sig_; i++) {
        VectorXd Zdif = Zsig.col(i) - Zpred;
        NormalizeAngleOnComponent(Zdif, 1);
        S = S + weights_(i) * Zdif * Zdif.transpose();
    }
    
    S = S + R_radar_;
    
    //UPDate state
    VectorXd z = meas_package.raw_measurements_;
    
    MatrixXd Tc = MatrixXd(n_x_, n_z);
    Tc.fill(0.0);
    
    for(int i = 0; i < n_sig_; i++) {
        VectorXd Zdif = Zsig.col(i) - Zpred;
        NormalizeAngleOnComponent(Zdif, 1);
        
        VectorXd Xdif = Xsig_pred_.col(i) - x_;
        
        NormalizeAngleOnComponent(Xdif, 3);
        Tc = Tc + weights_(i) * Xdif * Zdif.transpose();
    }
    
    //Kalman gain
    MatrixXd K = Tc * S.inverse();
    
    VectorXd Zdif = z - Zpred;
    
    NormalizeAngleOnComponent(Zdif, 1);
    //Update state and covariance
    x_ = x_ + K * Zdif;
    P_ = P_ - K * S * K.transpose();
    
    NIS_radar_ = Zdif.transpose() * S.inverse() * Zdif;
    NIS_file_radar_ << NIS_radar_ << endl;;
}

UKF::~UKF()
{
    NIS_file_radar_.close();
    NIS_file_laser_.close();
}

void UKF::NormalizeAngleOnComponent(VectorXd vector, int i) {
    while (vector(i)> M_PI) vector(i)-=2.*M_PI;
    while (vector(i)<-M_PI) vector(i)+=2.*M_PI;
}
