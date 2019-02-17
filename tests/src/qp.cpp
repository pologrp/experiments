#include <cassert>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
using namespace std;

#include "boost/program_options.hpp"
namespace po = boost::program_options;

#include "polo/polo.hpp"
using namespace polo;

template <class value_t, class index_t> struct quadratic {
  quadratic(const index_t d, const value_t t) : d{d} {
    vector<value_t> D(size_t(d) * d), Q(size_t(d) * d), tau(d), work(1);
    q = vector<value_t>(d);

    mt19937 gen(random_device{}());
    normal_distribution<value_t> standard;
    uniform_real_distribution<value_t> uniform(-1, 1);

    for (auto &val : Q)
      val = standard(gen);

    for (auto &val : q)
      val = standard(gen);

    int info = utility::matrix::lapack<value_t>::geqrfp(d, d, &Q[0], d, &tau[0],
                                                        &work[0], -1);
    assert(info == 0);

    int lwork = work[0];
    work.resize(lwork);
    info = utility::matrix::lapack<value_t>::geqrfp(d, d, &Q[0], d, &tau[0],
                                                    &work[0], lwork);
    assert(info == 0);

    D[0] = 1 / t;
    for (index_t i = 1; i < d - 1; i++)
      D[size_t(i) * d + i] = pow(t, uniform(gen));
    D[size_t(d) * d - 1] = t;

    info = utility::matrix::lapack<value_t>::ormqr(
        'L', 'T', d, d, d, &Q[0], d, &tau[0], &D[0], d, &work[0], -1);
    assert(info == 0);

    lwork = work[0];
    work.resize(lwork);
    info = utility::matrix::lapack<value_t>::ormqr(
        'L', 'T', d, d, d, &Q[0], d, &tau[0], &D[0], d, &work[0], lwork);
    assert(info == 0);

    info = utility::matrix::lapack<value_t>::ormqr(
        'R', 'N', d, d, d, &Q[0], d, &tau[0], &D[0], d, &work[0], -1);
    assert(info == 0);

    lwork = work[0];
    work.resize(lwork);
    info = utility::matrix::lapack<value_t>::ormqr(
        'R', 'N', d, d, d, &Q[0], d, &tau[0], &D[0], d, &work[0], lwork);
    assert(info == 0);

    this->Q = vector<value_t>((size_t(d) + 1) * d / 2);
    size_t idx{0};
    for (index_t col = 0; col < d; col++)
      for (index_t row = col; row < d; row++)
        this->Q[idx++] = D[size_t(col) * d + row];

    xopt_ = q;
    for (auto &val : xopt_)
      val *= -1;

    factor = this->Q;
    info = utility::matrix::lapack<value_t>::pptrf('L', d, &factor[0]);
    assert(info == 0);

    info = utility::matrix::lapack<value_t>::pptrs('L', d, 1, &factor[0],
                                                   &xopt_[0], d);
    assert(info == 0);
  }

  value_t operator()(const value_t *x, value_t *g) {
    vector<value_t> Qx(d);
    utility::matrix::blas<value_t>::spmv('L', d, 1, &Q[0], x, 1, 0, &Qx[0], 1);

    value_t fval{0};
    for (index_t idx = 0; idx < d; idx++) {
      fval += 0.5 * x[idx] * Qx[idx] + q[idx] * x[idx];
      g[idx] = Qx[idx] + q[idx];
    }
    return fval;
  }

  vector<value_t> eigenvalues() const {
    vector<value_t> D(d), e(d - 1), tau(d - 1), work(4 * size_t(d)), Q(this->Q);
    int info = utility::matrix::lapack<value_t>::sptrd('L', d, &Q[0], &D[0],
                                                       &e[0], &tau[0]);
    assert(info == 0);

    info = utility::matrix::lapack<value_t>::pteqr('N', d, &D[0], &e[0],
                                                   nullptr, 1, &work[0]);
    assert(info == 0);

    return D;
  }

  vector<value_t> xopt() const noexcept { return xopt_; }

  template <class T1, class T2>
  friend ostream &operator<<(ostream &, const quadratic<T1, T2> &);

private:
  index_t d;
  vector<value_t> Q, factor, q, xopt_;
};

template <class value_t, class index_t>
ostream &operator<<(ostream &os, const quadratic<value_t, index_t> &qp) {
  const size_t d = qp.d;
  size_t idx{0};
  for (size_t row = 0; row < d; row++) {
    for (size_t col = 0; col < d; col++)
      if (col > row)
        os << setw(10) << setprecision(6) << '.' << '\t';
      else
        os << setw(10) << setprecision(6) << qp.Q[idx++] << '\t';
    os << "\t\t" << setw(10) << setprecision(6) << qp.q[row] << '\n';
  }
  return os;
}

template <class T> ostream &operator<<(ostream &os, const vector<T> &vec) {
  const size_t d = vec.size();
  if (vec.size() > 10) {
    for (size_t idx = 0; idx < 5; idx++)
      os << vec[idx] << ' ';
    os << "..." << ' ';
    for (size_t idx = 5; idx > 0; idx--)
      os << vec[d - idx] << ' ';
  } else
    for (const auto &x : vec)
      os << x << ' ';
  return os;
}

