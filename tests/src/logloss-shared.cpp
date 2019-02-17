#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <utility>
using namespace std;

#include "boost/program_options.hpp"
namespace po = boost::program_options;

#include "polo/polo.hpp"
using namespace polo;

#include "auxiliary.hpp"

using index_t = int32_t;
using value_t = float;

int main(int argc, char *argv[]) {
  size_t id;
  index_t fid, K, M, Md, W;
  value_t lambda1;

  po::options_description options("Options");
  options.add_options()("help,h", "prints the help message")(
      "dataset-id,d", po::value<size_t>(&id),
      "sets the id of the dataset to load")(
      "file-id,f", po::value<index_t>(&fid), "sets the file id to load")(
      "batch-size,M", po::value<index_t>(&M)->default_value(1000),
      "sets the size of the mini-batches")(
      "block-size,B", po::value<index_t>(&Md)->default_value(1000),
      "sets the size of the coordinate blocks")(
      "lambda1,l", po::value<value_t>(&lambda1)->default_value(1e-6),
      "sets the l1 penalty")("max-iter,K",
                             po::value<index_t>(&K)->default_value(100000),
                             "sets the maximum number of iterations")(
      "nworkers,W", po::value<index_t>(&W),
      "sets the number of worker processes");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << options << '\n';
    return 0;
  }

  ifstream dslist("data/datasets.lst");
  if (!dslist) {
    cerr << "Error occured: data/datasets.lst could not be opened.\n";
    return 1;
  }

  vector<pair<string, bool>> datasets;
  string line;
  while (getline(dslist, line)) {
    istringstream ss(line);
    string dsname;
    bool dense;
    ss >> dsname >> boolalpha >> dense;
    datasets.emplace_back(std::move(dsname), dense);
  }

  if (!vm.count("dataset-id")) {
    cerr << "Dataset ID is not set.\n";
    cout << options << '\n';
    return 2;
  } else if (id >= datasets.size()) {
    cerr << "Dataset ID is set to " << id << ". Supported ID's are:\n";
    for (size_t idx = 0; idx < datasets.size(); idx++)
      cerr << "  - " << idx << ") " << datasets[idx].first << "("
           << (datasets[idx].second ? "dense" : "sparse") << ")\n";
    return 2;
  }

  if (!vm.count("file-id")) {
    cerr << "File ID is not set.\n";
    cout << options << '\n';
    return 3;
  }

  const string dsfile =
      "data/" + datasets[id].first + "-" + to_string(fid) + ".bin";
  loss::data<value_t, index_t> dataset;
  try {
    cout << "Loading dataset from " << dsfile << "...\n";
    dataset.load(dsfile, datasets[id].second);
    printinfo(dataset, 3, 5);
  } catch (const exception &ex) {
    cerr << "Error occurred: " << ex.what() << '\n';
    return 4;
  }
  loss::logistic<value_t, index_t> logloss(dataset);

  const index_t N = dataset.nsamples();
  const index_t d = dataset.nfeatures();
  const value_t L = 0.25 * M;
  const index_t B = N / M;

#ifdef SERIAL_MB
  algorithm::proxgradient<value_t, index_t, boosting::none, step::constant,
                          smoothing::none, prox::l1norm, execution::serial>
      alg;
  alg.step_parameters(1 / L / B);
  string suffix{"serial-mb"};
#elif defined SERIAL_MB_ADAM
  algorithm::proxgradient<value_t, index_t, boosting::momentum, step::constant,
                          smoothing::rmsprop, prox::l1norm, execution::serial>
      alg;
  alg.step_parameters(1. / B);
  alg.boosting_parameters(0.9, 0.1);
  alg.smoothing_parameters(0.999, 1E-8);
  string suffix = "serial-mb-adam-" + to_string(M);
#elif defined SERIAL_MB_AMSGRAD
  algorithm::proxgradient<value_t, index_t, boosting::momentum, step::constant,
                          smoothing::amsgrad, prox::l1norm, execution::serial>
      alg;
  alg.step_parameters(1. / B);
  alg.boosting_parameters(0.9, 0.1);
  alg.smoothing_parameters(0.999, 1E-8);
  string suffix = "serial-mb-amsgrad-" + to_string(M);
#elif defined SERIAL_MB_ADAM_BLOCK
#define BLOCK
  algorithm::proxgradient<value_t, index_t, boosting::momentum, step::constant,
                          smoothing::rmsprop, prox::l1norm, execution::serial>
      alg;
  alg.step_parameters(1. / B);
  alg.boosting_parameters(0.9, 0.1);
  alg.smoothing_parameters(0.999, 1E-8);
  string suffix = "serial-mb-adam-block-" + to_string(Md);
