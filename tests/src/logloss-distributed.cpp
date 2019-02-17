#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
using namespace std;

#include "boost/program_options.hpp"
namespace po = boost::program_options;

#ifdef MASTER
#define POLO_PARAMSERVER_MASTER
#elif defined WORKER
#define POLO_PARAMSERVER_WORKER
#endif

#include "polo/polo.hpp"
using namespace polo;

#include "auxiliary.hpp"

using index_t = int32_t;
using value_t = float;

template <class value_t, class index_t,
          template <class, class> class encoder_t = encoder::identity>
struct customencoder : private encoder_t<value_t, index_t> {
  using result_type = typename encoder_t<value_t, index_t>::result_type;

  customencoder() = default;

  template <class... Ts>
  customencoder(Ts &&... args)
      : encoder_t<value_t, index_t>(std::forward<Ts>(args)...) {}

  template <class RandomIt> result_type operator()(RandomIt gb, RandomIt ge) {
    result_type res = encoder_t<value_t, index_t>::operator()(gb, ge);
    bytes_.push_back(bytes_.back() + res.size());
    return res;
  }

  template <class RandomIt, class ForwardIt>
  result_type operator()(RandomIt gb, RandomIt ge, ForwardIt ib, ForwardIt ie) {
    result_type res = encoder_t<value_t, index_t>::operator()(gb, ge, ib, ie);
    bytes_.push_back(bytes_.back() + res.size());
    return res;
  }

  vector<size_t> bytes() const { return bytes_; }

private:
  vector<size_t> bytes_{0};
};

template <class value_t, class index_t>
using dynamic_t = encoder::dynamic<value_t, index_t, uint8_t>;

int main(int argc, char *argv[]) {
  size_t id;
  index_t fid, K;
  value_t lambda1;
  string maddress, saddress;

  po::options_description options("Options");
  options.add_options()("help,h", "prints the help message")(
      "dataset-id,d", po::value<size_t>(&id),
      "sets the id of the dataset to load")(
      "file-id,f", po::value<index_t>(&fid), "sets the file id to load")(
      "lambda1,l", po::value<value_t>(&lambda1)->default_value(1e-6),
      "sets the l1 penalty")("max-iter,K",
                             po::value<index_t>(&K)->default_value(100000),
                             "sets the maximum number of iterations")(
      "master-address,m", po::value<string>(&maddress),
      "sets the master's IP address")("scheduler-address,s",
                                      po::value<string>(&saddress),
                                      "sets the scheduler's IP address");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, options), vm);
  po::notify(vm);

  if (vm.count("help")) {
    cout << options << '\n';
    return 0;
  }

  if (!vm.count("scheduler-address")) {
    cerr << "Scheduler's address is not set.\n";
    cout << options << '\n';
    return 1;
  }

  ifstream dslist("data/datasets.lst");
  if (!dslist) {
    cerr << "Error occured: data/datasets.lst could not be opened.\n";
    return 2;
  }

  vector<tuple<string, bool, index_t, index_t>> datasets;
  string line;
  while (getline(dslist, line)) {
    istringstream ss(line);
    string dsname;
    bool dense;
    index_t N, d;
    ss >> dsname >> boolalpha >> dense >> N >> d;
    datasets.emplace_back(move(dsname), dense, N, d);
  }

#ifdef WORKER
  if (!vm.count("dataset-id")) {
    cerr << "Dataset ID is not set.\n";
    cout << options << '\n';
    return 3;
  } else if (id >= datasets.size()) {
    cerr << "Dataset ID is set to " << id << ". Supported ID's are:\n";
    for (size_t idx = 0; idx < datasets.size(); idx++)
      cerr << "  - " << idx << ") " << get<0>(datasets[idx]) << "("
           << (get<1>(datasets[idx]) ? "dense" : "sparse") << ")\n";
    return 3;
  }

  if (!vm.count("file-id")) {
    cerr << "File ID is not set.\n";
    cout << options << '\n';
    return 4;
  }

  const string dsfile =
      "data/" + get<0>(datasets[id]) + "-" + to_string(fid) + ".bin";
  loss::data<value_t, index_t> dataset;
  try {
    cout << "Loading dataset from " << dsfile << "...\n";
    dataset.load(dsfile, get<1>(datasets[id]));
    printinfo(dataset, 3, 5);
  } catch (const exception &ex) {
    cerr << "Error occurred: " << ex.what() << '\n';
    return 5;
  }
  loss::logistic<value_t, index_t> logloss(dataset);

  auto loss = logloss;
#else
  auto loss = nullptr;
#endif

  const index_t N = get<2>(datasets[id]);
  const index_t d = get<3>(datasets[id]);
  const value_t L = 0.25 * N;

  string suffix{"ps-piag"};

  algorithm::proxgradient<value_t, index_t, boosting::aggregated,
                          step::constant, smoothing::none, prox::l1norm,
                          execution::paramserver::executor>
      alg;

  alg.step_parameters(1 / L);
  alg.prox_parameters(lambda1);
  execution::paramserver::options psopts;
  psopts.worker_timeout(10000);
  psopts.scheduler_timeout(20000);
  psopts.worker_timeout(10000);
  psopts.master(maddress, 50000);
  psopts.scheduler(saddress, 40000, 40001, 40002);
  alg.execution_parameters(psopts);

  vector<value_t> x0(d);
  mt19937 generator(random_device{}());
  normal_distribution<value_t> dist(5, 3);
  transform(begin(x0), end(x0), begin(x0),
            [&](const value_t v) -> value_t { return dist(generator); });
  alg.initialize(x0);

  customlogger<value_t, index_t> logger;
  customencoder<value_t, index_t> enc;

  cout << "Experiment will run with:\n";
  cout << "  - ds     : " << get<0>(datasets[id]) << '\n';
  cout << "  - suffix : " << suffix << '\n';
  cout << "  - lambda1: " << lambda1 << '\n';
  cout << "  - K      : " << K << '\n';
  cout << "Scheduler is on " << saddress << '\n';
  cout << "Master is on " << maddress << '\n';
  auto tstart = chrono::high_resolution_clock::now();

  alg.solve(loss, logger, terminator::iteration<value_t, index_t>(K), enc);

#ifdef MASTER
  const string logfile = "results/" + get<0>(datasets[id]) + "-" +
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
#elif defined WORKER
  const string logfile = "results/" + get<0>(datasets[id]) + "-" +
                         to_string(fid) + "-" + suffix + "-comm.csv";
  ofstream file(logfile);
  file << "k,bytes\n";
  const auto bytes = enc.bytes();
  file << 1 << ',' << bytes[1] << '\n';
  for (size_t k = 100; k < bytes.size(); k += 100)
    file << k << ',' << bytes[k] << '\n';
#endif

  auto tend = chrono::high_resolution_clock::now();
  auto telapsed = chrono::duration_cast<chrono::seconds>(tend - tstart).count();
  auto hours = telapsed / 3600;
  auto minutes = (telapsed % 3600) / 60;
  auto seconds = (telapsed % 3600) % 60;
  cout << "Experiment took " << hours << ':' << minutes << ':' << seconds
       << ".\n";

  return 0;
}