#include "auxiliary.hpp"

using index_t = int32_t;
using value_t = float;

int main(int argc, char *argv[]) {
  index_t d, K;
  value_t L;

  po::options_description options("Options");
  options.add_options()("help,h", "prints the help screen")(
      "lipschitz,L", po::value<value_t>(&L),
      "sets the Lipschitz constant of the problem (>=1)")(
      "dimension,d", po::value<index_t>(&d),
      "sets the dimension of the decision vector (>=1)")(
      "max-iter,K", po::value<index_t>(&K)->default_value(1000),
      "sets the maximum number of iterations");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << options << '\n';
    return 0;
  } else if (!vm.count("lipschitz") || L < 1) {
    cerr << "Lipschitz constant must be set to be at least 1.\n";
    cout << options << '\n';
    return 1;
  } else if (!vm.count("dimension") || d < 1) {
    cerr << "Dimension must be set to be at least 1.\n";
    cout << options << '\n';
    return 2;
  } else if (K < 0) {
    cerr << "K must be set to be at least 0.\n";
    cout << options << '\n';
    return 3;
  }

  cout << "Generating the QP problem...\n";
  quadratic<value_t, index_t> qp(d, L);
  cout << "QP problem has been generated.\n";
  if (d <= 10)
    cout << qp << '\n';

  const value_t mu = 1 / L;
  const vector<value_t> xopt(qp.xopt());
  vector<value_t> gopt(d);
  const value_t fopt = qp(&xopt[0], &gopt[0]) - 1E-1;

  cout << "Eigenvalues of Q are : " << qp.eigenvalues() << '\n';
  cout << "Optimizer is at      : [ " << xopt << "]\n";
  cout << "Optimum value is     : " << fopt << '\n';
  cout << "Norm of the gradient : " << dist(gopt, vector<value_t>(d)) << '\n';

  mt19937 gen(random_device{}());
  normal_distribution<value_t> normal(5, 3);
  vector<value_t> x0(d);
  for (auto &val : x0)
    val = normal(gen);

  algorithm::proxgradient<value_t, index_t> gd;
  gd.step_parameters(2 / (mu + L));
  gd.initialize(x0);

  algorithm::proxgradient<value_t, index_t, boosting::nesterov> nesterov;
  nesterov.boosting_parameters(0.9, 1 / L);
  nesterov.initialize(x0);

  algorithm::proxgradient<value_t, index_t, boosting::momentum, step::constant,
                          smoothing::rmsprop>
      adam;
  adam.step_parameters(0.08);
  adam.boosting_parameters(0.9, 0.1);
  adam.smoothing_parameters(0.999, 1E-8);
  adam.initialize(x0);

  encoder::identity<value_t, index_t> enc;
  customlogger<value_t, index_t> logger;
  utility::terminator::maxiter<value_t, index_t> maxiter(K);

  auto tstart = chrono::high_resolution_clock::now();

  cout << "Starting Gradient Descent iterations...\n";
  gd.solve(qp, enc, maxiter, logger);
  ofstream file("results/qp-serial-gd.csv");
  file << "k,t,fval,|xk-xopt|,f-fopt\n";
  for (const auto &log : logger)
    file << log.getk() << ',' << log.gett() << ',' << log.getf() << ','
         << dist(log.getx(), xopt) << ',' << log.getf() - fopt << '\n';
  cout << "Gradient Descent iterations have finished.\n";

  cout << "Starting Nesterov iterations...\n";
  logger = customlogger<value_t, index_t>();
  nesterov.solve(qp, enc, maxiter, logger);
  file = ofstream("results/qp-serial-nesterov.csv");
  file << "k,t,fval,|xk-xopt|,f-fopt\n";
  for (const auto &log : logger)
    file << log.getk() << ',' << log.gett() << ',' << log.getf() << ','
         << dist(log.getx(), xopt) << ',' << log.getf() - fopt << '\n';
  cout << "Nesterov iterations have finished.\n";

  cout << "Starting Adam iterations...\n";
  logger = customlogger<value_t, index_t>();
  adam.solve(qp, enc, maxiter, logger);
  file = ofstream("results/qp-serial-adam.csv");
  file << "k,t,fval,|xk-xopt|,f-fopt\n";
  for (const auto &log : logger)
    file << log.getk() << ',' << log.gett() << ',' << log.getf() << ','
         << dist(log.getx(), xopt) << ',' << log.getf() - fopt << '\n';
  cout << "Adam iterations have finished.\n";

  auto tend = chrono::high_resolution_clock::now();
  auto telapsed = chrono::duration_cast<chrono::seconds>(tend - tstart).count();
  auto hours = telapsed / 3600;
  auto minutes = (telapsed % 3600) / 60;
  auto seconds = (telapsed % 3600) % 60;
  cout << "Experiments took " << hours << ':' << minutes << ':' << seconds
       << ".\n";

  return 0;
}