#elif defined CONSISTENT_MB_ADAM_BLOCK
#define BLOCK
  if (!vm.count("nworkers")) {
    cerr << "Number of workers is not set.\n";
    cout << options << '\n';
    return 5;
  }
  algorithm::proxgradient<value_t, index_t, boosting::momentum, step::constant,
                          smoothing::rmsprop, prox::l1norm,
                          execution::consistent>
      alg;
  alg.step_parameters(1. / B);
  alg.boosting_parameters(0.9, 0.1);
  alg.smoothing_parameters(0.999, 1E-8);
  alg.execution_parameters(W);
  string suffix =
      "consistent-mb-adam-block-" + to_string(Md) + "-" + to_string(W);
#elif defined INCONSISTENT_MB_ADAM_BLOCK
#define BLOCK
  if (!vm.count("nworkers")) {
    cerr << "Number of workers is not set.\n";
    cout << options << '\n';
    return 5;
  }
  algorithm::proxgradient<value_t, index_t, boosting::momentum, step::constant,
                          smoothing::rmsprop, prox::l1norm,
                          execution::inconsistent>
      alg;
  alg.step_parameters(1. / B);
  alg.boosting_parameters(0.9, 0.1);
  alg.smoothing_parameters(0.999, 1E-8);
  alg.execution_parameters(W);
  string suffix =
      "inconsistent-mb-adam-block-" + to_string(Md) + "-" + to_string(W);
#endif

  alg.prox_parameters(lambda1);

  vector<value_t> x0(d);
  mt19937 generator(random_device{}());
  normal_distribution<value_t> dist(5, 3);
  transform(begin(x0), end(x0), begin(x0),
            [&](const value_t v) -> value_t { return dist(generator); });
  alg.initialize(x0);

  customlogger<value_t, index_t> logger;
  utility::sampler::uniform<index_t> sampler;
  sampler.parameters(0, N - 1);

  cout << "Experiment will run with:\n";
  cout << "  - dsfile : " << dsfile << '\n';
  cout << "  - suffix : " << suffix << '\n';
  cout << "  - M      : " << M << '\n';
#ifdef BLOCK
  cout << "  - B      : " << Md << '\n';
#endif
  cout << "  - lambda1: " << lambda1 << '\n';
  cout << "  - K      : " << K << '\n';
  auto tstart = chrono::high_resolution_clock::now();

#ifdef BLOCK
  utility::sampler::uniform<index_t> blocksampler;
  blocksampler.parameters(0, d - 1);
  alg.solve(logloss, utility::sampler::component, sampler, M,
            utility::sampler::coordinate, blocksampler, Md,
            encoder::identity<value_t, index_t>{},
            utility::terminator::maxiter<value_t, index_t>(K), logger);
#else
  alg.solve(logloss, utility::sampler::component, sampler, M,
            encoder::identity<value_t, index_t>{},
            utility::terminator::maxiter<value_t, index_t>(K), logger);
#endif

  const string logfile = "results/" + datasets[id].first + "-" +
                         to_string(fid) + "-" + suffix + ".bin";
  cout << "Writing the logged states to " << logfile << "...\n";
  ofstream file(logfile, ios_base::binary);
  const index_t numlogs = distance(begin(logger), end(logger));
  file.write(reinterpret_cast<const char *>(&lambda1), sizeof(value_t));
  file.write(reinterpret_cast<const char *>(&numlogs), sizeof(index_t));
  file.write(reinterpret_cast<const char *>(&d), sizeof(index_t));
  for (const auto log : logger) {
    const auto k = log.getk();
    const auto t = log.gett();
    const auto &x = log.getx();
    file.write(reinterpret_cast<const char *>(&k), sizeof(index_t));
    file.write(reinterpret_cast<const char *>(&t), sizeof(value_t));
    file.write(reinterpret_cast<const char *>(&x[0]), d * sizeof(value_t));
  }

  auto tend = chrono::high_resolution_clock::now();
  auto telapsed = chrono::duration_cast<chrono::seconds>(tend - tstart).count();
  auto hours = telapsed / 3600;
  auto minutes = (telapsed % 3600) / 60;
  auto seconds = (telapsed % 3600) % 60;
  cout << "Experiment took " << hours << ':' << minutes << ':' << seconds
       << ".\n";

  return 0;
}
