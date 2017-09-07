#include "gp_optimize.h"

GPOptimize::GPOptimize()
{

}

void GPOptimize::initialize(double ls, double s_f, double s_n, vector<VectorXd>& x_pred)
{
    cfg_.set_cfg(ls, s_f, s_n);
    ls_ = ls;
    sigma_ = s_f;

    query_exposure_ = 0;
    is_optimal_ = false;

    set_predict(x_pred);
}

void GPOptimize::initialize(double ls, double s_f, double s_n)
{
    cfg_.set_cfg(ls, s_f, s_n);
    ls_ = ls;
    sigma_ = s_f;
    query_exposure_ = 0;
    is_optimal_ = false;
}

void GPOptimize::set_predict(vector<double>& x_pred)
{
    x_pred_.clear();
    for (auto x_val : x_pred) {
        VectorXd x(1);
        x << x_val;
        x_pred_.push_back(x);
    }
}

void GPOptimize::set_predict(vector<VectorXd>& x_pred)
{
    x_pred_ = x_pred;
}

bool GPOptimize::evaluate(double x_val, double y_val)
{
    add_data(x_val, y_val);
    train();
    predict();
    find_query_point();
    return is_optimal();
}

void GPOptimize::add_data(double x_val, double y_val)
{
    VectorXd x_vec(1);
    x_vec << x_val;

    VectorXd y_vec(1);
    y_vec << y_val;
    x_train_.push_back(x_vec);
    y_train_.push_back(y_vec);

    cout << "train size "  << x_train_.size() << endl;
}

MatrixXd GPOptimize::train()
{
    MatrixXd K = train(x_train_, y_train_);

    return K;
}

MatrixXd GPOptimize::train(vector<VectorXd> x_train, vector<VectorXd> y_train)
{
    int num_data = x_train.size();
    MatrixXd K(num_data, num_data);

    for (int i = 0; i < num_data; ++i) {
        for (int j = 0; j < num_data; ++j) {
            VectorXd x_i = x_train[i];
            VectorXd x_j = x_train[j];
            K.block(i,j,1,1) = gp_cov_k_SE(x_i, x_j, cfg_.ls(), cfg_.s_f());
            if (i != j)
                K.block(j,i,1,1) = K.block(i,j,1,1);
        }
    }

    K_ = K;
    return K;

}

void GPOptimize::predict()
{
    int n_train = x_train_.size();
    int n_pred = x_pred_.size();
    MatrixXd K_star(n_train, n_pred);

    for (int i = 0; i < n_train; ++i) {
        for (int j = 0; j < n_pred; ++j) {
            K_star.block(i,j,1,1) = gp_cov_k_SE(x_train_[i], x_pred_[j], cfg_.ls(), cfg_.s_f());
        }
    }

    double s_n = cfg_.s_n();
    MatrixXd N = (s_n*s_n)*MatrixXd::Identity(n_train, n_train);
    MatrixXd K = K_ + N;
    MatrixXd invK = K.inverse();

    VectorXd y_train(n_train);
    for (int i = 0; i < n_train; ++i) {
        y_train(i) = y_train_[i](0);
    }
    VectorXd mean = K_star.transpose() * invK * y_train;
    MatrixXd K_star2 = gp_cov_k_SE(x_pred_[0], x_pred_[0], cfg_.ls(), cfg_.s_f());
    MatrixXd var = MatrixXd::Constant(n_pred, n_pred, K_star2(0,0)) - K_star.transpose() * invK * K_star;

    y_pred_ = mean;
    var_pred_ = var;

}

void GPOptimize::find_query_point()
{
    ArrayXd var_diag = var_pred_.diagonal();
    int index;
    cost_ = var_diag.maxCoeff(&index);

    query_exposure_ = x_pred_[index](0);
    query_index_ = index;

    check_optimal();

}

MatrixXd GPOptimize::gp_cov_k_SE(VectorXd x_i, VectorXd x_j, double l, double s_f)
{
    int dim = x_i.size();
    double inv_l = 1/(l*l);
    MatrixXd M = MatrixXd::Identity(dim, dim);
    M = inv_l * M;

    VectorXd x_diff = x_i - x_j;
    MatrixXd cov;

    cov = -0.5*x_diff.transpose()*M*x_diff;
    cov = (s_f*s_f)*cov.exp();

    return cov;
}

void GPOptimize::check_optimal()
{
    double last_query = x_train_.back()(0);
    cout << " query_exposure_ " << query_exposure_;
    cout << " last _query " << last_query;
    if (abs(query_exposure_- last_query) < 15 || cost_ < 1000) {
        set_optimal();
    }
}

void GPOptimize::set_optimal()
{
    int index;
    y_pred_.maxCoeff(&index);
    optimal_exposure_ = x_pred_[index](0);
    is_optimal_ = true;
}